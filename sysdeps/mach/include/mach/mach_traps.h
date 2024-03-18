#ifndef _MACH_MACH_TRAPS_H
#include_next <mach/mach_traps.h>

#ifndef _ISOMAC
extern mach_port_t __mach_reply_port (void);
libc_hidden_proto (__mach_reply_port)
extern mach_port_t __mach_thread_self (void);
libc_hidden_proto (__mach_thread_self)
extern mach_port_t (__mach_task_self) (void);
libc_hidden_proto (__mach_task_self)
extern mach_port_t (__mach_host_self) (void);
libc_hidden_proto (__mach_host_self)
extern boolean_t __swtch (void);
libc_hidden_proto (__swtch)
extern boolean_t __swtch_pri (int priority);
libc_hidden_proto (__swtch_pri)
kern_return_t __thread_switch (mach_port_t new_thread,
			     int option, mach_msg_timeout_t option_time);
libc_hidden_proto (__thread_switch)
kern_return_t __evc_wait (unsigned int event);
libc_hidden_proto (__evc_wait)

/* Set current thread's state, as if with thread_set_state() RPC.
   This syscall is only really available in recent enough GNU Mach.  */
extern kern_return_t __thread_set_self_state (int flavor,
					      natural_t *new_state,
					      natural_t new_state_count);
libc_hidden_proto (__thread_set_self_state)
#endif
#endif
