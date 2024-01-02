/* Machine-dependent signal context structure for GNU Hurd.  AArch64 version.
   Copyright (C) 1991-2024 Free Software Foundation, Inc.
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

#ifndef _BITS_SIGCONTEXT_H
#define _BITS_SIGCONTEXT_H 1

#if !defined _SIGNAL_H && !defined _SYS_UCONTEXT_H
# error "Never use <bits/sigcontext.h> directly; include <signal.h> instead."
#endif

/* Signal handlers are actually called:
   void handler (int sig, int code, struct sigcontext *scp);  */

#include <bits/types/__sigset_t.h>
#include <mach/machine/fp_reg.h>

/* State of this thread when the signal was taken.  */
struct sigcontext
  {
    /* These first members are machine-independent.  */

    int sc_onstack;		/* Nonzero if running on sigstack.  */
    __sigset_t sc_mask;		/* Blocked signals to restore.  */

    /* MiG reply port this thread is using.  */
    unsigned int sc_reply_port;

    /* Port this thread is doing an interruptible RPC on.  */
    unsigned int sc_intr_port;

    /* Error code associated with this signal (interpreted as `error_t').  */
    int sc_error;

    /* Make sure the below members are properly aligned, and not packed
       together with sc_error -- otherwise the layout won't match that of
       aarch64_thread_state.  */
    int sc_pad1;

    /* All following members are machine-dependent.  The rest of this
       structure is written to be laid out identically to:
       {
	 struct aarch64_thread_state basic;
	 struct aarch64_float_state fpu;
       }
       trampoline.c knows this, so it must be changed if this changes.  */

#define sc_aarch64_thread_state sc_x[0] /* Beginning of correspondence.  */
    long sc_x[31];
    long sc_sp;
    long sc_pc;
    long sc_tpidr_el0;
    long sc_cpsr;

#define sc_aarch64_float_state sc_v[0]
    __int128_t sc_v[32];
    long sc_fpsr;
    long sc_fpcr;
  };

/* Traditional BSD names for some members.  */
#define sc_fp	sc_x[29]	/* Frame pointer.  */
#define sc_ps	sc_cpsr


/* The deprecated sigcode values below are passed as an extra, non-portable
   argument to regular signal handlers.  You should use SA_SIGINFO handlers
   instead, which use the standard POSIX signal codes.  */

/* Codes for SIGFPE.  */
#define FPE_INTOVF_TRAP		0x1 /* integer overflow */
#define FPE_INTDIV_FAULT	0x2 /* integer divide by zero */
#define FPE_FLTOVF_FAULT	0x3 /* floating overflow */
#define FPE_FLTDIV_FAULT	0x4 /* floating divide by zero */
#define FPE_FLTUND_FAULT	0x5 /* floating underflow */
#define FPE_SUBRNG_FAULT	0x7 /* BOUNDS instruction failed */
#define FPE_FLTDNR_FAULT	0x8 /* denormalized operand */
#define FPE_FLTINX_FAULT	0x9 /* floating loss of precision */
#define FPE_EMERR_FAULT		0xa /* mysterious emulation error 33 */
#define FPE_EMBND_FAULT		0xb /* emulation BOUNDS instruction failed */

#endif /* bits/sigcontext.h */
