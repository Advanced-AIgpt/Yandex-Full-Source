#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')

from general.normbase import *
import categories

# Cardinal numerals
print 'Morphology'

# For ноль, один, два, три, четыре, it is easiest to just specify the transformers directly.
# card_0_tagger = direct_tagger((replace("ноль", "0"), feats('numeral', 'card', 'nom', 'acc')),
#                               (replace("ноля", "0"), feats('numeral', 'card', 'gen')),
#                               (replace("нолю", "0"), feats('numeral', 'card', 'dat')),
#                               (replace("нолем", "0"), feats('numeral', 'card', 'instr')),
#                               (replace("ноле", "0"), feats('numeral', 'card', 'loc')))
card_1_tagger = direct_tagger((replace_stressed("од+ин", "1"), feats('numeral', 'card', 'mas', 'nom', 'acc')),
                              (replace_stressed("одн+о", "1"), feats('numeral', 'card', 'neu', 'nom', 'acc')),
                              (replace_stressed("одн+а", "1"), feats('numeral', 'card', 'fem', 'nom')),
                              (replace_stressed("одног+о", "1"), feats('numeral', 'card', 'mas', 'neu', 'gen')),
                              (replace_stressed("одн+ой", "1"), feats('numeral', 'card', 'fem', 'gen', 'dat', 'instr', 'loc')),
                              (replace_stressed("одном+у", "1"), feats('numeral', 'card',  'mas', 'neu', 'dat')),
                              (replace_stressed("одн+у", "1"), feats('numeral', 'card', 'fem', 'acc')),
                              (replace_stressed("одн+им", "1"), feats('numeral', 'card', 'mas', 'neu', 'instr')),
                              (replace_stressed("одн+ом", "1"), feats('numeral', 'card', 'mas', 'neu', 'loc')))
card_2_tagger = direct_tagger((replace_stressed("дв+а", "2"), feats('numeral', 'card', 'mas', 'neu', 'nom', 'acc')),
                              (replace_stressed("дв+е", "2"), feats('numeral', 'card', 'fem', 'nom', 'acc')),
                              (replace_stressed("дв+ух", "2"), feats('numeral', 'card', 'gen', 'loc')),
                              (replace_stressed("дв+ум", "2"), feats('numeral', 'card', 'dat')),
                              (replace_stressed("двум+я", "2"), feats('numeral', 'card', 'instr')))
card_3_tagger = direct_tagger((replace_stressed("тр+и", "3"), feats('numeral', 'card', 'nom', 'acc')),
                              (replace_stressed("тр+ёх", "3"), feats('numeral', 'card', 'gen', 'loc')),
                              (replace_stressed("тр+ём", "3"), feats('numeral', 'card', 'dat')),
                              (replace_stressed("трем+я", "3"), feats('numeral', 'card', 'instr')))
card_4_tagger = direct_tagger((replace_stressed("чет+ыре", "4"), feats('numeral', 'card', 'nom', 'acc')),
                              (replace_stressed("четыр+ёх", "4"), feats('numeral', 'card', 'gen', 'loc')),
                              (replace_stressed("четыр+ём", "4"), feats('numeral', 'card', 'dat')),
                              (replace_stressed("четырьм+я", "4"), feats('numeral', 'card', 'instr')))

# восемь is like all grater numbers, but with a fluent vowel, so it has to be taken care of separately
card_8_tagger = direct_tagger((replace_stressed("в+осемь", "8"), feats('numeral', 'card', 'nom', 'acc')),
                              (replace_stressed("восьм+и", "8"), feats('numeral', 'card', 'gen', 'dat', 'loc')),
                              (replace_stressed("восемь+ю", "8"), feats('numeral', 'card', 'instr')))

paradigm_5on = [("ь", feats('numeral', 'card', 'nom', 'acc')),
                ("и", feats('numeral', 'card', 'gen', 'dat', 'loc')),
                ("ью", feats('numeral', 'card', 'instr'))]
stems_5on = [replace("пят", "5"),
             replace("шест", "6"),
             replace("сем", "7"),
             replace("девят", "9"),
             replace("десят", "10"),
             replace("одиннадцат", "11"),
             replace("двенадцат", "12"),
             replace("тринадцат", "13"),
             replace("четырнадцат", "14"),
             replace("пятнадцат", "15"),
             replace("шестнадцат", "16"),
             replace("семнадцат", "17"),
             replace("восемнадцат", "18"),
             replace("девятнадцат", "19"),
             replace("двадцат", "20"),
             replace("тридцат", "30")]
card_5on_tagger = tagger(stems_5on, paradigm_5on)

card_40_tagger = direct_tagger((replace_stressed("с+орок", "40"), feats('numeral', 'card', 'nom', 'acc')),
                               (replace_stressed("сорок+а", "40"), feats('numeral', 'card', 'gen', 'dat', 'instr', 'loc')))

# 50-80 are easiest to describe in this somewhat crazy fashion
paradigm_50on = [("ьдесят", feats('numeral', 'card', 'nom', 'acc')),
                 ("идесяти", feats('numeral', 'card', 'gen', 'dat', 'loc')),
                 ("ьюдесятью", feats('numeral', 'card', 'instr'))]
