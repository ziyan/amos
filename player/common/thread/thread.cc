#include "thread.h"
#include <string.h>
#include <assert.h>

using namespace amos;

Thread::Thread()
{
	
}

Thread::~Thread()
{
}

void Thread::start()
{
	int rc;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	memset(&thread, 0, sizeof(pthread_t));
	rc = pthread_create(&thread, &attr, main, this);
	assert(!rc);

	pthread_attr_destroy(&attr);
}

void Thread::stop()
{
	int rc;
	void *status;

	pthread_cancel(thread);
	rc = pthread_join(thread, &status);
	assert(!rc);
}

void Thread::testcancel()
{
	int state;
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &state);
	pthread_testcancel();
	pthread_setcancelstate(state, 0);
}

void *Thread::main(void *object)
{
	assert(object);
	
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);
	
	((Thread*)object)->run();
	pthread_exit(0);
}


