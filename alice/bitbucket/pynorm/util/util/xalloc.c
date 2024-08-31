#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(_WIN32) && !defined (_WIN64)
  #include <unistd.h>
#endif // Windows

//#include "config.h"
//#include "util.h"
//#include "string-utils.h"
//#include "xalloc.h"

/* Panicking. Hope we have enough resources left at least for this. */
static char buf[] = "Memory allocation failed, aborting.\n";
static size_t const bufsz = sizeof(buf) - 1;

#define UNUSED(x) ((void)(x))

static void
panic()
{
#if !defined(_WIN32) && !defined (_WIN64)
  int v = write(fileno(stderr), buf, bufsz);
  UNUSED(v);
#endif // Windows
  abort();
}

static void *
xmalloc(size_t sz)
{
  void *p = malloc(sz);
  if (p == NULL && sz != 0) panic();
  return p;
}

static void *
xcalloc(size_t nmemb, size_t sz)
{
  void *p = calloc(nmemb, sz);
  if (p == NULL && nmemb != 0 && sz != 0) panic();
  return p;
}

static void *
xrealloc(void *p, size_t sz)
{
  p = realloc(p, sz);
  if (p == NULL && sz != 0) panic();
  return p;
}

static char *
xstrdup(char *s)
{
  char *ns = strdup(s);
  if (ns == NULL) panic();
  return ns;
}

#undef UNUSED