stems_50on = [replace("пят", "50"),
              replace("шест", "60"),
              replace("сем", "70")]
card_50on_tagger = tagger(stems_50on, paradigm_50on)

card_80_tagger =  direct_tagger((replace_stressed("в+осемьдесят", "80"), feats('numeral', 'card', 'nom', 'acc')),
                                (replace_stressed("восьм+идесяти", "80"), feats('numeral', 'card', 'gen', 'dat', 'loc')),
                                (replace_stressed("восемь+юдесятью", "80"), feats('numeral', 'card', 'instr')))

paradigm_90_100 = [("о", feats('numeral', 'card', 'nom', 'acc')),
                   ("а", feats('numeral', 'card', 'gen', 'dat', 'instr', 'loc'))]
stems_90_100 = [replace("девяност", "90"),
                replace("ст", "100")]
card_90_100_tagger = tagger(stems_90_100, paradigm_90_100)

# двести, триста, четыреста -- each unique
card_200_tagger = direct_tagger((replace_stressed("дв+ести", "200"), feats('numeral', 'card', 'nom', 'acc')),
                                (replace_stressed("двухс+от", "200"), feats('numeral', 'card', 'gen')),
                                (cost(replace_stressed("двухст+а", "200"), 0.01), feats('numeral', 'card', 'gen', 'loc')), # non normative
                                (replace_stressed("двумст+ам", "200"), feats('numeral', 'card', 'dat')),
                                (cost(replace_stressed("двумст+а", "200"), 0.01), feats('numeral', 'card', 'dat')), # non normative
                                (replace_stressed("двумяст+ами", "200"), feats('numeral', 'card', 'instr')),
                                (cost(replace_stressed("двумяст+а", "200"), 0.01), feats('numeral', 'card', 'instr')), # non normative
                                (replace_stressed("двухст+ах", "200"), feats('numeral', 'card', 'loc')))
card_300_tagger = direct_tagger((replace_stressed("тр+иста", "300"), feats('numeral', 'card', 'nom', 'acc')),
                                (replace_stressed("трёхс+от", "300"), feats('numeral', 'card', 'gen')),
                                (cost(replace_stressed("трёхст+а", "300"), 0.01), feats('numeral', 'card', 'gen', 'loc')), # non normative
                                (replace_stressed("трёмст+ам", "300"), feats('numeral', 'card', 'dat')),
                                (cost(replace_stressed("трёмст+а", "300"), 0.01), feats('numeral', 'card', 'dat')), # non normative
                                (replace_stressed("тремяст+ами", "300"), feats('numeral', 'card', 'instr')),
                                (cost(replace_stressed("тремяст+а", "300"), 0.01), feats('numeral', 'card', 'instr')), # non normative
                                (replace_stressed("трёхст+ах", "300"), feats('numeral', 'card', 'loc')))
card_400_tagger = direct_tagger((replace_stressed("чет+ыреста", "400"), feats('numeral', 'card', 'nom', 'acc')),
                                (replace_stressed("четырёхс+от", "400"), feats('numeral', 'card', 'gen')),
                                (cost(replace_stressed("четырёхст+а", "400"), 0.01), feats('numeral', 'card', 'gen', 'loc')), # non normative
                                (replace_stressed("четырёмст+ам", "400"), feats('numeral', 'card', 'dat')),
                                (cost(replace_stressed("четырёмст+а", "400"), 0.01), feats('numeral', 'card', 'dat')), # non normative
                                (replace_stressed("четырьмяст+ами", "400"), feats('numeral', 'card', 'instr')),
                                (cost(replace_stressed("четырьмяст+а", "400"), 0.01), feats('numeral', 'card', 'instr')), # non normative
                                (replace_stressed("четырёхст+ах", "400"), feats('numeral', 'card', 'loc')))

# 500-900, same way as 50-80
paradigm_500on = [("ьс+от", feats('numeral', 'card', 'nom', 'acc')),
                  ("ис+от", feats('numeral', 'card', 'gen')),
                  (cost(maybe_stressed("ист+а"), 0.0001), feats('numeral', 'card', 'dat', 'loc')), # non normative, but used
                  (cost(maybe_stressed("ист+а"), 0.0001), feats('numeral', 'card', 'gen', 'instr')), # non normative, but used
                  ("ист+ам", feats('numeral', 'card', 'dat')),
                  ("ьюст+ами", feats('numeral', 'card', 'instr')),
                  ("ист+ах", feats('numeral', 'card', 'loc'))]
stems_500on = [replace("пят", "500"),
               replace("шест", "600"),
               replace("сем", "700"),
               replace("девят", "900")]
card_500on_tagger = tagger(stems_500on, paradigm_500on)

