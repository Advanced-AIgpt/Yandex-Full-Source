#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')

from general.normbase import *
import categories

# Cardinal numerals
print 'Numerals'

# For один, два, три, четыре, it is easiest to just specify the transformers directly.
card_1_tagger = direct_tagger((replace("один", "1"), feats('numeral', 'card', 'mas', 'nom', 'acc')),
                              (replace("одне", "1"), feats('numeral', 'card', 'neu', 'nom', 'acc')),
                              (replace("одна", "1"), feats('numeral', 'card', 'fem', 'nom')),
                              (replace("одного", "1"), feats('numeral', 'card', 'mas', 'neu', 'gen')),
                              (replace("одної", "1"), feats('numeral', 'card', 'fem', 'gen', 'loc')),
                              (replace("одному", "1"), feats('numeral', 'card',  'mas', 'neu', 'dat')),
                              (replace("одній", "1"), feats('numeral', 'card',  'fem', 'dat')),
                              (replace("одну", "1"), feats('numeral', 'card', 'fem', 'acc')),
                              (replace("одним", "1"), feats('numeral', 'card', 'mas', 'neu', 'instr')))
                              
card_2_tagger = direct_tagger((replace("два", "2"), feats('numeral', 'card', 'mas', 'neu', 'nom', 'acc')),
                              (replace("дві", "2"), feats('numeral', 'card', 'fem', 'nom', 'acc')),
                              (replace("двух", "2"), feats('numeral', 'card', 'gen', 'loc')),
                              (replace("двох", "2"), feats('numeral', 'card', 'gen', 'loc')),
                              (replace("двум", "2"), feats('numeral', 'card', 'dat')),
                              (replace("двома", "2"), feats('numeral', 'card', 'instr')))

card_3_tagger = direct_tagger((replace("три", "3"), feats('numeral', 'card', 'nom', 'acc')),
                              (replace("трьох", "3"), feats('numeral', 'card', 'gen', 'loc')),
                              (replace("трьом", "3"), feats('numeral', 'card', 'dat')),
                              (replace("трьомя", "3"), feats('numeral', 'card', 'instr')))

card_4_tagger = direct_tagger((replace("чотири", "4"), feats('numeral', 'card', 'nom', 'acc')),
                              (replace("чотирьох", "4"), feats('numeral', 'card', 'gen', 'loc')),
                              (replace("чотирьом", "4"), feats('numeral', 'card', 'dat')),
                              (replace("чотирма", "4"), feats('numeral', 'card', 'instr')))



card_7_tagger = direct_tagger((replace("сім", "7"), feats('numeral', 'card', 'nom', 'acc')),
                              (replace("семи", "7"), feats('numeral', 'card', 'gen', 'loc', 'dat')),
                              (replace("сімох", "7"), feats('numeral', 'card', 'gen', 'dat', 'loc')),
                              (replace("сімома", "7"), feats('numeral', 'card', 'instr')))

card_8_tagger = direct_tagger((replace("вісім", "8"), feats('numeral', 'card', 'nom', 'acc')),
                              (replace("восьми", "8"), feats('numeral', 'card', 'gen', 'loc', 'dat')),
                              (replace("вісьмох", "8"), feats('numeral', 'card', 'gen', 'acc', 'loc')),
                              (replace("вісьмом", "8"), feats('numeral', 'card', 'dat')),
                              (replace("вісьма", "8"), feats('numeral', 'card', 'instr')),
                              (replace("вісьмома", "8"), feats('numeral', 'card', 'instr'))
)



paradigm_5on = [("ь", feats('numeral', 'card', 'nom', 'acc')),
                ("и", feats('numeral', 'card', 'gen', 'dat', 'loc')),
                ("ьма", feats('numeral', 'card', 'instr')),
                ("ьома", feats('numeral', 'card', 'instr'))]
stems_5on = [replace("п'ят", "5"),
             replace("шіст", "6"),
             replace("дев'ят", "9"),
             replace("десят", "10"),
             replace("одинадцят", "11"),
             replace("дванадцят", "12"),
             replace("тринадцят", "13"),
             replace("чотирнадцят", "14"),
             replace("п'ятнадцят", "15"),
             replace("шістнадцят", "16"),
             replace("сімнадцят", "17"),
             replace("вісімнадцят", "18"),
             replace("дев'ятнадцят", "19"),
             replace("двадцят", "20"),
             replace("тридцят", "30")]
