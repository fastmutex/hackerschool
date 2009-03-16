// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// $Id: time.h,v 1.1.1.1 2002/08/08 12:49:37 dev Exp $

#ifndef _time_h_
#define _time_h_

typedef struct _SYSTEMTIME {
	USHORT	wYear;
	USHORT	wMonth;
	USHORT	wDayOfWeek;
	USHORT	wDay;
	USHORT	wHour;
	USHORT	wMinute;
	USHORT	wSecond;
	USHORT	wMilliseconds;
} SYSTEMTIME;

void	KernelTimeToSystemTime(PLARGE_INTEGER KernelTime, SYSTEMTIME *lpSystemTime);

#endif