card_800_tagger =  direct_tagger((replace_stressed("восемьс+от", "800"), feats('numeral', 'card', 'nom', 'acc')),
                                 (replace_stressed("восьмис+от", "800"), feats('numeral', 'card', 'gen')),
                                 (cost(replace_stressed("восьмист+а", "800"), 0.0001), feats('numeral', 'card', 'gen', 'dat', 'loc')), # non normative, but used
                                 (replace_stressed("восьмист+ам", "800"), feats('numeral', 'card', 'dat')),
                                 (replace_stressed("восемьюст+ами", "800"), feats('numeral', 'card', 'instr')),
                                 (replace_stressed("восьмист+ах", "800"), feats('numeral', 'card', 'loc')))

card_tagger = (card_1_tagger | card_2_tagger | card_3_tagger | card_4_tagger | card_8_tagger |
               card_5on_tagger | card_40_tagger | card_50on_tagger | card_80_tagger | card_90_100_tagger |
               card_200_tagger | card_300_tagger | card_400_tagger | card_500on_tagger | card_800_tagger)

# Ordinals
ord_unstressed_hard_paradigm = [("ый", feats('numeral', 'ord', 'mas', 'nom', 'acc')),
                                (replace("ое", qq("о") + "е"), feats('numeral', 'ord', 'neu', 'nom', 'acc')),
                                (replace("ая", "я"), feats('numeral', 'ord', 'fem', 'nom')),
                                (replace("ого", "го"), feats('numeral', 'ord', 'mas', 'neu', 'gen')),
                                ("ой", feats('numeral', 'ord', 'fem', 'gen', 'dat', 'instr', 'loc')),
                                (replace("ому", "му"), feats('numeral', 'ord', 'mas', 'neu', 'dat')),
                                (replace("ую", "ю"), feats('numeral', 'ord', 'fem', 'acc')),
                                ("ым", feats('numeral', 'ord', 'mas', 'neu', 'instr') | feats('numeral', 'ord', 'pl', 'dat')),
                                ("ом", feats('numeral', 'ord', 'mas', 'neu', 'loc')),
                                ("ые", feats('numeral', 'ord', 'pl', 'nom', 'acc')),
                                (replace("ых", "х"), feats('numeral', 'ord', 'pl', 'gen', 'loc')),
                                (replace("ыми", "ми"), feats('numeral', 'ord', 'pl', 'instr'))]

ord_unstressed_hard_stems = [replace_stressed("п+ерв", "1"),
                             replace_stressed("четв+ёрт", "4"),
                             replace_stressed("п+ят", "5"),
                             replace_stressed("дев+ят", "9"),
                             replace_stressed("дес+ят", "10"),
                             replace_stressed("од+иннадцат", "11"),
                             replace_stressed("двен+адцат", "12"),
                             replace_stressed("трин+адцат", "13"),
                             replace_stressed("чет+ырнадцат", "14"),
                             replace_stressed("пятн+адцат", "15"),
                             replace_stressed("шестн+адцат", "16"),
                             replace_stressed("семн+адцат", "17"),
                             replace_stressed("восемн+адцат", "18"),
                             replace_stressed("девятн+адцат", "19"),
                             replace_stressed("двадц+ат", "20"),
                             replace_stressed("тридц+ат", "30"),
                             replace_stressed("пятидес+ят", "50"),
                             replace_stressed("шестидес+ят", "60"),
                             replace_stressed("семидес+ят", "70"),
                             replace_stressed("восьмидес+ят", "80"),
                             replace_stressed("девян+ост", "90"),
                             replace_stressed("с+от", "100"),
                             replace_stressed("т+ысячн", "1000"),
                             replace_stressed("милли+онн", "1000000"),
                             replace_stressed("милли+ардн", "1000000000")]

ord_unstressed_hard_tagger = (tagger(ord_unstressed_hard_stems, ord_unstressed_hard_paradigm) |
                              cost(tagger(ord_unstressed_hard_stems, ord_unstressed_hard_paradigm, drop_flection=False), 0.1))

ord_unstressed_soft_paradigm = [(replace("ий", "й"), feats('numeral', 'ord', 'mas', 'nom', 'acc')),
                                (replace("ье", "е"), feats('numeral', 'ord', 'neu', 'nom', 'acc')),
                                (replace("ья", "я"), feats('numeral', 'ord', 'fem', 'nom')),
                                (replace("ьего", "го"), feats('numeral', 'ord', 'mas', 'neu', 'gen')),
                                (replace("ьей", "й"), feats('numeral', 'ord', 'fem', 'gen', 'dat', 'instr', 'loc')),
                                (replace("ьему", "му"), feats('numeral', 'ord', 'mas', 'neu', 'dat')),
                                (replace("ью", "ю"), feats('numeral', 'ord', 'fem', 'acc')),
                                (replace("ьим", "им"), feats('numeral', 'ord', 'mas', 'neu', 'instr') | feats('numeral', 'ord', 'pl', 'dat')),
                                (replace("ьем", "ем"), feats('numeral', 'ord', 'mas', 'neu', 'loc')),
                                (replace("ьи", "и"), feats('numeral', 'ord', 'pl', 'nom', 'acc')),
                                (replace("ьих", "х"), feats('numeral', 'ord', 'pl', 'gen', 'loc')),
                                (replace("ьими", "ми"), feats('numeral', 'ord', 'pl', 'instr'))]
