/* timer.c */

#include "../ptpd.h"

TimeRepresentation elapsed;

void catch_alarm(int sig)
{
  elapsed.nanoseconds += TIMER_INTERVAL;
  while(elapsed.nanoseconds >= 1000000000)
  {
    elapsed.nanoseconds -= 1000000000;
    elapsed.seconds += 1;
  }
  
  DBGV("catch_alarm: elapsed %lu %ld\n", elapsed.seconds, elapsed.nanoseconds);
}

void initTimer(void)
{
  struct itimerval itimer;
  
  DBG("initTimer\n");
  
  signal(SIGALRM, SIG_IGN);
  
  elapsed.seconds = 0;
  elapsed.nanoseconds = 0;
  itimer.it_interval.tv_sec = 0;
  itimer.it_interval.tv_usec = TIMER_INTERVAL/1000;
  itimer.it_value.tv_sec = 0;
  itimer.it_value.tv_usec = itimer.it_interval.tv_usec;
  
  signal(SIGALRM, catch_alarm);
  setitimer(ITIMER_REAL, &itimer, 0);
}

void timerUpdate(IntervalTimer *itimer)
{
  Integer16 i;
  Integer32 delta;
  
  delta = elapsed.seconds;
  elapsed.seconds = 0;
  
  if(delta <= 0)
    return;
  
  for(i = 0; i < TIMER_ARRAY_SIZE; ++i)
  {
    if(itimer[i].interval > 0 && (itimer[i].left -= delta) <= 0)
    {
      itimer[i].left = itimer[i].interval;
      itimer[i].expire = TRUE;
      DBGV("timerUpdate: timer %u expired\n", i);
    }
  }
}

void timerStop(UInteger16 index, IntervalTimer *itimer)
{
  if(index >= TIMER_ARRAY_SIZE)
    return;
  
  itimer[index].interval = 0;
}

void timerStart(UInteger16 index, UInteger16 interval, IntervalTimer *itimer)
{
  if(index >= TIMER_ARRAY_SIZE)
    return;
  
  itimer[index].expire = FALSE;
  itimer[index].left = interval;
  itimer[index].interval = itimer[index].left;
  
  DBGV("timerStart: set timer %d to %d\n", index, interval);
}

Boolean timerExpired(UInteger16 index, IntervalTimer *itimer)
{
  timerUpdate(itimer);
  
  if(index >= TIMER_ARRAY_SIZE)
    return FALSE;
  
  if(!itimer[index].expire)
    return FALSE;
  
  itimer[index].expire = FALSE;
  
  return TRUE;
}

