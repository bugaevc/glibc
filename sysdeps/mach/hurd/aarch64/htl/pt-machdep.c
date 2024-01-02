/* Machine dependent pthreads code.  Hurd/AArch64 version.
   Copyright (C) 2000-2024 Free Software Foundation, Inc.
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
   License along with the GNU C Library;  if not, see
   <https://www.gnu.org/licenses/>.  */

#include <errno.h>
#include <assert.h>

#include <mach.h>
#include <mach/machine/thread_status.h>
#include <mach/mig_errors.h>
#include <mach/thread_status.h>

int
__thread_set_pcsptp (thread_t thread,
                     int set_pc, void *pc,
                     int set_sp, void *sp,
                     int set_tp, void *tp)
{
  error_t err;
  struct aarch64_thread_state state;
  mach_msg_type_number_t state_count;

  state_count = AARCH64_THREAD_STATE_COUNT;

  err = __thread_get_state (thread, AARCH64_THREAD_STATE,
                            (thread_state_t) &state, &state_count);
  if (err)
    return err;
  assert (state_count == AARCH64_THREAD_STATE_COUNT);

  if (set_pc)
    state.pc = (uintptr_t) pc;
  if (set_sp)
    state.sp = (uintptr_t) sp;
  if (set_tp)
    state.tpidr_el0 = (uintptr_t) tp;

  return __thread_set_state (thread, AARCH64_THREAD_STATE,
			     (thread_state_t) &state,
			     AARCH64_THREAD_STATE_COUNT);
}
