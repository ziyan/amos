#ifndef AMOS_COMMON_TIMER_H
#define AMOS_COMMON_TIMER_H

#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <stdint.h>

#define uS_IN_SEC 1000000

using namespace std;

#define TIMEVAL_TO_SEC(t) ((uint32_t)t.tv_sec + ((uint32_t)t.tv_usec/CLOCKS_PER_SEC))
#define TIMEVAL_TO_SEC_F(t) ((float)t.tv_sec + ((float)t.tv_usec/CLOCKS_PER_SEC))
#define TIMEVAL_TO_MS(t) (uint32_t)((t.tv_sec*1000) + (t.tv_usec/1000))
#define TIMEVAL_TO_MS_F(t) (((float)t.tv_sec*1000) + ((float)t.tv_usec/1000))
#define TIMEVAL_TO_uS(x) ((uint32_t)x.tv_sec * 1000 * 1000 + (uint32_t)x.tv_usec)
#define TIMEVAL_TO_uS_F(x) ((float)x.tv_sec * 1000 * 1000 + (float)x.tv_usec)
#define TIMEVAL_TO_uS_64(x) ((uint64_t)x.tv_sec * 1000 * 1000 + (uint64_t)x.tv_usec)

namespace amos
{
	class Timer{
		public:
			Timer(bool auto_start=false);

			bool isGood();

			bool isRunning();

			void start();

			void stop();

			void resume();

			/**
			  *	Clears all member variables and stops timer
			  */
			void reset();

			void print();

			void printLine();

			void toXml(ostream& stream);

			//void average(const Timer& timer);

			timeval getElapsedUser();

			timeval getElapsedSys();

			timeval getElapsedReal();

			/**	Adds the time delta to the initial time storing the sum
			  *	in the provided result timeval
			  *	@param	initial	Starting time
			  *	@param	delta	Time delta
			  *	@param	result	Stores resulting sum
			  *	@return Returns false if adding the two timevals will
			  *		result in an overflow causing incorrect time
			  *		representation.
			  */
			static bool addTimeval(timeval initial, timeval delta, timeval& result);

			/**	Subtracts initial time from final time storing the difference
			  *	in the result timeval.
			  *	@param	final	Ending time
			  *	@param	initial	Starting time
			  *	@param	result	Stores resulting time difference here
			  *	@return Returns false if subtraction causes underflow
			  */
			static bool subTimeval(const timeval final, const timeval initial, timeval& result);

			/**	Factors out whole seconds from the tv_usec field
			  *	adding any whole seconds to the tv_sec field instead.
			  *	If factoring will cause an overflow t is not updated
			  *	and false is returned.
			  *	@param	t	Timeval to factor
			  *	@return Returns false if factoring will cause an error,
			  *		returns true other wise.
			  */
			static bool factorTimeval(timeval& t);

			static int timevalToMs(timeval t);

			static void printTimeval(timeval& t);

			// Returns (now - tv_first) in microseconds
			static int getNow(struct timeval &tv);

			static uint64_t getSince(struct timeval &tv_prior);

			static uint64_t getSinceMs(struct timeval &tv_prior);

		protected:
			bool updateElapsed();

		private:

			bool running;	//Timer status
			bool overflow;

			//Timestamps of user/system/real times when timer started
			struct timeval real_start;
			struct timeval user_start;
			struct timeval sys_start;


			//Elapsed user/system/real times when timer stopped or paused
			struct timeval real_elapsed;
			struct timeval user_elapsed;
			struct timeval sys_elapsed;
	};
};
#endif // AMOS_COMMON_TIMER_H
