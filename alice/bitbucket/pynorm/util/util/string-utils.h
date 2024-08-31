#ifdef STRING_UTILS_H
#  error "string-utils.h: double inclusion"
#endif
#define STRING_UTILS_H
#ifdef __cplusplus
extern "C" {
#endif
/* Unless explicitly specified, returned strings are to be free()'d */

/* String buffers */

typedef struct str_buffer str_buffer_t;

str_buffer_t *make_str_buffer();
void str_buffer_add(str_buffer_t *sb, char *s);
void str_buffer_add_n(str_buffer_t *sb, char *s, int n);
void str_buffer_printf(str_buffer_t *sb, char *fmt, ...);


/* Return accumulated string and free the buffer.
   If len is not NULL, string length is put into *len.
   String is to be freed by free().
*/
char *str_buffer_finish(str_buffer_t *sb, int *len);

char *str_replace_all(char *where, char *from, char *to);

char *str_tolower(char *s);
char *str_toupper(char *s);

/* Remove leading and trailing whitespace.
   Works in place, modifying source.
*/
char *str_strip(char *str);

/* Count fields divided by delims */
int str_count_fields(char *str, char *delims);

char *str_aprintf(char *format, ...);

/* Screen characters from t_screen in str by '\'. '\' itself is always screened. */
char *str_screen(char *str, char *to_screen);

#define str_eq(s1, s2) (strcmp((s1), (s2)) == 0)

#define str_starts_with(what, prefix) (strncmp((what), (prefix), strlen(prefix)) == 0)
#define str_suffix_after(what, prefix) ((what) + strlen(prefix))

char *str_unquote(char *s);

/* Count occurrences of c in s */
int str_count(char *s, char c);

/* Only declare functions that use va_list if stdarg.h is included before us. */
#if defined(va_start)
  void str_buffer_vprintf(str_buffer_t *sb, char *fmt, va_list va);
  char *str_vaprintf(char *format, va_list va);
#endif

#ifdef __cplusplus
}
#endif
