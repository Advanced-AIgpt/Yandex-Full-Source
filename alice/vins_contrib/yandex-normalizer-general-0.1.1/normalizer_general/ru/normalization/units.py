#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('../..')

from general.normbase import *
from morphology import *
import number_conversions as n
import case_control

print "Units"

maybe_dot = qq(remove(ss(" ") + "."))

number_conversions_abbr = [
    ("тыс" + maybe_dot, "тысяч", 'fem'),
    ("млн" + maybe_dot, "миллион", 'mas'),
    ("млрд" + maybe_dot, "миллиард", 'mas'),
    ("трлн" + maybe_dot, "триллион", "mas")
    ]

memory_unit = [("кб", "килобайт", "mas"),
               ("кбит", "килобит", "mas"),
               ("мб", "мегабайт", "mas"),
               ("мбит", "мегабит", "mas"),
               ("гб", "гигабайт", "mas"),
               ("гбит", "гигабит", "mas"),
               ("тб", "терабайт", "mas"),
               ("тбит", "терабит", "mas")]

unit_conversions_no_subunits = [
    ("%", "процент", 'mas'),
    ("проц" + maybe_dot, "процент", 'mas'),
    ("‰", "промилле", 'neu'),
    ("°", "градус", 'mas'),
    ("#gram", "грамм", 'mas'),
    ("град" + maybe_dot, "градус", 'mas'),
    ("дн" + maybe_dot, "день", 'mas'),
    ("чел" + maybe_dot, "человек", 'mas'),
    ("#minute", "минут", 'fem'),
    ("#yr", "год", 'mas'),
    ("га", "гектар", "mas"),
    (failure(), "час", 'mas'),
    (failure(), "мил", "fem"),
    (failure(), "унци", "fem"),
    ("т" + maybe_dot, "тонн", "fem"),
    ] + number_conversions_abbr + memory_unit

unit_conversions_with_subunits = [
    ("км", "километр", 'mas', 3, "м", "метр", 'mas'),
    ("#kilometer", "километр", 'mas', 3, "м", "метр", 'mas'),
    ("кг", "килограмм", 'mas', 3, "г", "грамм", 'mas'),
    ("#kilogram", "килограмм", 'mas', 3, "г", "грамм", 'mas'),
    ("м",  "метр", 'mas', 2, "см", "сантиметр", 'mas'),
    ("#meter",  "метр", 'mas', 2, "см", "сантиметр", 'mas'),
    ("см",  "сантиметр", 'mas', 1, "мм", "миллиметр", 'mas'),
    ("#centimeter",  "сантиметр", 'mas', 1, "мм", "миллиметр", 'mas'),
    ]

unit_conversions_with_subunits_and_fronting = [
    ((anyof(["р","руб"]) + maybe_dot) | "₽", "рубл", 'mas', 2, anyof(["коп", "к"]) + maybe_dot, "копе", 'fem'),
    # No abbreviation for "цент"
    ("$", "доллар", 'mas', 2, "¢", "цент", 'mas'),
    ("€", "евро", 'mas', 2, failure(), "евроцент", 'mas'),
    ("£", "фунт", 'mas', 2, failure(), "пенс", 'mas'),
    ("₴", "гривн", "fem", 2, anyof(["коп", "к"]) + maybe_dot, "копе", 'fem'),
    (failure(), "крон", "fem", 2, failure(), failure(), failure()),
    (failure(), "крон", "fem", 2, failure(), "эре", "neu"),
    (anyof(("₺", "£", "₤")), "лир", "fem", 2, failure(), "куруш", "mas"),
    ("¥", anyof(["иен", "йен"]), "fem", 2, failure(), "сен", "mas"),
]

unit_modifiers = [
    ("2", "квадратн", "квадрат"),
    ("²", "квадратн", "квадрат"),
    ("3", "кубическ", "куб"),
    ]

unit_conversions_with_builtin_modifiers = [
    ("#square_kilometer", "квадратн", "километр", 'mas'),
]

distance_units = [("мм", "миллиметр", "mas"),
                  ("см", "сантиметр", "mas"),
                  ("м", "метр", "mas"),
                  ("км", "километр", "mas")]

composite_modifiers = [("рт" + maybe_dot + " ст" + maybe_dot, "ртутного столба"),
                       ("вод" + maybe_dot + " ст" + maybe_dot, "водяного столба")]

