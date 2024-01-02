/* Set thread_state for sighandler, and sigcontext to recover.  AArch64 version.
   Copyright (C) 1994-2024 Free Software Foundation, Inc.
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

#include <hurd/signal.h>
#include <hurd/userlink.h>
#include <thread_state.h>
#include <mach/exception.h>
#include <assert.h>
#include <errno.h>
#include "hurdfault.h"
#include <sys/ucontext.h>


/* Fill in a siginfo_t structure for SA_SIGINFO-enabled handlers.  */
static void fill_siginfo (siginfo_t *si, int signo,
			  const struct hurd_signal_detail *detail,
			  const struct machine_thread_all_state *state)
{
  si->si_signo = signo;
  si->si_errno = detail->error;
  si->si_code = detail->code;

  /* XXX We would need a protocol change for sig_post to include
   * this information.  */
  si->si_pid = -1;
  si->si_uid = -1;

  /* Address of the faulting instruction or memory access.  */
  if (detail->exc == EXC_BAD_ACCESS)
    si->si_addr = (void *) detail->exc_subcode;
  else
    si->si_addr = (void *) state->basic.pc;

  /* XXX On SIGCHLD, this should be the exit status of the child
   * process.  We would need a protocol change for the proc server
   * to send this information along with the signal.  */
  si->si_status = 0;

  si->si_band = 0;              /* SIGPOLL is not supported yet.  */
  si->si_value.sival_int = 0;   /* sigqueue() is not supported yet.  */
}

/* Fill in a ucontext_t structure SA_SIGINFO-enabled handlers.  */
static void fill_ucontext (ucontext_t *uc, const struct sigcontext *sc)
{
  uc->uc_flags = 0;
  uc->uc_link = NULL;
  uc->uc_sigmask = sc->sc_mask;
  uc->uc_stack.ss_sp = (__ptr_t) sc->sc_sp;
  uc->uc_stack.ss_size = 0;
  uc->uc_stack.ss_flags = 0;

  memcpy (&uc->uc_mcontext.gregs, &sc->sc_aarch64_thread_state,
	  sizeof (struct aarch64_thread_state));
  memcpy (&uc->uc_mcontext.fpregs, &sc->sc_aarch64_float_state,
	  sizeof (struct aarch64_float_state));
}

struct sigcontext *
_hurd_setup_sighandler (struct hurd_sigstate *ss, const struct sigaction *action,
			__sighandler_t handler,
			int signo, struct hurd_signal_detail *detail,
			int rpc_wait,
			struct machine_thread_all_state *state)
{
  void trampoline (void) attribute_hidden;
  void rpc_wait_trampoline (void) attribute_hidden;
  void *sigsp;
  struct sigcontext *scp;
  struct
    {
      union
        {
          int signo;
          /* Make sure signo takes up a pointer-sized slot on the stack.
             (This should already be the case considering the siginfop
             pointer below, but better be explicit.)  */
          void *_pointer_sized;
        };
      union
	{
	  /* Extra arguments for traditional signal handlers */
	  struct
	    {
	      long int sigcode;
	      struct sigcontext *scp;       /* Points to ctx, below.  */
	    } legacy;

	  /* Extra arguments for SA_SIGINFO handlers */
	  struct
	    {
	      siginfo_t *siginfop;          /* Points to siginfo, below.  */
	      ucontext_t *uctxp;            /* Points to uctx, below.  */
	    } posix;
	};

      void *_pad;

      /* NB: sigreturn assumes link is next to ctx.  */
      struct sigcontext ctx;
      struct hurd_userlink link;
      ucontext_t ucontext;
      siginfo_t siginfo;
    } *stackframe;

  if (ss->context)
    {
      /* We have a previous sigcontext that sigreturn was about
	 to restore when another signal arrived.  We will just base
	 our setup on that.  */
      if (! _hurdsig_catch_memory_fault (ss->context))
	{
	  memcpy (&state->basic, &ss->context->sc_aarch64_thread_state,
		  sizeof (state->basic));
	  memcpy (&state->fpu, &ss->context->sc_aarch64_float_state,
		  sizeof (state->fpu));
	  state->set |= (1 << AARCH64_THREAD_STATE) | (1 << AARCH64_FLOAT_STATE);
	}
    }