ord_unstressed_soft_stems = [replace_stressed("тр+ет", "3")]
ord_unstressed_soft_tagger = (tagger(ord_unstressed_soft_stems, ord_unstressed_soft_paradigm) |
                              cost(tagger(ord_unstressed_soft_stems, ord_unstressed_soft_paradigm, drop_flection=False), 0.1))

ord_stressed_paradigm = [(replace_stressed("+ой", "й"), feats('numeral', 'ord', 'mas', 'nom', 'acc')),
                         (replace_stressed("+ое", qq("о") + "е"), feats('numeral', 'ord', 'neu', 'nom', 'acc')),
                         (replace_stressed("+ая", "я"), feats('numeral', 'ord', 'fem', 'nom')),
                         (replace_stressed("+ого", "го"), feats('numeral', 'ord', 'mas', 'neu', 'gen')),
                         (replace_stressed("+ой", "й"), feats('numeral', 'ord', 'fem', 'gen', 'dat', 'instr', 'loc')),
                         (replace_stressed("+ому", "му"), feats('numeral', 'ord', 'mas', 'neu', 'dat')),
                         (replace_stressed("+ую", "ю"), feats('numeral', 'ord', 'fem', 'acc')),
                         (maybe_stressed("+ым"), feats('numeral', 'ord', 'mas', 'neu', 'instr') | feats('numeral', 'ord', 'pl', 'dat')),
                         (maybe_stressed("+ом"), feats('numeral', 'ord', 'mas', 'neu', 'loc')),
                         (maybe_stressed("+ые"), feats('numeral', 'ord', 'pl', 'nom', 'acc')),
                         (replace_stressed("+ых", "х"), feats('numeral', 'ord', 'pl', 'gen', 'loc')),
                         (replace_stressed("+ыми", "ми"), feats('numeral', 'ord', 'pl', 'instr'))]
ord_stressed_stems = [replace("втор", "2"),
                      replace("шест", "6"),
                      replace("седьм", "7"),
                      replace("восьм", "8"),
                      replace("сороков", "40")]
ord_stressed_tagger = (tagger(ord_stressed_stems, ord_stressed_paradigm) |
                       cost(tagger(ord_stressed_stems, ord_stressed_paradigm, drop_flection=False), 0.1))

ord_tagger = ord_unstressed_hard_tagger | ord_unstressed_soft_tagger | ord_stressed_tagger

# тысяча and миллион are standard nouns, but we don't have a general noun declination analyzer,
# so we treat them as a special case.
# Some other nouns are also needed for normalization.
noun_zt4_paradigm = [("а", feats('noun', 'sg', 'nom')),
                     ("и", feats('noun', 'sg', 'gen') | feats('noun', 'pl', 'nom', 'acc')),
                     ("е", feats('noun', 'sg', 'dat', 'loc')),
                     ("у", feats('noun', 'sg', 'acc')),
                     ("ью", feats('noun', 'sg', 'instr')),
                     ("ей", feats('noun', 'sg', 'instr')),
                     ("", feats('noun', 'pl', 'gen')),
                     ("ам", feats('noun', 'pl', 'dat')),
                     ("ами", feats('noun', 'pl', 'instr')),
                     ("ах", feats('noun', 'pl', 'loc'))]
noun_zt4_stems = ["т+ысяч"]
noun_zt4_tagger = tagger(noun_zt4_stems, noun_zt4_paradigm)

noun_zt1m_paradigm = [("", feats('noun', 'sg', 'nom', 'acc')),
                      ("а", feats('noun', 'sg', 'gen')),
                      ("у", feats('noun', 'sg', 'dat')),
                      ("ом", feats('noun', 'sg', 'instr')),
                      ("е", feats('noun', 'sg', 'loc')),
                      ("ы", feats('noun', 'pl', 'nom', 'acc')),
                      ("ов", feats('noun', 'pl', 'gen')),
                      ("ам", feats('noun', 'pl', 'dat')),
                      ("ами", feats('noun', 'pl', 'instr')),
                      ("ах", feats('noun', 'pl', 'loc'))]
noun_zt1m_stems = ["милли+он", "милли+ард", "трилли+он",
                   "проц+ент", "гр+адус",
                   "килом+етр", "м+етр", "сантим+етр", "миллим+етр",
                   "килогр+амм", "гр+амм",
                   "д+оллар", "ц+ент",
                   "час",
                   "соз+ыв",
                   "съ+езд", "гект+ар",
                   "квадр+ат", "к+уб",
                   "район", "бульв+ар", "просп+ект",
                   "про+езд", "эт+аж", "+офис",
                  ]
noun_zt1m_tagger = tagger(noun_zt1m_stems, noun_zt1m_paradigm)

