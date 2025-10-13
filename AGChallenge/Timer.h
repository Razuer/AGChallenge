#pragma once
#include <cmath>
#include <ctime>
#include <chrono>

namespace TimeCounters
{
	class CTimeCounter
	{
	public:
		CTimeCounter();
		~CTimeCounter() = default;

		// Start the timer now
		void vSetStartNow();
		// If returned value is false it means the timer was not set on start
		bool bGetTimePassed(double *pdTimePassedSec);
		bool bSetFinishOn(double dTimeToFinishSec);
		bool bIsFinished();

	private:
		using clock = std::chrono::steady_clock;
		bool b_start_inited;
		bool b_finish_inited;
		clock::time_point t_start;
		clock::time_point t_finish;
	};
} // namespace TimeCounters