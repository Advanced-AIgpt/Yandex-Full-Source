#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('../..')

from general.normbase import *
import categories

print 'Preprocess'

punctuation = anyof("\".,!?:;()")

def make_word_replacements(fname):
    res = failure()
    with open(fname) as fd:
        for ln in fd:
            ln = ln.strip().decode('utf-8')
            if ln == "" or ln.startswith("#"):
                continue
            [src, dest] = ln.split("\t")
            src = src.strip()
            if src[0] == "!":
                # Exact match
                res |= replace(src[1:], dest)
            elif src[0] == '.':
                # Only replace when there is _no_ dot after the abbreviation
                # (for geographic direction: one letter may just as well be part of a bigger
                # abbreviation, but then it will usually be followed by dot.)
                res |= (src[1:] + ss(" ") + "." |
                        cost(replace(src[1:], dest), 0.01))
            else:
                res |= replace(anycase(src), dest)
    return res

def make_downcase():
    upper = (u"ABCDEFGHIJKLMNOPQRSTUVWXYZ" +
             u"АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ" +
             u"ÄÇËÊÈĞİÖÓŞÜÛ")
    lower = (u"abcdefghijklmnopqrstuvwxyz" +
             u"абвгдеёжзийклмнопрстуфхцчшщъыьэюя" +
             u"äçëêèğıöóşüû")
    r = anyof([replace(u, l) for (u, l) in zip(upper, lower)])
    return r.optimize()

downcase = make_downcase()
replace_whitespace = replace(g.whitespace, " ")
lower_and_whitespace_cvt = ss(downcase | replace_whitespace | g.direct_transcription_word |
                              cost(g.any_symb, 0.01))
lower_and_whitespace_cvt.optimize()

# For capio, we don't lower
whitespace_cvt = convert_symbols(replace_whitespace)

# Add space at start so that word() works.
space_at_start = insert(" ") + ss(g.any_symb)
space_at_start = space_at_start.determinize().minimize()

# For Capio data: asteriska are ignored
remove_asterisks = convert_symbols(remove("*"))

# Separate punctuation from words.
# Do not puts paces around ".", ",", ":" in numbers, dates and times
# Special treatment for "." after a sequence of digits: means ordinal numer in Turkish
# Put a space at the end.
spaces_around_punctuation = (ss(g.digit + anyof(".,") + g.digit |
                                cost(insert(" ") + punctuation + insert(" "), 0.001) |
                                g.direct_transcription_word |
                                cost(g.any_symb, 0.1)) + insert(" "))

#spaces_around_punctuation = spaces_around_punctuation.determinize().minimize()

unconditional_replacement = make_unconditional_replacements("normalization/replacements.txt")
replace_unconditionally_cvt = convert_symbols(unconditional_replacement)

word_replacement_cvt = convert_words(make_word_replacements("normalization/word-replacements.txt"), permit_inner_space=True)

collapse_spaces = ss(replace(pp(" "), " ") | cost(g.any_symb, 0.01))
collapse_spaces.optimize()

remove_space_at_end = qq(ss(g.any_symb) + (g.any_symb - " ")) + ss(remove(" "))
remove_space_at_end.optimize()

# Read addresses and zipcodes by digits/digit groups
address_or_zipcode = (
    (rr(g.digit,2) +
     insert(" ") + (
         replace("00", "hundred") |
         cost(rr(g.digit, 2), 0.01)
     )) |
    (g.digit + insert(" ") +
     g.digit + insert(" ") +
     g.digit + insert(" ") +
     g.digit + insert(" ") +
     g.digit)
)

address_and_zipcode_cvt = convert_words(address_or_zipcode)

# Composing several simple transducers gives a surprisingly huge result, so keep them separate

# preprocess_cvt = (space_at_start >>
#                   replace_unconditionally_cvt >>
#                   spaces_around_punctuation >>
#                   collapse_spaces >>
#                   remove_space_at_end
#                   )
# preprocess_cvt.optimize()

plus_minus = (# don't touch "-" between digits
    pp(g.digit) + pp(replace(ss(" "), " ") + "-" +
                     replace(ss(" "), " ") + pp(g.digit)) |
    cost(replace("+", " plus ") + ss(" ") + g.digit, 0.01) |
    cost(replace("-", " minus ") + ss(" ") + g.digit, 0.01)
)
plus_minus_cvt = convert_symbols(plus_minus)

plus_only = (replace("+", " plus ") + ss(" ") + g.digit |
             (pp(" ") | insert(" ")) + "-" + (pp(" ") | insert(" ")) + g.digit)
plus_only_cvt = convert_symbols(plus_only)

long_digit_sequence = rr(g.digit, 12) + ss(g.digit)
# Read digits by two, with maybe three at the end, with a pause before each chunk
split_long_sequence = insert(" : ") + ss(rr(g.digit, 2) + insert(" , ")) + qq(rr(g.digit, 2, 3))

convert_digit_sequences = convert_words(long_digit_sequence >>
                                        split_long_sequence)
convert_digit_sequences.optimize()

split_digits_from_letters = (
    qq(pp(g.letter) + insert(" ")) +
    ss(pp(g.digit | ".") + insert(" ") + pp(g.letter) + insert(" ")) +
    pp(g.digit | ".") +
    # after the last group of digits, accept ordinal markers, and prefer this variant to arbitrary letter sequences
    (cost(anyof(["", "st", "nd", "rd", "th"]), -0.1) |
     qq(insert(" ") + pp(g.letter)))
)
split_digits_from_letters_cvt = convert_words(os.path.join(split_digits_from_letters))

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
