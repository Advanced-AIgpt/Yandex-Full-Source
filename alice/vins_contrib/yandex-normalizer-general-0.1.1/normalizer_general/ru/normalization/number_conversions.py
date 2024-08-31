#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('../..')

from general.normbase import *
from morphology import *

print 'Numbers: preparation'

# Choose case for "unit".
# This procedure does not convert numbers themselves, it only attaches
# gender features and inserts the unit term in the appropriate form.

## Handle modifier
def unit_with_case(unit, unit_gender, case, adjective=False, modifier=None, has_unit_already=False, has_modifier_already=False):
    """
    A pattern that matches a (perhaps composite) numeral + a unit name with an optional modifier
    in the right case and number.
    Adjective indicates whether the unit name has adjectival declination.
    Modifier, if present, is always assumed to be an adjective.
    """
    def insert_or_check_unit(in_feats):
        if has_unit_already:
            return word(check_form(unit, in_feats))
        else:
            return insert(" " + (unit + in_feats >> producer))

    def insert_or_check_modifier(in_feats):
        if has_modifier_already:
            if modifier:
                return word(check_form(modifier, in_feats))
            else:
                return word(pp(g.letter))
        else:
            return (empty() if not modifier else insert(" " + (modifier + in_feats >> producer)))

    number_feats = insert(feats('numeral', 'card', unit_gender, case))

    if adjective:
        number_for_234 = 'pl'
    else:
        number_for_234 = 'sg'
    if case == 'nom' or case == 'acc':
        r = (qq(anyof("+-")) +
             # двадцать один квадратный сантиметр
             ((qq(ss(g.digit) + (g.digit - "1")) +
               (("1" + number_feats +
                 insert_or_check_modifier(feats('adjective', unit_gender, case)) +
                 insert_or_check_unit(feats('noun', 'sg', case))) |
                # двадцать два квадратных сантиметра
                (anyof("234") + number_feats +
                 insert_or_check_modifier(feats('adjective', 'pl', 'gen')) +
                 insert_or_check_unit(feats('noun', number_for_234, 'gen'))) |
                # двадцать пять квадратных сантиметров
                (anyof("567890") + number_feats +
                 insert_or_check_modifier(feats('adjective', 'pl', 'gen')) +
                 insert_or_check_unit(feats('noun', 'pl', 'gen'))))) |
              # двенадцать квадратных сантиметров
              (qq(ss(g.digit)) + ("1" + g.digit) + number_feats +
               insert_or_check_modifier(feats('adjective', 'pl', 'gen')) +
               insert_or_check_unit(feats('noun', 'pl', 'gen'))) |
              # две целых одна десятая квадратного сантиметра
              (pp(g.digit) + anyof(".,") + pp(g.digit) + number_feats +
               insert_or_check_modifier(feats('adjective', 'sg', 'gen')) +
               insert_or_check_unit(feats('noun', 'sg', 'gen')))))
        return r.optimize()
    else:
        r = (qq(anyof("+-")) +
             # двадцати одному квадратному сантиметру
             ((qq(ss(g.digit) + (g.digit - "1")) + "1" + number_feats +
               insert_or_check_modifier(feats('adjective', 'sg', case)) +
               insert_or_check_unit(feats('noun', 'sg', case))) |
              # двадцати двум квадратным сантиметрам
              (qq(ss(g.digit)) + (g.digit - "1" |
                                  "11") + number_feats +
               insert_or_check_modifier(feats('adjective', 'pl', case)) +
               insert_or_check_unit(feats('noun', 'pl', case))) |
              # двум цедым одной десятой квадратного сантиметра
              (pp(g.digit) + anyof(".,") + pp(g.digit) + number_feats +
               insert_or_check_modifier(feats('adjective', 'sg', 'gen')) +
               insert_or_check_unit(feats('noun', 'sg', 'gen')))))
        return r.optimize()

def unit_after_number_abbr(unit, modifier=None):
    fst = ((empty() if not modifier else insert(" " + (modifier + feats('adjective', 'pl', 'gen') >> producer))) +
           insert(" " + (unit + feats('noun', 'pl', 'gen') >> producer)))
    return fst.optimize()

def make_conversion_with_ordinal_markers(file_with_markers):
    fst = failure()
    with open(file_with_markers) as f:
        for line in f:
            marker, gender, case = line.strip().split('\t')
            insert_feats = insert(feats("numeral", "ord", gender, case))
            fst |= insert_feats + (((" " | insert(" ")) + "-" + (" " | insert(" ")) + pp(g.digit) + insert_feats) | cost(empty(), 0.0001)) + (pp(" ") | insert(" ")) + marker
    fst = pp(g.digit) + fst
    return fst.optimize()

