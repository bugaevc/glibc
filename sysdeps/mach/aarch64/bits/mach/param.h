/* Old-style Unix parameters and limits.  aarch64 Mach version.
   Copyright (C) 1993-2024 Free Software Foundation, Inc.
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

#ifndef _SYS_PARAM_H
# error "Never use <bits/mach/param.h> directly; include <sys/param.h> instead."
#endif

/* There is no EXEC_PAGESIZE.  Use vm_page_size or getpagesize ()
   or sysconf (_SC_PAGESIZE) instead.  */
