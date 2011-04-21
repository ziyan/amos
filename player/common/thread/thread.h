#include <pthread.h>

namespace amos
{
	class Thread
	{
	public:
		Thread();
		virtual ~Thread();
		virtual void start();
		virtual void stop();

	protected:
		virtual void run() = 0;
		virtual void testcancel();

		static void *main(void *object);
		pthread_t thread;
	};
}