current_directory = os.path.dirname(os.path.abspath(__file__))
use_ordinal_markers = convert_words(make_conversion_with_ordinal_markers(os.path.join(current_directory, "ordinal_markers.txt")),
                                    permit_inner_space=True)

# Some preparation is needed to handle cases like "10кг" or "3-м"
# These endings are directly recognized by morphology.producer
valid_ord_ending = anyof(["й", "е", "я", "го", "му", "ю",
                          "м", "ом", "ые", "х", "ми",
                          "и", "ым", "им", "ое"])

prepare = ((ss(g.digit | anyof(".,")) + pp(g.digit) +
            # Remove as little as possible -- therefore cost(); limit the number of removed letters to 3.
            (("-" | insert("-")) + rr(cost(remove(g.letter), 0.001), 0, 3) + valid_ord_ending |
             cost(insert(" ") + pp(g.letter), 0.1))) |
           cost(ss(g.digit | anyof(".,")) + pp(g.digit) + (("-" + pp(g.letter)) | feats("numeral") | cost(insert(" ") + pp(g.word_sym_not_digit), 0.3)), 0.2))

prepare_cvt = convert_words(prepare, permit_inner_space=True)
prepare_ordinal = prepare_cvt.optimize()

# We might want to run numbers_case before or after converting + and - to words.
# Let us be ready for either possibility
plus_minus = (ss(" ") + "плюс" + ss(" ") |
              ss(" ") + "минус" + ss(" ") |
              "+" | "-")

# Case of a numeral may be given explicitly by a case control marker
numbers_case = anyof((remove("#" + case + pp(" ")) + qq(plus_minus) +
                      (g.digit + ss(g.digit | anyof(".,")) + (feats('numeral') |
                                                              insert(feats('numeral', 'card', case, 'mas'))) |
                       pp(g.digit) + "-" + pp(g.letter) + (feats('numeral', 'ord', case) |
                                                           insert("#" + case))))
                     for case in cat_values('case'))
numbers_case_control = convert_words(numbers_case, permit_inner_space=True)

print 'Cardinals'

card_0 = "0" + feats('numeral', 'card') >> producer
card_0 = card_0.optimize()

def card_1_9_of_feats(ff):
    r = anyof("123456789") + ff >> producer
    return r.optimize()

def card_10_19_of_feats(ff):
    r = "1" + g.digit + ff >> producer
    return r.optimize()

def card_20_99_of_feats(ff):
    x0 = anyof("23456789") + "0" + ff >> producer
    x1_9 = ((anyof("23456789") + insert("0" + ff) >> producer) +
            insert(" ") +
            (anyof("123456789") + ff >> producer))
    r = x0 | x1_9
    return r.optimize()

def card_2digits_of_feats(ff):
    r = (remove("0") + card_1_9_of_feats(ff) |
         card_10_19_of_feats(ff) |
         card_20_99_of_feats(ff))
    return r.optimize()

def card_100_999_of_feats(ff):
    r = ((anyof("123456789") + "00" + ff >> producer) |
         ((anyof("123456789") + insert("00" + ff) >> producer) +
          insert(" ") +
          card_2digits_of_feats(ff)))
    return r.optimize()

def card_3digits_of_feats(ff):
    r = (remove("00") + card_1_9_of_feats(ff) |
         remove("0") + card_2digits_of_feats(ff) |
         card_100_999_of_feats(ff))
    return r.optimize()

def card_1_999_of_feats(ff):
    r = (card_1_9_of_feats(ff) |
         card_10_19_of_feats(ff) |
         card_20_99_of_feats(ff) |
         card_100_999_of_feats(ff))
    return r.optimize()

card_1_999 = (anyof([card_1_999_of_feats(feats('numeral', 'card', case, gender))
                     for case in cat_values('case')
                     for gender in cat_values('gender')]))
card_1_999 = card_1_999.optimize()

def card_last_3_digits_of_feats(ff):
    r = (remove("00") + card_1_9_of_feats(ff) |
         remove("0") + (card_10_19_of_feats(ff) |
                        card_20_99_of_feats(ff)) |
         card_100_999_of_feats(ff))
    return r.optimize()

