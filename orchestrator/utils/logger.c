#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "logger.h"
#include "constants.h"

#ifdef DETAILED_LOGS
	#define _DEBUG 1
#endif

extern void logger(int LoggingLevel, const char *ModuleName, const char *File, int Line, const char *Format, ...)
{
	char Buffer[BUFFER_SIZE];
	int BufSize= BUFFER_SIZE - 1;
	va_list Args;
#ifndef LOG_1024X768
	char *LoggingLevelString;
#endif
	char CurrentTimeBuffer[64];
	struct timeval tv;
	time_t CurrentTime;

#ifdef LOG_ON_FILE
	// The file is open in append
	FILE *DestFile = fopen(LOG_FILE,"a");
#else
	FILE *DestFile= stdout;//stderr;
#endif

	if (LoggingLevel < LOGGING_LEVEL)
		return;

#ifndef LOG_1024X768
	switch(LoggingLevel)
	{
		case ORCH_DEBUG: LoggingLevelString= "DEBUG"; break;
		case ORCH_DEBUG_INFO: LoggingLevelString= "DEBUG-INFO"; break;
		case ORCH_WARNING: LoggingLevelString= "WARNING"; break;
		case ORCH_ERROR: LoggingLevelString= "ERROR"; break;
		case ORCH_INFO: LoggingLevelString= "INFO"; break;
		// The 'default' case is just to avoid a compiler warning
		default: LoggingLevelString= "UNKNOWN_LEVEL";
	}
#endif

	// Format input string
	va_start(Args, Format);
	vsnprintf(Buffer, BufSize, Format, Args);

	// Get current time and format it appropriately
	gettimeofday(&tv, NULL);
	CurrentTime=tv.tv_sec;
	strftime(CurrentTimeBuffer,sizeof(CurrentTimeBuffer),"%F-%T",localtime(&CurrentTime));

	// Print the first part of the message
#ifdef LOG_1024X768
	fprintf(DestFile, "[%s] ", CurrentTimeBuffer);
#else
	fprintf(DestFile, "[%s.%06ld] [%s] [%s] ", CurrentTimeBuffer, tv.tv_usec, ModuleName, LoggingLevelString);
#endif

#ifdef _DEBUG
	fprintf(DestFile, "[%s:%d] %s\n", File, Line, Buffer);
#else
	fprintf(DestFile, "%s\n", Buffer);
#endif

	fflush(DestFile);
}

extern void coloredLogger(char *color, int LoggingLevel, const char *ModuleName, const char *File, int Line, const char *Format, ...)
{
	char Buffer[BUFFER_SIZE];
	int BufSize= BUFFER_SIZE - 1;
	va_list Args;
#ifndef LOG_1024X768
	char *LoggingLevelString;
#endif
	char CurrentTimeBuffer[64];
	struct timeval tv;
	time_t CurrentTime;
	FILE *DestFile= stdout;//stderr;


	if (LoggingLevel < LOGGING_LEVEL)
		return;

#ifndef LOG_1024X768
	switch(LoggingLevel)
	{
		case ORCH_DEBUG: LoggingLevelString= "DEBUG"; break;
		case ORCH_DEBUG_INFO: LoggingLevelString= "DEBUG-INFO"; break;
		case ORCH_WARNING: LoggingLevelString= "WARNING"; break;
		case ORCH_ERROR: LoggingLevelString= "ERROR"; break;
		case ORCH_INFO: LoggingLevelString= "INFO"; break;
		// The 'default' case is just to avoid a compiler warning
		default: LoggingLevelString= "UNKNOWN_LEVEL";
	}
#endif

	// Format input string
	va_start(Args, Format);
	vsnprintf(Buffer, BufSize, Format, Args);

	// Get current time and format it appropriately
	gettimeofday(&tv, NULL);
	CurrentTime=tv.tv_sec;
	strftime(CurrentTimeBuffer,sizeof(CurrentTimeBuffer),"%F-%T",localtime(&CurrentTime));

	// Print the first part of the message
#ifdef LOG_1024X768
	fprintf(DestFile, "[%s] ", CurrentTimeBuffer);
#else
	fprintf(DestFile, "[%s.%06ld] [%s] [%s] ", CurrentTimeBuffer, tv.tv_usec, ModuleName, LoggingLevelString);
#endif

#ifdef _DEBUG
	fprintf(DestFile, "[%s%s:%d] %s" ANSI_COLOR_RESET "\n", color, File, Line, Buffer);
#else
	fprintf(DestFile, "%s%s" ANSI_COLOR_RESET "\n", color, Buffer);
#endif

};

