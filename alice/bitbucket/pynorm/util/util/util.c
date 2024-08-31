/**
 * @file util/util.c
 *
 * @date 2014
 *
 * @author Georgy Bronnikov <gogabr@yandex-team.ru>
 *
 * @copyright 2014 Yandex. All rights reserved.
 *
 * Small pieces that don't fit anywhere else.
 */

//#define _POSIX_C_SOURCE 200809L
#ifndef _GNU_SOURCE
#define _GNU_SOURCE /* for exp10f; how do I get it in other OSs? */
#endif
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(_WIN32) && !defined (_WIN64)
#  include <unistd.h> /* For isatty() in read_line(). */
#else
#  define isatty(f) false
#endif

#include "config.h"
#include "xalloc.h"

#include "log.h"
#include "mem_blob.h"

#include "util.h"

#if defined(ANDROID)
#  include <cpu-features.h>
#  define exp10f(arg) pow(10, arg)
#elif defined(__APPLE__)
#  include <sys/sysctl.h>
#  define exp10f __exp10f
#elif defined(_MSC_VER)
# define exp10f(arg) powf(10.0f, arg)
#endif

static const int MAX_NUMBER_LENGTH = 30;

static int
sprint_float(char *s, REAL v)
{
  if (ISNAN(v)) {
    return sprintf(s, " nan");
  } else if (ISINF(v) && v > 0) {
    return sprintf(s, " +inf");
  } else if (ISINF(v) && v < 0) {
    return sprintf(s, " -inf");
  }

  int r = 0;
  int rr;
  *(s++) = ' '; r++;
  if (v < 0) {
    *(s++) = '-'; r++;
    v = -v;
  } else {
    *(s++) = ' '; r++;
  }
  rr = sprintf(s, "%d", (int)v); s += rr; r += rr;
  *(s++) = '.'; r++;
  rr = sprintf(s, "%03d", (int)((v - (int)v) * 1000)); s += rr; r += rr;

  return r;
}

void
print_vector(int32_t sz, REAL *vec, char *header_fmt, ...)
{
  char *buf = xmalloc(strlen(header_fmt) + MAX_NUMBER_LENGTH * sz + 1);
  char *bp = buf;
  va_list va;
  va_start(va, header_fmt);
  bp += vsprintf(bp, header_fmt, va);
  if (vec == NULL) {
    bp += sprintf(bp, "(null)");
  } else {
    int32_t i;
    for (i = 0; i < sz; i++) {
      int r = sprint_float(bp, vec[i]);
      bp += r;
    }
  }
  log_debug(buf);
  va_end(va);
  free(buf);
}

void
print_matrix(int32_t nrows, int32_t ncols, REAL *mat, char *header_fmt, ...)
{
  char header_str[256];
  va_list va;
  va_start(va, header_fmt);

  vsprintf(header_str, header_fmt, va);
  if (mat == NULL) {
    log_debug("(null)");
  } else {
    int32_t i, j;
    int rr;
    char *bp;
    char *buf = xmalloc(strlen(header_fmt) + MAX_NUMBER_LENGTH * ncols + 1);
    for (i = 0; i < nrows; i++) {
      bp = buf;
      rr = sprintf(bp, "%s [%2d] ", header_str, i); bp += rr;
      for (j = 0; j < ncols; j++) {
        rr = sprint_float(bp, mat[i * ncols + j]); bp += rr;
      }
      log_debug(buf);
    }
    free(buf);
  }
  va_end(va);
}

void
print_int_vector(int32_t sz, int32_t *vec, char *header_fmt, ...)
{
  char buf[2048];
  char *bp = buf;
  va_list va;
  va_start(va, header_fmt);
  bp += vsprintf(bp, header_fmt, va);
  if (vec == NULL) {
    bp += sprintf(bp, "(null)");
  } else {
    int32_t i;
    for (i = 0; i < sz; i++) {
      int r = sprintf(bp, " %d", vec[i]);
      bp += r;
    }
  }
  log_debug(buf);
  va_end(va);
}

void *
read_whole_file(const char *fname, int32_t *szp)
{
  char *buf;
  FILE *fs = fopen(fname, "rb");
  int32_t sz;

  if (fs == NULL) {
    log_error("Could not read file %s into memory: %s", fname, strerror(errno));
    return NULL;
  }

  fseek(fs, 0, SEEK_END);
  sz = ftell(fs);
  rewind(fs);

  buf = xmalloc(sz+1);
  if (buf == NULL) {
    log_error("Could not allocate %d bytes for contents of %s", sz, fname);
    goto OUT;
  }

  {
    int32_t r;
    if ((r = fread(buf, 1, sz, fs)) != sz) {
      log_error("Error reading %d bytes as contents of %s: got %d, %s",
                sz, fname, r, strerror(errno));
      free(buf);
      buf = NULL;
      goto OUT;
    }
  }

  buf[sz] = '\0'; // make it safe

OUT:
  if (szp != NULL) *szp = sz;
  fclose(fs);
  return buf;
}

