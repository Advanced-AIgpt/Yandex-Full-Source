#!/usr/bin/env python
# -*- coding: utf-8 -*-
# vim:fileencoding=utf-8
import random
import re
import codecs
import sys
import json
import argparse
import math
import io
from collections import defaultdict
import numpy as np
import pymorphy2


morph = pymorphy2.MorphAnalyzer()


RUS_VOWEL_LETTERS = {
    u"а", u"у", u"е", u"ы", u"о", u"э", u"и", u"я", u"ю"
    }
RUS_CONSONANT_LETTERS = {
    u"й", u"ц", u"к", u"н", u"г", u"ш", u"щ", u"з", u"х", u"ф", u"в",
    u"п", u"р", u"л", u"д", u"ж", u"ч", u"с", u"м", u"т", u"б", u"ъ", u"ь"
    }
ENG_VOWEL_LETTERS = {
    'e', 'u', 'y', 'i', 'o', 'a'
    }
ENG_CONSONANT_LETTERS = {
    'w', 'r', 't', 'p', 's', 'd', 'f', 'g', 'h', 'j',
    'k', 'l', 'z', 'x', 'c', 'v', 'b', 'n', 'm'
    }
DIGITS = {
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9"
    }
RUS_LETTERS = RUS_VOWEL_LETTERS | RUS_CONSONANT_LETTERS
ENG_LETTERS = ENG_VOWEL_LETTERS | ENG_CONSONANT_LETTERS
RUS_LETTERS_AND_DIGITS = RUS_LETTERS | DIGITS
ALL_SYMBOLS = RUS_LETTERS | ENG_LETTERS | DIGITS
TRANSLITERATE_DICT = {
    'а': 'a', 'б': 'b', 'в': 'v', 'г': 'g', 'д': 'd', 'е': 'e', 'ё': 'e',
    'ж': 'zh', 'з': 'z', 'и': 'i', 'й': 'i', 'к': 'k', 'л': 'l', 'м': 'm',
    'н': 'n', 'о': 'o', 'п': 'p', 'р': 'r', 'с': 's', 'т': 't', 'у': 'u',
    'ф': 'f', 'х': 'h', 'ц': 'c', 'ч': 'ch', 'ш': 'sh', 'щ': 'sch', 'ъ': '',
    'ы': 'y', 'ь': '', 'э': 'e', 'ю': 'u', 'я': 'ya', 'А': 'A', 'Б': 'B',
    'В': 'V', 'Г': 'G', 'Д': 'D', 'Е': 'E', 'Ё': 'E', 'Ж': 'ZH', 'З': 'Z',
    'И': 'I', 'Й': 'I', 'К': 'K', 'Л': 'L', 'М': 'M', 'Н': 'N', 'О': 'O',
    'П': 'P', 'Р': 'R', 'С': 'S', 'Т': 'T', 'У': 'U', 'Ф': 'F', 'Х': 'H',
    'Ц': 'C', 'Ч': 'CH', 'Ш': 'SH', 'Щ': 'SCH', 'Ъ': '', 'Ы': 'Y', 'Ь': '',
    'Э': 'E', 'Ю': 'U', 'Я': 'YA', ',': '', '?': '', ' ': '_', '~': '',
    '!': '', '@': '', '#': '', '$': '', '%': '', '^': '', '&': '', '*': '',
    '(': '', ')': '', '-': '', '=': '', '+': '', ':': '', ';': '', '<': '',
    '>': '', '\'': '', '"': '', '\\': '', '/': '', '№': '', '[': '', ']': '',
    '{': '', '}': '', 'ґ': '', 'ї': '', 'є': '', 'Ґ': 'g', 'Ї': 'i', 'Є': 'e',
    '—': ''
    }


def findall_idx(input_str, req_sym):
    i = input_str.find(req_sym)
    while i != -1:
        yield i
        i = input_str.find(req_sym, i + 1)


def transliterate(input_word):
    '''
    Транслитирирует слово на кириллице.
    '''
    if set(input_word) & ENG_LETTERS:
        return input_word
    for key in TRANSLITERATE_DICT:
        decoded_key = key.decode('utf8')
        input_word = input_word.replace(decoded_key, TRANSLITERATE_DICT[key])
    return input_word