noun_zt1am_paradigm = [("", feats('noun', 'sg', 'nom', 'acc')),
                       ("а", feats('noun', 'sg', 'gen')),
                       ("у", feats('noun', 'sg', 'dat')),
                       ("ом", feats('noun', 'sg', 'instr')),
                       ("е", feats('noun', 'sg', 'loc')),
                       ("ы", feats('noun', 'pl', 'nom', 'acc')),
                       ("", feats('noun', 'pl', 'gen')),
                       ("ам", feats('noun', 'pl', 'dat')),
                       ("ами", feats('noun', 'pl', 'instr')),
                       ("ах", feats('noun', 'pl', 'loc'))]

memory_units = ["б+айт", "б+ит"]
unit_prefixes = ["", "кило", "мега", "гига", "тера", "пета"]
memory_size = [prefix + unit for unit in memory_units for prefix in unit_prefixes]
noun_zt1am_stems = memory_size
noun_zt1am_tagger = tagger(noun_zt1am_stems, noun_zt1am_paradigm)

noun_zt1f_paradigm = [("а", feats('noun', 'sg', 'nom')),
                      ("ы", feats('noun', 'sg', 'gen') | feats('noun', 'pl', 'nom', 'acc')),
                      ("е", feats('noun', 'sg', 'dat', 'loc')),
                      ("у", feats('noun', 'sg', 'acc')),
                      ("ой", feats('noun', 'sg', 'instr')),
                      ("", feats('noun', 'pl', 'gen')),
                      ("ам", feats('noun', 'pl', 'dat')),
                      ("ами", feats('noun', 'pl', 'instr')),
                      ("ах", feats('noun', 'pl', 'loc'))]
noun_zt1f_stems = ["мин+ут",
                   "сек+унд",
                   "гр+упп",
                   "олимпи+ад",
                   "кварт+ир",
                   "сред", "пятниц",
                   "суббот", "игр", "тонн",
                   "гр+ивн", "кр+он", "л+ир", "и+ен", "й+ен"]

noun_zt1f_tagger = tagger(noun_zt1f_stems, noun_zt1f_paradigm)

noun_zt1n_paradigm = [("о", feats('noun', 'sg', 'nom', "acc")),
                      ("а", feats('noun', 'sg', 'gen') | feats('noun', 'pl', 'nom', 'acc')),
                      ("у", feats('noun', 'sg', 'dat')),
                      ("е", feats('noun', 'sg', 'loc')),
                      ("ом", feats('noun', 'sg', 'instr')),
                      ("", feats('noun', 'pl', 'gen')),
                      ("ам", feats('noun', 'pl', 'dat')),
                      ("ами", feats('noun', 'pl', 'instr')),
                      ("ах", feats('noun', 'pl', 'loc'))]
noun_zt1n_stems = ["сел"]
noun_zt1n_tagger = tagger(noun_zt1n_stems, noun_zt1n_paradigm)

# Digit names are ordinary nouns
noun_zt2_paradigm = [("ь", feats('noun', 'sg', 'nom', 'acc')),
                     ("я", feats('noun', 'sg', 'gen')),
                     ("ю", feats('noun', 'sg', 'dat')),
                     ("ем", feats('noun', 'sg', 'instr')),
                     ("е", feats('noun', 'sg', 'loc')),
                     ("и", feats('noun', 'pl', 'nom', 'acc')),
                     ("ей", feats('noun', 'pl', 'gen')),
                     ("ям", feats('noun', 'pl', 'dat')),
                     ("ями", feats('noun', 'pl', 'instr')),
                     ("ях", feats('noun', 'pl', 'loc'))]
noun_zt2_stems = [replace("нол", "0@"),
                  replace("нул", "0@"),
                  "рубл", "фестив+ал"]
noun_zt2_tagger = tagger(noun_zt2_stems, noun_zt2_paradigm)

noun_zt2ef_paradigm = [("я", feats('noun', 'sg', 'nom')),
                       ("и", feats('noun', 'sg', 'gen')),
                       ("е", feats('noun', 'sg', 'dat')),
                       ("ю", feats('noun', 'sg', 'acc')),
                       ("е" + (Fst.convert("й") | "ю") , feats('noun', 'sg', 'instr')),
                       ("е", feats('noun', 'sg', 'loc')),
                       ("и", feats('noun', 'pl', 'nom', 'acc')),
                       ("ей", feats('noun', 'pl', 'gen')),
                       ("ям", feats('noun', 'pl', 'dat')),
                       ("ями", feats('noun', 'pl', 'instr')),
                       ("ях", feats('noun', 'pl', 'loc'))]
noun_zt2ef_stems = ["дер+евн", "м+ил", "+унци"]
noun_zt2ef_tagger = tagger(noun_zt2ef_stems, noun_zt2ef_paradigm)

noun_zt5_paradigm = [("а", feats('noun', 'sg', 'nom')),
                     ("ы", feats('noun', 'sg', 'gen') | feats('noun', 'pl', 'nom', 'acc')),
                     ("е", feats('noun', 'sg', 'dat', 'loc')),
                     ("у", feats('noun', 'sg', 'acc')),
                     ("ей", feats('noun', 'sg', 'instr')),
                     ("", feats('noun', 'pl', 'gen')),
                     ("ам", feats('noun', 'pl', 'dat')),
                     ("ами", feats('noun', 'pl', 'instr')),
                     ("ах", feats('noun', 'pl', 'loc'))]