card_5on_tagger = tagger(stems_5on, paradigm_5on)

card_40_tagger = direct_tagger((replace("сорок", "40"), feats('numeral', 'card', 'nom', 'acc')),
                               (replace("сорока", "40"), feats('numeral', 'card', 'gen', 'dat', 'instr', 'loc')))

# 50-80 are easiest to describe in this somewhat crazy fashion
paradigm_50on = [("десят", feats('numeral', 'card', 'nom', 'acc')),
                 ("десяти", feats('numeral', 'card', 'gen', 'dat', 'loc')),
                 ("десятьма", feats('numeral', 'card', 'instr'))]
stems_50on = [replace("п'ят", "50"),
              replace("шіст", "60"),
              replace("сім", "70"),
              replace("вісім", "80")]
card_50on_tagger = tagger(stems_50on, paradigm_50on)

paradigm_90_100 = [("о", feats('numeral', 'card', 'nom', 'acc')),
                   ("а", feats('numeral', 'card', 'gen', 'dat', 'instr', 'loc'))]
stems_90_100 = [replace("дев'яност", "90"),
                replace("ст", "100")]
card_90_100_tagger = tagger(stems_90_100, paradigm_90_100)

# двести, триста, четыреста -- each unique
card_200_tagger = direct_tagger((replace("двісті", "200"), feats('numeral', 'card', 'nom', 'acc')),
                                (replace("двухсот", "200"), feats('numeral', 'card', 'gen')),
                                (replace("двохсот", "200"), feats('numeral', 'card', 'gen')),
                                (replace("двохста", "200"), feats('numeral', 'card', 'gen', 'loc')), # non normative
                                (replace("двомстам", "200"), feats('numeral', 'card', 'dat')),
                                (replace("двомста", "200"), feats('numeral', 'card', 'dat')), # non normative
                                (replace("двомастами", "200"), feats('numeral', 'card', 'instr')),
                                (replace("двомаста", "200"), feats('numeral', 'card', 'instr')), # non normative
                                (replace("двохстах", "200"), feats('numeral', 'card', 'loc')))
card_300_tagger = direct_tagger((replace("триста", "300"), feats('numeral', 'card', 'nom', 'acc')),
                                (replace("трьохсот", "300"), feats('numeral', 'card', 'gen')),
                                (replace("трьохста", "300"), feats('numeral', 'card', 'gen', 'loc')), # non normative
                                (replace("трьомстам", "300"), feats('numeral', 'card', 'dat')),
                                (replace("трьомста", "300"), feats('numeral', 'card', 'dat')), # non normative
                                (replace("трьомастами", "300"), feats('numeral', 'card', 'instr')),
                                (replace("трьомаста", "300"), feats('numeral', 'card', 'instr')), # non normative
                                (replace("трьохстах", "300"), feats('numeral', 'card', 'loc')))
card_400_tagger = direct_tagger((replace("чотириста", "400"), feats('numeral', 'card', 'nom', 'acc')),
                                (replace("чотирьохсот", "400"), feats('numeral', 'card', 'gen')),
                                (replace("чотирьохста", "400"), feats('numeral', 'card', 'gen', 'loc')), # non normative
                                (replace("чотирьомстам", "400"), feats('numeral', 'card', 'dat')),
                                (replace("чотирьомста", "400"), feats('numeral', 'card', 'dat')), # non normative
                                (replace("чотирмастами", "400"), feats('numeral', 'card', 'instr')),
                                (replace("чотирмаста", "400"), feats('numeral', 'card', 'instr')), # non normative
                                (replace("чотирьохстах", "400"), feats('numeral', 'card', 'loc')))