def symbol_replace(query_text, random_symbol_indexes, same_group):
    '''
    Случайно заменяет символы в строке query_text на местах, указанных в random_symbol_indexes.
    Если same_group == True, заменяет гласную на гласную, а согласную на согласную.
    '''
    query_with_noise = query_text
    for symbol_ind in random_symbol_indexes:
        cur_symbol = query_with_noise[symbol_ind]
        if same_group:
            if cur_symbol in RUS_VOWEL_LETTERS:
                symbol_group = RUS_VOWEL_LETTERS
            elif cur_symbol in RUS_CONSONANT_LETTERS:
                symbol_group = RUS_CONSONANT_LETTERS
            elif cur_symbol in ENG_VOWEL_LETTERS:
                symbol_group = ENG_VOWEL_LETTERS
            elif cur_symbol in ENG_CONSONANT_LETTERS:
                symbol_group = ENG_CONSONANT_LETTERS
            else:
                symbol_group = DIGITS
            random_letter = np.random.choice(list(symbol_group-{cur_symbol}))
        else:
            if cur_symbol in RUS_LETTERS:
                symbol_group = RUS_LETTERS
            elif cur_symbol in ENG_LETTERS:
                symbol_group = ENG_LETTERS
            else:
                symbol_group = DIGITS
            random_letter = np.random.choice(list(symbol_group-{cur_symbol}))
        edited_query = ''.join(
            (query_with_noise[:symbol_ind],
             random_letter, query_with_noise[symbol_ind+1:])
        )
    return edited_query


def random_transliterate(query_text, num_of_editions=1):
    '''
    Транслитирирует num_of_editions случайных слов в строке query_text.
    '''
    if num_of_editions < 0:
        raise ValueError('Value of num_of_editions must be positive')
    words_list = re.findall(r'\w+', query_text, flags=re.UNICODE)
    if not words_list:
        return query_text
    num_of_words = len(words_list)
    if num_of_words < num_of_editions:
        num_of_editions = num_of_words
    words_with_translits = list(words_list)
    random_words_indexes = np.random.choice(num_of_words, num_of_editions,
                                            replace=False)
    for word_ind in random_words_indexes:
        words_with_translits[word_ind] = transliterate(words_list[word_ind])
    edited_query = ' '.join(words_with_translits)
    return edited_query


def random_symbol_replace(query_text, num_of_editions=1, same_group=False):
    '''
    Заменяет num_of_editions случайных символов в строке query_text.
    Если same_group == True, заменяет гласную на гласную, а согласную на согласную.
    '''
    if num_of_editions < 0:
        raise ValueError('Value of num_of_editions must be positive')
    words_list = re.findall(r'\w+', query_text, flags=re.UNICODE)
    if not words_list:
        return query_text
    possible_symbol_indexes = np.where(np.array(list(query_text)) != ' ')[0]
    possible_symbol_indexes = list(possible_symbol_indexes)
    if num_of_editions > len(possible_symbol_indexes):
        num_of_editions = len(possible_symbol_indexes)
    random_symbol_indexes = np.random.choice(possible_symbol_indexes,
                                             num_of_editions, replace=False)
    edited_query = symbol_replace(query_text, random_symbol_indexes,
                                  same_group)
    return edited_query


def random_word_replace(query_text, num_of_editions,
                        dict_length_groups, length_max_diff=2):
    '''
    Случайно заменяет num_of_editions слов в query_text.
    В dict_length_groups должен быть словарь вида {1: [*список слов длины 1*], 2: [*список слов длины 2*], ..}
    length_max_diff задает максимальную разницу в длине заменяемых слов.
    '''
    if num_of_editions < 0:
        raise ValueError('Value of num_of_editions must be positive')
    words_list = re.findall(r'\w+', query_text, flags=re.UNICODE)
    num_of_words = len(words_list)
    if not words_list:
        return query_text
    if num_of_editions > num_of_words:
        num_of_editions = num_of_words
    random_indexes = np.random.choice(num_of_words, num_of_editions,
                                      replace=False)
    for index in random_indexes:
        random_word_len = len(words_list[index])
        if random_word_len - length_max_diff < 1:
            lower_bound = 1
        else:
            lower_bound = random_word_len - length_max_diff
        upper_bound = random_word_len + length_max_diff
        length_interval = list(range(lower_bound, upper_bound + 1))
        dict_keys = dict_length_groups.keys()
        possible_word_lengths = set(dict_keys) & set(length_interval)
        if not possible_word_lengths:
            continue
        possible_words = list()
        for word_length in possible_word_lengths:
            possible_words += dict_length_groups[word_length]
        word_for_replacement = np.random.choice(possible_words)
        words_list[index] = word_for_replacement
    edited_query = ' '.join(words_list)
    return edited_query


