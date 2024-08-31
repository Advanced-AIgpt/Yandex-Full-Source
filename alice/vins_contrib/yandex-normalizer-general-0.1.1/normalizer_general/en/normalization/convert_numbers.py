#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('../..')

from general.normbase import cost, g, anyof, pp, qq, rr, ss, insert, remove, replace, \
    word, convert_words, separate
from numerals import cardinal_numeral, ordinal_numeral, producer

# TODO: Add a means of reading years, support fractions

# Common

nonzero_digit = anyof('123456789')
two_to_nine = anyof('23456789')
except_one = anyof('023456789')
digit_sequence = pp(g.digit)
three_digits = rr(g.digit, 3)

# Preprocessing: cardinal/ordinal disambiguation, features insertion

nonteen_prefix = (
    '' |  # 1st
    ss(g.digit) + except_one  # 21st, but 11th
)
th_prefix = ss(g.digit) + (anyof('0456789') | '1' + anyof('123'))

separator = qq(remove('-'))
special_ord_ending = anyof(('1' + separator + 'st', '2' + separator + 'nd', '3' + separator + 'rd'))
to_ordinal = pp(g.digit) + replace(qq('-') + anyof(('st', 'nd', 'rd', 'th')), ordinal_numeral)

insert_features = (
    digit_sequence + insert(cardinal_numeral) |
    (nonteen_prefix + special_ord_ending | th_prefix + separator + 'th') >> to_ordinal |
    # Context for ordinals
    cost(digit_sequence + insert(ordinal_numeral) + word(anyof(["street","avenue"])), -0.1)
)
insert_features_cvt = convert_words(insert_features, permit_inner_space=True)

prepare_numbers = convert_words(pp(g.digit) + qq('-') + anyof(('st', 'nd', 'rd', 'th')) |
                                cost(pp(g.digit) + insert(" ") + pp(g.word_sym_not_digit), 0.001)).optimize()

# Functions providing transducers for different groups of digits

def number_1_9_of_feats(features):
    result = nonzero_digit + features >> producer
    return result.optimize()


def number_10_19_of_feats(features):
    result = '1' + g.digit + features >> producer
    return result.optimize()


def number_20_99_of_feats(features):
    result = (
        two_to_nine + '0' + features >> producer |
        (
            (two_to_nine + insert('0' + cardinal_numeral) >> producer) +
            insert(' ') +
            (nonzero_digit + features >> producer)
        )
    )
    return result.optimize()


def number_2digits_of_feats(features):
    result = (
        remove('0') + number_1_9_of_feats(features) |
        number_10_19_of_feats(features) |
        number_20_99_of_feats(features)
    )
    return result.optimize()


def number_100_999_of_feats(features):
    x00 = ((nonzero_digit + replace('00' + features, cardinal_numeral)) >> producer) + insert(' hundred')
    if features == ordinal_numeral:
        x00 += insert('th')

    xxx = (
        ((nonzero_digit + insert(cardinal_numeral)) >> producer) + insert(' hundred')
    ) + insert(' ') + number_2digits_of_feats(features)

    return (x00 | xxx).optimize()


def number_3digits_of_feats(features):
    result = (
        remove('000') |
        remove('00') + number_1_9_of_feats(features) |
        remove('0') + number_2digits_of_feats(features) |
        number_100_999_of_feats(features)
    )
    return result.optimize()


def number_1_999_of_feats(features):
    result = (
        number_1_9_of_feats(features) |
        number_10_19_of_feats(features) |
        number_20_99_of_feats(features) |
        number_100_999_of_feats(features)
    )
    return result.optimize()


def zeroes_of_feats(features):
    suffix = 'th' if features == ordinal_numeral else ''
    result = replace('0', 'zero' + suffix) + remove(features)

    if features == cardinal_numeral:
        result |= (
            replace('00' + features, 'double') | replace('000' + features, 'triple')
        ) + insert(' oh')  # For reading split digit sequences

    return result.optimize()


def higher_order_numbers(order):
    result = (
        remove('000') |
        three_digits + insert(cardinal_numeral) >> (
            number_3digits_of_feats(cardinal_numeral) + insert(' ' + order)
        )
    )
    return result.optimize()

thousand = higher_order_numbers('thousand')
million = higher_order_numbers('million')
billion = higher_order_numbers('billion')


def higher_ordinals_suffix(higher, rest, features):
    '''
    Adds a 'th' suffix to thousands etc. in `higher` when `rest` is empty and `features` is "ordinal numeral".
    '''
    if features != ordinal_numeral:
        return higher + rest

    return higher + (rest >> pp(g.any_symb)) | higher + insert('th') + (rest >> '')


upto_three_digits = (
    nonzero_digit + rr(g.digit, 2) |
    insert('0') + nonzero_digit + g.digit |
    insert('00') + nonzero_digit
)


def compose_number_of_feats(features, major_higher, *other_higher):
    result = separate(three_digits + features >> number_3digits_of_feats(features))

    for item in reversed(other_higher):
        result = higher_ordinals_suffix(separate(three_digits >> item), result, features)

    result = higher_ordinals_suffix(
        upto_three_digits >> major_higher,
        result,
        features
    )
    return result


def number_of_feats(features):
    result = (
        compose_number_of_feats(features, billion, million, thousand) |
        compose_number_of_feats(features, million, thousand) |
        compose_number_of_feats(features, thousand) |
        number_1_999_of_feats(features) |
        zeroes_of_feats(features)
    )

    if features == cardinal_numeral:
        result |= (
            replace('00', 'double oh ') + number_1_9_of_feats(cardinal_numeral) |
            replace('0', 'oh ') + (
                number_1_9_of_feats(cardinal_numeral) |
                number_10_19_of_feats(cardinal_numeral) |
                number_20_99_of_feats(cardinal_numeral)
            )
        )  # For reading split digit sequences

    # Read out leading zeros, but prefer "oh" when it is appropriate.
    result = ss(cost(replace("0", "zero "), 0.1)) + result

    return result.optimize()


cardinal = number_of_feats(cardinal_numeral)
ordinal = number_of_feats(ordinal_numeral)
cardinal = cardinal.optimize()
ordinal = ordinal.optimize()

fraction = (
    (insert_features >> cardinal) +
    replace(".", " point ") +
    pp((g.digit + insert(cardinal_numeral) >> producer) + insert(" "))
)

all_numbers = cardinal | ordinal | fraction

numbers_cvt = convert_words(all_numbers)
numbers_cvt = numbers_cvt.optimize()


if __name__ == '__main__':
    from general.normbase import fst_print_random_sample
    automaton = insert_features >> all_numbers
    fst_print_random_sample(automaton)
    fst_print_random_sample('12th' >> automaton)
