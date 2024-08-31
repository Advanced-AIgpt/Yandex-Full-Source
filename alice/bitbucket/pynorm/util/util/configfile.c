/**
 * @file util/configfile.c
 *
 * @date 2014
 *
 * @author Georgy Bronnikov <gogabr@yandex-team.ru>
 *
 * @copyright 2014 Yandex. All rights reserved.
 *
 * Reading parameters from a text file.
 */
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE // for strtok_r under Darwin/iOS
#endif
//#define _POSIX_SOURCE // for strtok_r
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(_WIN32) && !defined (_WIN64)
#include <strings.h> // strcasecmp
#endif // !_WIN32 || _WIN64

#include <libcalg/compare-string.h>
#include <libcalg/hash-string.h>
#include <libcalg/hash-table.h>

#include "config.h"
#include "log.h"
#include "util.h"
#include "string-utils.h"
#include "xalloc.h"

#include "configfile.h"

struct configfile {
  char *buf;
  HashTable *entries;

  char **extras;
  int n_extras;
};

void
str2configfile_entry(char *source_str, char *delimeters, char **namep, char **valuep)
{
  str_strip(source_str);
  size_t name_len = strcspn(source_str, delimeters);
  char *value = NULL;
  if (name_len < strlen(source_str)) {
    source_str[name_len] = '\0';
    value = source_str + name_len + 1;
    value += strspn(value, delimeters);
  } else {
    value = "";
  }
  *namep = source_str;
  *valuep = value;
}

/*
   Create a configfile from string;
   the string becomes owned by the configfile structure.
*/
static configfile_t *
configfile_from_string_owned(char *buf)
{
  if (buf == NULL) {
    return NULL;
  }

  HashTable *entries = hash_table_new(string_hash, string_equal);

  {
    char *tokstate;
    for (char *p = strtok_r(buf, "\r\n", &tokstate);
         p != NULL;
         p = strtok_r(NULL, "\r\n", &tokstate)) {
      if (*p == '#') continue;

      char *name;
      char *value;
      str2configfile_entry(p, " =", &name, &value);
      if (hash_table_lookup(entries, name) != NULL) {
        log_warning("%s: key %s repeats more than once", __func__, name);
      }
      hash_table_insert(entries, name, value);
    }
  }

  {
    configfile_t *cf = xmalloc(sizeof(configfile_t));
    cf->buf = buf;
    cf->entries = entries;
    cf->extras = NULL;
    cf->n_extras = 0;
    return cf;
  }
}

configfile_t *
configfile_from_string(char *buf)
{
  return configfile_from_string_owned(xstrdup(buf));
}

configfile_t *
configfile_read(const char *fname)
{
  return configfile_from_string_owned(read_whole_file(fname, NULL));
}

configfile_t *
configfile_read_from_blob(mem_blob_t config)
{
  char *content = read_whole_blob(config);
  return configfile_from_string_owned(content);
}

configfile_t *
configfile_empty()
{
  configfile_t *cf = xmalloc(sizeof(configfile_t));
  cf->buf = NULL;
  cf->entries = hash_table_new(string_hash, string_equal);
  cf->extras = NULL;
  cf->n_extras = 0;
  return cf;
}

void
configfile_free(configfile_t *cf)
{
  if (cf == NULL) return;

  {
    int i;
    for (i = 0; i < cf->n_extras; i++) {
      free(cf->extras[i]);
    }
  }

  free(cf->extras);
  free(cf->buf);
  hash_table_free(cf->entries);
  free(cf);
}

/* All the reading functions receive a pointer to where they need to
   store the result. If a flag is not found, the value referred does not change,
   so the client does not need to bother checking.

   Return value: true if the flag is present.
*/
bool
configfile_get_bool(configfile_t *cf, char *name, bool *vp)
{
  char *s;
  if (!configfile_get_string(cf, name, &s)) return false;

  *vp = (strcmp(s, "") == 0) || // empty string means true here
        (strcasecmp(s, "true") == 0) ||
        (strcmp(s, "1") == 0);
  return true;
}

bool
configfile_get_int(configfile_t *cf, char *name, int32_t *vp)
{
  char *s;
  if (!configfile_get_string(cf, name, &s)) return false;

  *vp = atoi(s);
  return true;
}

bool
configfile_get_float(configfile_t *cf, char *name, REAL *vp)
{
  char *s;
  if (!configfile_get_string(cf, name, &s)) return false;

  *vp = string_to_float(s);
  return true;
}

// Returned string is owned by the configfile_t structure.
bool
configfile_get_string(configfile_t *cf, char *name, char **vp)
{
  char *v = hash_table_lookup(cf->entries, name);
  if (v == NULL) {
    return false;
  } else {
    *vp = v;
    return true;
  }
}

/* Set a config value.
   Copies the strings.
   Inefficient; don't use often.
*/
void
configfile_set(configfile_t *cf, char *name, char *value)
{
  char *name_copy = xstrdup(name);
  char *value_copy = xstrdup(value);

  /* Remember the copies as extras */
  {
    cf->extras = xrealloc(cf->extras, (cf->n_extras + 2) * sizeof(char*));
    cf->extras[cf->n_extras] = name_copy;
    cf->extras[cf->n_extras + 1] = value_copy;
    cf->n_extras += 2;
  }

  hash_table_insert(cf->entries, name_copy, value_copy);
}