void *
read_whole_blob(mem_blob_t blob)
{
    char* buf;
    buf = xmalloc(blob.size + sizeof(char));
    if (buf == NULL) {
        log_error("Could not allocate %d bytes for content of a blob", blob.size + 1);
        return NULL;
    }
    memcpy(buf, blob.data, blob.size);
    buf[blob.size] = '\0';

    return buf;
}


char *
astrcat(const char *s1, ...)
{
  char *res;
  int32_t len;
  va_list va1, va2;

  va_start(va1, s1);
  va_copy(va2, va1);

  len = strlen(s1);
  {
    const char *sx;
    for (sx = va_arg(va1, const char*);
         sx != NULL;
         sx = va_arg(va1, const char*)) {
      len += strlen(sx);
    }
  }

  res = xmalloc(len+1);
  {
    char *dp = stpcpy(res, (char *)s1);
    const char *sx;
    for (sx = va_arg(va2, const char*);
         sx != NULL;
         sx = va_arg(va2, const char*)) {
      dp = stpcpy(dp, (char *)sx);
    }
  }

  va_end(va2);
  va_end(va1);

  return res;
}

REAL
sigmoid(REAL x)
{
  return 1.0f / (1.0f + expf( -x ));
}

/* Locale-independent string->float conversion */
REAL
string_to_float(char *s)
{
    bool negative = false;
    REAL v = 0.0;
    if (*s == '-') {
        negative = true;
        s++;
    } else if (*s == '+') {
        s++;
    }

    while (isdigit(*s)) {
        v = v * 10.0f + (REAL)(*s - '0');
        s++;
    }

    if (*s == '.') {
        REAL pow_of_10 = 1.0;
        s++;
        while (isdigit(*s)) {
            pow_of_10 /= 10.0;
            v += (REAL)(*s - '0') * pow_of_10;
            s++;
        }
    }

    if (*s == 'e' || *s == 'E') {
        int expv;
        s++;
        expv = atoi(s);

        v *= (REAL)exp10f(expv);
    }

    if (negative) {
        v = -v;
    }

    return v;
}


static int
char_count(char *s, char c)
{
  int cou = 0;
  char *scp = s;
  while (scp != NULL) {
    scp = strchr(scp, c);
    if (scp != NULL) {
      scp++;
      cou++;
    }
  }

  return cou;
}

int *
string_to_int_array(char *s, int *npos)
{
  int ncolons;
  char *scopy;
  int *res;

  scopy  = xstrdup(s);
  ncolons = char_count(scopy, ':');

  /* For some uses, it makes sense to terminate the array with zero. */
  res = xcalloc(ncolons+2, sizeof(int));
  {
    char *cp;
    char *saveptr = NULL;
    int i;
    for (i = 0, cp = strtok_r(scopy, ":", &saveptr);
         cp != NULL;
         i++, cp = strtok_r(NULL, ":", &saveptr)) {
      res[i] = atoi(cp);
    }
  }
  if (npos != NULL) {
    *npos = ncolons + 1;
  }

  free(scopy);
  return res;
}

REAL *
string_to_float_array(char *s, int *npos)
{
  int ncolons;
  char *scopy;
  REAL *res;

  scopy  = xstrdup(s);
  ncolons = char_count(scopy, ':');

  /* For some uses, it makes sense to terminate the array with zero. */
  res = xcalloc(ncolons+2, sizeof(REAL));
  {
    char *cp;
    char *saveptr = NULL;
    int i;
    for (i = 0, cp = strtok_r(scopy, ":", &saveptr);
         cp != NULL;
         i++, cp = strtok_r(NULL, ":", &saveptr)) {
      res[i] = string_to_float(cp);
    }
  }
  if (npos != NULL) {
    *npos = ncolons + 1;
  }

  free(scopy);
  return res;
}

