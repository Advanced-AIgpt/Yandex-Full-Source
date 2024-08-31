#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('../..')

from general.normbase import g, replace, anyof, word, remove, insert, pp, ss, cost

replace_dict = {
    # Some ideas taken from http://www.omniglot.com/writing/english.htm
    "a": "ay",
    "b": "bee",
    "c": "see",
    "d": "dee",
    "e": "ee",
    "f": "ef",
    "g": "gee",
    "h": "aitch",
    "i": "i",
    "j": "jay",
    "k": "kay",
    "l": "el",
    "m": "em",
    "n": "en",
    "o": "o",
    "p": "pee",
    "q": "cue",
    "r": "ar",
    "s": "ess",
    "t": "tee",
    "u": "you",
    "v": "vee",
    "w": "double you",
    "x": "ex",
    "y": "wye",
    "z": "zed",

    ".": "dot",
    ",": "comma",
    ":": "colon",
    "(": "opening bracket",
    ")": "closing bracket",
    "\"": "quotation mark",
    "'": "apostrophe",
    ";": "semicolon",
    "!": "exclamation mark",
    "?": "question mark",
    "-": "hyphen",
    "--": "dash",

    "/": "slash",
    "\\": "backslash",
    "±": "plus minus",
    "€": "euro",
    "§": "paragraph",
    "©": "copyright",
    "®": "registered",
    "$": "dollar",
    "%": "persent",
    "‰": "promille",
    "№": "number",
    "<": "less than",
    ">": "greater than",
    "^": "circumflex",
    "|": "vertical slash",
    "~": "tilda",
    "_": "underscore",
    "@": "at",
    "=": "equals",
    "#": "sharp",
    "*": "asterisk",

    "0": "zero",
    "1": "one",
    "2": "two",
    "3": "three",
    "4": "four",
    "5": "five",
    "6": "six",
    "7": "seven",
    "8": "eight",
    "9": "nine",
}

replacer = anyof([insert(" ") + replace(xx, replace_dict[xx]) for xx in replace_dict])

convert_by_character = word(
    remove("#[character|") +
    ss(replacer | " ") +
    remove("]"),
    permit_inner_space=True,
    need_outer_space=False
)

cvt = ss(convert_by_character | cost(word(pp(g.any_symb)), 1)) + ss(" ")
