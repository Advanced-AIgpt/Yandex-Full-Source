#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('../..')

from general.normbase import *
import categories
import morphology
import case_control
import number_conversions

print 'Dates'

months = [(1, maybe_stressed("январ+я"), "янв"),
          (2, maybe_stressed("феврал+я"), "фев"),
          (3, maybe_stressed("м+арта"), "мар"),
          (4, maybe_stressed("апр+еля"), "апр"),
          (5, maybe_stressed("м+ая"), "мая"),
          (6, maybe_stressed("и+юня"), "июн"),
          (7, maybe_stressed("и+юля"), "июл"),
          (8, maybe_stressed("+августа"), "авг"),
          (9, maybe_stressed("сентябр+я"), "сен"),
          (10, maybe_stressed("октябр+я"), "окт"),
          (11, maybe_stressed("ноябр+я"), "ноя"),
          (12, maybe_stressed("декабр+я"), "дек"),
          ]

day_abbreviations = [("пн", "понедельник"),
                     ("вт", "вторник"),
                     ("ср", "сред"),
                     ("чт", "четверг"),
                     ("пт", "пятниц"),
                     ("сб", "суббот"),
                     ("вс", "воскресен")]

def day_with_case(case):
    day_fst = qq(remove("0") | anyof("123")) + g.digit + insert(feats('numeral', 'ord', 'neu', 'sg', case))
    return day_fst + qq((pp(" ") + "и" + pp(" ") | replace(ss(" ") + "-" + ss(" "), " - ")) + day_fst)

def year_with_case(case):
    # Just check for the right form of "год"
    number = cat_values("number")
    if case == 'gen':
        number = ['sg']
    raw = (
        # We definitely need at least one numeral
        pp(g.digit) +
        (anyof([insert(feats('numeral', 'ord', 'mas' if num == 'sg' else 'pl', case)) +
                word(morphology.check_form("год", feats('noun', num, case))) for num in number]) |
         (insert(feats('numeral', 'ord', 'mas', case)) +
         insert(" ") +
         word(replace("г" + qq(remove(ss(" ") + ".")), "год" + feats('noun', 'sg', case) >> morphology.producer),
              need_outer_space=False, permit_inner_space=True))))

    # Try to take cue from the previous word
    assisted = "#" + case + pp(" ") + raw
    return (assisted | cost(raw, 0.01)).optimize()

def ordinal_year(case):
    abbr_replacement = "годов" if case == "gen" else ("год" + feats("noun", "pl", case) >> morphology.producer)
    check_year_form = "годов" if case == "gen" else morphology.check_form("год", feats("noun", "pl", case))
    return ((pp(g.digit) + number_conversions.ord_tail + insert(feats('numeral', 'ord', 'pl', case)) >> number_conversions.ordinal) +
            pp(" ") + (replace("гг" + qq(remove(ss(" ") + ".")), abbr_replacement) | check_year_form))

def years_range_morphology_producer():
    return (replace("год" + feats('noun', 'pl', 'gen'), "годов") |
            cost(morphology.producer, 0.01))

def years_range_with_case(abbr, number, case):
    check = morphology.check_form("год", feats('noun', case))
    if case == 'gen':
        check = morphology.check_form("год", feats('noun', 'sg', case)) | "годов"
    return (pp(g.digit) + insert(feats('numeral', 'ord', case, 'mas')) +
            insert(" ") + "-" + insert(" ") +
            pp(g.digit) + insert(feats('numeral', 'ord', case, 'mas')) +
            (pp(" ") | insert(" ")) +
            (replace(abbr + qq(qq(" ") + "."), "год" + feats('noun', number, case) >> years_range_morphology_producer()) |
            check))

years_range = ("с" + qq(remove(word("#gen"))) +
               word(pp(g.digit) + qq(qq("-") + pp(g.letter)) + insert(feats('numeral', 'ord', 'gen', 'mas'))) +
               ((word("по") + qq(remove(word("#acc"))) +
                 word(pp(g.digit) + qq(qq("-") + pp(g.letter)) + insert(feats('numeral', 'ord', 'acc', 'mas')))) |
                (word("до") + qq(remove(word("#gen"))) +
                 word(pp(g.digit) + insert(feats('numeral', 'ord', 'gen', 'mas'))))) +
               (word("год" + qq(Fst("ы") | "а")) |
                ((pp(" ") | insert(" ")) + (replace("г" + qq(ss(" ") + "."), "год") | replace("гг" + qq(ss(" ") + "."), "годы")))) |
               case_control.use_mark(lambda c: years_range_with_case("г", 'sg', c), permit_inner_space=True) |
               case_control.use_mark(lambda c: years_range_with_case("гг", 'pl', c), default_case='gen', permit_inner_space=True))

# Insert the word "года"
insert_year =replace(qq(word("г" + ss(" ") + qq(remove(".")) | maybe_stressed("г+ода"),
                             need_outer_space=False, permit_inner_space=True)),
                     " года")

days_range = anyof([("c" + qq(remove(word("#gen"))) | insert("с ")) +
                    replace(first_abbr, first_stem + feats("noun", 'sg', 'gen') >> morphology.producer) +
                    ((" по" + qq(remove(word("#acc"))) + " ") | replace("-", " по ")) +
                    replace(second_abbr, second_stem + feats("noun", 'sg', 'acc') >> morphology.producer)
                   for abbr_index, (first_abbr, first_stem) in enumerate(day_abbreviations)
                   for second_abbr, second_stem in day_abbreviations[abbr_index + 1:]])

date = (
    ( # day, month, [year]
        #day
        case_control.use_mark(day_with_case, default_case='gen') +
        (   # Either month name + optional year
            (pp(" ") + anyof([(name | replace(abbr, name))
                              for (num, name, abbr) in months]) +
             qq(pp(" ") + rr(g.digit, 1, 4) + insert("-го") + insert_year)) |
            # or month number + obligatory year
            (remove(anyof("./-")) +
             anyof([replace(anyof([str(num), "%02d" % num]),
                            " " + name + " ")
                    for (num, name, abbr) in months]) +
             remove(anyof("./-"))) +
            (rr(g.digit, 2, 4) + insert("-го")) + insert_year)) |
    ( # year; simple use_mark does not work here, since we are trying to work with the word "год"
        (year_with_case('nom') |
         cost(anyof([year_with_case(case)
                    for case in cat_values('case')]), 0.01))) |
        case_control.use_mark(lambda case: ordinal_year(case), default_case='gen') |
    years_range |
    days_range |
    anyof([case_control.use_mark(lambda case: replace(abbreviation, stem + feats("noun", 'sg', case) >> morphology.producer))
          for abbreviation, stem in day_abbreviations]))

cvt = convert_words(date, permit_inner_space=True)
convert_date = cvt.optimize()
