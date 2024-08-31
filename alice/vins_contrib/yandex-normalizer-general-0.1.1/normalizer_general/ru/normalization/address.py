#! /usr/bin/env python2
# encoding: utf-8
import sys
import os
sys.path.append('..')
sys.path.append('../..')

from general.normbase import *
from morphology import *
import case_control

print "Addresses"

maybe_dot = qq(qq(" ") + ".")
remove_case_mark = remove("#" + anyof(cat_values('case')))

region_abbrs = [("обл" + maybe_dot, "област"),
                ("р-н" + rr(g.russian_letter, 0, 2), "район")]

city_abbrs = [("г" + maybe_dot, "город"),
              ("п" + qq("ос") + maybe_dot, "посёл"),
              ("д" + qq("ер") + maybe_dot, "деревн"),
              ("с" + qq(" ") + qq(remove_case_mark) + qq(" ") + ".", "сел")]

street_abbrs = [("ул" + maybe_dot, "улиц"),
                ("бул" + maybe_dot | "б-р", "бульвар"),
                ("пр" + qq("осп") + maybe_dot | "пр-кт", "проспект"),
                ("наб" + maybe_dot, "набережн"),
                ("пер" + maybe_dot, "переул"),
                ("пр-зд", "проезд"),
                ("ш" + maybe_dot, "шоссе")]

building_abbrs = [("кор" + qq("п") + maybe_dot, "корпус"),
                  ("ст" + qq("р") + maybe_dot, "строени"),
                  ("эт" + maybe_dot, "этаж")]

flat_abbrs = [("кв" + maybe_dot |
               "к" + qq(" ") + qq(remove_case_mark) + maybe_dot, "квартир"),
              ("оф" + maybe_dot, "офис")]

def accept_or_replace(stem, abbr, case):
    ff = feats("noun", "sg", case)
    return check_form(stem, ff) | replace(abbr, stem + ff >> producer)

def any_name_from_file(file_name):
    fst = failure()
    possible_replacements = {"ё" : "е", "-" : " "}
    with open(file_name) as f:
        for line in f:
            name = line.strip()
            fst |= name
            clear_name = name
            for symbol, replacement in possible_replacements.iteritems():
                fst |= name.replace(symbol, replacement)
                clear_name = clear_name.replace(symbol, replacement)
                fst |= clear_name
    return fst.optimize()

metro_name = any_name_from_file(os.path.join(os.path.dirname(os.path.abspath(__file__)), "metro.txt"))
city_name = any_name_from_file(os.path.join(os.path.dirname(os.path.abspath(__file__)), "cities.txt"))

# Convert "м" to "метро"
expand_metro = convert_words(replace("м" + maybe_dot, "метро") + pp(" ") + metro_name,
                             permit_inner_space = True).optimize()

# Street name or city name
city_or_street_name = (pp(g.russian_letter | g.digit) +
                       (rr(cost("-" + pp(g.russian_letter | g.digit), 0.01), 0, 2) |
                        rr(cost(" " + pp(g.russian_letter | g.digit), 0.01), 0, 2)))

separator = anyof(", ")

expand_region = lambda case: anyof([pp(g.russian_letter) + pp(separator) + accept_or_replace(stem, abbr, case) + pp(separator)
                                   for abbr, stem in region_abbrs])

expand_city = lambda case: anyof([accept_or_replace(stem, abbr, case) + pp(separator) + city_or_street_name + pp(separator)
                                 for abbr, stem in city_abbrs])

expand_street = lambda case: anyof([(accept_or_replace(stem, abbr, case) + pp(separator) + city_or_street_name + pp(separator)) |
                                    (city_or_street_name + pp(separator) + accept_or_replace(stem, abbr, case) + pp(separator))
                                   for abbr, stem in street_abbrs])

expand_building = lambda case: anyof([pp(separator) + accept_or_replace(stem, abbr, case) + pp(separator) + (pp(g.digit) | g.letter)
                                     for abbr, stem in building_abbrs])

expand_flat = lambda case: anyof([pp(separator) + accept_or_replace(stem, abbr, case) + pp(separator) + pp(g.digit)
                                 for abbr, stem in flat_abbrs])

expand_address = case_control.use_mark(lambda case: ss(expand_region(case)) +
                                                    qq(expand_city(case)) +
                                                    expand_street(case) +
                                                    qq(qq(accept_or_replace("дом", "д" + maybe_dot, case) + pp(separator)) + pp(g.digit) + ss(separator) +
                                                    qq(((pp(" ") | insert(" ")) + g.letter) | (replace("/", " дробь ") + ss(separator) + (g.russian_letter | pp(g.digit)))) +
                                                    ss(expand_building(case)) +
                                                    qq(expand_flat(case))),
                                       permit_inner_space=True)

expand_address = convert_words(expand_address, permit_inner_space=True).optimize()

# expand city
expand_city_with_name = case_control.use_mark(lambda case: (anyof([accept_or_replace(stem, abbr, case) + pp(separator) + city_name + ss(separator)
                                                            for abbr, stem in city_abbrs])),
                                              permit_inner_space=True)

expand_city_with_name = convert_words(expand_city_with_name, permit_inner_space=True).optimize()
