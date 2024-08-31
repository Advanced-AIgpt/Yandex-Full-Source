#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('../..')

from general.normbase import cost, qq, ss, pp, rr, anyof, word, glue_words, g, remove, insert, replace, unword, drop_output_nonterminals
from numerals import cardinal_numeral, ordinal_numeral, conjunction, word_seq_tagger

# Common
zero = word('0' + cardinal_numeral)  # includes 'oh', see numerals.py
d1_9 = anyof('123456789')

drop_leading_zeros = word(remove(ss('0')) + d1_9 + ss(g.any_symb))
drop_leading_zeros = drop_leading_zeros.optimize()

remove_whitespace = ss(g.word_sym_fst | remove(' '))


# Cardinals
print >> sys.stderr, 'Cardinals'

card_1_19 = word((qq('1') + d1_9 | '10') + cardinal_numeral)


d2_9 = anyof('23456789')


card_20_90 = word(d2_9 + '0' + cardinal_numeral)
card_2x_9x = glue_words(
    d2_9 + remove('0' + cardinal_numeral),
    d1_9 + cardinal_numeral
)
card_20_99 = card_2x_9x | card_20_90
card_1_99 = card_1_19 | card_20_99


# In the British English, 'and' is inserted before tens and units in the end
removed_conjunction = remove(conjunction)


# We should recognize 'seventeen hundred' - see https://en.wikipedia.org/wiki/English_numerals
# However, this is done separately because 'two thousand seventeen hundred' is unlikely to occur.
def make_hundreds_block(hundreds_start_fst, allow_and=False):
    x00 = glue_words(
        hundreds_start_fst,
        replace('100', '00') + cardinal_numeral,
        permit_inner_space=True
    )

    def maybe_with_and(word):
        return [qq(removed_conjunction), word] if allow_and else [word]

    x20_99 = glue_words(
        hundreds_start_fst,
        remove('100' + cardinal_numeral),
        *maybe_with_and(unword(card_20_99) >> remove_whitespace),
        permit_inner_space=True,
        permit_empty_words=allow_and
    )

    x10_19 = glue_words(
        hundreds_start_fst,
        remove('100' + cardinal_numeral),
        *maybe_with_and('1' + g.digit + cardinal_numeral),
        permit_empty_words=allow_and,
        permit_inner_space=True
    )

    x01_09 = glue_words(
        hundreds_start_fst,
        remove('100' + cardinal_numeral) + replace(qq(zero), '0'),
        *maybe_with_and(d1_9 + cardinal_numeral),
        permit_empty_words=allow_and,
        permit_inner_space=True
    )
    return x00 | x01_09 | x10_19 | x20_99


card_100_900_start = d1_9 + remove(cardinal_numeral)  # | insert('1')  # if we allow just 'hundred', see below
card_100_999 = make_hundreds_block(card_100_900_start)
card_100_999_with_and = make_hundreds_block(card_100_900_start, allow_and=True)

# For 'seventeen hundred seventy-three' common in the US English
card_several_hundred_start = drop_output_nonterminals(card_1_99)
card_counting_in_hundreds = make_hundreds_block(card_several_hundred_start)
# Insert `, allow_and=True` above to allow `two hundred and six`


card_1_999 = card_1_19 | card_20_99 | card_100_999
card_1_999_with_and = (qq(word(removed_conjunction)) + (card_1_19 | card_20_99)) | card_100_999_with_and


fill_with_zeroes = word(
    (
        insert('00') + g.digit |
        insert('0') + g.digit + g.digit |
        g.digit + g.digit + g.digit
    ) + cardinal_numeral
)
card_ones_block = card_1_999 >> fill_with_zeroes
card_ones_block_with_and = card_1_999_with_and >> fill_with_zeroes


def block_with_unit(zeroes):
    stem = '1' + zeroes

    # No count => 001 - now disabled as English numerals usually spell out the 'one'
    # (or at least 'a' in 'a thousand', we'll leave that in words as it is usually intended to be), see above for 100.
    # no_count = word(replace(stem, '001' + zeroes) + cardinal_numeral)

    common = glue_words(
        (g.digit + g.digit + g.digit + remove(cardinal_numeral)),
        replace(stem, zeroes) + cardinal_numeral
    )

    return common  # | no_count


make_card_thousands_block = block_with_unit('000')
make_card_millions_block = block_with_unit('000000')
make_card_billions_block = block_with_unit('000000000')

card_thousands_block = (qq(card_ones_block) + word(ss(g.any_symb))) >> make_card_thousands_block
card_millions_block = (qq(card_ones_block) + word(ss(g.any_symb))) >> make_card_millions_block
card_billions_block = (qq(card_ones_block) + word(ss(g.any_symb))) >> make_card_billions_block

