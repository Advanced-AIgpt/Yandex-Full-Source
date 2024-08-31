#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <libcalg/arraylist.h>

#include "util/config.h"
#include "util/util.h"
#include "util/log.h"
#include "util/xalloc.h"

#include "normalize.h"
#include "reverse-normalize.h"

static const char *CONVERT_NUMBER_FSTNAME = "numbers";
static const char *GLUE_PUNCTUATION_FSTNAME = "glue_punctuation";
static const float MINIMAL_SIGNIFICANT_TIME_INTERVAL = 0.001f;

char *CONVERT_NUMBER_FSTLIST[] = {"space_at_start", "numbers", "remove_space_at_start", NULL};
char *NORMALIZE_NUMBERS_BLACKLIST[] = {"phones", "times", "glue_punctuation", NULL};
char *GLUE_PUNCTUATION_FSTLIST[] = {"glue_punctuation", NULL};

typedef struct {
    int word_index;
    float cost;
    int reduction;
} number_length_reduction_t;

typedef struct {
    ArrayList *normalized_words;
    number_length_reduction_t *possible_length_reductions;
    int n_reductions;
} normalization_result_t;

void
recognition_result_free(recognition_result_t *result)
{
    int i;
    for (i = 0; i < result->n_words; ++i) {
        free(result->words[i].word_str);
    }
    free(result->words);
    free(result);
}

void
normalization_result_free(normalization_result_t *norm_result)
{
    unsigned int word_index;
    for (word_index = 0; word_index < norm_result->normalized_words->length; ++word_index) {
        free(norm_result->normalized_words->data[word_index]);
    }
    arraylist_free(norm_result->normalized_words);
    free(norm_result->possible_length_reductions);
    free(norm_result);
}

bool
is_number(char *word)
{
    if (word == NULL) {
        return false;
    }
    return strspn(word, "0123456789") == strlen(word);
}

int
fstlistlen(char **fstlist)
{
    int len = 0;
    if (fstlist == NULL) {
        return 0;
    }

    while (fstlist[len++] != NULL);
    return len - 1;
}

bool
numbers_maybe_joined(char *left_number, char *right_number)
{
    bool joinable = false;
    if ((left_number == NULL) || (right_number == NULL) ||
        (strlen(left_number) == 0) || (strlen(right_number) == 0)) {
        return false;
    }

    if (left_number[0] != '0') {
        unsigned int zero_suffix_length = 0;
        int char_index = 0;
        int left_number_length = strlen(left_number);
        for (char_index = left_number_length - 1; char_index > 0; --char_index) {
            if (left_number[char_index] == '0') {
                ++zero_suffix_length;
            } else {
                break;
            }
        }
        joinable = zero_suffix_length >= strlen(right_number);
    }
    return joinable;
}

char *
normalize_words_joined_by_space(norm_data_t *normalizer_data,
                                char **blacklist,
                                recognized_word_t *words,
                                int n_words,
                                char *buffer)
{
    int length = 0;
    int j, word_length;
    char *normalized = NULL;
    for (j = 0; j < n_words; ++j) {
        word_length = strlen(words[j].word_str);
        memcpy(buffer + length, words[j].word_str, word_length);
        buffer[length + word_length] = ' ';
        length += word_length + 1;
    }
    buffer[length - 1] = '\0';
    normalized = norm_run_with_blacklist(normalizer_data, buffer, blacklist);
    return normalized;
}