def random_word_delete(query_text, num_of_editions=1, mode=0):
    '''
    Случайно удаляет num_of_editions слов из строки query_text.

    mode = 0/1/2
    0 - удаление любого слова
    1 - удаление первого слова
    2 - удаление последнего слова
    '''
    if num_of_editions < 0:
        raise ValueError('Value of num_of_editions must be positive')
    words_list = np.array(re.findall(r'\w+', query_text, flags=re.UNICODE))
    num_of_words = len(words_list)
    # ! Не удаляет ничего, если в строке 1 слово
    if num_of_words <= 1:
        return query_text
    elif num_of_words <= num_of_editions:
        num_of_editions = num_of_words - 1
    if mode == 0:
        num_of_saved_words = len(words_list) - num_of_editions
        saved_words = np.random.choice(num_of_words, num_of_saved_words,
                                       replace=False)
        saved_words.sort()
        words_list = list(words_list[saved_words])
        edited_query = ' '.join(words_list)
    elif mode == 1:
        edited_query = ' '.join(words_list[num_of_editions:])
    elif mode == 2:
        edited_query = ' '.join(words_list[:-num_of_editions])
    return edited_query


def random_declination(query_text, num_of_editions=1):
    '''
    Изменяет форму у num_of_editions слов из строки.
    '''
    if num_of_editions < 0:
        raise ValueError('Value of num_of_editions must be positive')
    words_list = re.findall(r'\w+', query_text, flags=re.UNICODE)
    if not words_list:
        return query_text
    count_of_words = len(words_list)
    if count_of_words < num_of_editions:
        num_of_editions = count_of_words
    edited_words = list(words_list)
    random_word_indexes = np.random.choice(count_of_words, num_of_editions,
                                           replace=False)
    for word_ind in random_word_indexes:
        initial_word = words_list[word_ind]
        word_parse = morph.parse(initial_word)[0]
        word_declinations = set()
        for num_of_form in range(len(word_parse.lexeme)):
            cur_tag = word_parse.lexeme[num_of_form].tag
            declination_param = re.split(r'[, ]', str(cur_tag))
            declination_param = set(declination_param)
            new_word = word_parse.inflect(declination_param).word
            word_declinations.add(new_word)
        word_other_declinations = word_declinations - set(initial_word)
        num_of_declinations = len(word_other_declinations)
        if num_of_declinations == 0:
            continue
        else:
            word_other_declinations = list(word_other_declinations)
            new_declination = np.random.choice(word_other_declinations)
        edited_words[word_ind] = new_declination
    edited_query = ' '.join(edited_words)
    return edited_query


def random_inserts(query_text, num_of_editions=1,
                   dict_list=[], mode=0):
    '''
    Случайно вставляет num_of_editions слов в строку query_text.
    Корпус слов хранится в аргументе dict_list.
    Если он пустой - дублирует слова из query_text.

    mode = 0/1/2
    0 - вставка в любом месте
    1 - вставка в начало
    2 - вставка в конец
    '''
    if not dict_list:
        if re.findall(r'\w+', query_text, flags=re.UNICODE):
            dict_list = re.findall(r'\w+', query_text, flags=re.UNICODE)
        else:
            return query_text
    if num_of_editions < 0:
        raise ValueError('Value of num_of_editions must be positive')
    words_list = re.findall(r'\w+', query_text, flags=re.UNICODE)
    num_of_words = len(words_list)
    edited_words = list(words_list)
    for _ in range(num_of_editions):
        num_of_words = len(edited_words)
        if (num_of_words == 0) or (mode == 1):
            random_ind = 0
        elif mode == 2:
            random_ind = num_of_words
        elif mode == 0:
            random_ind = random.randrange(num_of_words + 1)
        random_word = np.random.choice(dict_list)
        edited_words.insert(random_ind, random_word)
    edited_query = ' '.join(edited_words)
    return edited_query