noun_zt5_stems = [replace_stressed("един+иц", "1@"), "+улиц"]
noun_zt5_tagger = tagger(noun_zt5_stems, noun_zt5_paradigm)

noun_zt3a_m_paradigm = [("ок", feats('noun', 'sg', 'nom', 'acc')),
                        ("ка", feats('noun', 'sg', 'gen')),
                        ("ку", feats('noun', 'sg', 'dat')),
                        ("ком", feats('noun', 'sg', 'instr')),
                        ("ке", feats('noun', 'sg', 'loc')),
                        ("ки", feats('noun', 'pl', 'nom', "acc")),
                        ("ков", feats('noun', 'pl', 'gen')),
                        ("кам", feats('noun', 'pl', 'dat')),
                        ("ками", feats('noun', 'pl', 'instr')),
                        ("ках", feats('noun', 'pl', 'loc'))]

noun_zt3a_m_stems = ["пос+ёл", "пере+ул"]
noun_zt3a_m_tagger = tagger(noun_zt3a_m_stems, noun_zt3a_m_paradigm)

noun_zt3a_gpl0_paradigm = [("", feats('noun', 'sg', 'nom', 'acc')),
                           ("а", feats('noun', 'sg', 'gen')),
                           ("у", feats('noun', 'sg', 'dat')),
                           ("ом", feats('noun', 'sg', 'instr')),
                           ("е", feats('noun', 'sg', 'loc')),
                           ("и", feats('noun', 'pl', 'nom')),
                           ("", feats('noun', 'pl', 'gen', 'acc')),
                           ("ам", feats('noun', 'pl', 'dat')),
                           ("ами", feats('noun', 'pl', 'instr')),
                           ("ах", feats('noun', 'pl', 'loc'))]
# Used only for counting, so "люди" form is not taken into account
noun_zt3a_gpl0_stems = ["челов+ек"]
noun_zt3a_gpl0_tagger = tagger(noun_zt3a_gpl0_stems, noun_zt3a_gpl0_paradigm)

noun_zt3am_paradigm = [("", feats('noun', 'sg', 'nom', 'acc')),
                       ("а", feats('noun', 'sg', 'gen')),
                       ("у", feats('noun', 'sg', 'dat')),
                       ("ом", feats('noun', 'sg', 'instr')),
                       ("е", feats('noun', 'sg', 'loc')),
                       ("и", feats('noun', 'pl', 'nom', 'acc')),
                       ("ов", feats('noun', 'pl', 'gen')),
                       ("ам", feats('noun', 'pl', 'dat')),
                       ("ами", feats('noun', 'pl', 'instr')),
                       ("ах", feats('noun', 'pl', 'loc'))]

noun_zt3am_stems = ["понед+ельник", "вт+орник",
                    "четверг"]
noun_zt3am_tagger = tagger(noun_zt3am_stems, noun_zt3am_paradigm)

noun_zt3c_paradigm =  [("", feats('noun', 'sg', 'nom', 'acc')),
                       ("а", feats('noun', 'sg', 'gen')),
                       ("у", feats('noun', 'sg', 'dat')),
                       ("ом", feats('noun', 'sg', 'instr')),
                       ("е", feats('noun', 'sg', 'loc')),
                       ("a", feats('noun', 'pl', 'nom', 'acc')),
                       ("ов", feats('noun', 'pl', 'gen')),
                       ("ам", feats('noun', 'pl', 'dat')),
                       ("ами", feats('noun', 'pl', 'instr')),
                       ("ах", feats('noun', 'pl', 'loc'))]
noun_zt3c_stems = ["век", "счёт", "счет",
                   "город", "дом", "корпус",
                  ]
noun_zt3c_tagger = tagger(noun_zt3c_stems, noun_zt3c_paradigm)

noun_zt3ss_paradigm = [("йка", feats('noun', 'sg', 'nom')),
                       ("йки", feats('noun', 'sg', 'gen') | feats('noun', 'pl', 'nom', 'acc')),
                       ("йке", feats('noun', 'sg', 'dat', 'loc')),
                       ("йку", feats('noun', 'sg', 'acc')),
                       ("йкой", feats('noun', 'sg', 'instr')),
                       ("ек", feats('noun', 'pl', 'gen')),
                       ("йкам", feats('noun', 'pl', 'dat')),
                       ("йками", feats('noun', 'pl', 'instr')),
                       ("йках", feats('noun', 'pl', 'loc'))]
noun_zt3ss_stems = [replace_stressed("дв+о", "2@"),
                    replace_stressed("тр+о", "3@"),
                    "коп+е"]
noun_zt3ss_tagger = tagger(noun_zt3ss_stems, noun_zt3ss_paradigm)

