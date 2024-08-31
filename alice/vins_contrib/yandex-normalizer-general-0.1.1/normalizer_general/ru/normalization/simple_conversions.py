#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('../..')

from general.normbase import *
import categories

print 'Preprocess'

punctuation = anyof(".,!?:;()[]{}%\"\'")

def make_downcase():
    upper = (u"ABCDEFGHIJKLMNOPQRSTUVWXYZ" +
             u"АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ" +
             u"ÄÁÀÂÃĀÇËÉÈÊẼĒĞİÏÍÌÎĨĪÖÓÒÔÕŌŞÜÚÙÛŨŪ")
    lower = (u"abcdefghijklmnopqrstuvwxyz" +
             u"абвгдеёжзийклмнопрстуфхцчшщъыьэюя" +
             u"äáàâãāçëéèêẽēğıïíìîĩīöóòôõōşüúùûũū")
    r = (anyof(replace(u, l) for (u, l) in zip(upper, lower)) |
         # Replace Unicode glyph combinations with single codes
         replace(u"е\u0308", u"ё") |
         replace(u"Е\u0308", u"ё") |
         replace(u"и\u0306", u"й") |
         replace(u"И\u0306", u"й"))
    return r.optimize()

downcase = make_downcase()
replace_whitespace = replace(g.whitespace, " ")
lower_and_whitespace_cvt = ss(cost(downcase, 0.012) |
                              replace_whitespace |
                              g.direct_transcription_word |
                              g.chem_word |
                              cost(g.any_symb, 1))
lower_and_whitespace = lower_and_whitespace_cvt.optimize()

# Add space at start so that word() works.
space_at_start = insert(" ") + ss(g.any_symb)
space_at_start = space_at_start.determinize().minimize()

# Separate punctuation from words.
# Do not puts paces around ".", ",", ":" in numbers, dates and times
# Put a space at the end.
spaces_around_punctuation = (
    ss(cost(pp(pp(g.digit) + anyof(".,:")) + pp(g.digit) |
            pp(qq("#" + insert(" ")) + pp(g.digit) + qq(insert(" ") + "#")) |
            cost(insert(" ") + punctuation + insert(" "), 0.0001),
            0.001) |
       g.direct_transcription_word |
       g.read_by_char_word |
       g.chem_word |
       cost(g.any_symb, 0.1)) +
    insert(" ")
)

replace_profanity = word(word_list_from_file("reverse-normalization/antimat.txt") >> (g.any_symb + ss(replace(g.any_symb, "*"))))
profanity = pp(word(g.letter + pp("*")) |
               replace_profanity |
               cost(replace("*", " звездочка "), 0.01) |
               cost(g.any_symb, 1))
profanity = profanity.optimize()

#spaces_around_punctuation = spaces_around_punctuation.determinize().minimize()

def replace_unconditionally(replacements_file_path):
    unconditional_replacement = make_unconditional_replacements(replacements_file_path)
    replace_unconditionally_cvt = ss(unconditional_replacement |
                                     g.direct_transcription_word |
                                     g.read_by_char_word |
                                     cost(g.any_symb, 1))
    return replace_unconditionally_cvt.optimize()

collapse_spaces = ss(replace(pp(" "), " ") | cost(g.any_symb, 0.01))
collapse_spaces = collapse_spaces.optimize()

remove_space_at_end = qq(ss(g.any_symb) + (g.any_symb - " ")) + ss(remove(" "))
remove_space_at_end = remove_space_at_end.optimize()

replace_sharp = ss(((" " | insert(" ")) + replace("#", "решетка") + (" " | insert(" "))) |
                   g.direct_transcription_word |
                   g.read_by_char_word |
                   cost(g.any_symb, 1)).optimize()

# Composing several simple transducers gives a surprisingly huge result, so keep them separate

# preprocess_cvt = (space_at_start >>
#                   replace_unconditionally_cvt >>
#                   spaces_around_punctuation >>
#                   collapse_spaces >>
#                   remove_space_at_end
#                   )
# preprocess_cvt = preprocess_cvt.optimize()

replace_slash = replace(qq(" ") + "/" + qq(" "), " слэш ")
domain_names = ["com", "net", "edu", "info", "org", "mil", "int", "ru", "ua", "eu", "xxx", "рф", "by", "kz", "gov"]
replace_separator = replace(qq(" ") + "." + qq(" "), " точка ") | replace("-", " дефис ") | replace("_", " подчеркивание ")

transform_url = (qq(remove("http" + qq("s") + qq(" ") + ":" + qq(" ") + "/" + qq(" ") + "/")) +
                 pp(word(pp(g.digit | g.letter), need_outer_space=False) +
                 word(replace_separator, need_outer_space=False)) +
                 qq(" ") + anyof(domain_names) +
                 qq(word(pp(replace_slash + pp(g.digit | g.letter)), need_outer_space=False)) +
                 qq(remove('/')))

