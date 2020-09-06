#include "log.h"

#include <stdarg.h>

struct log *g_log = &(struct log){NULL, VER_INFO};

void init_global_log(FILE *file) { g_log->fp = file; }

void write_log(struct log *log_instance, enum log_verbosity verbosity,
               const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  if (verbosity <= log_instance->verbosity && log_instance->fp) {
    vfprintf(log_instance->fp, fmt, args);
  }
  va_end(args);
}
