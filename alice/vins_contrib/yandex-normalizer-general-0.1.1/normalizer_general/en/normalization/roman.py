#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('../..')

from general.normbase import *
import categories
#import morphology

print 'Roman numerals'

roman_digit = anyof(u"IVXLCDM")

roman_ones = (replace(u"I", "1") |
              replace(u"II", "2") |
              replace(u"III", "3") |
              replace(u"IV", "4") |
              replace(u"V", "5") |
              replace(u"VI", "6") |
              replace(u"VII", "7") |
              replace(u"VIII", "8") |
              replace(u"IX", "9"))

roman_tens = ((replace(u"X", "1") |
               replace(u"XX", "2") |
               replace(u"XXX", "3") |
               replace(u"XL", "4") |
               replace(u"L", "5") |
               replace(u"LX", "6") |
               replace(u"LXX", "7") |
               replace(u"LXXX", "8") |
               replace(u"XC", "9")) +
              (insert("0") | roman_ones))

roman_hundreds = ((replace(u"C", "1") |
                   replace(u"CC", "2") |
                   replace(u"CCC", "3") |
                   replace(u"CD", "4") |
                   replace(u"D", "5") |
                   replace(u"DC", "6") |
                   replace(u"DCC", "7") |
                   replace(u"DCCC", "8") |
                   replace(u"CM", "9")) +
                  (insert(u"00") |
                   insert(u"0") + roman_ones |
                   roman_tens))

roman_thousands = ((replace(u"M", "1") |
                    replace(u"MM", "2") |
                    replace(u"MMM", "3")) +
                   (insert("000") |
                    insert("00") + roman_ones |
                    insert("0") + roman_tens |
                    roman_hundreds))

roman = (roman_ones | roman_tens | roman_hundreds | roman_thousands)

stop_words = anyof(["I", "MIX", roman_digit + ss(" ") + "."])

# Roman number, maybe with an ordinal marker and a suffix
# Suffixes would be split off from the number by simple.spaces_around_punctuation, so undo that.
# Also, don't replace stop words.
roman_w = (cost(stop_words, -0.1) |
           (roman +
            qq(remove(ss(" ")) + qq("-") + remove(ss(" ")) + anyof(["st","nd","rd","th"])))
).optimize()

cvt = convert_words(roman_w, permit_inner_space=True)
