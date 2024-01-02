/* Copyright (C) 1991-2024 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */

#include <hurd.h>
#include <hurd/signal.h>
#include <hurd/msg.h>
#include <stdlib.h>

/* This is run on the thread stack after restoring it, to be able to
   unlock SS off sigstack.  */
void
__sigreturn2 (struct sigcontext *scp,
	      struct hurd_sigstate *ss)
{
  error_t err;
  mach_port_t reply_port;
  _hurd_sigstate_unlock (ss);

  /* Destroy the MiG reply port used by the signal handler, and restore the
     reply port in use by the thread when interrupted.

     We cannot use the original reply port for our RPCs that we do here, since
     we could unexpectedly receive/consume a reply message meant for the user
     (in particular, msg_sig_post_reply), and also since we would deallocate
     the port if *our* RPC fails, which we don't want to do since the user
     still has the old name.  And so, temporarily set MACH_PORT_DEAD as our
     reply name, and make sure destroying the port is the very last RPC we
     do.  */
  reply_port = THREAD_GETMEM (THREAD_SELF, reply_port);
  THREAD_SETMEM (THREAD_SELF, reply_port, MACH_PORT_DEAD);
  if (__glibc_likely (MACH_PORT_VALID (reply_port)))
    (void) __mach_port_mod_refs (__mach_task_self (), reply_port,
                                 MACH_PORT_RIGHT_RECEIVE, -1);
  THREAD_SETMEM (THREAD_SELF, reply_port, scp->sc_reply_port);

  /* Restore thread state.  */
  err = __thread_set_self_state (AARCH64_FLOAT_STATE,
				 (thread_state_t) &scp->sc_aarch64_float_state,
				 AARCH64_FLOAT_STATE_COUNT);
  assert_perror (err);
  err = __thread_set_self_state (AARCH64_THREAD_STATE,
				 (thread_state_t) &scp->sc_aarch64_thread_state,
				 AARCH64_THREAD_STATE_COUNT);
  assert_perror (err);
  __builtin_unreachable ();
}

int
__sigreturn (struct sigcontext *scp)
{
  struct hurd_sigstate *ss;
  struct hurd_userlink *link = (void *) &scp[1];
  uintptr_t usp;

  if (__glibc_unlikely (scp == NULL || (scp->sc_mask & _SIG_CANT_MASK)))
    return __hurd_fail (EINVAL);

  ss = _hurd_self_sigstate ();
  _hurd_sigstate_lock (ss);

  /* Remove the link on the `active resources' chain added by
     _hurd_setup_sighandler.  Its purpose was to make sure
     that we got called; now we have, it is done.  */
  _hurd_userlink_unlink (link);

  /* Restore the set of blocked signals, and the intr_port slot.  */
  ss->blocked = scp->sc_mask;
  ss->intr_port = scp->sc_intr_port;

  /* Check for pending signals that were blocked by the old set.  */
  if (_hurd_sigstate_pending (ss) & ~ss->blocked)
    {
      /* There are pending signals that just became unblocked.  Wake up the
	 signal thread to deliver them.  But first, squirrel away SCP where
	 the signal thread will notice it if it runs another handler, and
	 arrange to have us called over again in the new reality.  */
      ss->context = scp;
      _hurd_sigstate_unlock (ss);
      __msg_sig_post (_hurd_msgport, 0, 0, __mach_task_self ());
      /* If a pending signal was handled, sig_post never returned.
	 If it did return, the pending signal didn't run a handler;
	 proceed as usual.  */
      _hurd_sigstate_lock (ss);
      ss->context = NULL;
    }

  if (scp->sc_onstack)
    ss->sigaltstack.ss_flags &= ~SS_ONSTACK;

  /* Copy the signal context onto the user's stack, to be able to release the
     altstack (by unlocking sigstate).  Note that unless an altstack is used,
     the sigcontext will itself be located on the user's stack, so we may well
     be overwriting it here (or later in __sigreturn2).  */

  usp = ALIGN_DOWN(scp->sc_sp - sizeof (struct sigcontext), 16);
  memmove ((void *) usp, scp, sizeof (struct sigcontext));

  /* Switch to the user's stack that we have just prepared, and call
     __sigreturn2.  Clobber "memory" to make sure GCC flushes the stack
     setup to actual memory.  */
  {
    register uintptr_t usp_x0 asm("x0") = usp;
    register struct hurd_sigstate *ss_x1 asm("x1") = ss;

    asm volatile ("mov sp, %0\n"
		  "b __sigreturn2"
		  :: "r" (usp_x0), "r" (ss_x1)
		  : "memory");
    __builtin_unreachable ();
  }
}

weak_alias (__sigreturn, sigreturn)
