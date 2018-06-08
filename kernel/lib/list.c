#include <lib/list.h>

/* insert node between next and prev */
void list_insert(list_head_t *new, list_head_t *next, list_head_t *prev)
{
    prev->next = new;
    next->prev = new;
    new->prev = prev;
    new->next = next;
}

/* insert t after s */
void list_append(list_head_t *s, list_head_t *t)
{
    t->next = s->next;
    t->prev = s;

    s->next->prev = t;
    s->next = t;
}

/* insert t before s */
void list_prepend(list_head_t *s, list_head_t *t)
{
    t->prev = s->prev;
    t->next = s;

    s->prev->next = t;
    s->prev = t;
}

void list_remove(list_head_t *s)
{
    s->next->prev = s->prev;
    s->prev->next = s->next;
}

void list_init(list_head_t *s)
{
    s->next = s;
    s->prev = s;
}

