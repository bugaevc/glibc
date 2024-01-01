/* Copyright (C) 1998-2024 Free Software Foundation, Inc.

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

/* System V/AArch64 ABI compliant context switching support.  */

#ifndef _SYS_UCONTEXT_H
#define _SYS_UCONTEXT_H	1

#include <features.h>

#include <bits/types/sigset_t.h>
#include <bits/types/stack_t.h>

#ifdef __USE_MISC
# define __ctx(fld) fld
#else
# define __ctx(fld) __ ## fld
#endif

/* Type for general register.  */
__extension__ typedef long long int greg_t;

/* gregset_t is laid out to match mach/aarch64/thread_status.h:struct aarch64_thread_state */
typedef struct
{
  greg_t __ctx(x)[31];
  greg_t __ctx(sp);
  greg_t __ctx(pc);
  greg_t __ctx(tpidr_el0);
  unsigned long __ctx(cpsr);
} gregset_t;

/* fpregset_t is laid out to match mach/aarch64/thread_status.h:struct aarch64_float_state */
typedef struct
{
  __int128_t __ctx(v)[32];
  unsigned long __ctx(fpsr);
  unsigned long __ctx(fpcr);
} fpregset_t;

typedef struct
{
  gregset_t __ctx(gregs);
  fpregset_t __ctx(fpregs);
} mcontext_t;

typedef struct ucontext_t
{
  unsigned long __ctx(uc_flags);
  struct ucontext_t *uc_link;
  stack_t uc_stack;
  sigset_t uc_sigmask;
  mcontext_t uc_mcontext;
} ucontext_t;

#undef __ctx

#endif /* sys/ucontext.h */
