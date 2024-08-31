#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('../..')

from general.normbase import *
import categories

print 'Preprocess'

punctuation = anyof("\".,!?:;()")

def make_downcase():
    upper = (u"ABCDEFGHIJKLMNOPQRSTUVWXYZ" +
             u"АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ" +
             u"ҐІЄЇ" +
             u"ÄÁÀÂÃĀÇËÉÈÊẼĒĞİÏÍÌÎĨĪÖÓÒÔÕŌŞÜÚÙÛŨŪ")
    lower = (u"abcdefghijklmnopqrstuvwxyz" +
             u"абвгдеёжзийклмнопрстуфхцчшщъыьэюя" +
             u"ґієї" +
             u"äáàâãāçëéèêẽēğıïíìîĩīöóòôõōşüúùûũū")

    upper = (u"ABCDEFGHIJKLMNOPQRSTUVWXYZ" +
             u"АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ" +
             u"ÄÇËÊÈĞİÖÓŞÜÛ")
    lower = (u"abcdefghijklmnopqrstuvwxyz" +
             u"абвгдеёжзийклмнопрстуфхцчшщъыьэюя" +
             u"äçëêèğıöóşüû")
    r = anyof([replace(u, l) for (u, l) in zip(upper, lower)])
    r.optimize()
    return r

downcase = make_downcase()
replace_whitespace = replace(g.whitespace, " ")
lower_and_whitespace_cvt = ss(downcase | replace_whitespace | g.direct_transcription_word |
                              cost(g.any_symb, 0.01))
lower_and_whitespace_cvt.optimize()

# Add space at start so that word() works.
space_at_start = insert(" ") + ss(g.any_symb)
space_at_start = space_at_start.determinize().minimize()

# Separate punctuation from words.
# Do not puts paces around ".", ",", ":" in numbers, dates and times
# Special treatment for "." after a sequence of digits: means ordinal numer in Turkish
# Put a space at the end
spaces_around_punctuation = (ss(g.digit + anyof(".,") + g.digit |
                               cost(insert(" ") + punctuation + insert(" "), 0.001) |
                               g.direct_transcription_word |
                               cost(g.any_symb, 0.1)) + insert(" "))
#spaces_around_punctuation = spaces_around_punctuation.determinize().minimize()

unconditional_replacement = make_unconditional_replacements("replacements.txt")
replace_unconditionally_cvt = ss(unconditional_replacement |
                                 g.direct_transcription_word |
                                 g.read_by_char_word |
                                 cost(g.any_symb, 1))
replace_unconditionally_cvt.optimize()

collapse_spaces = ss(replace(pp(" "), " ") | cost(g.any_symb, 0.01))
collapse_spaces.optimize()

remove_space_at_end = qq(ss(g.any_symb) + (g.any_symb - " ")) + ss(remove(" "))
remove_space_at_end.optimize()

# Composing several simple transducers gives a surprisingly huge result, so keep them separate

# preprocess_cvt = (space_at_start >>
#                   replace_unconditionally_cvt >>
#                   spaces_around_punctuation >>
#                   collapse_spaces >>
#                   remove_space_at_end
#                   )
# preprocess_cvt.optimize()

plus_minus = (replace("+", " plus ") + ss(" ") + g.digit |
              replace("-", " minus ") + ss(" ") + g.digit)
plus_minus_cvt = ss(plus_minus |
                    g.direct_transcription_word |
                    cost(g.any_symb, 0.01))
plus_minus_cvt.optimize()

long_digit_sequence = rr(g.digit, 12) + ss(g.digit)
# Read digits by two, with maybe three at the end, with a pause before each chunk
split_long_sequence = insert(" : ") + ss(rr(g.digit, 2) + insert(" , ")) + qq(rr(g.digit, 2, 3))

convert_digit_sequences = convert_words(long_digit_sequence >>
                                        split_long_sequence)
convert_digit_sequences.optimize()

glue_punctuation = punctuation + ss(remove(ss(" ")) + punctuation)
glue_punctuation_cvt = ss(glue_punctuation |
                          g.direct_transcription_word |
                          cost(g.any_symb, 0.01))
#convert_words(glue_punctuation, permit_inner_space=True)
glue_punctuation_cvt.optimize()

# First collapse spaces, then remove the last one
final_spaces_adjustment = (remove(ss(" ")) +
                           qq(ss(pp(g.word_sym_fst) +
                                 replace(pp(" "), " ")) +
                              pp(g.word_sym_fst) + remove(ss(" "))))
final_spaces_adjustment.optimize()