# 500-900, same way as 50-80
paradigm_500on = [("сот", feats('numeral', 'card', 'nom', 'acc')),
                  ("исот", feats('numeral', 'card', 'gen')),
                  ("иста", feats('numeral', 'card', 'gen', 'dat', 'instr', 'loc')), # non normative, but used
                  ("истам", feats('numeral', 'card', 'dat')),
                  ("имастами", feats('numeral', 'card', 'instr')),
                  ("ьмастами", feats('numeral', 'card', 'instr')),
                  ("омастами", feats('numeral', 'card', 'instr')),
                  ("ьомастами", feats('numeral', 'card', 'instr')),
                  ("истах", feats('numeral', 'card', 'loc'))]
stems_500on = [replace("п'ят", "500"),
               replace("шіст", "600"),
               replace("шест", "600"),
               replace("сім", "700"),
               replace("сем", "700"),
               replace("вісім", "800"),
               replace("восьм", "800"),
               replace("дев'ят", "900")]
card_500on_tagger = tagger(stems_500on, paradigm_500on)

# тисяча and мільйон are standard nouns, but we don't have a general noun declination analyzer,
# so we treat them as a special case.
noun_zt4_paradigm = [("а", feats('noun', 'sg', 'nom')),
                     ("і", feats('noun', 'sg', 'gen', 'dat', 'loc') | feats('noun', 'pl', 'nom', 'acc')),
                     ("у", feats('noun', 'sg', 'acc')),
                     ("ею", feats('noun', 'sg', 'instr')),
                     ("", feats('noun', 'pl', 'gen')),
                     ("ам", feats('noun', 'pl', 'dat')),
                     ("ами", feats('noun', 'pl', 'instr')),
                     ("ах", feats('noun', 'pl', 'loc'))]
noun_zt4_stems = ["тисяч"]
noun_zt4_tagger = tagger(noun_zt4_stems, noun_zt4_paradigm)

noun_zt1_paradigm = [("", feats('noun', 'sg', 'nom', 'acc')),
                     ("а", feats('noun', 'sg', 'gen')),
                     ("у", feats('noun', 'sg', 'dat')),
                     ("ом", feats('noun', 'sg', 'instr')),
                     ("е", feats('noun', 'sg', 'loc')),
                     ("и", feats('noun', 'pl', 'nom', 'acc')),
                     ("ів", feats('noun', 'pl', 'gen')),
                     ("ам", feats('noun', 'pl', 'dat')),
                     ("ами", feats('noun', 'pl', 'instr')),
                     ("ах", feats('noun', 'pl', 'loc'))]
noun_zt1_stems = ["мільйон", "мільярд"]
noun_zt1_tagger = tagger(noun_zt1_stems, noun_zt1_paradigm)

card_tagger = card_1_tagger | card_2_tagger | card_3_tagger | card_4_tagger | card_7_tagger | card_8_tagger | \
              card_5on_tagger | card_40_tagger | card_50on_tagger | card_90_100_tagger | \
              card_200_tagger | card_300_tagger | card_400_tagger | card_500on_tagger | \
              noun_zt4_tagger | noun_zt1_tagger

# Ordinals

ord_unstressed_hard_paradigm = [(replace("ий", "й"), feats('numeral', 'ord', 'mas', 'nom', 'acc')),
                                ("е", feats('numeral', 'ord', 'neu', 'nom', 'acc')),
                                ("а", feats('numeral', 'ord', 'fem', 'nom')),
                                ("у", feats('numeral', 'ord', 'fem', 'acc')),
                                (replace("ого", "го"), feats('numeral', 'ord', 'mas', 'neu', 'gen')),
                                (replace("ої", "ї"), feats('numeral', 'ord', 'fem', 'gen')),
                                (replace("ою", "ю"), feats('numeral', 'ord', 'fem', 'instr')),
                                (replace("ій", "й"), feats('numeral', 'ord', 'fem', 'dat', 'loc')),
                                (replace("ому", "му"), feats('numeral', 'ord', 'mas', 'neu', 'dat', 'loc')),                              
                                ("им", feats('numeral', 'ord', 'mas', 'neu', 'instr') | feats('numeral', 'ord', 'pl', 'dat')),
                                ("і", feats('numeral', 'ord', 'pl', 'nom', 'acc')),
                                ("их", feats('numeral', 'ord', 'pl', 'gen', 'loc')),
                                (replace("ими", "ми"), feats('numeral', 'ord', 'pl', 'instr'))]