int **
string_to_int_array_array(char *s, int *npos)
{
  int ncolons;
  char *scopy;
  int **res;

  scopy  = xstrdup(s);
  ncolons = char_count(scopy, ':');

  res = xmalloc((ncolons + 2) * sizeof(int*));
  {
    char *scopy_substr;
    char *saveptr = NULL;
    int i;
    for (i = 0, scopy_substr = strtok_r(scopy, ":", &saveptr);
         scopy_substr != NULL;
         i++, scopy_substr = strtok_r(NULL, ":", &saveptr)) {
      int ncommas = char_count(scopy_substr, ',');
      int *sub_res = xmalloc((ncommas + 2) * sizeof(int));
      char *cp;
      char *saveptr = NULL;
      int j;
      for (j = 0, cp = strtok_r(scopy_substr, ",", &saveptr);
           cp != NULL;
           j++, cp = strtok_r(NULL, ",", &saveptr)) {
        sub_res[j] = atoi(cp);
      }
      sub_res[j] = 0; // Terminating sub-array with zero
      res[i] = sub_res;
    }
    res[i] = NULL; // Terminating array with NULL
  }
  if (npos != NULL) {
    *npos = ncolons + 1;
  }

  free(scopy);
  return res;
}

char **
string_to_string_array(char *s, int *npos, char delim)
{
  int ncolons;
  int npos_local;
  char *scopy;
  char **res;

  char delim_str[2];
  delim_str[0] = delim;
  delim_str[1] = '\0';

  /* Corner case: we don't want to lose the start of scopy if there's a delimiter at the start. */
  while (*s == delim) {
    s++;
  }

  scopy  = xstrdup(s);
  ncolons = char_count(scopy, delim);

  /* For some uses, it makes sense to terminate the array with zero. */
  res = xcalloc(ncolons+2, sizeof(char*));
  /* Count fields separately from colons, since we ignore empty fields. */
  npos_local = 0;
  {
    char *cp;
    char *saveptr = NULL;
    int i;
    for (i = 0, cp = strtok_r(scopy, delim_str, &saveptr);
         cp != NULL;
         i++, cp = strtok_r(NULL, delim_str, &saveptr)) {
      res[i] = cp;
      npos_local++;
    }
  }
  if (npos != NULL) {
    *npos = npos_local;
  }
  if (npos_local == 0) {
    free(scopy);	// no other way to reach this memory block.
  }

  return res;
}

char **
string_array_append(char **sa, char *new_item, int *npos)
{
  if (sa == NULL || new_item == NULL) {
    goto BAD;
  }

  size_t old_len = 0;
  for (char **p = sa; *p != NULL; ++p) {
    ++old_len;
  }

  if (old_len == 0) {
    sa = xrealloc(sa, 2 * sizeof(char*));

    sa[1] = NULL;
    sa[0] = xstrdup(new_item);

    if (npos != NULL) {
      *npos = 1;
    }
    return sa;
  }

  size_t old_buff_len = (sa[old_len-1] - sa[0]) + strlen(sa[old_len-1]) + 1;
  size_t new_item_len = strlen(new_item);
  size_t new_buff_len = old_buff_len + new_item_len + 1;
  char *old_buff_ptr = sa[0];
  char *new_buff_ptr = xrealloc(old_buff_ptr, new_buff_len * sizeof(char));

  int new_len = old_len + 1;
  sa = xrealloc(sa, (new_len + 1) * sizeof(char*));

  {
    size_t offset;
    for (size_t i = 0; i < old_len; ++i) {
      offset = sa[i] - old_buff_ptr;
      sa[i] = new_buff_ptr + offset;
    }
    sa[new_len - 1] = new_buff_ptr + old_buff_len;
    strcpy(sa[new_len - 1], new_item);
  }

  if (npos != NULL) {
    *npos = new_len;
  }
  return sa;

BAD:
  string_array_free(sa);
  return NULL;
}

/* Should only be used to free arrays obtained from string_to_int_array_array */
void
int_array_array_free(int **iaa)
{
  if (iaa != NULL) {
    int i;
    for (i = 0; iaa[i] != NULL; ++i) {
      free(iaa[i]);
    }
    free(iaa);
  }
}

/* Should only be used to free arrays obtained from string_to_string_array */
void
string_array_free(char **sa)
{
  if (sa != NULL) free(*sa);
  free(sa);
}

bool
have_neon()
{
#if defined(__arm__) || defined(_M_ARM)
  bool neon = false;

#  if defined(ANDROID)
    static AndroidCpuFamily family = 0;
    static uint64_t features = 0;

    if (family == 0) {
      family = android_getCpuFamily();
      features = android_getCpuFeatures();
    }
    if ((family == ANDROID_CPU_FAMILY_ARM) &&
        (features & ANDROID_CPU_ARM_FEATURE_NEON)) {
      neon = true;
    }
#  elif defined(__APPLE__) || defined(_MSC_VER) || defined(__linux)
    neon = true;
#  endif  // ANDROID

  if (neon) {
    return true;
  } else {
    return false;
  }

#elif defined(__aarch64__)
  // NEON always present
  return true;

#else // !__arm__ && !__aarch64__

  return false;

#endif // __arm__ && !__aarch64__
}