# Remove spaces between groups of digits
digit_seq = qq(anyof("+-")) + g.digit + ss(g.digit | remove(" "))
digit_dot_seq = qq(anyof("+-")) + g.digit + ss(g.digit | anyof(".,") | remove(" "))
digit_dot_unit_seq = digit_dot_seq + pp(" ") + pp(g.letter)
digit_dot_unit_mod_seq = digit_dot_unit_seq + pp(" ") + pp(g.letter)

def check_digits_with_number_abbr(case, number_abbr_description):
    number_unit = number_abbr_description[1]
    number_gender = number_abbr_description[2]
    return ((word(digit_dot_seq, need_outer_space=False, permit_empty_words=False) +
              (feats("numeral") | insert(feats('numeral', 'card', number_gender, case) + " ")) +
              word(check_form(number_unit, feats("noun")), need_outer_space = False)) | cost(empty(), 0.005)).optimize()

# Converts unit that stands after тысяч, миллион ...
def make_unit_conversion_number_as_word(unit, unit_gender, case, modifier=None):
    return (word(digit_dot_seq, need_outer_space=False, permit_empty_words=False) +
            anyof([(feats("numeral") | insert(feats('numeral', 'card', number_gender, case if case != "gen" else "nom") + " ")) +
                   word(check_form(number_unit, feats("noun", case)), need_outer_space = False)
                   for number_abbr, number_unit, number_gender in number_conversions_abbr]) +
            n.unit_after_number_abbr(unit, modifier)).optimize()

def make_conversion_1level(unit, gender, case):
    return digit_dot_seq >> n.unit_with_case(unit, gender, case)

def make_unit_conversion_no_subunits(abbr, unit, gender, case):
    with_abbr = ((make_conversion_1level(unit, gender, case) | (make_unit_conversion_number_as_word(unit, gender, case) + " ")) +
                 remove(word(abbr, need_outer_space=False, permit_inner_space=True)))
    with_full_unit = digit_dot_unit_seq >> n.unit_with_case(unit, gender, case, has_unit_already = True)
    return (with_abbr | with_full_unit).optimize()

# |
#            # Standalone symbol -- too risky
#            replace(abbr, (unit + feats('noun', case, 'sg') >> producer)))

def make_unit_conversion_with_subunits(big_abbr, big_unit, big_gender,
                                       ndigits,
                                       small_abbr, small_unit, small_gender,
                                       case):
    return ((((digit_seq >> make_conversion_1level(big_unit, big_gender, case)) +
              qq(replace(anyof(".,"), " ") +
                 # Don't read out the initial zeros
                 (cost(rr(g.digit, ndigits) >> remove(ss("0")) + (g.digit - "0") + ss(g.digit) >> make_conversion_1level(small_unit, small_gender, case), 0.001) |
                  replace(rr("0", ndigits), "ровно")))) +
             remove(word(big_abbr, need_outer_space=False, permit_inner_space=True))) |
            ((digit_seq >> make_conversion_1level(big_unit, big_gender, case)) +
             remove(word(big_abbr, need_outer_space=False, permit_inner_space=True)) +
             word(rr(g.digit, 1, ndigits) >> make_conversion_1level(small_unit, small_gender, case)) +
             remove(word(small_abbr, need_outer_space=False, permit_inner_space=True))) |
            # This variant should be dispreferred
            cost(make_unit_conversion_no_subunits(big_abbr, big_unit, big_gender, case), 0.1) |
            # This variant should also be dispreferred, to give "15р 21к" a chance.
            cost(make_unit_conversion_no_subunits(small_abbr, small_unit, small_gender, case), 0.1)).optimize()

def make_unit_conversion_with_fronting(big_abbr, big_unit, big_gender,
                                       ndigits,
                                       small_unit, small_gender,	# small_abbr is not needed
                                       case):
    def insert_big_unit_with_cost(c):
        return (cost(word(digit_seq >> make_conversion_1level(big_unit, big_gender, case), need_outer_space=False), c) |
                cost(n.unit_after_number_abbr(big_unit), c + 0.0001))

    # Convert fronting with chain of number abbreviations
    number_abbrs_chain = insert_big_unit_with_cost(0)
    for index, number_abbr_description in enumerate(number_conversions_abbr[:-1]):
        number_abbrs_chain = (check_digits_with_number_abbr(case, number_abbr_description) + number_abbrs_chain) | insert_big_unit_with_cost((index + 1) * 0.05)
    number_abbrs_chain = check_digits_with_number_abbr(case, number_conversions_abbr[-1]) + number_abbrs_chain
    return ((remove(big_abbr) +
             (number_abbrs_chain |
              cost(word((digit_seq >> make_conversion_1level(big_unit, big_gender, case)) +
                        qq(replace(anyof(".,"), " ") +
                           # Don't read out the initial zeros
                           (cost(rr(g.digit, ndigits) >> remove(ss("0")) + (g.digit - "0") + ss(g.digit) >> make_conversion_1level(small_unit, small_gender, case), 0.001) |
                            replace(rr("0", ndigits), "ровно"))),
                        need_outer_space=False), 0.01) |
              # This variant should be dispreferred
              cost(make_unit_conversion_no_subunits(big_abbr, big_unit, big_gender, case), 0.1))) >> (word(digit_dot_seq, need_outer_space=False) + ss(g.any_symb))).optimize()