ord_unstressed_hard_stems = [replace("перш", "1"),
                             replace("четверт", "4"),
                             replace("п'ят", "5"),
                             replace("шост", "6"),
                             replace("дев'ят", "9"),
                             replace("десят", "10"),
                             replace("одинадцят", "11"),
                             replace("дванадцят", "12"),
                             replace("тринадцят", "13"),
                             replace("чотирнадцят", "14"),
                             replace("п'ятнадцят", "15"),
                             replace("шістнадцят", "16"),
                             replace("сімнадцят", "17"),
                             replace("вісімнадцят", "18"),
                             replace("дев'ятнадцят", "19"),
                             replace("двадцят", "20"),
                             replace("тридцят", "30"),
                             replace("п'ятдесят", "50"),
                             replace("шістдесят", "60"),
                             replace("сімдесят", "70"),
                             replace("вісімдесят", "80"),
                             replace("дев'яност", "90"),
                             replace("сот", "100"),
                             replace("тисячн", "1000"),
                             replace("мільйонн", "1000000"),
                             replace("мільярдн", "1000000000")]
ord_unstressed_hard_tagger = tagger(ord_unstressed_hard_stems, ord_unstressed_hard_paradigm, drop_flection=False)

ord_unstressed_soft_paradigm = [(replace("ій", "й"), feats('numeral', 'ord', 'mas', 'nom', 'acc') | feats('numeral', 'ord', 'fem', 'gen', 'dat', 'instr', 'loc')),
                                ("є", feats('numeral', 'ord', 'neu', 'nom', 'acc')),
                                ("я", feats('numeral', 'ord', 'fem', 'nom')),
                                (replace("ього", "го"), feats('numeral', 'ord', 'mas', 'neu', 'gen')),
                                (replace("ьому", "му"), feats('numeral', 'ord', 'mas', 'neu', 'dat')),
                                ("ю", feats('numeral', 'ord', 'fem', 'acc')),
                                (replace("ім", "м"), feats('numeral', 'ord', 'mas', 'neu', 'instr') | feats('numeral', 'ord', 'pl', 'dat')),
                                (replace("ьому", "му"), feats('numeral', 'ord', 'mas', 'neu', 'loc')),
                                ("і", feats('numeral', 'ord', 'pl', 'nom', 'acc')),
                                ("іх", feats('numeral', 'ord', 'pl', 'gen', 'loc')),
                                (replace("іми", "ми"), feats('numeral', 'ord', 'pl', 'instr'))]
ord_unstressed_soft_stems = [replace("трет", "3")]
ord_unstressed_soft_tagger = tagger(ord_unstressed_soft_stems, ord_unstressed_soft_paradigm, drop_flection=False)

ord_stressed_paradigm = [(replace("ий", "й"), feats('numeral', 'ord', 'mas', 'nom', 'acc')),
                         ("е", feats('numeral', 'ord', 'neu', 'nom', 'acc')),
                         ("а", feats('numeral', 'ord', 'fem', 'nom')),
                         (replace("ого", "го"), feats('numeral', 'ord', 'mas', 'neu', 'gen')),
                         (replace("ій", "й"), feats('numeral', 'ord', 'fem', 'gen', 'dat', 'loc')),
                         (replace("ою", "ю"), feats('numeral', 'ord', 'fem', 'instr')),
                         (replace("ому", "му"), feats('numeral', 'ord', 'mas', 'neu', 'dat', 'loc')),
                         ("у", feats('numeral', 'ord', 'fem', 'acc')),
                         ("им", feats('numeral', 'ord', 'mas', 'neu', 'instr') | feats('numeral', 'ord', 'pl', 'dat')),
                         ("і", feats('numeral', 'ord', 'pl', 'nom', 'acc')),
                         ("их", feats('numeral', 'ord', 'pl', 'gen', 'loc')),
                         (replace("ими", "ми"), feats('numeral', 'ord', 'pl', 'instr'))]
ord_stressed_stems = [replace("друг", "2"),
                      replace("шост", "6"),
                      replace("сьом", "7"),
                      replace("восьм", "8"),
                      replace("сороков", "40")]
ord_stressed_tagger = tagger(ord_stressed_stems, ord_stressed_paradigm, drop_flection=False)

