#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('../..')

from general.normbase import *
import categories
import morphology

print 'Roman numerals'

# Note: after Turkish simple.lower_and_whitespace, "I" will be converted to "ı", not "i".
roman_digit = anyof(u"ıvxlcdm")

roman_ones = (replace(u"ı", "1") |
              replace(u"ıı", "2") |
              replace(u"ııı", "3") |
              replace(u"ıv", "4") |
              replace(u"v", "5") |
              replace(u"vı", "6") |
              replace(u"vıı", "7") |
              replace(u"vııı", "8") |
              replace(u"ıx", "9"))

roman_tens = ((replace(u"x", "1") |
               replace(u"xx", "2") |
               replace(u"xxx", "3") |
               replace(u"xl", "4") |
               replace(u"l", "5") |
               replace(u"lx", "6") |
               replace(u"lxx", "7") |
               replace(u"lxxx", "8") |
               replace(u"xc", "9")) +
              (insert("0") | roman_ones))

roman_hundreds = ((replace(u"c", "1") |
                   replace(u"cc", "2") |
                   replace(u"ccc", "3") |
                   replace(u"cd", "4") |
                   replace(u"d", "5") |
                   replace(u"dc", "6") |
                   replace(u"dcc", "7") |
                   replace(u"dccc", "8") |
                   replace(u"cm", "9")) +
                  (insert(u"00") |
                   insert(u"0") + roman_ones |
                   roman_tens))

roman_thousands = ((replace(u"m", "1") |
                    replace(u"mm", "2") |
                    replace(u"mmm", "3")) +
                   (insert("000") |
                    insert("00") + roman_ones |
                    insert("0") + roman_tens |
                    roman_hundreds))

roman = (roman_ones | roman_tens | roman_hundreds | roman_thousands)

# Roman number, maybe with an ordinal marker and a suffix
# Suffixes would be split off from the number by simple.spaces_around_punctuation, so undo that.
roman_w = (roman +
           qq(remove(ss(" ")) + ".") +
           qq(remove(ss(" ")) + "'" + remove(ss(" ")) + pp(g.letter))
).optimize()

cvt = convert_words(roman_w, permit_inner_space=True)