  if (! machine_get_basic_state (ss->thread, state))
    return NULL;

  if ((action->sa_flags & SA_ONSTACK)
      && !(ss->sigaltstack.ss_flags & (SS_DISABLE|SS_ONSTACK)))
    {
      sigsp = ss->sigaltstack.ss_sp + ss->sigaltstack.ss_size;
      ss->sigaltstack.ss_flags |= SS_ONSTACK;
    }
  else
    /* No red zone on aarch64-gnu.  */
    sigsp = (void *) state->basic.sp;

  /* Push the arguments to call `trampoline' on the stack.
     Note that user's SP doesn't strictly have to be 16-aligned, since
     AArch64 only requires 16-alignment for the stack pointer when
     making accesses relative to it.  */
  sigsp = PTR_ALIGN_DOWN (sigsp - sizeof (*stackframe), 16);
  stackframe = sigsp;

  _Static_assert ((void *) (&stackframe->_pad + 1) == (void *) &stackframe->ctx,
		  "trampoline expects no extra padding between "
		  "_pad and ctx");

  if (_hurdsig_catch_memory_fault (stackframe))
    {
      /* We got a fault trying to write the stack frame.
	 We cannot set up the signal handler.
	 Returning NULL tells our caller, who will nuke us with a SIGILL.  */
      return NULL;
    }
  else
    {
      int ok;

      extern void _hurdsig_longjmp_from_handler (void *, jmp_buf, int);

      /* Add a link to the thread's active-resources list.  We mark this as
	 the only user of the "resource", so the cleanup function will be
	 called by any longjmp which is unwinding past the signal frame.
	 The cleanup function (in sigunwind.c) will make sure that all the
	 appropriate cleanups done by sigreturn are taken care of.  */
      stackframe->link.cleanup = &_hurdsig_longjmp_from_handler;
      stackframe->link.cleanup_data = &stackframe->ctx;
      stackframe->link.resource.next = NULL;
      stackframe->link.resource.prevp = NULL;
      stackframe->link.thread.next = ss->active_resources;
      stackframe->link.thread.prevp = &ss->active_resources;
      if (stackframe->link.thread.next)
	stackframe->link.thread.next->thread.prevp
	  = &stackframe->link.thread.next;
      ss->active_resources = &stackframe->link;

      /* Set up the sigcontext from the current state of the thread.  */

      scp = &stackframe->ctx;
      scp->sc_onstack = ss->sigaltstack.ss_flags & SS_ONSTACK ? 1 : 0;

      /* struct sigcontext is laid out so that starting at sc_x[0] mimics a
	 struct aarch64_thread_state.  */
      _Static_assert (offsetof (struct sigcontext, sc_aarch64_thread_state)
		      % __alignof__ (struct aarch64_thread_state) == 0,
		      "sc_aarch64_thread_state layout mismatch");
      memcpy (&scp->sc_aarch64_thread_state,
	      &state->basic, sizeof (state->basic));

      /* struct sigcontext is laid out so that starting at sc_v[0] mimics a
	 struct aarch64_float_state.  */
      _Static_assert (offsetof (struct sigcontext, sc_aarch64_float_state)
		      % __alignof__ (struct aarch64_float_state) == 0,
		      "sc_aarch64_float_state layout mismatch");
      ok = machine_get_state (ss->thread, state, AARCH64_FLOAT_STATE,
			      &state->fpu, &scp->sc_aarch64_float_state,
			      sizeof (state->fpu));

      /* Set up the arguments for the signal handler.  */
      stackframe->signo = signo;
      if (action->sa_flags & SA_SIGINFO)
	{
	  stackframe->posix.siginfop = &stackframe->siginfo;
	  stackframe->posix.uctxp = &stackframe->ucontext;
	  fill_siginfo (&stackframe->siginfo, signo, detail, state);
	  fill_ucontext (&stackframe->ucontext, scp);
	}
      else
	{
	  if (detail->exc)
	    {
	      int nsigno;
	      _hurd_exception2signal_legacy (detail, &nsigno);
	      assert (nsigno == signo);
	    }
	  else
	    detail->code = 0;

	  stackframe->legacy.sigcode = detail->code;
	  stackframe->legacy.scp = &stackframe->ctx;
	}

      _hurdsig_end_catch_fault ();

      if (!ok)
	return NULL;
    }

