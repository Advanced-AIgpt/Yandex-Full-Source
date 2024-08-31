#pragma once

#ifndef REVERSE_NORMALIZE_H
#  define REVERSE_NORMALIZE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char *word_str;
    float start_time;
    float end_time;
    float confidence;
} recognized_word_t;

typedef struct {
    int n_words;
    recognized_word_t *words;
} recognition_result_t;

/* Result to be freed by free()
   blacklist is NULL-terminated array */
char *reverse_normalize(norm_data_t *normalizer_data,
                        recognition_result_t *recognized,
                        char **blacklist,
                        int expected_number_length);

#ifdef __cplusplus
}
#endif

#endif