ord_tagger = ord_unstressed_hard_tagger | ord_unstressed_soft_tagger | ord_stressed_tagger

# Digit names are ordinary nouns
noun_zt2_paradigm = [("ь", feats('noun', 'sg', 'nom', 'acc')),
                     ("я", feats('noun', 'sg', 'gen')),
                     ("ю", feats('noun', 'sg', 'dat')),
                     ("еві", feats('noun', 'sg', 'dat')),
                     ("ем", feats('noun', 'sg', 'instr')),
                     ("і", feats('noun', 'sg', 'loc') | feats('noun', 'pl', 'nom', 'acc') ),
                     ("ів", feats('noun', 'pl', 'gen')),
                     ("ям", feats('noun', 'pl', 'dat')),
                     ("ями", feats('noun', 'pl', 'instr')),
                     ("ях", feats('noun', 'pl', 'loc'))]
noun_zt2_stems = [replace("нол", "0@"),
                  replace("нул", "0@")]
noun_zt2_tagger = tagger(noun_zt2_stems, noun_zt2_paradigm)

noun_zt5_paradigm = [("я", feats('noun', 'sg', 'nom')),
                     ("і", feats('noun', 'sg', 'gen', 'dat', 'loc') | feats('noun', 'pl', 'nom', 'acc')),
                     ("ю", feats('noun', 'sg', 'acc')),
                     ("ею", feats('noun', 'sg', 'instr')),
                     ("ь", feats('noun', 'pl', 'gen')),
                     ("ям", feats('noun', 'pl', 'dat')),
                     ("ями", feats('noun', 'pl', 'instr')),
                     ("ях", feats('noun', 'pl', 'loc'))]
noun_zt5_stems = [replace("одиниц", "1@")]
noun_zt5_tagger = tagger(noun_zt5_stems, noun_zt5_paradigm)

noun_zt3ss_paradigm = [("йка", feats('noun', 'sg', 'nom')),
                       ("йки", feats('noun', 'sg', 'gen') | feats('noun', 'pl', 'nom', 'acc')),
                       ("йці", feats('noun', 'sg', 'dat', 'loc')),
                       ("йку", feats('noun', 'sg', 'acc')),
                       ("йкою", feats('noun', 'sg', 'instr')),
                       ("йок", feats('noun', 'pl', 'gen')),
                       ("йкам", feats('noun', 'pl', 'dat')),
                       ("йками", feats('noun', 'pl', 'instr')),
                       ("йках", feats('noun', 'pl', 'loc'))]
noun_zt3ss_stems = [replace("дві", "2@"),
                    replace("трі", "3@")]
noun_zt3ss_tagger = tagger(noun_zt3ss_stems, noun_zt3ss_paradigm)

noun_zt3sh_paradigm =  [("ка", feats('noun', 'sg', 'nom')),
                        ("ки", feats('noun', 'sg', 'gen') | feats('noun', 'pl', 'nom', 'acc')),
                        ("ці", feats('noun', 'sg', 'dat', 'loc')),
                        ("ку", feats('noun', 'sg', 'acc')),
                        ("кою", feats('noun', 'sg', 'instr')),
                        ("ок", feats('noun', 'pl', 'gen')),
                        ("кам", feats('noun', 'pl', 'dat')),
                        ("ками", feats('noun', 'pl', 'instr')),
                        ("ках", feats('noun', 'pl', 'loc'))]
noun_zt3sh_stems = [replace("четвір", "4@"),
                    replace("п'ятір", "5@"),
                    replace("шестір", "6@"),
                    replace("семір", "7@"),
                    replace("вісім", "8@"),
                    replace("дев'ят", "9@")]
noun_zt3sh_tagger = tagger(noun_zt3sh_stems, noun_zt3sh_paradigm)

digit_tagger = noun_zt2_tagger | noun_zt5_tagger | noun_zt3ss_tagger | noun_zt3sh_tagger

tagger = card_tagger | ord_tagger | digit_tagger

producer = tagger.invert()

tagger = tagger.optimize()
producer = producer.optimize()

if __name__ == '__main__':
    print 'tagger:', tagger
    fst_save(tagger, 'numerals')