def random_swap(query_text, num_of_editions=1, near=False):
    """
    Меняет местами num_of_editions раз слова из query_text.
    Если near == True, меняет местами рядом стоящие слова.
    """
    if num_of_editions < 0:
        raise ValueError('Value of num_of_editions must be positive')
    words_list = re.findall(r'\w+', query_text, flags=re.UNICODE)
    if len(words_list) < 2:
        return query_text
    for _ in range(num_of_editions):
        if near:
            index = random.randrange(len(words_list) - 1)
            get = words_list[index + 1], words_list[index]
            words_list[index], words_list[index + 1] = get
        else:
            indexes = np.random.choice(len(words_list),
                                       2, replace=False)
            get = words_list[indexes[1]], words_list[indexes[0]]
            words_list[indexes[0]], words_list[indexes[1]] = get
    edited_query = ' '.join(words_list)
    return edited_query


def random_dublicate(query_text, num_of_editions=1, near=True):
    """
    Дублирует num_of_editions слов в query_text на случайное место.
    Если near == True, дублирует их рядом с собой.
    """
    if num_of_editions < 0:
        raise ValueError('Value of num_of_editions must be positive')
    words_list = re.findall(r'\w+', query_text, flags=re.UNICODE)
    if not words_list:
        return query_text
    for _ in range(num_of_editions):
        if near:
            random_ind = random.randrange(len(words_list))
            words_list.insert(random_ind, words_list[random_ind])
        else:
            word_ind, insert_ind = np.random.choice(len(words_list) + 1,
                                                    2, replace=False)
            if word_ind > insert_ind:
                word_ind, insert_ind = insert_ind, word_ind
            words_list.insert(insert_ind, words_list[word_ind])
    edited_query = ' '.join(words_list)
    return edited_query


def random_space_insert(query_text, num_of_editions=1):
    """
    Разбивает слова в query_text пробелами в случайных num_of_editions местах.
    """
    if num_of_editions < 0:
        raise ValueError('Value of num_of_editions must be positive')
    words_list = re.findall(r'\w+', query_text, flags=re.UNICODE)
    if not words_list:
        return query_text
    for _ in range(num_of_editions):
        space_indexes = set()
        for i in findall_idx(query_text, ' '):
            space_indexes.update([i, i + 1])
        space_indexes = space_indexes | {0, len(query_text)}
        possible_indexes_to_insert = [i for i in range(len(query_text))
                                      if i not in space_indexes]
        if possible_indexes_to_insert:
            random_idx = np.random.choice(possible_indexes_to_insert)
            query_text = u'{} {}'.format(query_text[:random_idx],
                                         query_text[random_idx:])
        else:
            break
    return query_text


def random_stop_word_insert(query_text, stop_words, num_of_editions=1, mode=0):
    '''
    Случайно вставляет num_of_editions стоп-слов в query_text.
    Список стоп слова передается в stop_words.

    mode = 0/1
    0 - вставка случайного стоп слова в случайное место
    1 - вставка случайного стоп слова в начало/конец
    '''
    if num_of_editions < 0:
        raise ValueError('Value of num_of_editions must be positive')
    words_list = re.findall(r'\w+', query_text, flags=re.UNICODE)
    num_of_words = len(words_list)
    for _ in range(num_of_editions):
        if mode == 0:
            insert_ind = random.randrange(num_of_words + 1)
        elif mode == 1:
            insert_ind = np.random.choice([0, len(words_list)])
        random_stop_word = np.random.choice(stop_words)
        words_list.insert(insert_ind, random_stop_word)
    edited_query = ' '.join(words_list)
    return edited_query


def stop_words_deletion(query_text, stop_words):
    """
    Удаляет все стоп слова из query_text.
    Список стоп слова передается в stop_words.
    """
    words_list = re.findall(r'\w+', query_text, flags=re.UNICODE)
    edited_query = []
    for word in words_list:
        if word not in stop_words:
            edited_query.append(word)
    edited_query = ' '.join(edited_query)
    return edited_query


FUNC_DICT = {
    1: random_transliterate, 2: random_symbol_replace, 3: random_word_delete,
    4: random_word_replace, 5: random_declination, 6: random_inserts,
    7: random_swap, 8: random_dublicate, 9: random_space_insert,
    10: random_stop_word_insert, 11: stop_words_deletion
    }


FUNC_NAMES = {
    1: u"Транслит", 2: u"Замена символа",
    2.1: u"Замена символа (та же группа)", 3: u"Удаление",
    3.1: u"Удаление в начале", 3.2: u"Удаление в конце",
    4: u"Замена", 5: u"Склонение", 6: u"Вставка",
    6.1: u"Вставка в начале", 6.2: u"Вставка в конце",
    7: u"Перестановка", 7.1: u"Перестановка рядом",
    8: u"Дублирование", 8.1: u"Дублирование после себя",
    9: u"Вставка пробела", 10: u"Вставка стоп-слова",
    10.1: u"Вставка стоп-слова в начало/конец",
    11: u"Удаление стоп-слов"
    }


