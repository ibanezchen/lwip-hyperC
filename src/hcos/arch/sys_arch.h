#ifndef SYS_ARCH0811
#define SYS_ARCH0811

#include <hcos/sem.h>
#include <hcos/mut.h>
#include <hcos/mq.h>
#include <hcos/task.h>
#include <hcos/tmr.h>
#include <hcos/irq.h>
#include <hcos/dbg.h>

#include "arch/cc.h"
#include "lwip/err.h"

typedef sem_t  sys_sem_t;

#define sys_sem_valid(s)	(1)

#define sys_sem_set_invalid(mbox) 

typedef mut_t  sys_mutex_t;

typedef mq_t	sys_mbox_t;

#define sys_mbox_valid(m)		((m)->b!=0)

#define sys_mbox_set_invalid(m)

typedef task_t* sys_thread_t;

#define sys_jiffies tmr_ticks

#define SYS_ARCH_DECL_PROTECT(lev)	unsigned lev

#define SYS_ARCH_PROTECT(lev)	lev = irq_lock()

#define SYS_ARCH_UNPROTECT(lev)	irq_restore(lev)

#endif