noun_zt3sh_paradigm =  [("ка", feats('noun', 'sg', 'nom')),
                        ("ки", feats('noun', 'sg', 'gen') | feats('noun', 'pl', 'nom', 'acc')),
                        ("ке", feats('noun', 'sg', 'dat', 'loc')),
                        ("ку", feats('noun', 'sg', 'acc')),
                        ("кой", feats('noun', 'sg', 'instr')),
                        ("ок", feats('noun', 'pl', 'gen')),
                        ("кам", feats('noun', 'pl', 'dat')),
                        ("ками", feats('noun', 'pl', 'instr')),
                        ("ках", feats('noun', 'pl', 'loc'))]
noun_zt3sh_stems = [replace_stressed("четв+ёр", "4@"),
                    replace_stressed("пят+ёр", "5@"),
                    replace_stressed("шест+ёр", "6@"),
                    replace_stressed("сем+ёр", "7@"),
                    replace_stressed("восьм+ёр", "8@"),
                    replace_stressed("дев+ят", "9@")]
noun_zt3sh_tagger = tagger(noun_zt3sh_stems, noun_zt3sh_paradigm)

noun_zt6c_paradigm = [("ье", feats('noun', 'sg', 'nom', 'acc')),
                      ("ья", feats('noun', 'sg', 'gen') | feats('noun', 'pl', 'nom', 'acc')),
                      ("ью", feats('noun', 'sg', 'dat')),
                      ("ьем", feats('noun', 'sg', 'instr')),
                      ("ьи", feats('noun', 'sg', 'loc')),
                      ("ий", feats('noun', 'pl', 'gen')),
                      ("ьям", feats('noun', 'pl', 'dat')),
                      ("ьями", feats('noun', 'pl', 'instr')),
                      ("ьях", feats('noun', 'pl', 'loc'))]
noun_zt6c_stems = ["воскресен"]
noun_zt6c_tagger = tagger(noun_zt6c_stems, noun_zt6c_paradigm)

noun_zt7n_paradigm = [("е", feats('noun', 'sg', 'nom', 'acc')),
                      ("я", feats('noun', 'sg', 'gen') | feats('noun', 'pl', 'nom', 'acc')),
                      ("ю", feats('noun', 'sg', 'dat')),
                      ("ем", feats('noun', 'sg', 'instr')),
                      ("и", feats('noun', 'sg', 'loc')),
                      ("й", feats('noun', 'pl', 'gen')),
                      ("ям", feats('noun', 'pl', 'dat')),
                      ("ями", feats('noun', 'pl', 'instr')),
                      ("ях", feats('noun', 'pl', 'loc'))]
noun_zt7n_stems = ["стол+ети",
                   "тысячел+ети",
                   "стро+ени",
                   ]
noun_zt7n_tagger = tagger(noun_zt7n_stems, noun_zt7n_paradigm)

noun_zt8e_paradigm = [("ь", feats('noun', 'sg', 'nom', 'acc')),
                      ("и", feats('noun', 'sg', 'gen', 'dat', 'loc') | feats('noun', 'pl', 'nom', 'acc')),
                      ("ью", feats('noun', 'sg', 'instr')),
                      ("ей", feats('noun', 'pl', 'gen')),
                      ("ям", feats('noun', 'pl', 'dat')),
                      ("ями", feats('noun', 'pl', 'instr')),
                      ("ях", feats('noun', 'pl', 'loc'))]
noun_zt8e_stems = ["степен", "област"]
noun_zt8e_tagger = tagger(noun_zt8e_stems, noun_zt8e_paradigm)

noun_adjfem_paradigm = [("ая", feats('noun', 'sg', 'nom')),
                        ("ой", feats('noun', 'sg', 'gen', 'dat', 'instr', 'loc')),
                        ("ую", feats('noun', 'sg', 'acc')),
                        ("ые", feats('noun', 'pl', 'nom', 'acc')),
                        ("ых", feats('noun', 'pl', 'gen', 'loc')),
                        ("ым", feats('noun', 'pl', 'dat')),
                        ("ыми", feats('noun', 'pl', 'instr'))]
noun_adjfem_stems = ["ц+ел",
                     "дес+ят",
                     "с+от",
                     "т+ысячн",
                     "десятит+ысячн",
                     "стот+ысячн",
                     "милли+онн",
                     "н+абережн",
                     ]
noun_adjfem_tagger = tagger(noun_adjfem_stems, noun_adjfem_paradigm)

noun_zero_paradigm = [("", feats('noun', 'sg', 'pl', 'nom', 'gen', 'dat', 'acc', 'instr', 'loc'))]
noun_zero_stems = ["пром+илле", "+евро", "шосс+е"]

noun_zero_tagger = tagger(noun_zero_stems, noun_zero_paradigm)

# The remove() is  here so that stress marks get _inserted_ when analyzing
stress = (remove(maybe_stressed("+")) | cost("", 0.01))

