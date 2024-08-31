#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('../..')

import shutil

from general.normbase import *
import morphology
import simple_conversions as simple
import roman
import time_cvt
import date
import units
import case_to_features
import numbers
import latitude_longitude

target_folder = "norm"

shutil.rmtree(target_folder, ignore_errors=True)
os.mkdir(target_folder)

fst_syms_save(os.path.join(target_folder, "symbols"))

with open(os.path.join(target_folder, "sequence.txt"), "w") as of:
    def save(fst, name):
        fst_save(fst, os.path.join(target_folder, name))
        print >>of, name

    save(simple.lower_and_whitespace_cvt, "lower_and_whitespace")
    save(simple.space_at_start, "space_at_start")
    save(simple.replace_punctuation_cvt, "replace_punctuation")
    save(simple.spaces_around_punctuation, "spaces_around_punctuation")
    save(simple.collapse_spaces, "collapse_spaces")
    save(latitude_longitude.cvt, "latitude_longitude")
    save(simple.ignore_marks_near_numbers_cvt, "ignore_marks_near_numbers")
    save(simple.replace_unconditionally_cvt, "replace_unconditionally")
    save(units.cvt, "units")
    save(roman.cvt, "roman")
    save(time_cvt.cvt, "time")
    save(date.cvt, "date")
    save(simple.ranges_cvt, "ranges")
    save(simple.split_l2d_cvt, "split_l2d")
    save(case_to_features.cvt, "case_to_features")
    save(numbers.cvt, "number")
    save(simple.glue_punctuation_cvt, "glue_punctuation")
    save(simple.remove_space_at_end, "remove_space_at_end")
    save(simple.final_spaces_adjustment, "final_spaces_adjustment")

with open(os.path.join(target_folder, "flags.txt"), "w") as ff:
    print >>ff, "report-intermediate false"
