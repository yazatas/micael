#ifndef __TASK_H__
#define __TASK_H__

#include <fs/fs.h>
#include <lib/list.h>
#include <mm/mmu.h>
#include <kernel/common.h>
#include <sync/wait.h>

#define MAX_THREADS 16
#define KSTACK_SIZE 0x1000 /* 4096 bytes */

typedef int pid_t;

typedef enum {
    TIF_NO_FLAGS     = 0 << 0,
    TIF_NEED_RESCHED = 1 << 0,
} THREAD_FLAGS;

typedef enum {
    T_READY     = 0 << 0,
    T_RUNNING   = 1 << 0,
    T_BLOCKED   = 1 << 1,
    T_ZOMBIE    = 1 << 2,
    T_UNSTARTED = 1 << 3,
} thread_state_t;

typedef struct exec_state {
#ifdef __x86_64__
    uint64_t eax, ecx, edx, ebx, ebp, esi, edi;
    uint64_t isr_num, err_num;
    uint64_t eip, cs, eflags, esp, ss;
#else
    /* pushed/popped manually */
    uint16_t gs, fs, es, ds;

    /* pusha */
    uint32_t edi, esi, ebp, esp;
    uint32_t ebx, edx, ecx, eax;

    /* pushed manually */
    uint32_t isr_num, err_num;

    /* pushed by the cpu */
    uint32_t eip, cs, eflags, esp, ss;
#endif
} exec_state_t;

typedef struct thread {
    thread_state_t state;
    list_head_t list;

    unsigned flags;
    unsigned exec_runtime;
    unsigned total_runtime;

    void *kstack_top;
    void *kstack_bottom;

    exec_state_t *exec_state;
} thread_t;

typedef struct task {
    struct task *parent;

    size_t nthreads;
    thread_t *threads;

    /* TODO: remove */
    const char *name;

    pid_t pid;
    gid_t gid;
    int exit_status;

    /* open files, devices etc. */
    fs_ctx_t   *fs_ctx;
    file_ctx_t *file_ctx;

    list_head_t list;
    list_head_t children;

    wait_queue_t wq;   /* wait queue object used for blocking the task execution */

    void *dir;         /* virtual  address of the page directory  */
    unsigned long cr3; /* physical address of the page directory */
} task_t;

int sched_task_add_thread(task_t *parent, thread_t *child);
thread_t *sched_thread_create(void *(*func)(void *), void *arg);
task_t *sched_task_create(const char *name);
task_t *sched_task_fork(task_t *t);

/* release memory of all but currently running thread */
void sched_free_threads(task_t *t);

/* create caches for threads and tasks
 * return 0 on success and -errno on error 
 *
 * NOTE: this function should be called be sched_init and no one else */
int sched_task_init(void);

#endif /* __TASK_H__ */