# For "год", we only need it in dates, so we use a suppletive paradigm with GenPl "лет" and
# Loc2 "год+у"
year_tagger = direct_tagger(("год", feats('noun', 'sg', 'nom', 'acc')),
                            ("год" + remove("а"), feats('noun', 'sg', 'gen')),
                            ("г" + stress + "од" + remove("у"), feats('noun', 'sg', 'dat')),
                            ("год" + remove("ом"), feats('noun', 'sg', 'instr')),
                            ("год" + remove(stress + "у"), feats('noun', 'sg', 'loc')),
                            ("год" + remove("ы"), feats('noun', 'pl', 'nom', 'acc')),
                            (replace("лет", "год"), feats('noun', 'pl', 'gen')),
                            ("год" + remove("ам"), feats('noun', 'pl', 'dat')),
                            ("год" + remove("ами"), feats('noun', 'pl', 'instr')),
                            ("год" + remove("ах"), feats('noun', 'pl', 'loc')))

day_tagger = direct_tagger(("д+ень", feats('noun', 'sg', 'nom', 'acc')),
                            (replace_stressed("дн+я", "день"), feats('noun', 'sg', 'gen')),
                            (replace_stressed("дн+ю", "день"), feats('noun', 'sg', 'dat')),
                            (replace_stressed("дн+ём", "день"), feats('noun', 'sg', 'instr')),
                            (replace_stressed("дн+е", "день"), feats('noun', 'sg', 'loc')),
                            (replace_stressed("дн+и", "день"), feats('noun', 'pl', 'nom', 'acc')),
                            (replace_stressed("дн+ей", "день"), feats('noun', 'pl', 'gen')),
                            (replace_stressed("дн+ям", "день"), feats('noun', 'pl', 'dat')),
                            (replace_stressed("дн+ями", "день"), feats('noun', 'pl', 'instr')),
                            (replace_stressed("дн+ях", "день"), feats('noun', 'pl', 'loc')))

noun_tagger = (noun_zt1m_tagger | noun_zt1f_tagger | noun_zt1am_tagger | noun_zt1n_tagger |
               noun_zt2_tagger | noun_zt2ef_tagger |
               noun_zt3a_gpl0_tagger | noun_zt3c_tagger | noun_zt3ss_tagger |
               noun_zt3sh_tagger | noun_zt3a_m_tagger | noun_zt3am_tagger |
               noun_zt4_tagger |
               noun_zt5_tagger |
               noun_zt6c_tagger |
               noun_zt7n_tagger |
               noun_zt8e_tagger |
               noun_adjfem_tagger | noun_zero_tagger |
               year_tagger | day_tagger)

adj_z1a_paradigm = [("ый", feats('adjective', 'mas', 'nom', 'acc')),
                    ("ого", feats('adjective', 'mas', 'neu', 'gen')),
                    ("ому", feats('adjective', 'mas', 'neu', 'dat')),
                    ("ым", feats('adjective', 'mas', 'neu', 'instr')),
                    ("ом", feats('adjective', 'mas', 'neu', 'loc')),
                    ("ое", feats('adjective', 'neu', 'nom', 'acc')),
                    ("ая", feats('adjective', 'fem', 'nom')),
                    ("ой", feats('adjective', 'fem', 'gen', 'dat', 'loc')),
                    ("ую", feats('adjective', 'fem', 'acc')),
                    ("ые", feats('adjective', 'pl', 'nom', 'acc')),
                    ("ых", feats('adjective', 'pl', 'gen', 'loc')),
                    ("ым", feats('adjective', 'pl', 'dat')),
                    ("ыми", feats('adjective', 'pl', 'instr'))]
adj_z1a_stems = ["квадр+атн"]
adj_z1a_tagger = tagger(adj_z1a_stems, adj_z1a_paradigm)

adj_z3a_paradigm = [("ий", feats('adjective', 'mas', 'nom', 'acc')),
                    ("ого", feats('adjective', 'mas', 'neu', 'gen')),
                    ("ому", feats('adjective', 'mas', 'neu', 'dat')),
                    ("им", feats('adjective', 'mas', 'neu', 'instr')),
                    ("ом", feats('adjective', 'mas', 'neu', 'loc')),
                    ("ое", feats('adjective', 'neu', 'nom', 'acc')),
                    ("ая", feats('adjective', 'fem', 'nom')),
                    ("ой", feats('adjective', 'fem', 'gen', 'dat', 'loc')),
                    ("ую", feats('adjective', 'fem', 'acc')),
                    ("ие", feats('adjective', 'pl', 'nom', 'acc')),
                    ("их", feats('adjective', 'pl', 'gen', 'loc')),
                    ("им", feats('adjective', 'pl', 'dat')),
                    ("ими", feats('adjective', 'pl', 'instr'))]
adj_z3a_stems = ["куб+ическ"]
adj_z3a_tagger = tagger(adj_z3a_stems, adj_z3a_paradigm)

adj_tagger = adj_z1a_tagger | adj_z3a_tagger

tagger = card_tagger | ord_tagger | noun_tagger | adj_tagger

producer = tagger.invert()

tagger = tagger.optimize()
producer = producer.optimize()

def check_form(target_stem, feats):
    return tagger >> target_stem + feats >> producer


if __name__ == '__main__':
    print 'tagger:', tagger
    fst_save(tagger, 'numerals')
