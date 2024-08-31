#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xalloc.h"
#include "config.h"
#include "util.h"

#include "string-utils.h"

struct str_buffer {
  char *s;
  int len;
  int cap;
};

str_buffer_t *
make_str_buffer()
{
  str_buffer_t *sb = xmalloc(sizeof(str_buffer_t));
  sb->len = 0;
  sb->cap = 1;
  sb->s = xcalloc(sb->cap + 1, sizeof(char));
  return sb;
}

void
str_buffer_add_n(str_buffer_t *sb, char *s, int n)
{
  if (sb->cap < sb->len + n) {
    int needed_cap = sb->cap;
    while (needed_cap < sb->len + n) {
      needed_cap *= 2;
    }
    sb->s = xrealloc(sb->s, needed_cap + 1);
    sb->cap = needed_cap;
  }

  strncpy(sb->s + sb->len, s, n);
  sb->len += n;
  sb->s[sb->len] = '\0';
}

void
str_buffer_add(str_buffer_t *sb, char *s)
{
  str_buffer_add_n(sb, s, strlen(s));
}

void
str_buffer_printf(str_buffer_t *sb, char *fmt, ...)
{
  va_list va;
  va_start(va, fmt);
  str_buffer_vprintf(sb, fmt, va);
  va_end(va);
}

void
str_buffer_vprintf(str_buffer_t *sb, char *fmt, va_list va)
{
  char *addition = str_vaprintf(fmt, va);
  str_buffer_add(sb, addition);
  free(addition);
}

/* Return accumulated string and free the buffer.
   String is to be freed by free().
*/
char *
str_buffer_finish(str_buffer_t *sb, int *len)
{
  char *res = sb->s;
  if (len != NULL) *len = sb->len;
  free(sb);
  return res;
}

char *
str_replace_all(char *where, char *from, char *to)
{
  str_buffer_t *sb = make_str_buffer();
  char *whp = where;
  char *ss;
  while ((ss = strstr(whp, from)) != NULL) {
    str_buffer_add_n(sb, whp, ss - whp);
    str_buffer_add(sb, to);
    whp = ss + strlen(from);
  }
  str_buffer_add(sb, whp);
  return str_buffer_finish(sb, NULL);
}

char *
str_tolower(char *s)
{
  char *r = xstrdup(s);
  for (char *p = r; *p != '\0'; p++) {
    *p = tolower(*p);
  }
  return r;
}

char *
str_toupper(char *s)
{
  char *r = xstrdup(s);
  for (char *p = r; *p != '\0'; p++) {
    *p = toupper(*p);
  }
  return r;
}

/* Remove leading and trailing whitespace.
   Works in place, modifying source.
*/
char *
str_strip(char *str)
{
  int len = strlen(str);
  int start_ix;
  int end_ix;
  for (start_ix = 0; start_ix < len; start_ix++) {
    if (!isspace(str[start_ix])) break;
  }
  for (end_ix = len; end_ix > 0; end_ix--) {
    if (!isspace(str[end_ix - 1])) break;
  }
  if (start_ix > end_ix) {
    *str = '\0';
  } else {
    memmove(str, str + start_ix, end_ix - start_ix);
    str[end_ix - start_ix] = '\0';
  }
  return str;
}

/* Count fields divided by delims */
int
str_count_fields(char *str, char *delims)
{
  int n = 0;
  char *p = str + strspn(str, delims);
  while (*p != '\0') {
    p += strcspn(p, delims);
    p += strspn(p, delims);
    n++;
  }
  return n;
}

char *
str_aprintf(char *format, ...)
{
  char *res;
  va_list va;

  va_start(va, format);
  res = str_vaprintf(format, va);
  va_end(va);
  return res;
}

char *
str_vaprintf(char *format, va_list va)
{
  char zbuf[1];
  va_list va2;
  va_copy(va2, va);
  int r = vsnprintf(zbuf, 1, format, va2);
  va_end(va2);
  char *p = xmalloc(r + 1);
  vsnprintf(p, r + 1, format, va);
  return p;
}

char *
str_screen(char *str, char *to_screen)
{
  str_buffer_t *sbuf = make_str_buffer();
  char *ss;
  for (ss = str; *ss; ss++) {
    if (strchr(to_screen, *ss) || *ss == '\\') {
      str_buffer_add(sbuf, "\\");
    }
    str_buffer_add_n(sbuf, ss, 1);
  }
  return str_buffer_finish(sbuf, NULL);
}

struct tr_entry {
  char from;
  char to;
} tr_tab[] = {
  {'a', '\a'},
  {'b', '\b'},
  {'f', '\f'},
  {'n', '\n'},
  {'r', '\r'},
  {'t', '\t'},
  {'v', '\v'},
  {'\0', '\0'}
};

char *
str_unquote(char *s)
{
  char *sp;
  char *rp;
  char *r = xmalloc(strlen(s) + 1);
  rp = r;
  sp = s;
  while (*sp != '\0') {
    if (*sp == '\\') {
      bool found = false;
      struct tr_entry *te;
      sp++;
      if (*sp == '\0') break;
      for (te = tr_tab; te->from != '\0'; te++) {
        if (*sp == te->from) {
          found = true;
          *(rp++) = te->to;
          break;
        }
      }
      if (!found) {
        *(rp++) = *(sp++);
      }
    } else {
      *(rp++) = *(sp++);
    }
  }
  *rp = '\0';
  return r;
}


/* Count occurrences of c in s */
int
str_count(char *s, char c)
{
  char *p;
  int count = 0;

  while ((p = strchr(s, c)) != NULL) {
    count++;
    s = p + 1;
  }

  return count;
}

#ifdef TEST

#undef NDEBUG
#include <assert.h>

int
main(int argc, char *argv[])
{
  UNUSED(argc); UNUSED(argv);
  
  {
    char *s;
    int len;
    str_buffer_t *sb = make_str_buffer();
    str_buffer_add(sb, "aa");
    str_buffer_add_n(sb, "bbb", 2);
    s = str_buffer_finish(sb, &len);
    assert(str_eq(s, "aabb"));
    assert(len == 4);
  }

  {
    char *s = str_replace_all("aabbccaabbccaa", "aa", "xxx");
    assert(str_eq(s, "xxxbbccxxxbbccxxx"));
    free(s);
  }
  {
    char  *s = xstrdup("   aaa ");
    str_strip(s);
    assert(str_eq(s, "aaa"));
    free(s);
  }
  {
    char *s = "aBc AbC";
    char *l = str_tolower(s);
    assert(str_eq(l, "abc abc"));
    free(l);
  }
  {
    char *s = "aBc AbC";
    char *u = str_toupper(s);
    assert(str_eq(u, "ABC ABC"));
    free(u);
  }
  {
    char *fmt = "Xuxu %d";
    int n = 3333;
    char *sp = str_aprintf(fmt, n);
    assert(str_eq(sp, "Xuxu 3333"));
  }

  {
    char *s = "abc|def||";
    int n = str_count(s, '|');
    assert(n == 3);
  }

  /* Need more tests. */
  return 0;
}

#endif