normalization_result_t *
create_normalization_result(int chunks_count, char **normalized_chunks, float *intervals)
{
    int chunk_index;
    char *token;
    char *safestr = NULL;
    char *previous_word;
    unsigned int word_index = 0;
    normalization_result_t *norm_result = xmalloc(sizeof(normalization_result_t));
    norm_result->normalized_words = arraylist_new(10);
    norm_result->possible_length_reductions = xmalloc(chunks_count * sizeof(number_length_reduction_t));
    norm_result->n_reductions = 0;
    for (chunk_index = 0; chunk_index < chunks_count; ++chunk_index) {
        token = strtok_r(normalized_chunks[chunk_index], " ", &safestr);
        previous_word = NULL;
        if (norm_result->normalized_words->length > 0) {
            previous_word = (char*)norm_result->normalized_words->data[norm_result->normalized_words->length - 1];
        }
        if (is_number(previous_word) &&
            is_number(token) &&
            numbers_maybe_joined(previous_word, token)) {
            number_length_reduction_t *length_reduction = &norm_result->possible_length_reductions[norm_result->n_reductions++];
            length_reduction->word_index = word_index;
            length_reduction->cost = intervals[chunk_index - 1];
            length_reduction->reduction = strlen(token);
        }
        for (;
             token != NULL;
             (token = strtok_r(NULL, " ", &safestr))) {
            arraylist_append(norm_result->normalized_words, xstrdup(token));
            word_index++;
        }
    }
    return norm_result;
}

normalization_result_t *
split_recognition_result_by_silence_between_numbers(norm_data_t *normalizer_data,
                                                    char **blacklist,
                                                    recognition_result_t *recognized)
{
    int i;
    char **converted_words = xmalloc(recognized->n_words * sizeof(char*));
    char **normalized_chunks = xmalloc(recognized->n_words * sizeof(char*));
    float *intervals = xmalloc(recognized->n_words * sizeof(float));
    int chunks_count = 0;
    int current_chunk_start = 0;
    char *buffer = NULL;
    normalization_result_t *norm_result = NULL;

    for (i = 0; i < recognized->n_words; ++i) {
        converted_words[i] = norm_run_with_whitelist(normalizer_data,
                                                     recognized->words[i].word_str,
                                                     CONVERT_NUMBER_FSTLIST);
    }

    {
        int string_length = 0;
        for (i = 0; i < recognized->n_words; ++i) {
            string_length += strlen(recognized->words[i].word_str) + 1;
        }
        buffer = xmalloc(string_length * sizeof(char));
    }

    for (i = 1; i < recognized->n_words; ++i) {
        if ((recognized->words[i].start_time - recognized->words[i - 1].end_time >= MINIMAL_SIGNIFICANT_TIME_INTERVAL) &&
            (is_number(converted_words[i])) &&
            (is_number(converted_words[i - 1])) &&
            (numbers_maybe_joined(converted_words[i - 1], converted_words[i]))) {

            intervals[chunks_count] = recognized->words[i].start_time - recognized->words[i - 1].end_time;
            normalized_chunks[chunks_count++] = normalize_words_joined_by_space(normalizer_data,
                                                                                blacklist,
                                                                                recognized->words + current_chunk_start,
                                                                                i - current_chunk_start,
                                                                                buffer);
            current_chunk_start = i;
        }
    }
    normalized_chunks[chunks_count++] = normalize_words_joined_by_space(normalizer_data,
                                                                        blacklist,
                                                                        recognized->words + current_chunk_start,
                                                                        recognized->n_words - current_chunk_start,
                                                                        buffer);
    for (i = 0; i < recognized->n_words; ++i) {
        free(converted_words[i]);
    }
    free(converted_words);
    free(buffer);

    norm_result = create_normalization_result(chunks_count, normalized_chunks, intervals);
    for (i = 0; i < chunks_count; ++i) {
        free(normalized_chunks[i]);
    }
    free(normalized_chunks);
    free(intervals);
    return norm_result;
}

char *
join_words(char **words, int n_words)
{
    int result_length = 0;
    int word_length = 0;
    int index;
    char *result_string = NULL;
    for (index = 0; index < n_words; ++index) {
        word_length = strlen(words[index]);
        if (word_length > 0) {
            result_length += word_length + 1;
        }
    }

    result_string = xmalloc(result_length * sizeof(char));
    result_length = 0;
    for (index = 0; index < n_words; ++index) {
        if (words[index][0] != '\0') {
            word_length = strlen(words[index]);
            memcpy(result_string + result_length, words[index], word_length);
            result_length += word_length + 1;
            result_string[result_length - 1] = ' ';
        }
    }
    result_string[result_length - 1] = '\0';
    return result_string;
}

