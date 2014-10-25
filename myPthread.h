#ifndef __MYPTHREAD_h__
#define __MYPTHREAD_h__

#include <pthread.h>
#include <assert.h>

void
Pthread_mutex_lock(pthread_mutex_t *m)
{
  int rc = pthread_mutex_lock(m);
  assert(rc == 0);
}

void
Pthread_mutex_unlock(pthread_mutex_t *m)
{
  int rc = pthread_mutex_unlock(m);
  assert(rc == 0);
}

#endif
