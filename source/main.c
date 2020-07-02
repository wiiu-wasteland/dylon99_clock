/*
 * Clock v1.1
 * Copyright (C) 2020 dylon99, rw-r-r-0644
 */

#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <whb/proc.h>
#include "console.h"

int main(int argc, char **argv)
{
	WHBProcInit();
	console_init();

	while (WHBProcIsRunning())
	{
		console_clear();

		OSCalendarTime tm;
		OSTicksToCalendarTime(OSGetTime(), &tm);
		tm.tm_mon = tm.tm_mon + 1;

		console_printf("\n\n\n\n\n\n\n");
		console_printf("This is the current time and date: \n");
		console_printf("\n");
		console_printf("Date: %d:%02d:%d\n", tm.tm_mday, tm.tm_mon, tm.tm_year);
		console_printf("Time: %d:%d:%02d\n", tm.tm_hour, tm.tm_min, tm.tm_sec);

		OSSleepTicks(OSMillisecondsToTicks(10));
	}

	console_printf("\n");
	console_printf("Leaving the app...");

	console_deinit();
	WHBProcShutdown();

	return 0;
}
