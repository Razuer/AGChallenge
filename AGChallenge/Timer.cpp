
//#include  "stdafx.h"
#include "Timer.h"
using namespace TimeCounters;



CTimeCounter::CTimeCounter()
{
    b_start_inited = false;
    b_finish_inited = false;
}//CTimeCounter::CTimeCounter()



void CTimeCounter::vSetStartNow()
{
    b_start_inited = true;
    b_finish_inited = false;
    t_start = clock::now();
}//void  CTimeCounter::vSetStartNow()


//if returned value is false it means the timer was not set on start
bool CTimeCounter::bGetTimePassed(double *pdTimePassedSec)
{
    if (!b_start_inited) return false;
    auto now = clock::now();
    std::chrono::duration<double> diff = now - t_start;
    *pdTimePassedSec = diff.count();
    return true;
}//bool  CTimeCounter::bGetTimePassed(double  *pdTimePassedMs)


bool CTimeCounter::bSetFinishOn(double dTimeToFinishSec)
{
    if (!b_start_inited || dTimeToFinishSec <= 0) return false;
    b_finish_inited = true;
    t_finish = t_start + std::chrono::duration_cast<clock::duration>(std::chrono::duration<double>(dTimeToFinishSec));
    return true;
}//bool  CTimeCounter::bSetFinishOn(double  dTimeToFinishMs)


bool CTimeCounter::bIsFinished()
{
    if (!b_start_inited || !b_finish_inited)
        return true;

    auto now = clock::now();
    return now > t_finish;
} // bool  CTimeCounter::bIsFinished()






