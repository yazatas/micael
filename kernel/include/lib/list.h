#ifndef __LIST_H__
#define __LIST_H__

typedef struct list_head {
    struct list_head *next, *prev;
} list_head_t;

void list_init(list_head_t *s);
void list_remove(list_head_t *s);
void list_append(list_head_t *s, list_head_t *t);
void list_prepend(list_head_t *s, list_head_t *t);
void list_insert(list_head_t *new, list_head_t *next, list_head_t *prev);

#endif /* __LIST_H__ */