  /* Modify the thread state to call the trampoline code on the new stack.  */
  state->basic.sp = (uintptr_t) sigsp;

  if (rpc_wait)
    {
      /* The signalee thread was blocked in a mach_msg_trap system call,
         still waiting for a reply.  We will have it run the special
         trampoline code which retries the message receive before running
         the signal handler.

         To do this we change the OPTION argument (in x1) to enable only
         message reception, since the request message has already been
         sent.  */

      assert (state->basic.x[1] & MACH_RCV_MSG);
      /* Disable the message-send, since it has already completed.  The
         calls we retry need only wait to receive the reply message.  */
      state->basic.x[1] &= ~MACH_SEND_MSG;

      /* Limit the time to receive the reply message, in case the server
         claimed that `interrupt_operation' succeeded but in fact the RPC
         is hung.  */
      state->basic.x[1] |= MACH_RCV_TIMEOUT;
      state->basic.x[5] = _hurd_interrupted_rpc_timeout;

      state->basic.pc = (uintptr_t) rpc_wait_trampoline;
      /* After doing the message receive, the trampoline code will need to
         update the x0 value to be restored by sigreturn.  To simplify
         the assembly code, we pass the address of its slot in SCP to the
         trampoline code in x21.  */
      state->basic.x[21] = (uintptr_t) &scp->sc_x[0];
    }
  else
    state->basic.pc = (uintptr_t) trampoline;

  /* We pass the handler function to the trampoline code in x20.  */
  state->basic.x[20] = (uintptr_t) handler;

  /* Clear pstate, notably reset BTYPE to 0.  */
  state->basic.cpsr = 0;

  return scp;
}

asm ("rpc_wait_trampoline:\n"
  /* This is the entry point when we have an RPC reply message to receive
     before running the handler.  The MACH_MSG_SEND bit has already been
     cleared in the OPTION argument in our x1.  For our convenience, x21
     points to the sc_x[0] member of the sigcontext.

     When the sigcontext was saved, x0 was MACH_RCV_INTERRUPTED.  To repeat
     the message call, we need to restore the original message buffer
     pointer; intr-msg.h keeps a backup copy of it in x9 specifically for
     this purpose.  */
     "mov x0, x9\n"
     "svc #0\n"
     /* Now the message receive has completed and the original caller of
        the RPC (i.e. the code running when the signal arrived) needs to
        see the final return value of the message receive in x0.  So store
        the new x0 value into the sc_x[0] member of the sigcontext (whose
        address is in x21 to make this code simpler).  */
     "str x0, [x21]\n"

     "trampoline:\n"
     /* Entry point for running the handler normally.  The arguments to the
        handler function are on the top of the stack:

        [sp]		SIGNO
        [sp, #8]	SIGCODE
        [sp, #16]	SCP
        [sp, #24]	_pad

        Pop them off to the registers, to pass as arguments to the handler.
     */
     "ldp x0, x1, [sp], #16\n"
     "ldr x2, [sp], #16\n"
     /* Call handler (signo, sigcode, scp).  Note that this is an indirect
        call not using x16/x17, so this requires the signal handler to start
        with a proper "bti c" marker.  */
     "blr x20\n"
     /* Call __sigreturn (), passing &CTX as SCP.  CTX is on the top of
        the stack.  If __sigreturn () fails, we're in trouble.  */
     "mov x0, sp\n"
     "bl __sigreturn\n"
     "udf #0xdead");