int
number_of_cores()
{
#if defined(__linux)
  int min, max;
  int rr;
  FILE *fd = fopen("/sys/devices/system/cpu/possible", "r");
  if (fd == NULL) return 1;
  rr = fscanf(fd,"%d-%d", &min, &max);
  if (rr != 2) return 1;
  fclose(fd);
  return max - min + 1;
#elif defined(ANDROID)
  return android_getCpuCount();
#elif defined(__APPLE__)
  size_t len;
  unsigned int ncpu;
  len = sizeof(ncpu);
  sysctlbyname("hw.physicalcpu", &ncpu, &len, NULL, 0);
  return ncpu;
#else
  /* !!!! Need to write this code for other OSs */
  return 1;
#endif
}

/* Reads a line from `f`; otherwise behaves as read_line()`. */
static char *
fread_line(FILE *f)
{
  enum {
    LEN = 128
  };

  if (isatty(fileno(f))) {
    fputs("> ", stdout);
    fflush(stdout);
  }

  {
    int len = LEN;
    char *s = xmalloc(len);
    *s = '\0';

    while (true) {
      char *ss = s + strlen(s);
      char *r = fgets(ss, len - (ss - s), f);

      if (r == NULL && *s == '\0') {
          /* Nothing read; return NULL */
          free(s);
          return NULL;
      } else if (r == NULL && *s != '\0') {
        /* Return what we read from the previous fgets-es */
        return s;
      } else if (r != NULL && r[strlen(r) - 1] == '\n') {
        /* Got EOL */
        r[strlen(r) - 1] = '\0';
        return s;
      } else {
        /* Need to read more */

        assert(r != NULL && r[strlen(r) - 1] != '\n');

        len *= 2;
        s = xrealloc(s, len);
      }
    }
  }
}

/* Reads a line from stdin, with a prompt if at a tty.
   Final '\n' is removed.
 */
char *
read_line()
{
  return fread_line(stdin);
}

char *
extract_filename_from_path(char *path)
{
    char *name_start;
    char *name_end;
    int name_length;
    char *name;

    name_start = strrchr(path, PATH_SEP[strlen(PATH_SEP) - 1]);
    if (name_start == NULL) {
        name_start = path;
    } else {
        name_start++;
    }

    name_end = strrchr(name_start, '.');
    if (name_end == NULL) {
        name_end = name_start + strlen(name_start);
    }

    name_length = name_end - name_start;
    name = xmalloc((name_length + 1) * sizeof(char));
    memcpy(name, name_start, name_length);
    name[name_length] = '\0';
    return name;
}

#if (defined(ANDROID) && defined(__arm__)) || (defined(ANDROID) && defined(__i386__)) || defined(_MSC_VER)

/* !!!! On Android/x86 in release mode the following code
   leads to a recursive call to stpcpy.
   The optimizer is a little bit too clever.
*/
/* char * */
/* stpcpy(char *dest, char *src) */
/* { */
/*   strcpy(dest, src); */
/*   return dest + strlen(dest); */
/* } */

char *
stpcpy(char *dest, char *src)
{
  char *dp = dest;
  char *sp = src;
  while (*sp != '\0') {
    *(dp++) = *(sp++);
  }
  *dp = '\0';
  return dp;
}

#endif

#if defined(_MSC_VER)

int
strcasecmp(char *s1, char *s2)
{
  for (; *s1 != '\0'; s1++, s2++) {
    char cl1 = tolower(*s1);
    char cl2 = tolower(*s2);
    if (cl1 < cl2) {
      return -1;
    } else if (cl1 > cl2) {
      return 1;
    }
  }

  if (*s2 != '\0') {
    return -1;
  }

  return 0;
}

#elif __STDC_VERSION__ < 201112L || defined(__UCLIBC__)

/* Not defined in older versions of the language */
float
exp10f(float x)
{
  return powf(10.0f, x);
}

#endif

int32_t
rand_from_state (unsigned int *seed)
{
  if (seed == NULL) {
    return rand();
  }

  unsigned int next = *seed;
  int32_t result;
  next *= 1102188465;
  next += 23456;
  result = (unsigned int) (next / 65536) % 2048;
  next *= 1102188465;
  next += 23456;
  result <<= 10;
  result ^= (unsigned int) (next / 65536) % 1024;
  next *= 1102188465;
  next += 23456;
  result <<= 10;
  result ^= (unsigned int) (next / 65536) % 1024;
  *seed = next;
  return result;
}