raw_cardinal = card_ones_block_with_and
raw_cardinal |= card_thousands_block + qq(raw_cardinal | card_ones_block_with_and)
raw_cardinal |= card_millions_block + qq(raw_cardinal | card_ones_block_with_and)
raw_cardinal |= card_billions_block + qq(raw_cardinal | card_ones_block_with_and)

card_glue_blocks = (
    glue_words(
        qq(g.digit + g.digit + g.digit + remove('000000000' + cardinal_numeral)),
        (g.digit + g.digit + g.digit + remove('000000' + cardinal_numeral)) | insert('000'),
        (g.digit + g.digit + g.digit + remove('000' + cardinal_numeral)) | insert('000'),
        (g.digit + g.digit + g.digit + remove(cardinal_numeral)) | insert('000')
    ) + insert(cardinal_numeral)
)

cardinal = (raw_cardinal >> card_glue_blocks >> drop_leading_zeros) | card_counting_in_hundreds | zero
cardinal = cardinal.optimize()


# Ordinals
print >> sys.stderr, 'Ordinals'


def replace_at_end(source, target):
    return ss(g.any_symb) + replace(source, target)


replace_ordinal_tag_at_end = replace_at_end(ordinal_numeral, cardinal_numeral)
tagged_ordinal_to_normalized_cardinal = replace_ordinal_tag_at_end >> cardinal


def ends_in_single(char):
    return ((ss(g.any_symb) + anyof('023456789') + char) | word(char)) + cardinal_numeral


ends_not_in_single_123 = (ss(g.any_symb) + '1' + anyof('123') | ss(g.any_symb) + anyof('0456789')) + cardinal_numeral
replace_ending = (
    ends_in_single('1') >> replace_at_end('1' + cardinal_numeral, '1st' + ordinal_numeral) |
    ends_in_single('2') >> replace_at_end('2' + cardinal_numeral, '2nd' + ordinal_numeral) |
    ends_in_single('3') >> replace_at_end('3' + cardinal_numeral, '3rd' + ordinal_numeral) |
    ends_not_in_single_123 >> replace_at_end(cardinal_numeral, 'th' + ordinal_numeral)
)


ordinal = tagged_ordinal_to_normalized_cardinal >> replace_ending
ordinal = ordinal.optimize()


print >> sys.stderr, 'Digit sequences'


# Multiples in digit sequences should probably be implemented by introducing 'times' for the cases like '3 times 5'
# and 'double' for 'double oh'.
# Sequences of at least five digits.
# http://www.eslcafe.com/grammar/saying_phone_numbers.html claims it isn't used often in spelling phone numbers though.
# For now, just glue adjacent single-digit and -teen numbers and don't even bother removing 'card' markers in between.
# Allowing for all the 2-digit numbers introduces ambiguities like 'three seventy eight zero' => '3 78 0' / '3 70 8 0'.
card_0_19 = card_1_19 | zero
element = card_0_19
# card_0_99 = card_1_99 | zero
# element = card_0_99
digit_seq = element + rr(element >> remove_whitespace, 4) + ss(element >> remove_whitespace)
digit_seq = digit_seq.optimize()


# Number in general
print >> sys.stderr, 'General numbers'

convert_number = cardinal | ordinal | digit_seq
convert_number = convert_number.optimize()

# Convert numbers
numbers_step = ss(cost((word_seq_tagger >> convert_number), 0.01) | cost(word(ss(g.any_symb)), 1))
numbers_step = numbers_step.optimize()

# Postprocess: remove grammar markers
postprocess = drop_output_nonterminals(ss(g.any_symb))
postprocess = postprocess.optimize()

convert_numbers = (numbers_step >> postprocess).optimize()

if __name__ == '__main__':
    from general.normbase import fst_print_random_sample
    import numerals

    number_normalizer = (numerals.word_seq_tagger >> number).optimize()
    fst_print_random_sample(number_normalizer)
    fst_print_random_sample(' one hundred and ten' >> number_normalizer)
    fst_print_random_sample(' one seven nine oh four' >> number_normalizer)
    fst_print_random_sample(' one hundred thirty two' >> number_normalizer)
    fst_print_random_sample(
        ' twenty four billion  eighty seven million  three hundred thousand  seven hundred thirty second' >>
        number_normalizer
    )
    fst_print_random_sample(' twenty two hundred thirty six' >> number_normalizer)
    fst_print_random_sample(' twenty one hundred' >> number_normalizer)
    fst_print_random_sample(' sixty two hundred six' >> number_normalizer)
    fst_print_random_sample(' sixty two hundred oh six' >> number_normalizer)
