/* Perform a `longjmp' on a Mach thread_state.  AArch64 version.
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

#include <setjmp.h>
#include <jmpbuf-offsets.h>
#include <mach/thread_status.h>


/* Set up STATE to do the equivalent of `longjmp (ENV, VAL);'.  */

void
_hurd_longjmp_thread_state (void *state, jmp_buf env, int val)
{
  struct aarch64_thread_state *ts = state;

  ts->x[19] = env[0].__jmpbuf[JB_X19];
  ts->x[20] = env[0].__jmpbuf[JB_X20];
  ts->x[21] = env[0].__jmpbuf[JB_X21];
  ts->x[22] = env[0].__jmpbuf[JB_X22];
  ts->x[23] = env[0].__jmpbuf[JB_X23];
  ts->x[24] = env[0].__jmpbuf[JB_X24];
  ts->x[25] = env[0].__jmpbuf[JB_X25];
  ts->x[26] = env[0].__jmpbuf[JB_X26];
  ts->x[27] = env[0].__jmpbuf[JB_X27];
  ts->x[28] = env[0].__jmpbuf[JB_X28];
  ts->x[29] = env[0].__jmpbuf[JB_X29];

  /* XXX: We're ignoring all the d[] (SIMD) registers.
     Is that fine?  */

  ts->pc = PTR_DEMANGLE (env[0].__jmpbuf[JB_LR]);
  ts->sp = _jmpbuf_sp (env[0].__jmpbuf);
  ts->x[0] = val ?: 1;
}
