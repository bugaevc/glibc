/* Startup code for statically linked Hurd/AArch64 binaries.
   Copyright (C) 1998-2024 Free Software Foundation, Inc.
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

#include <sysdep.h>

/* This is the actual entry point for statically linked aarch64-gnu executables,
   the very first code to run in a process.  */

	.text
ENTRY(_start)
	/* Set up the initial stack frame.  */
	cfi_undefined (x30)
	mov	x29, #0
	mov	x30, #0

	/* Pre-fill GOT entries for select ifunc routines that may get
	   called during _hurd_stack_setup () with baseline implementations.  */
	adrp	x1, __memcpy_generic
	add	x1, x1, #:lo12:__memcpy_generic
	adrp	x0, :got:memcpy
	str	x1, [x0, :got_lo12:memcpy]

	adrp	x1, __strlen_generic
	add	x1, x1, #:lo12:__strlen_generic
	adrp	x0, :got:strlen
	str	x1, [x0, :got_lo12:strlen]

	mov	x0, sp
	bl	_hurd_stack_setup

	/* Jump to the regular entry point.  */
	mov	x0, #0
	b	_start1
END(_start)

#define _start _start1
#include <sysdeps/aarch64/start.S>
