# distutils: language=c++
# coding: utf-8
# cython: language_level=3, boundscheck=False, wraparound=False, embedsignature=True, linetrace=True, c_string_type=str, c_string_encoding=ascii

cimport cython
from libcpp.vector cimport vector


def pos_aware_levenshtein(
    str word1,
    str word2,
    double[:, ::1] substitute_costs,
    double[::1] insert_costs,
    double[::1] delete_costs,
):
    """
    Вычисляет расстояние Левенштейна с увеличенным штрафом за операции в начале слова
    :param word1: первое слово
    :param word2: второе слово
    :param substitute_costs: веса замен символов, np.ndarray
    :param insert_costs: веса удалений символов, np.array
    :param delete_costs: веса удалений символов, np.array
    """
    return c_pos_aware_levenshtein(
        word1, len(word1),
        word2, len(word2),
        substitute_costs,
        insert_costs,
        delete_costs,
    )


cdef double get_pos_aware_cost(double cost, size_t pos, size_t word_len):
    """
    Подстраивает стоимость операции под позицию
    :param cost: базовая стоимость операции взвешенного Левенштейна
    :param pos: позиция в слове, на которой производится операция
    :param word_len: длина слова
    """
    if word_len > 4 and pos <= 3:
        return 4 * cost
    if word_len <= 4:
        return 1.8 * cost
    return cost


cdef double c_pos_aware_levenshtein(
    str word1, size_t len1,
    str word2, size_t len2,
    double[:, ::1] substitute_costs,
    double[::1] insert_costs,
    double[::1] delete_costs,
):
    cdef size_t min_len = min(len1, len2)
    cdef vector[vector[double]] costs = vector[vector[double]](len1 + 1, vector[double](len2 + 1))
    cdef vector[vector[int]] backtrace = vector[vector[int]](len1 + 1, vector[int](len2 + 1))  # 0 is correct, 1 is sub, 2 is insert, 3 is delete

    cdef size_t i, j
    cdef double sub_cost, ins_cost, del_cost, cur_min_cost

    costs[0][0] = 0.
    backtrace[0][0] = 0
    for i in range(1, len1 + 1):
        costs[i][0] = costs[i - 1][0] + delete_costs[word1[i - 1]]
        backtrace[i][0] = 3
    for j in range(1, len2 + 1):
        costs[0][j] = costs[0][j - 1] + insert_costs[word2[j - 1]]
        backtrace[0][j] = 2

    for i in range(1, len1 + 1):
        for j in range(1, len2 + 1):
            if word1[i - 1] == word2[j - 1]:
                costs[i][j] = costs[i - 1][j - 1]
                backtrace[i][j] = 0  # C
            else:
                sub_cost = costs[i - 1][j - 1] + substitute_costs[word1[i - 1], word2[j - 1]]
                ins_cost = costs[i][j - 1] + insert_costs[word2[j - 1]]
                del_cost = costs[i - 1][j] + delete_costs[word1[i - 1]]

                cur_min_cost = min(sub_cost, ins_cost, del_cost)
                costs[i][j] = cur_min_cost
                if cur_min_cost == sub_cost:
                    backtrace[i][j] = 1  # S
                elif cur_min_cost == ins_cost:
                    backtrace[i][j] = 2  # I
                elif cur_min_cost == del_cost:
                    backtrace[i][j] = 3  # D

    cdef double pos_aware_cost = 0
    cdef size_t cur_i = len1
    cdef size_t cur_j = len2
    while cur_i > 0 or cur_j > 0:
        op = backtrace[cur_i][cur_j]
        if op == 0:  # C
            cur_i -= 1
            cur_j -= 1
        elif op == 1:  # S
            pos_aware_cost += get_pos_aware_cost(
                costs[cur_i][cur_j] - costs[cur_i - 1][cur_j - 1],
                min(cur_i, cur_j),
                min_len
            )
            cur_i -= 1
            cur_j -= 1
        elif op == 2:  # I
            pos_aware_cost += get_pos_aware_cost(costs[cur_i][cur_j] - costs[cur_i][cur_j - 1], cur_j, len2)
            cur_j -= 1
        elif op == 3:  # D
            pos_aware_cost += get_pos_aware_cost(costs[cur_i][cur_j] - costs[cur_i - 1][cur_j], cur_i, len1)
            cur_i -= 1
    return pos_aware_cost