def make_unit_conversion_with_modifier(abbr, unit, gender, case):
    r = anyof([(((digit_dot_seq >> n.unit_with_case(unit, gender, case, modifier=mod_text)) |
                 make_unit_conversion_number_as_word(unit, gender, case, mod_text)) +
                remove(ss(" ") + abbr + mod_sym)) |
               (digit_dot_unit_mod_seq >> n.unit_with_case(unit, gender, case, modifier=mod_text, has_unit_already = True, has_modifier_already = True)) 
               for mod_sym, mod_text, mod_name in unit_modifiers])
    return r.optimize()

def make_unit_conversion_with_builtin_modifier(abbr, modifier, unit, gender, case):
    r = ((digit_dot_seq >> n.unit_with_case(unit, gender, case, modifier=modifier)) +
         remove(ss(" ") + abbr))
    return r.optimize()

def make_conversion_unit_with_composite_modifier(abbreviation, unit, gender, case, modifier_abbreviation, modifier):
    return ((digit_dot_seq >> n.unit_with_case(unit, gender, case)) +
            remove(word(abbreviation, need_outer_space=False, permit_inner_space=True)) + pp(" ") +
            replace(word(modifier_abbreviation, need_outer_space=False, permit_inner_space=True), modifier))

def make_conversion_currency_with_modifier(unit, gender, case):
    return digit_dot_unit_mod_seq >> n.unit_with_case(unit, gender, case, has_unit_already = True, has_modifier_already = True)

# Case may be determined by context
unit_conversions = (anyof([case_control.use_mark(lambda c: make_unit_conversion_no_subunits(abbr, unit, gender, c),
                                                 permit_inner_space=True)
                           for abbr, unit, gender in unit_conversions_no_subunits
                           ]) |
                    anyof([case_control.use_mark(lambda c: make_unit_conversion_with_subunits(big_abbr, big_unit, big_gender,
                                                                                              ndigits,
                                                                                              small_abbr, small_unit, small_gender,
                                                                                              c),
                                                 permit_inner_space=True)
                           for big_abbr, big_unit, big_gender, ndigits, small_abbr, small_unit, small_gender in
                             # fronting is optional
                             (unit_conversions_with_subunits + unit_conversions_with_subunits_and_fronting)]) |
                    anyof([case_control.use_mark(lambda c: make_unit_conversion_with_modifier(abbr, unit, gender, c),
                                                 permit_inner_space=True)
                           for abbr, unit, gender in unit_conversions_no_subunits]) |
                    anyof(case_control.use_mark(lambda c: make_unit_conversion_with_builtin_modifier(abbr, modifier, unit, gender, c),
                                                permit_inner_space=True)
                          for abbr, modifier, unit, gender in unit_conversions_with_builtin_modifiers) |
                    anyof([case_control.use_mark(lambda c: (make_unit_conversion_with_modifier(big_abbr, big_unit, big_gender, c) |
                                                            make_unit_conversion_with_modifier(small_abbr, small_unit, small_gender, c)),
                                                 permit_inner_space=True)
                           for big_abbr, big_unit, big_gender, ndigits, small_abbr, small_unit, small_gender in
                             # fronting is optional
                             (unit_conversions_with_subunits + unit_conversions_with_subunits_and_fronting)]) |
                    anyof([case_control.use_mark(lambda c: make_unit_conversion_with_fronting(big_abbr, big_unit, big_gender,
                                                                                              ndigits,
                                                                                              small_unit, small_gender,
                                                                                              c),
                                                 permit_inner_space=True)
                           for big_abbr, big_unit, big_gender, ndigits, small_abbr, small_unit, small_gender in
                             unit_conversions_with_subunits_and_fronting]) |
                    anyof([case_control.use_mark(lambda case: make_conversion_unit_with_composite_modifier(abbreviation, unit, gender, case, modifier_abbreviation, modifier),
                                                 permit_inner_space=True)
                           for abbreviation, unit, gender in distance_units
                           for modifier_abbreviation, modifier in composite_modifiers]) |
                    anyof([case_control.use_mark(lambda case: make_conversion_currency_with_modifier(big_unit, big_gender, case),
                                                 permit_inner_space=True)
                           for big_abbr, big_unit, big_gender, ndigits, small_abbr, small_unit, small_gender in
                             unit_conversions_with_subunits_and_fronting]))
