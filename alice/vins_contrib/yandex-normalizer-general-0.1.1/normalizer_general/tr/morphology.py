#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')

from general.normbase import *
import categories

print "Morphology"

UNVOICED_CONSONANTS = u"çfhkpqsştx"
unvoiced_consonant = anyof(UNVOICED_CONSONANTS)
VOICED_CONSONANTS = u"bcdgğjlmnrvwyz"
voiced_consonant = anyof (VOICED_CONSONANTS)
CONSONANTS = UNVOICED_CONSONANTS + VOICED_CONSONANTS
consonant = anyof(CONSONANTS)
VOWELS = u"aeiıoöuü"
vowel = anyof(VOWELS)
ext_vowel = anyof(VOWELS) | anyof("AI")

# Each dictionary entry is (wide, narrow, wide_after_A)
SERIES_BEHAVIOUR = {
        'a': ('a', 'ı', 'a'),
        'e': ('e', 'i', 'e'),
        'o': ('o', 'u', 'a'),
        'ö': ('ö', 'ü', 'e'),
}

def make_harmonized_seq(series):
    wide, narrow, series_after_A = SERIES_BEHAVIOUR[series]
    wide_after_A, narrow_after_A, _ = SERIES_BEHAVIOUR[series_after_A]
    r = (ss(consonant) +
         anyof([wide, narrow]) +
         ss(consonant |
            wide | narrow |
            replace("I", narrow)) +
         qq(replace("A", wide_after_A) +
            ss(consonant |
               wide_after_A | narrow_after_A |
               replace("I", narrow_after_A) |
               replace("A", wide_after_A))))
    return r.optimize()

harmonized_seq = anyof([make_harmonized_seq(s) for s in SERIES_BEHAVIOUR.keys()])

harmony = pp(harmonized_seq)
harmony = harmony.optimize()

do_voice_consonant = anyof([replace(f, t) for f,t in zip(u"çkpt", u"cğbd")])

# Why do I need to resort to sets? Why doesn't simple `consonant - anyof(u"çkp")` work?
consonant_with_voicing = (anyof(set(CONSONANTS) - set(u"çkpt")) | do_voice_consonant)

do_unvoice_consonant = anyof([replace(f,t)
                              # Does anything other than 'd' actually occur in suffixes?
                              for f, t in zip(u"bcdgğv",
                                              u"pçtkkf")])
consonant_with_unvoicing = (anyof(set(CONSONANTS) - set(u"bcdgğv")) | do_unvoice_consonant)

no_voicing_words = rr(g.any_symb, 0, 2) + consonant | "kırk" | "dört"

# !!!! The following will accept both "dörde" and "dörte", "kırka" and "kırğa".
# That's probably OK for reverse normalization, but for forward normalization we shall have to deal with this.

# !!!! Not handled: fluid vowels ("akıl -> aklı")
def join_morphs(first, second):
    r = (
        # voicing of the last consonant, inserting an epenthetic vowel,
        # with at least four symbols total (!!!! Not an accurate criterion)
        ((first >> rr(g.any_symb, 3) + ss(g.any_symb) + consonant_with_voicing) +
         (second >> qq(remove("(") + ext_vowel + qq(remove(")")) + ss(g.any_symb)))) |
        # No voicing for short stems and "kırk"
        ((first >> no_voicing_words) +
         (second >> qq(remove("(") + ext_vowel + qq(remove(")")) + ss(g.any_symb)))) |
        # Not inserting an epenthetic vowel after a vowel
        ((first >> ss(g.any_symb) + ext_vowel) +
         (second >> remove("(" + ext_vowel + ")") + ss(g.any_symb))) |
        # Inserting a consonant after a vowel
        ((first >> ss(g.any_symb) + ext_vowel) +
         (second >> qq(remove("(")) + consonant + qq(remove(")") + ss(g.any_symb)))) |
        # Not inserting a consonant after a consonant. Voicing for longer stems (!!! Not an accurate criterion)
        ((first >> rr(g.any_symb, 3) + ss(g.any_symb) + consonant_with_voicing) +
         (second >> remove("(" + consonant + ")") + ext_vowel + ss(g.any_symb))) |
        # No voicing for short stems and "kırk"
        ((first >> no_voicing_words) +
         (second >> remove("(" + consonant + ")") + ext_vowel + ss(g.any_symb))) |
        # No voicing, no epenthetic cononant when there's another consonant after the epenthetic one (e.g. "(y)la")
        ((first >> pp(g.any_symb) + consonant) +
         (second >> remove("(" + consonant + ")") + consonant + ss(g.any_symb))) |
        # No epenthesis. This only happens when the second morpheme starts with a consonant or is empty
        # May need to unvoice the first consonant of the suffix
        ((first >> ss(g.any_symb) + unvoiced_consonant) +
         (second >> (consonant_with_unvoicing + ss(g.any_symb) | ""))) |
        ((first >> ss(g.any_symb) + (voiced_consonant | ext_vowel)) +
         (second >> consonant + ss(g.any_symb))) |
        (first + (second >> empty())))
    return r.optimize()

# No features are inserted for 'nom', since basre stem usually does not
# mean 'nominative'.
CASES = [
#    ('nom', u""),
    ('gen', u"(n)In"),
    ('acc', u"(y)I"),
    ('dat', u"(y)A"),
    ('loc', u"dA"),
    ('abl', u"dAn"),
    ('comit', u"(y)lA"),
    ]

NUMERALS = [
    (u"sıfır", "0"),
    (u"bir", "1"),
    (u"iki", "2"),
    (u"üç", "3"),
    (u"dört", "4"),
    (u"beş", "5"),
    (u"altı", "6"),
    (u"yedi", "7"),
    (u"sekiz", "8"),
    (u"dokuz", "9"),
    (u"on", "10"),
    (u"yirmi", "20"),
    (u"otuz", "30"),
    (u"kırk", "40"),
    (u"elli", "50"),
    (u"altmış", "60"),
    (u"yetmiş", "70"),
    (u"seksen", "80"),
    (u"doksan", "90"),
    (u"yüz", "100"),
    (u"bin", "1000"),
    (u"milyon", "1000000"),
    (u"milyar", "1000000000"),
    ]

card_tagger = anyof([(anyof([replace((join_morphs(stem, flex) >> harmony).invert(), value + feats('numeral', 'card', case))
                             for case, flex in CASES]) |
                      replace(stem, value))	# for nominative
                     for stem, value in NUMERALS])

ord_tagger = anyof([(anyof([replace((join_morphs(join_morphs(stem, "(I)ncI"), flex) >> harmony).invert(), value + "." + feats('numeral', 'ord', case))
                            for case, flex in CASES]) |
                     replace((join_morphs(stem, "(I)ncI") >> harmony).invert(), value + "."))	# for nominative
                    for stem, value in NUMERALS])

tagger = card_tagger | ord_tagger
producer = tagger.invert()

tagger = tagger.optimize()
producer = producer.optimize()

if __name__ == '__main__':
    print 'tagger:', tagger
    fst_save(tagger, 'numerals')
