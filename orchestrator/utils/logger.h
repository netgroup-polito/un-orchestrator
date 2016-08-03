#pragma once

#include <stdio.h>	// vsnprintf
#include <stdarg.h>	// va_list

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

/*
 * The following are the logging levels we have.
 *
 * Depending on the value of the 'LOGGING_LEVEL', all the messages
 * that are at level < LOGGING_LEVEL will be ignored and will not
 * be printed on screen.
 */
enum
{
  // Used to print DEBUG information.
  ORCH_DEBUG = 1,

  // Used to print DEBUG information, with a priority that is higher than standard DEBUG messages.
  ORCH_DEBUG_INFO,

  // Used to print WARNING information, which may suggest that something is wrong.
  ORCH_WARNING,

  // Used to print ERROR information. This level should always be turned on.
  ORCH_ERROR,

  // Used to print general INFO that should always be shown on screen.
  ORCH_INFO
};


#ifdef __cplusplus
extern "C" {
#endif


/*!
	\brief Formats a message string and prints it on screen.

	This function is basically a printf() enriched with some parameters
	needed to log messages a better way.
	This functions prints everything on a standard output file, on a single line.

	\param LoggingLevel Level of this logging message. It may not be printed on screen depending on the current logging threshold.
	\param ModuleName Name of the module that is generating this message. It would be the first text printed on each line.
	\param File Name of the file in which this function has been invoked.
	\param Line Line in which this function has been invoked.
	\param Format Format-control string, according to syntax of the printf() function.
*/
extern void logger(int LoggingLevel, const char *ModuleName, const char *File, int Line, const char *Format, ...);

//IVANO: TODO: merge with the function above (if possible)
extern void coloredLogger(char *color, int LoggingLevel, const char *ModuleName, const char *File, int Line, const char *Format, ...);

#define UN_LOG(LEVEL, FORMAT, ...) 					 						\
	do {									 								\
		logger(LEVEL, LOG_MODULE_NAME, __FILE__, __LINE__, FORMAT, ##__VA_ARGS__);\
	} while(0)

/*
 * Logging macros:
 * There is a different macro for each logging level, they just require have
 * the log message and its arguments. (Like printf).
 * In order to use these macros, the variable LOG_MODULE_NAME has to be defined
 * in the module.
 * For example:
 *  static const char LOG_MODULE_NAME[] = "KVM-Manager";
 */

#define ULOG_DBG(FORMAT, ...) 					 						\
	do {									 								\
		logger(ORCH_DEBUG, LOG_MODULE_NAME, __FILE__, __LINE__, FORMAT, ##__VA_ARGS__);\
	} while(0)

#define ULOG_DBG_INFO(FORMAT, ...) 					 						\
	do {									 								\
		logger(ORCH_DEBUG_INFO, LOG_MODULE_NAME, __FILE__, __LINE__, FORMAT, ##__VA_ARGS__);\
	} while(0)

#define ULOG_WARN(FORMAT, ...) 					 						\
	do {									 								\
		logger(ORCH_WARNING, LOG_MODULE_NAME, __FILE__, __LINE__, FORMAT, ##__VA_ARGS__);\
	} while(0)

#define ULOG_ERR(FORMAT, ...) 					 						\
	do {									 								\
		logger(ORCH_ERROR, LOG_MODULE_NAME, __FILE__, __LINE__, FORMAT, ##__VA_ARGS__);\
	} while(0)

#define ULOG_INFO(FORMAT, ...) 					 						\
	do {									 								\
		logger(ORCH_INFO, LOG_MODULE_NAME, __FILE__, __LINE__, FORMAT, ##__VA_ARGS__);\
	} while(0)

#ifdef __cplusplus
}
#endif