def higher_group(stem, stems_gender, case):
    if case in ['nom', 'acc']:
        x_ends_in_2_9_case = 'gen'
        x_ends_in_2_4_number = 'sg'
    else:
        x_ends_in_2_9_case = case
        x_ends_in_2_4_number = 'pl'
    r = (remove(ss("0")) +
         (replace(ss("0") + "1", stem + feats('noun', 'sg', case) >> producer) | 	# exactly 1
          cost(((qq(ss(g.digit) + anyof("023456789")) + "1" +				# ends in 1; penalize slightly so just 1 is preferred.
                 insert(feats('numeral', 'card', 'sg', case, stems_gender))) >>		# insert feats for card_1_999 to chew on
                (card_1_999 + insert(" " +
                                     (stem + feats('noun', 'sg', case) >> producer)))), 0.001) |
          ((qq(ss(g.digit) + anyof("023456789")) + anyof("234") +
            insert(feats('numeral', 'card', 'pl', case, stems_gender))) >>		# ends in [234]
           (card_1_999 + insert(" " +
                                (stem + feats('noun', x_ends_in_2_4_number, x_ends_in_2_9_case) >> producer)))) |
          (qq(ss(g.digit)) + ("1" + g.digit | anyof("567890")) +
           insert(feats('numeral', 'card', 'pl', case))) >> 				# 1x or ends in [567890]
          (card_1_999 + insert(" " +
                               (stem + feats('noun', 'pl', x_ends_in_2_9_case) >> producer)))))
    return r.optimize()

def thousands_group_of_case(case):
    return higher_group("тысяч", 'fem', case)

def millions_group_of_case(case):
    return higher_group("миллион", 'mas', case)

def billions_group_of_case(case):
    return higher_group("миллиард", 'mas', case)

def card_of_case_gender(case, gender):
    r = (
        ((rr(g.digit, 1, 3) >> billions_group_of_case(case)) +
         (rr(g.digit, 3) >> (remove("000") | insert(" ") + millions_group_of_case(case))) +
         (rr(g.digit, 3) >> (remove("000") | insert(" ") + thousands_group_of_case(case))) +
         (rr(g.digit, 3) + feats('numeral') >> (remove("000" + feats('numeral', case, gender)) |
                                                insert(" ") + card_last_3_digits_of_feats(feats('numeral', 'card', case, gender))))) |
        ((rr(g.digit, 1, 3) >> millions_group_of_case(case)) +
         (rr(g.digit, 3) >> (remove("000") | insert(" ") + thousands_group_of_case(case))) +
         (rr(g.digit, 3) + feats('numeral') >> (remove("000" + feats('numeral', case, gender)) |
                                                insert(" ") + card_last_3_digits_of_feats(feats('numeral', 'card', case, gender))))) |
         ((rr(g.digit, 1, 3) >> thousands_group_of_case(case)) +
          (rr(g.digit, 3) + feats('numeral') >> (remove("000" + feats('numeral', case, gender)) |
                                                 insert(" ") + card_last_3_digits_of_feats(feats('numeral', 'card', case, gender))))) |
         card_1_999_of_feats(feats('numeral', 'card', case, gender)) |
         # In normbase.py, 'нол' is treated as a name of a digit, not number, so here we
         # have to insert '@' -- the sign of a digit name.
         ("0" + replace(feats('numeral', 'card', case, gender),
                        '@' + feats('noun', 'sg', case)) >> producer))
    return r.optimize()

cardinal = anyof([card_of_case_gender(case, gender)
                  for case in cat_values('case')
                  for gender in cat_values('gender')])
cardinal = cardinal.optimize()

print 'Ordinals'

# Abbreviated ending may be too short; try to insert a vowel.
ord_tail = ("-" + (pp(g.letter) |
                   insert(anyof(u"оеыи")) + anyof(u"еймх")))
ord_feats = feats('numeral', 'ord')

def ord_1_9_of_feats(ff):
    r = (anyof("0123456789") + ord_tail + ff) >> producer
    return r.optimize()

def ord_10_99_of_feats(ff):
    pre_test = rr(g.digit, 2) + ord_tail + ff
    translate = (producer |
                 ((anyof("23456789") + insert("0") + insert(feats('numeral', 'card', 'nom')) >> producer) +
                  insert(" ") +
                  ord_1_9_of_feats(ff)))
    r = pre_test >> translate
    return r.optimize()

def ord_2digits_of_feats(ff):
    return (remove("0") + ord_1_9_of_feats(ff) |
            ord_10_99_of_feats(ff))

