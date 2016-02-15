#include <stdio.h>

#define _DBG 0
#include <hcos/dbg.h>
#include <hcos/tmr.h>
#include <hcos/bmp.h>

#include "lwip/sys.h"
#include "sys_arch.h"
#include "soc-cfg.h"

static bmp_t* mbox_p;

err_t sys_sem_new(sys_sem_t *s, u8_t count)
{
	dbg("%s sem_new %x\n", _task_cur->name, (unsigned)s);
	sem_init(s, count);
	return ERR_OK;
}

void sys_sem_free(sys_sem_t *s){}

void sys_sem_signal(sys_sem_t *s)
{
	dbg("%s sem_sig %x\n", _task_cur->name, (unsigned)s);
	sem_post(s);
}

static unsigned to_ticks(unsigned ms)
{
	return ms? tmr_ms2ticks(ms):WAIT;
}

u32_t sys_arch_sem_wait(sys_sem_t *s, u32_t timeout)
{
	unsigned t = tmr_ticks;
	timeout = to_ticks(timeout);
	dbg("%s sem_wait %x\n", _task_cur->name, (unsigned)s);
	dbg("TIMEMOUT %d\n", timeout);
	if(sem_get(s, timeout)){
		dbg("sem_wait: Time\n");
		return SYS_ARCH_TIMEOUT;
	}
	dbg("%s sem_wait: %x OK\n", _task_cur->name, (unsigned)s);
	return tmr_ticks2ms(tmr_ticks - t);
}

err_t sys_mutex_new(sys_mutex_t *u)
{
	dbg("mutex_new\n");
	mut_init(u);
	return ERR_OK;
}
	
void sys_mutex_lock(sys_mutex_t *u)
{
	dbg("mutex_lock\n");
	mut_lock(u, WAIT);
}

void sys_mutex_unlock(sys_mutex_t *u)
{
	dbg("mutex_unlock\n");
	mut_unlock(u);
}

void sys_mutex_free(sys_mutex_t *u)
{}

err_t sys_mbox_new(sys_mbox_t *m, int size)
{
	// ignore size and use MBOX_SZ
	dbg("%s mbox_new %x %d\n", _task_cur->name, (unsigned)m, size);
	void* b = bmp_get(mbox_p, WAIT_NO);
	_assert(b);
	mq_init(m, 1, b, MBOX_BSZ);
	return ERR_OK;
}

void sys_mbox_free(sys_mbox_t *m)
{
	dbg("mbox_free\n");
	bmp_put(mbox_p, m->b);
	m->b = 0;
}

void sys_mbox_post(sys_mbox_t *m, void *msg)
{
	dbg("%s mbox_post %x\n", _task_cur->name, (unsigned)msg);
	mq_put(m, (unsigned*)&msg, WAIT);
}

err_t sys_mbox_trypost(sys_mbox_t *m, void *msg)
{
	dbg("mbox_try_post %x\n", (unsigned)msg);
	return mq_put(m, (unsigned*)&msg, WAIT_NO)? ERR_MEM: ERR_OK;
}

u32_t sys_arch_mbox_fetch(sys_mbox_t *m, void **msg, u32_t timeout)
{
	unsigned t = tmr_ticks;
	timeout = to_ticks(timeout);
	dbg("%s mbox_fetch \n", _task_cur->name);
	dbg("TIMEMOUT %d\n", timeout);
	if(mq_get(m, (unsigned*)msg, timeout)){
		dbg("mbox_fetch: Time\n");
		return SYS_ARCH_TIMEOUT;
	}
	dbg("%s mbox_fetch: %x\n", _task_cur->name, (unsigned)(*msg));
	return tmr_ticks2ms(tmr_ticks - t);
}

u32_t sys_arch_mbox_tryfetch(sys_mbox_t *m, void **msg)
{
	dbg("mbox_try_fetch\n");
	return mq_get(m, (unsigned*)msg, WAIT_NO)? SYS_MBOX_EMPTY: ERR_OK;
}

u32_t sys_now(void)
{
	return tmr_ticks2ms(tmr_ticks);
}

sys_thread_t sys_thread_new(
	const char *name, 
	void(* thread)(void *arg), 
	void *arg, int stacksize, int prio)
{
	dbg("%s sys_thread_new %s %d\n", _task_cur->name, name, stacksize);
	return task_new(name, thread, prio, stacksize, -1, arg);
}

void sys_msleep(u32_t ms)
{
	dbg("sleep %d\n", ms);
	task_sleep(tmr_ms2ticks(ms));
}

void sys_init(void)
{
	mbox_p = _alloc(bmp_sz(MBOX_NO));
	bmp_init(mbox_p, _alloc( MBOX_BSZ * MBOX_NO ), MBOX_BSZ, MBOX_NO);
}

void sys_print(char* s)
{
	printf(s);
}
