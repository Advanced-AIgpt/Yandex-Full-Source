/**
 * @file util/log.c
 *
 * @date 2014
 *
 * @author Georgy Bronnikov <gogabr@yandex-team.ru>
 *
 * @copyright 2014 Yandex. All rights reserved.
 *
 * Logging routines.
 */

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <libcalg/arraylist.h>

#include "config.h"
#include "log.h"
#include "util.h"
#include "string-utils.h"
#include "xalloc.h"


#if defined (ANDROID)
#  include <android/log.h>
#elif defined(_MSC_VER)
#  include <windows.h>
#  include <stdlib.h>
#  include <stdio.h>
#else
#  include <stdarg.h>
#  include <stdio.h>
#endif

#if !defined(ANDROID)
#  if defined(_MSC_VER)

static char *bubuf = NULL;
static char *bbb;

char *
log_buf()
{
  return bubuf;
}

void
log_reset()
{
  bbb = bubuf;
  *bbb = '\0';
}

static void
vlog_stderr(char *level, char *fmt, va_list va)
{
  int const BUFSIZE = 512 * 1024;
  int const MAXLINE = 1024;
  if (bubuf == NULL) {
    bubuf = xmalloc(BUFSIZE);
    log_reset();
  } else if (bbb - bubuf > BUFSIZE - MAXLINE) {
	log_reset();
  }
  bbb = stpcpy(bbb, level);
  bbb = stpcpy(bbb, ": ");
  bbb += vsprintf(bbb, fmt, va);
  bbb = stpcpy(bbb, "\n");
}

#  else // !_MSC_VER

static void
vlog_stderr(char *level, char *fmt, va_list va)
{
  fputs(level, stderr);
  fputs(": ", stderr);
  vfprintf(stderr, fmt, va);
  fputc('\n', stderr);
}

#  endif // MSC_VER

void
vlog_error(char *fmt, va_list va)
{
  vlog_stderr("ERROR", fmt, va);
}

void
vlog_warning(char *fmt, va_list va)
{
  vlog_stderr("WARN ", fmt, va);
}

void
vlog_info(char *fmt, va_list va)
{
  vlog_stderr("INFO ", fmt, va);
}

void
vlog_debug(char *fmt, va_list va)
{
  vlog_stderr("DEBUG", fmt, va);
}

#endif

#if defined(ANDROID)
static char *TAG = "libdecode";

void
vlog_error(char *fmt, va_list va)
{
  __android_log_vprint(ANDROID_LOG_ERROR, TAG, fmt, va);
}

void
vlog_warning(char *fmt, va_list va)
{
  __android_log_vprint(ANDROID_LOG_WARN, TAG, fmt, va);
}

void
vlog_info(char *fmt, va_list va)
{
  __android_log_vprint(ANDROID_LOG_INFO, TAG, fmt, va);
}

void
vlog_debug(char *fmt, va_list va)
{
  __android_log_vprint(ANDROID_LOG_DEBUG, TAG, fmt, va);
}
#endif

static void vlog_custom(char *level, char *fmt, va_list va);

void
log_error(char *fmt, ...)
{
  va_list va;
  va_start(va, fmt);
  vlog_error(fmt, va);
  vlog_custom("ERROR", fmt, va);
  va_end(va);
}

void
log_warning(char *fmt, ...)
{
  va_list va;
  va_start(va, fmt);
  vlog_warning(fmt, va);
  vlog_custom("WARN ", fmt, va);
  va_end(va);
}

void
log_info(char *fmt, ...)
{
  va_list va;
  va_start(va, fmt);
  vlog_info(fmt, va);
  vlog_custom("INFO ", fmt, va);
  va_end(va);
}

void
log_debug(char *fmt, ...)
{
  va_list va;
  va_start(va, fmt);
  vlog_debug(fmt, va);
  vlog_custom("DEBUG", fmt, va);
  va_end(va);
}

/* Custom loggers */
static ArrayList *custom_loggers = NULL;

static void
vlog_custom(char *level, char *fmt, va_list va)
{
  if (custom_loggers == NULL) return;

  char *msg = str_vaprintf(fmt, va);
  for (size_t i = 0; i < custom_loggers->length; i++) {
    log_custom_logger_t *logger = custom_loggers->data[i];
    logger->logfun(logger, level, msg);
  }
  free(msg);
}

void
log_add_custom(log_custom_logger_t *logger)
{
  if (custom_loggers == NULL) {
    custom_loggers = arraylist_new(0);
  }
  arraylist_append(custom_loggers, logger);
}

/* `logger` is not destroyed. */
void
log_remove_custom(log_custom_logger_t *logger)
{
  if (custom_loggers != NULL) {
    for (size_t i = 0; i < custom_loggers->length; i++) {
      if (logger == custom_loggers->data[i]) {
        arraylist_remove(custom_loggers, i);
        return;
      }
    }
    
  }
  log_warning("%s: logger not found", __func__);
}