def main(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument('--input_table')
    parser.add_argument('--operation_settings')
    parser.add_argument('--input_column')
    parser.add_argument('--output_table')
    parser.add_argument('--settings_description')
    parser.add_argument('--stop_words')
    args = parser.parse_args(argv[1:])
    with io.open(args.input_table, 'r', encoding="utf-8") as f:
        input_table = json.load(f)
    with io.open(args.operation_settings, 'r', encoding="utf-8") as f:
        operation_settings = json.load(f)
    with io.open(args.stop_words, 'r', encoding="utf-8") as f:
        stop_words = json.load(f)
    input_column = args.input_column
    dict_str = ''
    for row in input_table:
        dict_str += codecs.utf_8_decode(row[input_column].encode('utf8'))[0]
        dict_str += ' '
    dict_list = set(re.findall(r'\w+', dict_str, flags=re.UNICODE))
    dict_list = list(dict_list)
    dict_length_groups = defaultdict(list)
    for word in dict_list:
        dict_length_groups[len(word)].append(word)
    description_dict = defaultdict(list)
    is_description_dict_done = False
    for row in input_table:
        texts_with_noise = {}
        for key in operation_settings:
            operation_seq = operation_settings[key]
            input_text = row[input_column]
            input_text = codecs.utf_8_decode(input_text.encode('utf8'))[0]
            num_of_operations = len(operation_seq)
            for i in range(num_of_operations):
                if not is_description_dict_done:
                    description_dict[key].append(FUNC_NAMES[operation_seq[i]])
                repeats_counter = 1
                if not isinstance(operation_seq[i], (float, int)):
                    raise Exception('Incorrect format of the '
                                    'operation settings')
                elif (operation_seq[i] not in FUNC_NAMES.keys()):
                    raise Exception('Incorrect format of the operation '
                                    'settings: the operation is not in '
                                    'the list')
                if (i + 1 != num_of_operations and
                        operation_seq[i + 1] == operation_seq[i]):
                    repeats_counter += 1
                    continue
                operation_param, ground_operation = math.modf(operation_seq[i])
                ground_operation = int(ground_operation)
                operation_param = round(operation_param, 1)
                cur_function = FUNC_DICT[ground_operation]
                # Дальше следует парсинг json операций
                if operation_param == 0.0: 
                    if cur_function == random_inserts:
                        input_text = cur_function(input_text, repeats_counter,
                                                  dict_list)
                    elif cur_function == random_word_replace:
                        input_text = cur_function(input_text, repeats_counter,
                                                  dict_length_groups)
                    elif cur_function == random_stop_word_insert:
                        input_text = cur_function(input_text, stop_words,
                                                  repeats_counter)
                    elif cur_function == stop_words_deletion:
                        input_text = cur_function(input_text, stop_words)
                    else:
                        input_text = cur_function(input_text, repeats_counter)
                elif operation_param == 0.1:
                    if cur_function == random_inserts:
                        input_text = cur_function(input_text, repeats_counter,
                                                  dict_list, 1)
                    elif cur_function == random_word_delete:
                        input_text = cur_function(input_text,
                                                  repeats_counter, 1)
                    elif cur_function == random_stop_word_insert:
                        input_text = cur_function(input_text, stop_words,
                                                  repeats_counter, 1)
                    elif cur_function == random_dublicate:
                        input_text = cur_function(input_text,
                                                  repeats_counter,
                                                  False)
                elif operation_param == 0.2:
                    if cur_function == random_inserts:
                        input_text = cur_function(input_text, repeats_counter,
                                                  dict_list, 2)
                    elif cur_function == random_word_delete:
                        input_text = cur_function(input_text,
                                                  repeats_counter, 2)
            texts_with_noise['text_{}'.format(key)] = input_text
        row['texts_with_noise'] = texts_with_noise
        is_description_dict_done = True
    description_dict = {key: ', '.join(description_dict[key])
                        for key in description_dict.keys()}
    with io.open(args.settings_description, 'w', encoding='utf-8') as f:
        f.write(json.dumps(description_dict, ensure_ascii=False))
    with io.open(args.output_table, 'w', encoding='utf-8') as f:
        f.write(json.dumps(input_table, ensure_ascii=False))


if __name__ == '__main__':
    main(sys.argv)
