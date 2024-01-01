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

#ifndef _MACH_AARCH64_SYSDEP_H
#define _MACH_AARCH64_SYSDEP_H 1

/* Defines RTLD_PRIVATE_ERRNO and USE_DL_SYSINFO.  */
#include <dl-sysdep.h>
#include <tls.h>
/* Get the hwcap definitions.  */
#include <mach/aarch64/mach_aarch64_types.h>

#define LOSE asm volatile ("udf #0xdead")

#define STACK_GROWTH_DOWN

/* Get the machine-independent Mach definitions.  */
#include <sysdeps/mach/sysdep.h>

#undef ENTRY
#undef ALIGN

#define RETURN_TO(sp, pc, retval)					      \
do									      \
  {									      \
    register long __rv asm ("x0") = (retval);				      \
    asm volatile ("mov sp, %0\n\t"					      \
		  "ret %1"						      \
		  :: "r" (sp), "r" (pc), "r" (__rv));			      \
    __builtin_unreachable ();						      \
  }									      \
while (0)

#include <sysdeps/aarch64/sysdep.h>
#include <sysdeps/unix/sysdep.h>

#endif /* mach/aarch64/sysdep.h */
