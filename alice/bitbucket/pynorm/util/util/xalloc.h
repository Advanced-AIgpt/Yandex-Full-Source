/**
 * @file util/xalloc.h
 *
 * @date 2018
 *
 * @author Georgy Bronnikov <gogabr@yandex-team.ru>
 *
 * @copyright 2018 Yandex. All rights reserved.
 *
 * Small pieces that don't fit anywhere else.
 */

#ifdef XALLOC_H
#error "xalloc.h: double inclusion"
#endif
#define XALLOC_H

#include "xalloc.c"

/*#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif
*/
/* Allocating functions that panic on failure. */

// void *xmalloc(size_t sz);
// void *xcalloc(size_t nmemb, size_t sz);
// void *xrealloc(void *p, size_t sz);
// char *xstrdup(char *s);

#ifdef __cplusplus
}
#endif
