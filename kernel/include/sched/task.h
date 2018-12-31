#ifndef __TASK_H__
#define __TASK_H__

#include <lib/list.h>
#include <mm/mmu.h>

#define MAX_THREADS 16
#define KSTACK_SIZE 0x400 /* 1024 bytes */

typedef enum {
    T_READY   = 0 << 0,
    T_RUNNING = 1 << 0,
    T_BLOCKED = 1 << 1
} thread_state_t;

typedef struct exec_state {
    /* pushed/popped manually */
    uint16_t gs, fs, es, ds;

    /* pusha */
    uint32_t edi, esi, ebp, esp;
    uint32_t ebx, edx, ecx, eax;

    /* pushed manually */
    uint32_t isr_num, err_num;

    /* pushed by the cpu */
    uint32_t eip, cs, eflags, useresp, ss;
} exec_state_t;

typedef struct thread {
    thread_state_t state;

    void *kstack_top;
    void *kstack_bottom;

    exec_state_t *exec_state;
} thread_t;

typedef struct task {
    struct task *parent;

    thread_t *threads[16];

    /* TODO: remove */
    const char *name;

    list_head_t list;
    list_head_t children;

    pdir_t *dir;
    uint32_t cr3;
} task_t;

int sched_task_add_thread(task_t *parent, thread_t *child);
thread_t *sched_thread_create(void *(*func)(void *), void *arg);
task_t *sched_task_create(const char *name);
task_t *sched_task_fork(task_t *t);

/* create caches for threads and tasks
 * return 0 on success and -errno on error 
 *
 * NOTE: this function should be called be sched_init and no one else */
int sched_task_init(void);

#endif /* __TASK_H__ */
