#ifndef __MUTEX_H__
#define __MUTEX_H__

#include <stdint.h>

typedef volatile int8_t mutex_t;

/* return 0 if locking the mutex succeeded
 * and -1 if it's already locked 
 *
 * block execution of thread until mutex 
 * has been released */
int mutex_lock(mutex_t *m);

/* return 0 if locking the mutex succeeded
 * and -1 if it's already locked */
int mutex_trylock(mutex_t *m);

/* if mutex is locked, unlock it and return 0,
 * if mutex is not locked, return -1 */
int mutex_unlock(mutex_t *m);

#endif /* end of include guard: __MUTEX_H__ */
