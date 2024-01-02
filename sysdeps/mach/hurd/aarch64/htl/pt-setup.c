/* Setup thread stack.  Hurd/AArch64 version.
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

#include <stdint.h>
#include <assert.h>
#include <mach.h>
#include <hurd.h>

#include <thread_state.h>
#include <pt-internal.h>

/* Set up the stack for THREAD.  Return the stack pointer
   for the new thread.  */
static void *
stack_setup (struct __pthread *thread)
{
  error_t err;
  uintptr_t bottom, top;

  /* Calculate the top of the new stack.  */
  bottom = (uintptr_t) thread->stackaddr;
  top = bottom + thread->stacksize + round_page (thread->guardsize);

  if (thread->guardsize)
    {
      err = __vm_protect (__mach_task_self (), (vm_address_t) bottom,
			  thread->guardsize, 0, 0);
      assert_perror (err);
    }

  return (void *) top;
}

int
__pthread_setup (struct __pthread *thread,
		 void (*entry_point) (struct __pthread *, void *(*)(void *),
				      void *),
		 void *(*start_routine) (void *),
		 void *arg)
{
  struct aarch64_thread_state state;

  if (thread->kernel_thread == __hurd_thread_self ())
    return 0;

  thread->mcontext.pc = entry_point;
  thread->mcontext.sp = stack_setup (thread);

  /* Set up the state to call entry_point (thread, start_routine, arg) */
  memset (&state, 0, sizeof (state));
  state.sp = (uintptr_t) thread->mcontext.sp;
  state.pc = (uintptr_t) thread->mcontext.pc;
  state.x[0] = (uintptr_t) thread;
  state.x[1] = (uintptr_t) start_routine;
  state.x[2] = (uintptr_t) arg;
  state.tpidr_el0 = (uintptr_t) thread->tcb;

  return __thread_set_state (thread->kernel_thread, AARCH64_THREAD_STATE,
			     (thread_state_t) &state,
			     AARCH64_THREAD_STATE_COUNT);
}
