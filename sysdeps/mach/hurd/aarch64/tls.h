/* Copyright (C) 2005-2024 Free Software Foundation, Inc.

   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation; either version 2.1 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */

#ifndef _AARCH64_TLS_H
#define _AARCH64_TLS_H	1

/* Some things really need not be machine-dependent.  */
#include <sysdeps/mach/hurd/tls.h>

#include <dl-sysdep.h>

#ifndef __ASSEMBLER__
# include <assert.h>
# include <stdbool.h>
# include <stddef.h>
# include <stdint.h>
# include <dl-dtv.h>
# include <errno.h>
# include <thread_state.h>
#endif /* __ASSEMBLER__ */

#ifndef __ASSEMBLER__

/* Get system call information.  */
# include <sysdep.h>

/* The TP points to the start of the thread blocks.  */
# define TLS_DTV_AT_TP	1
# define TLS_TCB_AT_TP	0

typedef struct
{
  /* Used by the exception handling implementation in the dynamic loader.  */
  struct rtld_catch *rtld_catch;

  struct hurd_sigstate *_hurd_sigstate;
  mach_port_t reply_port;      /* This thread's reply port.  */

  int gscope_flag;
} tcbprehead_t;

typedef struct
{
  dtv_t *dtv;
  void *private;
} tcbhead_t;

/* This is the size of the initial TCB.  */
# define TLS_INIT_TCB_SIZE	sizeof (tcbhead_t)

/* This is the size we need before TCB.  */
# define TLS_PRE_TCB_SIZE	sizeof (tcbprehead_t)

# define TCB_ALIGNMENT		64

/* Install new dtv for current thread.  */
# define INSTALL_NEW_DTV(dtv) \
  (THREAD_DTV() = (dtv))

/* Return the address of the dtv for the current thread.  */
# define THREAD_DTV() \
  (((tcbhead_t *) __builtin_thread_pointer ())->dtv)

/* Return the thread descriptor for the current thread.  */
# define THREAD_SELF \
 ((tcbprehead_t *)__builtin_thread_pointer () - 1)

/* Read member of the thread descriptor directly.  */
# define THREAD_GETMEM(descr, member) \
  ((descr)->member)

/* Write member of the thread descriptor directly.  */
# define THREAD_SETMEM(descr, member, value) \
  ((descr)->member = (value))

/* Return the TCB address of a thread given its state.
   Note: this is expensive.  */
static inline tcbprehead_t * __attribute__ ((unused))
THREAD_TCB (thread_t thread,
            struct machine_thread_all_state *all_state)
{
  int ok;
  const struct aarch64_thread_state *state;
  tcbhead_t *tcb;

  ok = machine_get_basic_state (thread, all_state);
  assert (ok);
  state = &((struct machine_thread_all_state *) all_state)->basic;
  tcb = (tcbhead_t *) state->tpidr_el0;
  return (tcbprehead_t *) tcb - 1;
}

/* From hurd.h, reproduced here to avoid a circular include.  */
extern thread_t __hurd_thread_self (void);
libc_hidden_proto (__hurd_thread_self);

/* Set up TLS in the new thread of a fork child, copying from the original.  */
static inline kern_return_t __attribute__ ((unused))
_hurd_tls_fork (thread_t child, thread_t orig,
                struct aarch64_thread_state *state)
{
  error_t err;
  struct aarch64_thread_state orig_state;
  mach_msg_type_number_t state_count = AARCH64_THREAD_STATE_COUNT;

  if (orig != __hurd_thread_self ())
    {
      err = __thread_get_state (orig, AARCH64_THREAD_STATE,
				(thread_state_t) &orig_state,
				&state_count);
      if (err)
        return err;
      assert (state_count == AARCH64_THREAD_STATE_COUNT);
      state->tpidr_el0 = orig_state.tpidr_el0;
    }
  else
    state->tpidr_el0 = (uintptr_t) __builtin_thread_pointer ();
  return 0;
}

static inline kern_return_t __attribute__ ((unused))
_hurd_tls_new (thread_t child, tcbhead_t *tcb)
{
  error_t err;
  struct aarch64_thread_state state;
  mach_msg_type_number_t state_count = AARCH64_THREAD_STATE_COUNT;

  err = __thread_get_state (child, AARCH64_THREAD_STATE,
			    (thread_state_t) &state,
			    &state_count);
  if (err)
    return err;
  assert (state_count == AARCH64_THREAD_STATE_COUNT);

  state.tpidr_el0 = (uintptr_t) tcb;

  return __thread_set_state (child, AARCH64_THREAD_STATE,
			     (thread_state_t) &state,
			     state_count);
}

# if !defined (SHARED) || IS_IN (rtld)
#  define __LIBC_NO_TLS() __builtin_expect (!__builtin_thread_pointer (), 0)

static inline bool __attribute__ ((unused))
_hurd_tls_init (tcbhead_t *tcb, bool full)
{
  extern mach_port_t __hurd_reply_port0;

  if (full)
    /* Take over the reply port we've been using.  */
    (((tcbprehead_t *) tcb) - 1)->reply_port = __hurd_reply_port0;

  __asm __volatile ("msr tpidr_el0, %0" : : "r" (tcb));
  if (full)
    /* This port is now owned by the TCB.  */
    __hurd_reply_port0 = MACH_PORT_NULL;
  return true;
}

#  define TLS_INIT_TP(descr) _hurd_tls_init ((tcbhead_t *) (descr), 1)
# else /* defined (SHARED) && !IS_IN (rtld) */
#  define __LIBC_NO_TLS() 0
# endif

/* Global scope switch support.  */
# define THREAD_GSCOPE_FLAG_UNUSED 0
# define THREAD_GSCOPE_FLAG_USED   1
# define THREAD_GSCOPE_FLAG_WAIT   2

# define THREAD_GSCOPE_SET_FLAG() \
  do									     \
    {									     \
      THREAD_SELF->gscope_flag = THREAD_GSCOPE_FLAG_USED;		     \
      atomic_write_barrier ();						     \
    }									     \
  while (0)

# define THREAD_GSCOPE_RESET_FLAG() \
  do									     \
    { int __flag							     \
	= atomic_exchange_release (&THREAD_SELF->gscope_flag,		     \
				   THREAD_GSCOPE_FLAG_UNUSED);		     \
      if (__flag == THREAD_GSCOPE_FLAG_WAIT)				     \
	lll_wake (THREAD_SELF->gscope_flag, LLL_PRIVATE);		     \
    }									     \
  while (0)

# endif /* __ASSEMBLER__ */

#endif	/* tls.h */
