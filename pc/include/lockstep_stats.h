#ifndef LOCKSTEP_STATS_H
#define LOCKSTEP_STATS_H

#ifdef NETPLAY
/* The timer computes the average with unsigned division. */
extern int lockstepStatDelta;
extern unsigned lockstepStatAverage;
#endif

#endif
