#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('../..')

from general.normbase import *
import categories

print 'Simple Conversions'

# For Turkish. "'" is not a punctuation mark
punctuation = anyof(".,!?:;()\"-")

def make_unconditional_replacements(fname, insert_spaces):
    r = empty()
    with open(fname) as rf:
        for ln in rf:
            # Spaces at the left edge may be meaningful
            words = ln.rstrip().split('\t')
            if len(words) == 0:
                continue
            w_from = words[0]
            w_to = " ".join(words[1:])
            if insert_spaces:
                w_to = " " + w_to + " "
            r = r | replace(w_from,  w_to)
    return r.optimize()

# This is Turkish, with its specific lowering rules for I and İ
def make_downcase():
    upper = (u"ABCDEFGHİJKLMNOPQRSTUVWXYZ" +
             u"АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ" +
             u"ÄÁÂÃÇËÊÈÉĞIÎÖÓŌŞÜÛ")
    lower = (u"abcdefghijklmnopqrstuvwxyz" +
             u"абвгдеёжзийклмнопрстуфхцчшщъыьэюя" +
             u"äáâãçëêèéğıîöóōşüû")
    r = (anyof(replace(u, l) for (u, l) in zip(upper, lower)) |
         replace(u"a\u0302", u"â") |
         replace(u"A\u0302", u"â") |
         replace(u"c\u0327", u"ç") |
         replace(u"C\u0327", u"ç") |
         replace(u"g\u0306", u"ğ") |
         replace(u"G\u0306", u"ğ") |
         replace(u"o\u0308", u"ö") |
         replace(u"O\u0308", u"ö") |
         replace(u"s\u0327", u"ş") |
         replace(u"S\u0327", u"ş") |
         replace(u"u\u0308", u"ü") |
         replace(u"U\u0308", u"ü"))
    return r.optimize()

downcase = make_downcase()

replace_whitespace = replace(g.whitespace, " ")
lower_and_whitespace_cvt = ss(cost(downcase, 0.012) |
                              replace_whitespace |
                              g.direct_transcription_word |
                              g.chem_word |
                              cost(g.any_symb, 1))
lower_and_whitespace_cvt = lower_and_whitespace_cvt.optimize()

# Add space at start so that word() works.
space_at_start = insert(" ") + ss(g.any_symb)
space_at_start = space_at_start.determinize().minimize()

# Separate punctuation from words.
# Do not put spaces around ".", ",", ":" in numbers, dates and times,
# space before "." after a digit(an ordinal marker in Turkish orthography)
spaces_around_punctuation = ss(pp(pp(g.digit) + anyof(".,:\"-")) + pp(g.digit) |
                               pp(g.digit) + qq(".") + "'" + g.letter |
                               cost(pp(g.digit) + "." + insert(" "), 0.001) |
                               cost(insert(" ") + punctuation + insert(" "), 0.002) |
                               g.direct_transcription_word |
                               cost(g.any_symb, 0.1))
#spaces_around_punctuation = spaces_around_punctuation.determinize().minimize()

punctuation_replacement = make_unconditional_replacements('punctuation.txt', False)
replace_punctuation_cvt = convert_symbols(punctuation_replacement)

unconditional_replacement = make_unconditional_replacements('replacements.txt', True)
replace_unconditionally_cvt = ss(unconditional_replacement |
                                 g.direct_transcription_word |
                                 g.read_by_char_word |
                                 cost(g.any_symb, 1))
replace_unconditionally_cvt = replace_unconditionally_cvt.optimize()

collapse_spaces = ss(replace(pp(" "), " ") | cost(g.any_symb, 0.01))
collapse_spaces = collapse_spaces.optimize()

remove_space_at_end = qq(ss(g.any_symb) + (g.any_symb - " ")) + ss(remove(" "))
remove_space_at_end = remove_space_at_end.optimize()

# A temporary solution, most likely:
# ignore special symbols in combinations like "#3", "'70s" and "1980' kadar"
ignore_marks_near_numbers = (
    " " + remove("#") + g.digit |
    " " + remove("'") + g.digit |
    g.digit + remove("'") + " "
)
ignore_marks_near_numbers_cvt = convert_symbols(ignore_marks_near_numbers)

ranges = (pp(g.digit) +
          replace("-", " - ") +
          pp(g.digit) + qq(qq("'") + pp(g.letter)))
ranges_cvt = convert_words(ranges)

long_digit_sequence = rr(g.digit, 12) + ss(g.digit)
# Read digits by two, with maybe three at the end, all in nominative
split_long_sequence = (ss(rr(g.digit, 2) + insert(" ")) +
                       qq(rr(g.digit, 2, 3)))

convert_digit_sequences = convert_words(long_digit_sequence >>
                                        split_long_sequence)
convert_digit_sequences = convert_digit_sequences.optimize()

# Split mixed words
# FIXME: !!!!!! For d2l, there is no proper time to apply the transformation
# without mixing it with legitimate ordinal numerals, so it's never used right now.
split_l2d = (g.letter | anyof(":()")) + insert(" ") + g.digit
split_d2l = g.digit + insert(" ") + (g.letter | anyof(":()"))
split_l2d_cvt = convert_symbols(split_l2d)
split_d2l_cvt = convert_symbols(split_d2l)

glue_punctuation = punctuation + ss(remove(ss(" ")) + punctuation)
glue_punctuation_cvt = ss(glue_punctuation |
                          g.direct_transcription_word |
                          cost(g.any_symb, 0.01))
#convert_words(glue_punctuation, permit_inner_space=True)
glue_punctuation_cvt = glue_punctuation_cvt.optimize()

# First collapse spaces, then remove the last one
final_spaces_adjustment = (remove(ss(" ")) +
                           qq(ss(pp(g.word_sym_fst) +
                                 replace(pp(" "), " ")) +
                              pp(g.word_sym_fst) + remove(ss(" "))))
final_spaces_adjustment = final_spaces_adjustment.optimize()

