#include <kernel/gdt.h>
#include <mm/mmu.h>
#include <mm/heap.h>
#include <mm/cache.h>
#include <sched/task.h>

#include <errno.h>
#include <string.h>

static cache_t *task_cache = NULL;
static cache_t *thread_cache = NULL;

extern uint32_t stack_bottom;

int sched_task_init(void)
{
    if ((task_cache = cache_create(sizeof(task_t), C_NOFLAGS)) == NULL)
        return -errno;

    if ((thread_cache = cache_create(sizeof(thread_t), C_NOFLAGS)) == NULL)
        return -errno;

    return 0;
}

thread_t *sched_thread_create(void *(*func)(void *), void *arg)
{
    thread_t *t = cache_alloc_entry(thread_cache, C_NOFLAGS);

    t->state         = T_READY;
    t->kstack_top    = kmalloc(KSTACK_SIZE);
    t->kstack_bottom = (uint8_t *)t->kstack_top + KSTACK_SIZE;
    t->exec_state    = (exec_state_t *)((uint8_t *)t->kstack_bottom - sizeof(exec_state_t));

    list_init(&t->list);
    memset(t->exec_state, 0, sizeof(exec_state_t));

    /* TODO: comment */
    /* TODO: push arg to stack */

    t->exec_state->eax = 0x1336;
    t->exec_state->ecx = 0x1337;
    t->exec_state->edx = 0x1338;
    t->exec_state->ebx = 0x1339;

    t->exec_state->fs = 0x10; /* TODO: SEG_KERNEL_DATA?? */
    t->exec_state->gs = 0x10;
    t->exec_state->ds = 0x10;
    t->exec_state->es = 0x10;

    t->exec_state->eip     = (uint32_t)func;
    t->exec_state->eflags  = 1 << 9;
    t->exec_state->ss      = SEG_KERNEL_DATA;
    t->exec_state->cs      = SEG_KERNEL_CODE;
    t->exec_state->useresp = (uint32_t)t->exec_state + 4;

    return t;
}

task_t *sched_task_create(const char *name)
{
    task_t *t   = cache_alloc_entry(task_cache, C_NOFLAGS);
    t->parent   = NULL;
    t->name     = name;
    t->nthreads = 0;

    list_init_null(&t->children);
    list_init_null(&t->list);

    t->dir = mmu_build_pagedir();
    t->cr3 = (uint32_t)vmm_v_to_p(t->dir);

    return t;
}

int sched_task_add_thread(task_t *parent, thread_t *child)
{
    if (parent->nthreads == 0) {
        parent->threads = child;
    } else {
        thread_t *last = container_of(parent->threads->list.prev, thread_t, list);
        list_append(&last->list, &child->list);
    }

    parent->nthreads++;

    return 0;
}

task_t *sched_task_fork(task_t *t)
{
    (void)t;

    return NULL;
}
