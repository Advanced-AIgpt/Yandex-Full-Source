/* Externally visible file -- standard guards */
#ifndef NORMALIZE_H
#  define NORMALIZE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct norm_data norm_data_t;

typedef struct mem_blob mem_blob_t;
typedef struct fst_data fst_data_t;

norm_data_t *norm_data_read(const char *path);
norm_data_t *norm_data_read_from_blobs(mem_blob_t flags, mem_blob_t symbols, fst_data_t* fst_data, size_t fst_data_size);
void norm_data_free(norm_data_t *rd);

/* Get model version.
   The returned value belongs to the config.
*/
char *norm_model_version(norm_data_t *normalizer_data);

/* Result will be NULL-terminated array.
   Result to be freed by free(). */
char **norm_data_get_normalizer_names(norm_data_t *normalizer_data);

/* Result to be freed by free()
   Whitelist and blacklist are NULL-terminated arrays */
char *norm_run(norm_data_t *normalizer_data, char *string);
char *norm_run_with_whitelist(norm_data_t *normalizer_data, char *string, char **whitelist);
char *norm_run_with_blacklist(norm_data_t *normalizer_data, char *string, char **blacklist);

#ifdef __cplusplus
}
#endif

#endif