unit_conversions = unit_conversions.optimize()

# A special treatment for °c and °f
expand_degree_type = (replace("°c", "° цельсия") |
                      replace("°f", "° фаренгейта") |
                      replace("°k", "° кельвина"))
expand_degrees = ss(expand_degree_type |
                    cost(g.any_symb, 0.001)).optimize()

# Converts numbers abbreviations
convert_number_abbr = convert_words(anyof([case_control.use_mark(lambda c: (digit_dot_seq >> n.unit_with_case(unit, gender, c)) + remove(word(abbr, need_outer_space=False, permit_inner_space=True)),
                                                                 permit_inner_space=True,
                                                                 need_outer_space=False)
                                           for abbr, unit, gender in number_conversions_abbr]),
                                    f_cost=0.0,
                                    permit_inner_space=True,
                                    need_outer_space=False)

convert_units = (expand_degrees >> convert_words(unit_conversions, permit_inner_space=True)).optimize()

# Convert abbr/abbr

def convert_unit_slash_unit(case, first_abbr, first_unit, first_gender, second_abbr, second_unit, second_gender):
    return (make_conversion_1level(first_unit, first_gender, case) + ss(" ") +
            remove(first_abbr) + ss(remove(" ")) + replace("/", " в ") + ss(remove(" ")) +
            replace(second_abbr, second_unit + feats('noun', second_gender, 'sg', 'acc') >> producer))

def convert_unit_slash_unit_with_modifier(case, first_abbr, first_unit, first_gender, second_abbr, second_unit, second_gender, modifier_after_unit=False):
    second_unit_replacement = second_unit + feats('noun', second_gender, 'sg', 'acc') >> producer
    if modifier_after_unit:
        unit_with_modifier_replacement = lambda mod_sym, mod_name: second_unit_replacement + " в " + (mod_name + feats("noun", 'sg', 'mas', 'loc') >> producer)
    else:
        unit_with_modifier_replacement = lambda mod_sym, mod_name: (mod_text + feats('adjective', second_gender, 'sg', 'acc') >> producer) + " " + second_unit_replacement
    return anyof([make_conversion_1level(first_unit, first_gender, case) + ss(" ") +
                  remove(first_abbr) + ss(remove(" ")) + replace("/", " на ") + ss(remove(" ")) +
                  (replace(second_abbr + mod_sym, unit_with_modifier_replacement(mod_sym, mod_name)) |
                   cost(replace(second_abbr, second_unit_replacement), 0.01))
                 for mod_sym, mod_text, mod_name in unit_modifiers])

# Sometimes english letter "c" occur instead of russian.
time_units = [("с" | Fst.convert("c"), "секунд", "fem"),
              ("ч", "час", "mas")]

weight_units = [("г", "грамм", "mas"),
                ("кг", "килограмм", "mas")]

people_unit = [("чел", "человек", "mas")]

convert_units_with_slash = convert_words(anyof([case_control.use_mark(lambda case: convert_unit_slash_unit_with_modifier(case, modifier_after_unit=True, *(first_unit + second_unit)),
                                                                      permit_inner_space=True)
                                                for first_unit in distance_units
                                                for second_unit in time_units]) |
                                         anyof([case_control.use_mark(lambda case: convert_unit_slash_unit_with_modifier(case, *(first_unit + second_unit)),
                                                                      permit_inner_space=True)
                                                for first_unit in weight_units + people_unit
                                                for second_unit in distance_units]) |
                                         cost(anyof([case_control.use_mark(lambda case: convert_unit_slash_unit(case, *(first_unit + second_unit)),
                                                                           permit_inner_space=True)
                                                    for first_unit in distance_units + weight_units + people_unit + memory_unit
                                                    for second_unit in time_units]), 0.001),
                                         permit_inner_space=True,
                                         need_outer_space=False).optimize()
