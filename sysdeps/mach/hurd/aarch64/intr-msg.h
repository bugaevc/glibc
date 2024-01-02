/* Machine-dependent details of interruptible RPC messaging.  AArch64 version.
   Copyright (C) 1995-2024 Free Software Foundation, Inc.
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


/* Note that we must mark OPTION and TIMEOUT as outputs of this operation,
   to indicate that the signal thread might mutate them as part
   of sending us to a signal handler.  */

#define INTR_MSG_TRAP(msg, option, send_size, rcv_size, rcv_name, timeout, notify, cancel_p, intr_port_p) \
({									      \
  register uintptr_t /* error_t */ err asm ("x0");			      \
  register uintptr_t option_x1 asm ("x1") = option;			      \
  register uintptr_t send_size_x2 asm ("x2") = send_size;		      \
  register uintptr_t rcv_size_x3 asm ("x3") = rcv_size;			      \
  register uintptr_t rcv_name_x4 asm ("x4") = rcv_name;			      \
  register uintptr_t timeout_x5 asm ("x5") = timeout;			      \
  register uintptr_t notify_x6 asm ("x6") = notify;			      \
  register void *msg_x9 asm ("x9") = msg;  /* used by trampoline.c */	      \
  asm volatile ("\n"							      \
       ".globl _hurd_intr_rpc_msg_about_to\n"				      \
       ".globl _hurd_intr_rpc_msg_setup_done\n"				      \
       ".globl _hurd_intr_rpc_msg_in_trap\n"				      \
       /* Clear x0 before we do the check for cancel below.  This is to
          detect x0 being set to non-zero (actually MACH_SEND_INTERRUPTED)
          from the outside (namely, _hurdsig_abort_rpcs), which signals us
          to skip the trap we were about to enter.  */			      \
       "	mov %[err], #0\n"					      \
       "_hurd_intr_rpc_msg_about_to:\n"					      \
       /* We need to make a last check of cancel, in case we got interrupted
          right before _hurd_intr_rpc_msg_about_to.  */			      \
       "	ldr w8, %[cancel]\n"					      \
       "	cbz w8, _hurd_intr_rpc_msg_do\n"			      \
       /* We got interrupted, note so and return EINTR.  */		      \
       "	str wzr, %[intr_port]\n"				      \
       "	mov %[err], %[eintr_lo]\n"				      \
       "	movk %[err], %[eintr_hi], lsl 16\n"			      \
       "	b _hurd_intr_rpc_msg_sp_restored\n"			      \
       "_hurd_intr_rpc_msg_do:\n"					      \
       /* Ok, prepare the mach_msg_trap arguments.  We pass all the 7 args
          in registers.  */						      \
       "_hurd_intr_rpc_msg_setup_done:\n"				      \
       /* From here on, it is safe to make us jump over the syscall.  Now
          check if we have been told to skip the syscall while running
          the above.  */						      \
       "	cbnz %[err], _hurd_intr_rpc_msg_in_trap\n"		      \
       /* Do the actual syscall.  */					      \
       "	mov %[err], %[msg]\n"					      \
       "	mov x8, #-25\n"						      \
       "_hurd_intr_rpc_msg_do_trap:\n"					      \
       "	svc #0\n" /* status in %[err] */			      \
       "_hurd_intr_rpc_msg_in_trap:\n"					      \
       "_hurd_intr_rpc_msg_sp_restored:\n"				      \
       : [err] "=r" (err), "+r" (option_x1),				      \
         [intr_port] "=m" (*intr_port_p),				      \
         "+r" (timeout_x5)						      \
       : [msg] "r" (msg_x9), "r" (send_size_x2), "r" (rcv_size_x3),	      \
         "r" (rcv_name_x4), "r" (notify_x6),				      \
         [cancel] "m" (*cancel_p),					      \
         [eintr_lo] "i" (EINTR & 0xffff), [eintr_hi] "i" (EINTR >> 16));      \
  option = option_x1;							      \
  timeout = timeout_x5;							      \
  err;									      \
})

#include "hurdfault.h"

/* This cannot be an inline function because it calls setjmp.  */
#define SYSCALL_EXAMINE(state, callno)					      \
({									      \
  int result;								      \
  unsigned int *p = (unsigned int *) (state)->pc - 4;			      \
  if (_hurdsig_catch_memory_fault (p))					      \
    return 0;								      \
  if (result = (*p == 0xd4000001))					      \
    /* The PC is just after a "svc #0" instruction.
       This is a system call in progress; x8 holds the call number.  */	      \
    *(callno) = (state)->x[8];						      \
  _hurdsig_end_catch_fault ();						      \
  result;								      \
})


/* This cannot be an inline function because it calls setjmp.  */
#define MSG_EXAMINE(state, msgid, rcvname, send_name, opt, tmout)	      \
({									      \
  int ret = 0;								      \
  const struct machine_thread_state *s = (state);			      \
  const mach_msg_header_t *msg = (const void *) s->x[0];		      \
  *(opt) = s->x[1];							      \
  *(rcvname) = s->x[4];							      \
  *(tmout) = s->x[5];							      \
  if (msg == 0)								      \
    {									      \
      *(send_name) = MACH_PORT_NULL;					      \
      *(msgid) = 0;							      \
    }									      \
  else									      \
    {									      \
      ret = _hurdsig_catch_memory_fault (msg) ? -1 : 0;			      \
      if (ret == 0)							      \
        {								      \
          *(send_name) = msg->msgh_remote_port;				      \
          *(msgid) = msg->msgh_id;					      \
          _hurdsig_end_catch_fault ();					      \
        }								      \
    }									      \
  ret;									      \
})