int
count_numbers(char **words, int n_words)
{
    int count = 0;
    int word_index;
    bool previous_word_is_number = false;
    if (words == NULL) {
        return 0;
    }
    for (word_index = 0; word_index < n_words; ++word_index) {
        if (is_number(words[word_index])) {
            if (!previous_word_is_number) {
                ++count;
            }
            previous_word_is_number = true;
        } else {
            previous_word_is_number = false;
        }
    }
    return count;
}

char *
reduce_number_length_to_expected(normalization_result_t *norm_result,
                                 int expected_reduction,
                                 int max_length_reduction,
                                 float max_cost)
{
    int index, j = 0;
    char **words = (char**)norm_result->normalized_words->data;
    /* cost_table[i][j] is minimal cost needed to reduce number length by j while use for that only first i possible reductions.
       Reduction is a pair of numbers that we can merge to one number.
       reduction_index_table[i][j] contains index of possible reduction that used for compute cost_table[i][j]. */
    float *cost_table_buffer = xmalloc(norm_result->n_reductions * max_length_reduction * sizeof(float));
    float **cost_table = xmalloc(norm_result->n_reductions * sizeof(float*));
    int *reduction_table_buffer = xmalloc(norm_result->n_reductions * max_length_reduction * sizeof(int));
    int **reduction_index_table = xmalloc(norm_result->n_reductions * sizeof(int*));
    int current_max_reduction;
    char *result_str = NULL;
    number_length_reduction_t *length_reduction = NULL;

    // initialize tables
    for (index = 0; index < norm_result->n_reductions; ++index) {
        int shift = index * max_length_reduction;
        cost_table[index] = cost_table_buffer + shift;
        reduction_index_table[index] = reduction_table_buffer + shift;
        for (j = 0; j < max_length_reduction; ++j) {
            cost_table[index][j] = 2 * max_cost;
            reduction_index_table[index][j] = -1;
        }
    }
    cost_table[0][0] = 0;
    length_reduction = &norm_result->possible_length_reductions[0];
    cost_table[0][length_reduction->reduction] = length_reduction->cost;
    reduction_index_table[0][length_reduction->reduction] = 0;

    // fill cost_table with recurrent formula
    current_max_reduction = length_reduction->reduction;
    for (index = 1; index < norm_result->n_reductions; ++index) {
        length_reduction = &norm_result->possible_length_reductions[index];
        current_max_reduction += length_reduction->reduction;
        for (j = 0; j <= current_max_reduction; ++j) {
            cost_table[index][j] = cost_table[index - 1][j];
            reduction_index_table[index][j] = reduction_index_table[index - 1][j];
            if ((j >= length_reduction->reduction) &&
                (cost_table[index - 1][j - length_reduction->reduction] + length_reduction->cost < cost_table[index][j])) {
                cost_table[index][j] = cost_table[index - 1][j - length_reduction->reduction] + length_reduction->cost;
                reduction_index_table[index][j] = index;
            }
        }
    }

    // reconstruct sequance of number length reductions
    if (reduction_index_table[norm_result->n_reductions - 1][expected_reduction] != -1) {
        index = norm_result->n_reductions - 1;
        while (expected_reduction > 0) {
            if (reduction_index_table[index][expected_reduction] >= 0) {
                length_reduction = &norm_result->possible_length_reductions[reduction_index_table[index][expected_reduction]];
                memcpy(words[length_reduction->word_index - 1] + strlen(words[length_reduction->word_index - 1]) - length_reduction->reduction,
                       words[length_reduction->word_index],
                       length_reduction->reduction);
                words[length_reduction->word_index][0] = '\0';
                expected_reduction -= norm_result->possible_length_reductions[reduction_index_table[index--][expected_reduction]].reduction;
            } else {
                --index;
            }
        }
        result_str = join_words(words, norm_result->normalized_words->length);
    }
    free(cost_table);
    free(reduction_index_table);
    free(cost_table_buffer);
    free(reduction_table_buffer);
    return result_str;
}

