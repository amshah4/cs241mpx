#ifndef __LIBDRM_H__
#define __LIBDRM_H__

#include <pthread.h>
#include "queue.h"
#include "libwfg.h"

enum drmmode_t
{
	NO_DEADLOCK_CHECKING,
	DEADLOCK_PREVENTION,
	DEADLOCK_DETECTION,
	DEADLOCK_AVOIDANCE
};

typedef struct _drm_t
{
	pthread_mutex_t *ptMutex;

} drm_t;

void drm_setmode(enum drmmode_t mode);
void drm_init(drm_t *mutex);
int drm_lock(drm_t *mutex);
int drm_unlock(drm_t *mutex);
void drm_destroy(drm_t *mutex);
void drm_cleanup();

#endif
