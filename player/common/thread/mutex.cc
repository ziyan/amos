#include "mutex.h"

using namespace amos;

Mutex::Mutex()
{
	int rc;
	rc = pthread_mutex_init(&mutex, 0);
	assert(!rc);
}

Mutex::~Mutex()
{
	int rc;
	rc = pthread_mutex_destroy(&mutex);
	assert(!rc);
}

void Mutex::lock()
{
	int rc;
	rc = pthread_mutex_lock(&mutex);
	assert(!rc);
}

void Mutex::unlock()
{
	int rc;
	rc = pthread_mutex_unlock(&mutex);
	assert(!rc);
}