transform_url = convert_words(transform_url, permit_inner_space=True).optimize()

transform_email = (ss(word(pp(g.digit | g.letter), need_outer_space=False) + replace_separator) +
                   pp(g.digit | g.letter) + "@" +
                   pp(pp(g.digit | g.letter) + replace_separator) +
                   qq(" ") + anyof(domain_names))

transform_email = convert_words(transform_email, permit_inner_space=True).optimize()

# Convert ranges to "от ... до ..." with cases
# May fail with expressions like "18-21 год"
ranges = (insert("от #gen ") + pp(g.digit) +
          replace("-", " ") +
          insert("до #gen ") + pp(g.digit))
ranges_cvt = convert_words(ranges)
number_ranges = ranges_cvt.optimize()

# "Состояли в браке с 2006 по 2011";
# take care of case markers, which may be present at the time this rule is applied.
case_mark = "#" + pp(g.letter)
marriage = (
    "в" + qq(word(case_mark)) + word("браке") +
    qq(word("с" + qq(word(case_mark)) + pp(" ") + pp(g.digit) + (ss(" ") + "г" + qq(".") |
                                                                 word("года") |
                                                                 insert(" года")),
            permit_inner_space=True)) +
    qq(word("по" + qq(word(case_mark)) + pp(" ") + pp(g.digit) + (ss(" ") + "г" + qq(".") |
                                                                  word("год") |
                                                                  insert(" год")),
            permit_inner_space=True))
).optimize()

marriage = convert_words(marriage, permit_inner_space=True)

plus_minus = (
    # "+" is read as "плюс" before numbers and at the end of a word.
    # In other places it may mark stress position.
    replace("+", " плюс ")  + (g.digit | " ") |
    # "-" is read as "минус" before numbers, but not between letters and numbers
    # (should be ignored in expressions like "дом-2").
    " " + replace("-", " минус ") + g.digit |
    # Remove "-" that serves as a hyphen before digits
    g.letter + replace("-", " ") + g.digit |
    pp(g.digit + insert(" ") + "-" + insert(" ")) + g.digit
)
plus_minus_cvt = ss(plus_minus |
                    g.direct_transcription_word |
                    cost(g.any_symb, 0.01))
plus_minus = plus_minus_cvt.optimize()

slash_cvt = pp(g.digit) + (pp(" ") | insert(" ")) + replace("/", "дробь") + (pp(" ") | insert(" ")) + pp(g.digit)
slash_cvt = ss(slash_cvt |
               g.direct_transcription_word |
               cost(g.any_symb, 0.01))
convert_slash_between_numbers = slash_cvt.optimize()

num_nom_feats = feats('numeral', 'card', 'mas', 'nom')

long_digit_sequence = rr(g.digit, 12) + ss(g.digit)
# Read digits by two, with maybe three at the end, all in nominative
split_long_sequence = (ss(rr(g.digit, 2) + insert(num_nom_feats) + insert(" ")) +
                       qq(rr(g.digit, 2, 3) + insert(num_nom_feats)))

convert_digit_sequences = convert_words(long_digit_sequence >>
                                        split_long_sequence)
convert_digit_sequences = convert_digit_sequences.optimize()

# Split mixed words
# FIXME: !!!!!! For d2l, there is no proper time to apply the transformation
# without mixing it with legitimate ordinal numerals, so it's never used right now.
split_l2d_op = (g.letter | anyof(":()")) + insert(" ") + g.digit
split_d2l_op = g.digit + insert(" ") + (g.letter | anyof(":()"))
split_l2d = convert_symbols(split_l2d_op)
split_d2l = convert_symbols(split_d2l_op)

glue_punctuation = punctuation + ss(remove(ss(" ")) + punctuation)
glue_punctuation_cvt = ss(glue_punctuation |
                          g.direct_transcription_word |
                          cost(g.any_symb, 0.01))
#convert_words(glue_punctuation, permit_inner_space=True)
glue_punctuation = glue_punctuation_cvt.optimize()

# Remove all punctuation except end of sentence marks.
remove_punctuation = ss(replace(punctuation | anyof("-{}%"), " ") |
                        g.direct_transcription_word |
                        cost(g.any_symb, 0.01)).optimize()

# Replace ё -> е
replace_yo = ss(replace("ё", "е") |
                g.direct_transcription_word |
                cost(g.any_symb, 0.01)).optimize()

# Split words with hyphens
split_words_with_hyphen = convert_words(pp(g.word_sym_fst) + replace("-", " ") + pp(g.word_sym_fst))
split_words_with_hyphen = split_words_with_hyphen.optimize()

# First collapse spaces, then remove the last one
final_spaces_adjustment = (remove(ss(" ")) +
                           qq(ss(pp(g.word_sym_fst) +
                                 replace(pp(" "), " ")) +
                              pp(g.word_sym_fst) + remove(ss(" "))))
final_spaces_adjustment = final_spaces_adjustment.optimize()
