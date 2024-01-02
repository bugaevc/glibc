/* Translate Mach exception codes into signal numbers.  AArch64 version.
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

#include <hurd.h>
#include <hurd/signal.h>
#include <mach/exception.h>

/* Translate the Mach exception codes, as received in an `exception_raise' RPC,
   into a signal number and signal subcode.  */

static void
exception2signal (struct hurd_signal_detail *detail, int *signo, int posix)
{
  detail->error = 0;

  switch (detail->exc)
    {
    default:
      *signo = SIGIOT;
      detail->code = detail->exc;
      break;

    case EXC_BAD_ACCESS:
      switch (detail->exc_code)
        {
	case KERN_INVALID_ADDRESS:
	case KERN_MEMORY_FAILURE:
	  *signo = SIGSEGV;
	  detail->code = posix ? SEGV_MAPERR : detail->exc_subcode;
	  break;

	case KERN_PROTECTION_FAILURE:
	case KERN_WRITE_PROTECTION_FAILURE:
	  *signo = SIGSEGV;
	  detail->code = posix ? SEGV_ACCERR : detail->exc_subcode;
	  break;

	case EXC_AARCH64_MTE:
	  *signo = SIGSEGV;
	  /* TODO: Should be SEGV_MTESERR */
	  detail->code = posix ? SEGV_ACCERR : detail->exc_subcode;
	  break;

	case EXC_AARCH64_BTI:
	  *signo = SIGILL;
	  /* XXX: Consider adopting ILL_BTCFI from OpenBSD.  */
	  /* XXX: exc_subcode / signal code contains BTYPE */
	  detail->code = posix ? ILL_ILLOPN : detail->exc_subcode;
	  break;

	case EXC_AARCH64_AL:
	case EXC_AARCH64_AL_PC:
	case EXC_AARCH64_AL_SP:
	  *signo = SIGBUS;
	  detail->code = posix ? BUS_ADRALN : detail->exc_subcode;
	  break;

	default:
	  *signo = SIGBUS;
	  detail->code = posix ? BUS_ADRERR : detail->exc_subcode;
	  break;
	}
      detail->error = detail->exc_code;
      break;

    case EXC_BAD_INSTRUCTION:
      *signo = SIGILL;
      switch (detail->exc_code)
        {
        case EXC_AARCH64_SVC:
          detail->code = posix ? ILL_ILLTRP : 0;
          break;

	default:
	  detail->code = posix ? ILL_ILLOPC : 0;
	  break;
        }
      break;

    case EXC_ARITHMETIC:
      *signo = SIGFPE;
      switch (detail->exc_code)
	{
	case EXC_AARCH64_IDF:
	  detail->code = posix ? FPE_FLTIDO : 0;
	  break;
	case EXC_AARCH64_IXF:
	  detail->code = posix ? FPE_FLTRES : FPE_FLTINX_FAULT;
	  break;
	case EXC_AARCH64_UFF:
	  detail->code = posix ? FPE_FLTUND : FPE_FLTDNR_FAULT;
	  break;
	case EXC_AARCH64_OFF:
	  detail->code = posix ? FPE_FLTOVF : FPE_FLTOVF_FAULT;
	  break;
	case EXC_AARCH64_DZF:
	  detail->code = posix ? FPE_FLTDIV : FPE_FLTDIV_FAULT;
	  break;
	case EXC_AARCH64_IOF:
	  /* NB: We used to send SIGILL here but we can't distinguish
	     POSIX vs. legacy with respect to what signal we send.  */
	  detail->code = posix ? FPE_FLTINV : 0 /*ILL_FPEOPR_FAULT*/;
	  break;
	default:
	  detail->code = 0;
	}
      break;

    case EXC_EMULATION:
    case EXC_SOFTWARE:
      *signo = SIGEMT;
      detail->code = 0;
      break;


    case EXC_BREAKPOINT:
      *signo = SIGTRAP;
      detail->code = posix ? TRAP_BRKPT : 0;
      break;
    }
}
libc_hidden_def (_hurd_exception2signal)

void
_hurd_exception2signal (struct hurd_signal_detail *detail, int *signo)
{
  exception2signal (detail, signo, 1);
}

void
_hurd_exception2signal_legacy (struct hurd_signal_detail *detail, int *signo)
{
  exception2signal (detail, signo, 0);
}
