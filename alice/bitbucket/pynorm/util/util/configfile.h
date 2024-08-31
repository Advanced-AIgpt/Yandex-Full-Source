/**
 * @file util/configfile.h
 *
 * @date 2014
 *
 * @author Georgy Bronnikov <gogabr@yandex-team.ru>
 *
 * @copyright 2014 Yandex. All rights reserved.
 *
 * Reading parameters from a text file.
 */

#ifdef CONFIGFILE_H
#error "configfile.h: double inclusion"
#endif
#define CONFIGFILE_H

#include "mem_blob.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* A simple and inefficient config file reader.
   I looked for a ready-made library but found none worth importing.
*/

typedef struct configfile configfile_t;

configfile_t *configfile_read(const char *fname);
configfile_t *configfile_read_from_blob(mem_blob_t config);

configfile_t *configfile_from_string(char *str); // string is copied
configfile_t *configfile_empty(void);
void configfile_free(configfile_t *cf);

/* All the reading functions receive a pointer to where they need to
   store the result. If a flag is not found, the value referred does not change,
   so the client does need to bother checking.

   Return value: true if the flag is present.
*/
bool configfile_get_bool(configfile_t *cf, char *name, bool *vp);
bool configfile_get_int(configfile_t *cf, char *name, int32_t *vp);
bool configfile_get_float(configfile_t *cf, char *name, REAL *vp);

// Returned string is owned by the configfile_t structure.
bool configfile_get_string(configfile_t *cf, char *name, char **vp);

/* Set a config value.
   Copies the strings.
   Inefficient; don't use often.
*/
void configfile_set(configfile_t *cf, char *name, char *value);

#ifdef __cplusplus
}
#endif
