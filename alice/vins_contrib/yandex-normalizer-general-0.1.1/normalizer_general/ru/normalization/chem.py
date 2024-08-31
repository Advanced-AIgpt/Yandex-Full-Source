#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('../..')

from general.normbase import *
import categories

print 'Chemical formulas'

ELEMENTS = [
    ("H", "+аш"),
    ("He", "гелий"),

    ("Li", "литий"),
    ("Be", "бериллий"),
    ("B", "бор"),
    ("C", "ц+э"),
    ("N", "+эн"),
    ("O", "+о"),
    ("F", "фтор"),
    ("Ne", "неон"),

    ("Na", "натрий"),
    ("Mg", "магний"),
    ("Al", "алюминий"),
    ("Si", "силициум"),
    ("P", "п+э"),
    ("S", "+эс"),
    ("Cl", "хлор"),
    ("Ar", "аргон"),

    ("K", "калий"),
    ("Ca", "кальций"),
    ("Sc", "скандий"),
    ("Ti", "титан"),
    ("V", "ванадий"),
    ("Cr", "хром"),
    ("Mn", "марганец"),
    ("Fe", "феррум"),
    ("Co", "кобальт"),
    ("Ni", "никель"),

    ("Cu", "купрум"),
    ("Zn", "цинк"),
    ("Ga", "галлий"),
    ("Ge", "германий"),
    ("As", "арсеникум"),
    ("Se", "селен"),
    ("Br", "бром"),
    ("Kr", "криптон"),

    ("Rb", "рубидий"),
    ("Sr", "стронций"),
    ("Y", "иттрий"),
    ("Zr", "цирконий"),
    ("Nb", "ниобий"),
    ("Mo", "молибден"),
    ("Tc", "технеций"),
    ("Ru", "рутений"),
    ("Rh", "родий"),
    ("Pd", "палладий"),

    ("Ag", "аргентум"),
    ("Cd", "кадмий"),
    ("In", "индий"),
    ("Sn", "станнум"),
    ("Sb", "стибиум"),
    ("Te", "теллур"),
    ("I", "иод"),
    ("Xe", "ксенон"),

    ("Cs", "цезий"),
    ("Ba", "барий"),

    ("La", "лантан"),
    ("Ce", "церий"),
    ("Pr", "празеодим"),
    ("Nd", "неодим"),
    ("Pm", "прометий"),
    ("Sm", "самарий"),
    ("Eu", "европий"),
    ("Gd", "гадолиний"),
    ("Tb", "тербий"),
    ("Dy", "диспрозий"),
    ("Ho", "гольмий"),
    ("Er", "эрбий"),
    ("Tm", "тулий"),
    ("Yb", "иттербий"),
    ("Lu", "лютеций"),

    ("Hf", "гафний"),
    ("Ta", "тантал"),
    ("W", "вольфрам"),
    ("Re", "рений"),
    ("Os", "осмий"),
    ("Ir", "иридий"),
    ("Pt", "платина"),

    ("Au", "аурум"),
    ("Hg", "гидраргирум"),
    ("Tl", "таллий"),
    ("Pb", "плюмбум"),
    ("Bi", "висмут"),
    ("Po", "полоний"),
    ("At", "астат"),
    ("Rn", "радон"),

    ("Fr", "франций"),
    ("Ra", "радий"),

    ("Ac", "актиний"),
    ("Th", "торий"),
    ("Pa", "протактиний"),
    ("U", "уран"),
    ("Np", "нептуний"),
    ("Pu", "плутоний"),
    ("Am", "америций"),
    ("Cm", "кюрий"),
    ("Bk", "берклий"),
    ("Cf", "калифорний"),
    ("Es", "эйнштейний"),
    ("Fm", "фермий"),
    ("Md", "менделевий"),
    ("No", "нобелий"),
    ("Lr", "лоуренсий"),

    ("Rf", "резерфордий"),
    ("Db", "дубний"),
    ("Sg", "сиборгий"),
    ("Bh", "борий"),
    ("Hn", "ханий"),
    ("Mt", "мейтнерий"),

    ("(", "левая скобка"),
    (")", "правая скобка"),
    ]

one_element = (anyof([replace(e, " " + reading) for e, reading in ELEMENTS]) |
              insert(" ") + pp(g.digit) + insert(feats('numeral', 'card', 'nom', 'mas')) |
               " ")

# To be used in spec_codes.py
handle = (remove("#[chem|") +
          ss(one_element) +
          remove("]#"))