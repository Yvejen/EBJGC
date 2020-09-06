#ifndef _H_LOG_
#define _H_LOG_

#include <stdio.h>

enum log_verbosity {
  VER_FATAL = 1,
  VER_ERROR,
  VER_WARN,
  VER_INFO,
  VER_VERBOSE,
  VER_DEBUG
};

struct log {
  FILE *fp;
  enum log_verbosity verbosity;
};

/*Global log struct instantiated in log.c*/
extern struct log *g_log;

void write_log(struct log *log_instance, enum log_verbosity verbosity,
               const char *fmt, ...);

#define log_fatal(...) write_log(g_log, VER_FATAL, __VA_ARGS__)
#define log_error(...) write_log(g_log, VER_ERROR, __VA_ARGS__)
#define log_warn(...) write_log(g_log, VER_WARN, __VA_ARGS__)
#define log_info(...) write_log(g_log, VER_INFO, __VA_ARGS__)
#define log_verbose(...) write_log(g_log, VER_VERBOSE, __VA_ARGS__)
#define log_debug(...) write_log(g_log, VER_DEBUG, __VA_ARGS__)

#endif