char *
process_number(normalization_result_t *norm_result, int expected_number_length)
{
    char **words = (char**)norm_result->normalized_words->data;
    int n_words = norm_result->normalized_words->length;
    int number_length = 0;
    int word_index;
    int max_length_reduction = 0;
    int index;
    float max_cost = 0.0;
    if (count_numbers(words, n_words) != 1) {
        return NULL;
    }
    for (word_index = 0; word_index < n_words; ++word_index) {
        if (is_number(words[word_index])) {
            number_length += strlen(words[word_index]);
        }
    }
    for (index = 0; index < norm_result->n_reductions; ++index) {
        max_length_reduction += norm_result->possible_length_reductions[index].reduction;
        max_cost += norm_result->possible_length_reductions[index].cost;
    }
    if ((number_length == 0) ||
        (number_length < expected_number_length) ||
        (number_length - expected_number_length > max_length_reduction)) {
        return NULL;
    } else if (number_length == expected_number_length) {
        return join_words(words, n_words);
    }
    max_length_reduction += 1;
    return reduce_number_length_to_expected(norm_result,
                                            number_length - expected_number_length,
                                            max_length_reduction,
                                            max_cost);
}

char *
standard_norm_run(norm_data_t *normalizer_data,
                  recognition_result_t *result,
                  char **blacklist)
{
    int i;
    char **words = xmalloc(result->n_words * sizeof(char*));
    char *result_str = NULL;
    for (i = 0; i < result->n_words; ++i) {
        words[i] = result->words[i].word_str;
    }

    {
        char *joined_words = join_words(words, result->n_words);
        result_str = norm_run_with_blacklist(normalizer_data, joined_words, blacklist);
        free(joined_words);
    }

    free(words);
    return result_str;
}

bool
string_array_contains_item(char **array, const char *item)
{
    bool contains = false;
    char **current_item = array;
    if (array == NULL) {
        return false;
    }

    while (*current_item) {
        if (strcmp(*current_item, item) == 0) {
            contains = true;
            break;
        }
        ++current_item;
    }

    return contains;
}

char *
reverse_normalize(norm_data_t *normalizer_data,
                  recognition_result_t *recognized,
                  char **blacklist,
                  int expected_number_length)
{
    bool numbers_fst_banned = string_array_contains_item(blacklist, CONVERT_NUMBER_FSTNAME);
    bool words_have_timings = false;
    char *result_str = NULL;
    char **common_fst_banlist;
    normalization_result_t *norm_result = NULL;

    if ((recognized->n_words > 0) &&
        (recognized->words[recognized->n_words - 1].end_time - recognized->words[0].start_time > MINIMAL_SIGNIFICANT_TIME_INTERVAL)) {
        words_have_timings = true;
    }
    if ((expected_number_length == 0) || (numbers_fst_banned) || (!words_have_timings)) {
        return standard_norm_run(normalizer_data, recognized, blacklist);
    }

    {
        int i;
        int common_list_length = fstlistlen(NORMALIZE_NUMBERS_BLACKLIST);
        int banlist_length = fstlistlen(blacklist);
        common_fst_banlist = xcalloc(common_list_length + banlist_length + 1, sizeof(char*));
        for (i = 0; i < common_list_length; ++i) {
            common_fst_banlist[i] = NORMALIZE_NUMBERS_BLACKLIST[i];
        }
        if (blacklist != NULL) {
            for (i = 0; i < banlist_length; ++i) {
                common_fst_banlist[common_list_length + i] = blacklist[i];
            }
        }
    }

    norm_result = split_recognition_result_by_silence_between_numbers(normalizer_data, common_fst_banlist, recognized);
    if (norm_result != NULL) {
        result_str = process_number(norm_result, expected_number_length);
        normalization_result_free(norm_result);
    }

    if (result_str == NULL) {
        result_str = standard_norm_run(normalizer_data, recognized, common_fst_banlist);
    } else if (!string_array_contains_item(blacklist, GLUE_PUNCTUATION_FSTNAME)) {
        char *intermediate_result = result_str;
        result_str = norm_run_with_whitelist(normalizer_data, intermediate_result, GLUE_PUNCTUATION_FSTLIST);
        free(intermediate_result);
    }

    free(common_fst_banlist);
    return result_str;
}
