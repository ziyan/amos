#ifndef MUTEX_H
#define MUTEX_H

#include <pthread.h>
#include <assert.h>

namespace amos
{
	class Mutex
	{
	public:
		Mutex();
		virtual ~Mutex();
		
		virtual void lock();
		virtual void unlock();

	protected:
		pthread_mutex_t mutex;
	};
	
	class MutexLock
	{
	public:
		MutexLock(Mutex *mutex) {
			assert(mutex);
			this->mutex = mutex;
			this->mutex->lock();
		}
		virtual ~MutexLock() {
			this->mutex->unlock();
		}
	protected:
		Mutex *mutex;
	};
}

#endif