# exact number of hundreds
def ord_100_900_of_feats(ff):
    r = (("100" + ord_tail + ff >> producer) |
         ((anyof("23456789") + insert(feats('numeral', 'card', 'gen')) >> producer) +
          (insert("1") + "00" + ord_tail + ff >> producer)))
    return r.optimize()

def ord_100_999_of_feats(ff):
    pre_test = rr(g.digit, 3) + "-" + pp(g.letter) + ff
    translate = (ord_100_900_of_feats(ff) |
                 ((anyof("123456789") + insert("00") + insert(feats('numeral', 'card', 'nom')) >> producer) +
                  insert(" ") +
                  ord_2digits_of_feats(ff)))
    r = pre_test >> translate
    return r.optimize()

def ord_1_999_of_feats(ff):
    return (ord_1_9_of_feats(ff) |
            ord_10_99_of_feats(ff) |
            ord_100_999_of_feats(ff))

ord_1_999 = anyof([ord_1_999_of_feats(feats('numeral', 'ord', case, number, gender))
                  for case in cat_values('case')
                  for number in cat_values('number')
                  for gender in cat_values('gender')])

ord_1_999  = ord_1_999.optimize()

ord_ones_group = remove(ss("0")) + ord_1_999

def ord_big_of_feats(zeroes, ff):
    # тысячный
    just_one = remove(rr("0",0,2)) + "1" + zeroes + ord_tail + ff >> producer
    # стотысячный
    hundred = replace("100", "сто") + (insert("1") + zeroes + ord_tail + ff >> producer)
    # двадцатидвухтысячный, make it slightly costlier than сто to avoid parasitic variant
    ends_in_other = cost(((rr(g.digit, 0, 2) + anyof("023456789") |
                           qq(g.digit) + "11") + insert(feats('numeral', 'card', 'gen')) >> cardinal) +
                         ((insert("1") + zeroes + ord_tail + ff) >> producer), 0.01)
    # двадцатиоднотысячный
    ends_in_one = ((((qq(g.digit) + anyof("23456789") |
                      anyof("123456789") + "0") + insert("0" + feats('numeral', 'card', 'gen'))) >> cardinal) +
                   replace("1", "одно")  +
                   (insert("1") + zeroes + ord_tail + ff >> producer))
    r = just_one | hundred | ends_in_other | ends_in_one
    return r.optimize()

def ord_thousands_of_feats(ff):
    return ord_big_of_feats("000", ff)

ord_thousands = ord_thousands_of_feats(ord_feats)

def ord_millions_of_feats(ff):
    return ord_big_of_feats("000000", ff)

ord_millions = ord_millions_of_feats(ord_feats)

def ord_billions_of_feats(ff):
    return ord_big_of_feats("000000000", ff)

ord_billions = ord_billions_of_feats(ord_feats)

ord_small_end = (ord_1_999 |
                 ((pp(g.digit) + insert("000" + feats('numeral', 'card', 'nom')) >> cardinal) +
                  insert(" ") +
                  (rr(g.digit, 3) + ord_tail + ord_feats >> ord_ones_group)))
ord_small_end = ord_small_end.optimize()

ord_ending_in_thousands = (ord_thousands |
                           ((pp(g.digit) + insert("000000" + feats('numeral', 'card', 'nom')) >> cardinal) +
                            insert(" ") +
                            (rr(g.digit, 3) + "000" + ord_tail + ord_feats >> ord_thousands)))

ord_ending_in_millions = (ord_millions |
                          ((pp(g.digit) + insert("000000000" + feats('numeral', 'card', 'nom')) >> cardinal) +
                           insert(" ") +
                           (rr(g.digit, 3) + "000000" + ord_tail + ord_feats >> ord_millions)))

ord_ending_in_billions = ord_billions

ordinal = (ord_small_end |
           ord_ending_in_thousands |
           ord_ending_in_millions |
           ord_ending_in_billions)
ordinal = ordinal.optimize()

