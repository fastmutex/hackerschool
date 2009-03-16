// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// $Id: time.c,v 1.1.1.1 2002/08/08 12:49:37 dev Exp $

#include <ntddk.h>
#include "time.h"

#define TICKSPERMIN        600000000
#define TICKSPERSEC        10000000
#define TICKSPERMSEC       10000
#define SECSPERDAY         86400
#define SECSPERHOUR        3600
#define SECSPERMIN         60
#define MINSPERHOUR        60
#define HOURSPERDAY        24
#define EPOCHWEEKDAY       0
#define DAYSPERWEEK        7
#define EPOCHYEAR          1601
#define DAYSPERNORMALYEAR  365
#define DAYSPERLEAPYEAR    366
#define MONSPERYEAR        12

static const int YearLengths[2] = {DAYSPERNORMALYEAR, DAYSPERLEAPYEAR};
static const int MonthLengths[2][MONSPERYEAR] = {
	{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
	{ 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};

static int	IsLeapYear(int Year);

void
KernelTimeToSystemTime(PLARGE_INTEGER KernelTime, SYSTEMTIME *lpSystemTime)
{
	const int *Months;
	int LeapSecondCorrections, SecondsInDay, CurYear;
	int LeapYear, CurMonth;
	long Days;
	__int64 Time = KernelTime->QuadPart;
	
	/* Extract millisecond from time and convert time into seconds */
	lpSystemTime->wMilliseconds = (USHORT)((Time % TICKSPERSEC) / TICKSPERMSEC);
	Time = Time / TICKSPERSEC;
	
	LeapSecondCorrections = 0;
	
	/* Split the time into days and seconds within the day */
	Days = (long)(Time / SECSPERDAY);
	SecondsInDay = (long)(Time % SECSPERDAY);
	
	/* Adjust the values for GMT and leap seconds */
	SecondsInDay += LeapSecondCorrections;
	while (SecondsInDay < 0) {
		SecondsInDay += SECSPERDAY;
		Days--;
	}
	while (SecondsInDay >= SECSPERDAY) {
		SecondsInDay -= SECSPERDAY;
		Days++;
	}
	
	/* compute time of day */
	lpSystemTime->wHour = (USHORT)(SecondsInDay / SECSPERHOUR);
	SecondsInDay = SecondsInDay % SECSPERHOUR;
	lpSystemTime->wMinute = (USHORT)(SecondsInDay / SECSPERMIN);
	lpSystemTime->wSecond = (USHORT)(SecondsInDay % SECSPERMIN);
	
	/* compute day of week */
	lpSystemTime->wDayOfWeek = (USHORT)((EPOCHWEEKDAY + Days) % DAYSPERWEEK);
	
	/* compute year */
	CurYear = EPOCHYEAR;

	for (;;) {
		LeapYear = IsLeapYear(CurYear);
		if (Days < (long)YearLengths[LeapYear])
			break;
		CurYear++;
		Days = Days - (long)YearLengths[LeapYear];
	}
	lpSystemTime->wYear = (USHORT)CurYear;
	
	/* Compute month of year */
	Months = MonthLengths[LeapYear];
	for (CurMonth = 0; Days >= (long)Months[CurMonth]; CurMonth++)
		Days = Days - (long)Months[CurMonth];
	lpSystemTime->wMonth = (USHORT)(CurMonth + 1);
	lpSystemTime->wDay = (USHORT)(Days + 1);
}

int
IsLeapYear(int Year)
{
	return Year % 4 == 0 && (Year % 100 != 0 || Year % 400 == 0) ? 1 : 0;
}
