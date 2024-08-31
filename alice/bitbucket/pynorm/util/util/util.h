/**
 * @file util/util.h
 *
 * @date 2014
 *
 * @author Georgy Bronnikov <gogabr@yandex-team.ru>
 *
 * @copyright 2014 Yandex. All rights reserved.
 *
 * Small pieces that don't fit anywhere else.
 */

#ifdef UTIL_H
#error "util.h: double inclusion"
#endif
#define UTIL_H

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) < (y) ? (y) : (x))
#define RAND_FROM_STATE_MAX 0x7FFFFFFF // 2^31 - 1
#define RANDF(state) (((REAL)rand_from_state(state) + (REAL)1.0) / ((REAL)RAND_FROM_STATE_MAX + (REAL)2.0)) /* copied from Kaldi; should be in (0.0 .. 1.0) */

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct mem_blob mem_blob_t;

void print_vector(int32_t sz, REAL *vec, char *header_fmt, ...);
void print_matrix(int32_t nrows, int32_t ncols, REAL *mat, char *header_fmt, ...);

void print_int_vector(int32_t sz, int32_t *v, char *header_fmt, ...);

// To be freed by free()
void *read_whole_file(const char *fname, int32_t *szp);
void *read_whole_blob(mem_blob_t blob);


// Concatenate strings and return a malloced result.
char *astrcat(const char *s1, ...);

// Convert a colon-separated string into an array of integers.
// The array will be terminated by a zero entry.
// Npos is allowed to be NULL; in this case it is not written.
// Returned value to be freed by free()
int *string_to_int_array(char *s, int *npos);
REAL *string_to_float_array(char *s, int *npos);
int **string_to_int_array_array(char *s, int *npos);

char **string_to_string_array(char *s, int *npos, char delim);
char **string_array_append(char **sa, char *new_item, int *npos);

/* Should only be used to free arrays obtained from string_to_int_array_array */
void int_array_array_free(int **iaa);
/* Should only be used to free arrays obtained from string_to_string_array */
void string_array_free(char **sa);

/* Locale-independent string->float conversion */
REAL string_to_float(char *s);

REAL sigmoid(REAL x);

bool have_neon(void);
int number_of_cores(void);

/* Reads a line from stdin, with a prompt if at a tty.
   Final '\n' is removed.
 */
char *read_line();

/* Errors for io operations. */
typedef int err_t;
enum {
    ERR_OK = 0,
    ERR_IO,
    ERR_SEMANTIC,
    ERR_EOF
};

/* To be freed by free()
   Result is filename without extension. */
char *extract_filename_from_path(char *path);

#define UNUSED(x) ((void)(x))

#if defined(_MSC_VER)
/* In MS C, alloca conflicts with stack switching */
#define stack_alloc(sz) malloc(sz)
#define stack_free(p) free(p)
#else
#define stack_alloc(sz) alloca(sz)
#define stack_free(p) {}
#endif

#if (defined(ANDROID) && defined(__arm__)) || (defined(ANDROID) && defined(__i386__)) || defined(_MSC_VER)
char *stpcpy(char *dest, char *src);
#endif

#if !defined(__cplusplus)
/* Not defined in older versions of the language */
float exp10f(float x);
#endif

#if defined(_MSC_VER)
#define strtok_r strtok_s
#define strdup _strdup /* MSVC is grumpy about the plain name. */

#define va_copy(dest, src) ((dest) = (src))

int strcasecmp(char *s1, char *s2);

#endif

int rand_from_state (unsigned int *seed);
#ifdef __cplusplus
}
#endif