# This should probably share part of the code with ord_big_of_feats()
def make_number_compound():
    # сто-, тысяче-, миллионо-
    hundred = replace("100", "сто")
    thousand = replace("1000", "тысяче")
    million = replace("1000000", "миллионо")
    # двадцатидвух-, make it slightly costlier than сто to avoid parasitic variant
    ends_in_other = cost(((ss(g.digit) + anyof("023456789") |
                           ss(g.digit) + "11") + insert(feats('numeral', 'card', 'gen')) >> cardinal), 0.01)
    # двадцатиодно-
    ends_in_one = ((qq((ss(g.digit) + anyof("23456789") |
                        ss(g.digit) + anyof("123456789") + "0") + insert("0" + feats('numeral', 'card', 'gen'))) >> cardinal) +
                   replace("1", "одно"))
    # The tail should have at least 4 digits
    r = (hundred | thousand| million |
         ends_in_other | ends_in_one) + qq(remove("-")) + rr(g.letter_or_plus, 4) + ss(g.letter_or_plus) + qq(remove(anyof(["#" + case for case in cat_values("case")])))
    return r.optimize()

number_compound = make_number_compound()

# Allow feats to be specified by previous steps. If not, add them.
# Give a slight penalty to cases other than No
insert_features = (pp(g.digit) + (("-" + pp(g.letter) +
                                   (feats('numeral', 'ord') |
                                    # In numbers_case, we only specify case. Here, we are prepared to
                                    # compose with the code that is able to choose gender, so replace with proper feats()
                                    anyof(replace("#" + case, feats('numeral', 'ord', case))
                                          for case in cat_values('case')) |
                                    insert(feats('numeral', 'ord', 'nom')) |
                                    cost(insert(feats('numeral', 'ord')), 0.01) |
                                    # In some cases, wrong case is suggested (в + Loc where Acc is also possible).
                                    # Allow ourselves to override this, at a cost.
                                    cost(anyof(replace("#" + case, feats('numeral', 'ord'))
                                               for case in cat_values('case')),
                                         0.5))) |
                                  # Without explicit ending in the input,
                                  # explicit 'ord' feature should be present.
                                  (insert("-" + valid_ord_ending) + feats('numeral', 'ord'))))
insert_ordinal_feats = insert_features.optimize()

insert_cardinal_feats = (pp(g.digit | anyof(".,")) + (feats('numeral', 'card') |
                                                      insert(feats('numeral', 'card', 'nom', 'mas'))))

convert_ordinal = ((insert_ordinal_feats >> (ss(replace("0", "ноль ")) + ordinal)) |
                   number_compound)
convert_ordinal = convert_words(convert_ordinal).optimize()

prepare_cardinal = pp((pp(g.digit) + qq(anyof(".,") + pp(g.digit)) + (feats("numeral") |
                                                                      insert(" ") + cost(g.any_symb, 0.1))) |
                      cost(g.any_symb, 0.1))

convert_cardinal = insert_cardinal_feats >> (ss(replace("0", "ноль ")) + cardinal)
convert_cardinal = convert_words(convert_cardinal).optimize()

print "Fractions"

# Any less ugly way to express this?

spell_digit = (replace("0", " ноль") |
               insert(" ") + ((g.digit - "0") + insert(feats('numeral', 'card', 'nom', 'mas')) >> producer))

skip_zeros = (ss(remove("0")) + (g.digit - "0") + ss(g.any_symb) |
              # Retain the last zero
              ss(remove("0")) + "0")

# By the time fraction converter is run, dots and commas will be surrounded by spaces, so take that into account.
def integral_part(case):
    # word(pp(g.letter)) takes care of the "целая" word
    r = unit_with_case("цел", 'fem', case, adjective=True)
    return r.optimize()

def fractional_part(case):
    r = ((g.digit >> unit_with_case("десят", 'fem', case, adjective=True)) |
         (rr(g.digit, 2) >> skip_zeros >> unit_with_case("сот", 'fem', case, adjective=True)) |
         (rr(g.digit, 3) >> skip_zeros >> unit_with_case("тысячн", 'fem', case, adjective=True)) |
         (rr(g.digit, 4) >> skip_zeros >> unit_with_case("десятитысячн", 'fem', case, adjective=True)) |
         (rr(g.digit, 5) >> skip_zeros >> unit_with_case("стотысячн", 'fem', case, adjective=True)) |
         (rr(g.digit, 6) >> skip_zeros >> unit_with_case("миллионн", 'fem', case, adjective=True)) |
         (insert("дробная часть ") +
          (rr(g.digit, 7) + ss(g.digit) >> pp(spell_digit)) +
          insert(" конец дробной части")))
    return r.optimize()

fraction = (insert_cardinal_feats >> anyof([(integral_part(case) +
                                             replace(anyof(".,"), " ") +
                                             fractional_part(case) +
                                             remove(feats('numeral', 'card', case)))
                                            for case in cat_values('case')]))
fraction = fraction.optimize()

convert_fraction = convert_words(fraction)
