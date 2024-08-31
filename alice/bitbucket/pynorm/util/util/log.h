/**
 * @file util/log.h
 *
 * @date 2014
 *
 * @author Georgy Bronnikov <gogabr@yandex-team.ru>
 *
 * @copyright 2014 Yandex. All rights reserved.
 *
 * Logging routines.
 */

#ifdef LOG_H
#error "log.h: double_inclusion"
#endif
#define LOG_H

void log_error(char *fmt, ...);
void log_warning(char *fmt, ...);
void log_info(char *fmt, ...);
void log_debug(char *fmt, ...);

#if defined(va_list) || defined(_VA_LIST) /* don't include va_ functions unless stdarg.h comes before us */

  void vlog_error(char *fmt, va_list va);
  void vlog_warning(char *fmt, va_list va);
  void vlog_info(char *fmt, va_list va);
  void vlog_debug(char *fmt, va_list va);

#endif

/* Custom loggers */
/*
  `log_add_custom()` provides an object that will receive all log messages.
  `log_remove_custom()` excludes such an object from the list.

  !!!  These functions are not thread safe and change global state. !!!
 */
typedef struct log_custom_logger log_custom_logger_t;
typedef void log_custom_ft(log_custom_logger_t *logger, char *level, char *msg);

struct log_custom_logger {
  log_custom_ft *logfun;
};

void log_add_custom(log_custom_logger_t *logger);

/* `logger` is not destroyed. */
void log_remove_custom(log_custom_logger_t *logger);
