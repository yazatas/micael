#include <sync/mutex.h>
#include <kernel/kprint.h>
#include <sched/kthread.h>

int mutex_trylock(mutex_t *m)
{
	if (*m == -1)
		return -1;
	*m = -1;
	return 0;

}

int mutex_lock(mutex_t *m)
{
	while (*m == -1) {
		/* kdebug("mutex is locked"); */
	}

	*m = -1;
	return 0;
}

int mutex_unlock(mutex_t *m)
{
	if (*m == 0)
		return -1;
	*m = 0;
	return 0;
}
