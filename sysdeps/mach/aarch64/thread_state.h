/* Mach thread state definitions for machine-independent code.  AArch64 version.
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

#ifndef _MACH_AARCH64_THREAD_STATE_H
#define _MACH_AARCH64_THREAD_STATE_H 1

#include <mach/machine/thread_status.h>
#include <libc-pointer-arith.h>

#define MACHINE_NEW_THREAD_STATE_FLAVOR	AARCH64_THREAD_STATE
#define MACHINE_THREAD_STATE_FLAVOR	AARCH64_THREAD_STATE
#define MACHINE_THREAD_STATE_COUNT	AARCH64_THREAD_STATE_COUNT

#define machine_thread_state aarch64_thread_state

#define PC pc
#define SP sp
#define SYSRETURN x[0]

#define MACHINE_THREAD_STATE_SETUP_CALL(ts, stack, size, func)		      \
  ((ts)->sp = PTR_ALIGN_DOWN ((uintptr_t) (stack) + (size), 16),	      \
   (ts)->pc = (uintptr_t) func,						      \
   (ts)->cpsr = 0x0)	/* notably, reset BTYPE */

struct machine_thread_all_state
  {
    struct aarch64_thread_state basic;
    struct aarch64_float_state fpu;
    int set;
  };

#include <sysdeps/mach/thread_state.h>

#endif /* mach/aarch64/thread_state.h */
