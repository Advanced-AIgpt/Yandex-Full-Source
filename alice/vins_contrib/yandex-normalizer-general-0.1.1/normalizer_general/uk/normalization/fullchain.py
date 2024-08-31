#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('../..')

from general.normbase import FSTSequenceSaver
import simple_conversions as simple
import numbers
import by_character


with FSTSequenceSaver('norm') as saver:
    saver.add(simple.lower_and_whitespace_cvt, 'lower_and_whitespace_cvt')
    saver.add(simple.space_at_start, 'space_at_start')
    saver.add(simple.spaces_around_punctuation, 'spaces_around_punctuation')
    saver.add(simple.collapse_spaces, 'collapse_spaces')
    saver.add(simple.replace_unconditionally_cvt, 'replace_unconditionally_cvt')
    saver.add(simple.plus_minus_cvt, 'plus_minus_cvt')
    saver.add(simple.convert_digit_sequences, 'convert_digit_sequences')
    saver.add(numbers.numbers_cvt, 'numbers_cvt')
    saver.add(by_character.cvt, 'by_character_cvt')
    saver.add(simple.glue_punctuation_cvt, 'glue_punctuation_cvt')
    saver.add(simple.final_spaces_adjustment, 'final_spaces_adjustment')
    saver.add(simple.remove_space_at_end, 'remove_space_at_end')
