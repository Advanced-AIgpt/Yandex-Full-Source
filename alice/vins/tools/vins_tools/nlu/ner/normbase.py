# encoding: utf-8
import codecs
import collections
import math
import os
import platform
import random
import shutil
import subprocess
import tempfile

import sys
import fst

# Global state

alphabet = (
    u'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ' +
    u'абвгдеёжзийклмнопрстуфхцчшщъыьэюяАБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ' +
    u'äáàâãāåçëéèêẽēğıïíìîĩīöóòôõōşüúùûũūßœæÿýñøδξðšłấńủćữđụśþ' +
    u'ÄÁÀÂÃĀÅÇËÉÈÊẼĒĞİÏÍÌÎĨĪÖÓÒÔÕŌŞÜÚÙÛŨŪŒÆŸÝÑØΔΞÐŠŁẤŃỦĆỮĐỤŚÞ' +
    u'ґієїҐІЄЇ¿¡' +
    u' \t\r\n\u00a0' +
    u'!"#%&\'()*+,-./0123456789:;<=>?@[\\]^_`{|}~’‘´ ́ ̀' +
    u'±×€™§©®$‰№°⁰¹²³⁴⁵⁶⁷⁸⁹«»•’“”…₽£¢₴₺£₤¥‐‑‒–—―✝º÷' +
    # Glyphs that can occur in combinations that look like Russian letters
    u'\u0302\u0306\u0308\u0327'
    )

# { cat_name : set([grammeme1, grammeme2, ...]) }
cat_dict = dict()

# { pos_name : [category1, category2, ...] }
pos_dict = dict()


class SymSets(object):
    """
    Stores main symbol categories: letters, whitespaces etc.
    """
    def __init__(self, alphabet_seq):
        self.any = set(alphabet_seq)
        self.latin_letter = set(u"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ")
        self.russian_letter = set(u"абвгдеёжзийклмнопрстуфхцчшщъыьэюяАБВГДЕЁЖХИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ")
        self.letter = self.latin_letter | self.russian_letter | set(
            u'äáàâãāçëéèêẽēğıïíìîĩīöóòôõōşüúùûũūßœÿýñøδξðšłấńủćữđụśþ'
            u'ÄÁÀÂÃĀÇËÉÈÊẼĒĞİÏÍÌÎĨĪÖÓÒÔÕŌŞÜÚÙÛŨŪŒŸÝÑØΔΞÐŠŁẤŃỦĆỮĐỤŚÞ'
            u'ґієїҐІЄЇ\'')
        self.letter_or_plus = self.letter | set("+")
        self.digit = set(u"0123456789")
        self.upper_case_digit = set(u"⁰¹²³⁴⁵⁶⁷⁸⁹")
        self.whitespace = set(u" \t\r\n\u00a0")
        self.word_sym = self.any - self.whitespace
        self.word_sym_not_digit = self.any - self.whitespace - self.digit


# (I have to keep the fields encapsulated in a class instance, so that changes
#  are visible in modules that do "from normbase import *".)
class GeneralInfo(object):
    """
    A place to store symtab information and fsts for character classes
    One instance is created, then updated in categories_defined()
    """
    def __init__(self):
        # Just create the fields in a non-functional state
        self.symsets = None
        self.symtab = None
        self.any_symb = None
        self.latin_letter = None
        self.russian_letter = None
        self.letter = None
        self.letter_or_plus = None
        self.digit = None
        self.word_sym_fst = None
        self.whitespace = None
        self.direct_transcription_word = None
        self.chem_word = None
        self.word_sym_not_digit = None
        self.upper_case_digit = None
        self.read_by_char_word = None

    # To get full set of arcs in word_sym_fst, `anyof' does not work --
    # it converts category names to strings. So we have to produce elementarry fsts manually.
    def single_arc_fst(self, a, symtab):
        f = fst.Acceptor(symtab)
        f.add_arc(0, 1, a)
        f[1].final = True
        return f

    def update(self):
        f = fst.linear_chain(alphabet)
        allsymbols = set(alphabet)
        for cat in cat_dict:
            for gm in cat_dict[cat]:
                f.add_arc(0, 1, gm)
                allsymbols.add(gm)
        self.symtab = f.isyms
        self.symsets = SymSets(allsymbols)

        # From here on, symtab is defined, so we are allowed to produce Fst-s
        # (We could not do that before, since Fst constructor references g.symtab.)

        def any_of_set(symbols):
            return anyof([self.single_arc_fst(a, self.symtab) for a in symbols])

        self.any_symb = any_of_set(self.symsets.any)

        self.latin_letter = anyof(self.symsets.latin_letter)
        self.russian_letter = anyof(self.symsets.russian_letter)
        self.letter = anyof(self.symsets.letter)
        self.letter_or_plus = anyof(self.symsets.letter_or_plus)
        self.digit = anyof(self.symsets.digit)
        self.upper_case_digit = anyof(self.symsets.upper_case_digit)
        self.whitespace = anyof(self.symsets.whitespace)
        self.word_sym_fst = any_of_set(self.symsets.word_sym)
        self.word_sym_not_digit = any_of_set(self.symsets.word_sym_not_digit)

        # Material between "<[" and "]>" needs to be skipped by the normalizer.
        # Material between "#[character|" and "]" is also treated in a special way.
        self.direct_transcription_word = word(
            "<[" + ss(self.any_symb - "]") + "]>",
            permit_inner_space=True,
            need_outer_space=False
        )
        self.read_by_char_word = word(
            "#[character|" + ss(self.any_symb - ']') + "]#",
            permit_inner_space=True,
            need_outer_space=False
        )
        self.chem_word = word(
            "#[chem|" + ss(self.digit | self.letter | anyof("() ")) + "]#",
            permit_inner_space=True,
            need_outer_space=False
        )


g = GeneralInfo()


# Wrap fst in order to have a better syntax and a slightly higher level operations.
class Fst(object):
    """
    Fst: Weighted Finite State Transducer.
    A cover over pyfst.fst with operations on a somewhat higher level.
    """
    def __init__(self, src=None):
        if src is None:
            src = u""  # empty Fst
        if isinstance(src, str):
            src = src.decode('utf-8')
        if isinstance(src, unicode):
            if src == u"":
                # special case
                self.f = fst.Acceptor(g.symtab)
                self.f[0].final = True
            else:
                self.f = fst.linear_chain(src, g.symtab)

        if isinstance(src, fst.StdVectorFst):
            self.f = src

        if isinstance(src, Fst):
            self.f = src.f  # don't make a copy?

    @staticmethod
    def convert(what):
        """Fst.convert(what): make what into an Fst, if it isn't already."""
        if isinstance(what, Fst):
            return what
        return Fst(what)

    # This cannot be produced via a standard constructor
    @staticmethod
    def failure():
        """Fst.failure(): An fst that always fails"""
        r = Fst()
        r.f = fst.Acceptor(g.symtab)
        return r

    def __len__(self):
        return len(self.f)

    def num_arcs(self):
        return self.f.num_arcs()

    def __repr__(self):
        return '<Fst with {0} states, {1} arcs>'.format(len(self), self.num_arcs())

    def copy(self):
        return Fst(self.f.copy())

    def write(self, filename, keep_isyms=False, keep_osyms=False):
        """Save Fst to a file."""
        self.f.write(filename, keep_isyms, keep_osyms)

    def determinize(self):
        """Determinize an Fst, return the result."""
        # in pyfst, determinize is non-destructive
        return Fst(self.f.determinize())

    def minimize(self):
        """Minimize an Fst, return the result."""
        # in pyfst, minimize() is destructive. Make it non-destructive.
        r = self.copy()
        r.f.minimize()
        return r

    def remove_epsilon(self):
        """Remove epsilons from Fst, return the result."""
        # in pyfst, remove_epsilon() is destructive. Make it non-destructive.
        r = self.copy()
        r.f.remove_epsilon()
        return r

    def optimize(self):
        """Optimize Fst, return the result."""
        # in pyfst, optimize is non-destructive
        return Fst(self.f.optimize())

    def compose(self, other):
        """Compose two Fsts, return the result. Abbreviation: x >> y"""
        other = Fst.convert(other)
        t = other.f.copy()
        t.arc_sort_input()
        return Fst(self.f.compose(t))

    def __rshift__(x, y):
        return x.compose(y)

    def __rrshift__(y, x):
        x = Fst.convert(x)
        return x.__rshift__(y)

    def intersect(self, other):
        """Intersection of two Fsts. Abbreviation: x & y"""
        other = Fst.convert(other)
        return Fst(self.f.intersect(other.f))

    def __and__(x, y):
        return x.intersect(y)

    def __rand__(y, x):
        x = Fst.convert(x)
        return x.__and__(y)

    def union(self, other):
        """Union of two Fsts. Abbreviation: x | y"""
        other = Fst.convert(other)
        return Fst(self.f.union(other.f))

    def __or__(x, y):
        return x.union(y)

    def __ror__(y, x):
        x = Fst.convert(x)
        return x.__or__(y)

    @staticmethod
    def union_seq(ss):
        """Union of a sequence of Fsts."""
        r = Fst.failure()
        for s in ss:
            s = Fst.convert(s)
            r.f.set_union(s.f)
        return r

    def concatenation(self, other):
        """Concatenate two Fsts, return the result. Abbreviation: x + y"""
        other = Fst.convert(other)
        return Fst(self.f.concatenation(other.f))

    def __add__(x, y):
        return x.concatenation(y)

    def __radd__(y, x):
        x = Fst.convert(x)
        return x.__add__(y)

    def difference(self, other):
        """Difference of two acceptor Fsts. Abbreviation: x - y"""
        other = Fst.convert(other)
        return Fst(self.f.difference(other.f))

    def __sub__(x, y):
        return x.difference(y)

    def __rsub__(y, x):
        x = Fst.convert(x)
        return x.__sub__(y)

    def closure(self):
        """Repeat an Fst zero or more times, return the result. Abbreviation: ss(x)"""
        return Fst(self.f.closure())

    def ss(self):
        """Repeat an Fst zero or more times, return the result."""
        return self.closure()

    def closure_plus(self):
        """Repeat an Fst one or more times, return the result. Abbreviation: pp(x)"""
        return Fst(self.f.closure_plus())

    def pp(self):
        """Repeat an Fst one or more times, return the result."""
        return self.closure_plus()

    def option(self):
        """Repeat Fst either zero or one time. Abbreviation: qq(x)"""
        new_start = empty()
        r = new_start.concatenation(self)
        r.f[0].final = True
        return r

    def qq(self):
        """Repeat Fst either zero or one time."""
        return self.option()

    def inverse(self):
        """Inverse of an Fst."""
        return Fst(self.f.inverse())

    def reverse(self):
        """Reverse the direction of an Fst."""
        return Fst(self.f.reverse())

    def project_input(self):
        """Copy the input of the Fst to its output."""
        r = self.copy()
        r.f.project_input()
        return r

    def project_output(self):
        """Copy the output of the Fst to its input."""
        r = self.copy()
        r.f.project_output()
        return r

    def relabel(self, imap=None, omap=None):
        """Relabel arcs of an Fst according to imap, omap."""
        imap = imap or {}
        omap = omap or {}
        r = self.copy()
        r.f.relabel(imap, omap)
        return r

    def connect(self):
        r = self.copy()
        r.f.connect()
        return r

    def arc_sum_map(self):
        return Fst(self.f.arc_sum_map())

    def drop_input(self):
        """
        Replace all input symbols in wf with epsilons.
        """
        drop = dict([(s, 'ε') for s in g.symsets.any])
        keep = dict([(s, s) for s in g.symsets.any])
        r = self.copy()
        r.f.relabel(drop, keep)
        return r

    def drop_output(self):
        """
        Replace all output symbols in wf with epsilons.
        """
        drop = dict([(s, 'ε') for s in g.symsets.any])
        keep = dict([(s, s) for s in g.symsets.any])
        r = self.copy()
        r.f.relabel(keep, drop)
        return r

    def replace(self, with_what):
        """
        A transducer that accepts what self does and produces what with_what does.
        This is _not_ the same as fst.replace().
        Shortcut self // with_other
        """
        with_what = Fst.convert(with_what)
        return self.drop_output() + with_what.drop_input()

    def __floordiv__(x, y):
        return x.replace(y)

    def __rfloordiv__(y, x):
        x = Fst.convert(x)
        return x.__floordiv__(y)

    def invert(self):
        r = self.copy()
        r.f.invert()
        return r


def failure():
    """
    An fst that always fails.
    """
    return Fst.failure()


def empty():
    """
    An fst that accepts an empty sequence.
    """
    return Fst()


# x?
def qq(x):
    """
    Either x or an empty string.
    """
    x = Fst.convert(x)
    return x.qq()


# x*
def ss(x):
    """
    Repeat x zero or more times.
    """
    x = Fst.convert(x)
    return x.ss()


# x+
def pp(x):
    """
    Repeat x one or more times.
    """
    x = Fst.convert(x)
    return x.pp()


# x{n,m}
def rr(x, n, m=None):
    """
    Repeat x from n (minimum) to m (maximum) times.
    If m is None, exactly n repetitions are produced.
    """
    if not m:
        m = n
    x = Fst.convert(x)

    r = empty()
    for i in xrange(0, n):
        r = r + x
    for i in xrange(n, m):
        r = r + qq(x)
    return r


def anyof(seq):
    """
    Disjunction of all elements of seq.
    """
    r = failure()
    for s in seq:
        s = Fst.convert(s)
        r = r | s
    r.remove_epsilon()  # is this safe?
    return r


def category(name, *grammemes):
    """
    Declare a category and its grammemes.
    """
    cat_dict[name] = set(grammemes)


def cat_values(catname):
    """
    List of all possible values for a category.
    """
    return cat_dict[catname]


def pos(name, *cats):
    """
    Declare a part of speech and its categories.
    """
    pos_dict[name] = cats


# Call after defining all the categories and POS's
def categories_defined():
    """
    Freeze the symbol table that will be used for all the FSTs we generate,
    update fsts for symbol classes to use the new symbol table.
    """
    g.update()


def feats(pos, *grammemes):
    """
    Generate an FST for a given POS, with given grammemes.
    If no grammemes are specified for a given category, all values are accepted.
    If more than one grammeme is specified, the result is a disjunction of values.
    """
    grammemes = set(grammemes)
    cats = pos_dict[pos]
    f = fst.Acceptor(g.symtab)
    i = 0
    for c in cats:
        all_gms_for_c = cat_dict[c]
        cg = all_gms_for_c & grammemes
        if len(cg) == 0:
            cg = all_gms_for_c
        for gm in cg:
            f.add_arc(i, i+1, gm)
        i = i+1
    f[i].final = True
    return Fst(f)


def drop_input(wf):
    """
    Replace all input symbols in wf with epsilons.
    """
    wf = Fst.convert(wf)
    return wf.drop_input()


def drop_output(wf):
    """
    Replace all output symbols in wf with epsilons.
    """
    wf = Fst.convert(wf)
    return wf.drop_output()


def drop_input_nonterminals(wf):
    """
    Replace all nonterminal input symbols in wf with epsilons.
    """
    wf = Fst.convert(wf)
    drop = dict([(s, 'ε') for s in g.symsets.any])
    for c in alphabet:
        # don't drop terminals
        drop[c] = c
    keep = dict([(s, s) for s in g.symsets.any])
    return wf.relabel(drop, keep)


def drop_output_nonterminals(wf):
    """
    Replace all nonterminal output symbols in wf with epsilons.
    """
    wf = Fst.convert(wf)
    drop = dict([(s, 'ε') for s in g.symsets.any])
    for c in alphabet:
        # don't drop terminals
        drop[c] = c
    keep = dict([(s, s) for s in g.symsets.any])
    return wf.relabel(keep, drop)


def drop_input_terminals(wf):
    """
    Replace all terminal input symbols in wf with epsilons.
    """
    wf = Fst.convert(wf)
    drop = dict([(s, 'ε') for s in alphabet])
    keep = dict()
    return wf.relabel(drop, keep)


def drop_output_terminals(wf):
    """
    Replace all terminal output symbols in wf with epsilons.
    """
    wf = Fst.convert(wf)
    drop = dict([(s, 'ε') for s in alphabet])
    keep = dict()
    return wf.relabel(keep, drop)


def replace(in_fst, out_fst):
    """
    Create a transducer that accepts in_fst and outputs out_string.
    """
    in_fst = Fst.convert(in_fst)
    return in_fst.replace(out_fst)


def replace_stressed(in_str, out_fst):
    """
    Create a transducer that accepts in_str (maybe stressed str) and outputs out_fst.
    """
    return replace(maybe_stressed(in_str), out_fst)


def insert(out_fst):
    """
    FST that accepts empty input and returns out_fst.
    Same as drop_input.
    """
    return drop_input(out_fst)


def maybe_insert(fst):
    """
    Fst that accepts its argument or inserts it.
    """
    return fst | insert(fst)


def remove(in_fst):
    """
    FST that accepts empty input and returns in_fst.
    Same as drop_output.
    """
    return drop_output(in_fst)


def maybe_stressed(in_str):
    """
    Create FST that accepts word and stressed word.
    """
    cost_value = 0.01
    if isinstance(in_str, Fst):
        return in_str
    if '+' in in_str or "ё" in in_str:
        result_fst = cost(in_str, cost_value)
        unstressed_str = in_str.replace('+', '', 1)
        result_fst |= cost(Fst.convert(unstressed_str), cost_value)
        result_fst |= cost(Fst.convert(in_str.replace('ё', 'е', 1)), cost_value)
        result_fst |= Fst.convert(unstressed_str.replace('ё', 'е', 1))
    else:
        result_fst = Fst.convert(in_str)
    return result_fst.optimize()


def tagger(stems, paradigm, drop_flection=True):
    if isinstance(stems, list):
        stems_fst = failure()
        for st in stems:
            st = maybe_stressed(st)
            stems_fst = stems_fst | st
        stems_fst = stems_fst.optimize()
    else:
        stems_fst = stems

    if isinstance(paradigm, list):
        paradigm_fst = failure()
        for fl, feat in paradigm:
            fl = maybe_stressed(fl)
            paradigm_fst = paradigm_fst | (fl + feat)
        paradigm_fst = paradigm_fst.optimize()
    else:
        paradigm_fst = paradigm

    if drop_flection:
        border = Fst.convert("")
        flections = drop_output_terminals(drop_input_nonterminals(paradigm_fst))
        flections = flections.optimize()
    else:
        border = replace("", "-")
        flections = drop_input_nonterminals(paradigm_fst)

    r = stems_fst + border + flections
    return r.optimize()


# Sometimes it is easier to just list all the forms.
def direct_tagger(*descr):
    r = failure()
    for (st, feats) in descr:
        st = maybe_stressed(st)
        r = r | (st + drop_input_nonterminals(feats))
    return r.optimize()


def separate(pattern):
    """
    Return an FST that matches an inserted space followed by any non-empty `pattern` match,
    and additionally reacts to empty strings like `pattern` would do.
    """
    empty_pattern = empty() >> pattern
    return empty_pattern | insert(' ') + (pp(g.any_symb) >> pattern)


def word(pat, **kwargs):
    """
    A word matching pat. In the default mode, word should be preceded by whitespace.
    Keyword arguments:
       need_outer_space=False: preceding whitespace is optional;
       permit_inner_space=True: there may be whitespace inside pat;
       permit_empty_words=False: the match of pat should not be an empty string.
    When permit_empty_words=True (default), empty sequences are accepted, without consuming the
    leading whitespace, so long as the empty string satisfies pat.
    """
    need_outer_space = kwargs.get('need_outer_space', True)
    permit_inner_space = kwargs.get('permit_inner_space', False)
    permit_empty_words = kwargs.get('permit_empty_words', True)

    space = pp(g.whitespace)
    if not need_outer_space:
        space = qq(space)

    pat = Fst.convert(pat)
    if not permit_inner_space:
        pat = (ss(g.word_sym_fst) >> pat)

    if permit_empty_words:
        empty_p = empty() >> pat
    else:
        empty_p = failure()

    res = (space + pat) | empty_p
    return res.optimize()


def unword(fst):
    """
    Take an FST that accepts a space at the beginning (like those generated by word()) and get rid of that behavior.
    """
    return insert(' ') + ss(g.any_symb) >> fst >> remove(' ') + ss(g.any_symb)


def glue_words(*args, **kwargs):
    """
    Args is a sequence of patterns.
    Match several words, each matching its corresponding element of args. Remove whitespace between them.
    In the default mode, each word should be preceded by whitespace.
    Keyword arguments:
       need_outer_space=False: preceding whitespace is optional;
       permit_inner_space=True: there may be whitespace inside pat, an element of *args;
       permit_empty_words=False: the match of pat should not be an empty string.
    When permit_empty_words=True (default), empty sequences are accepted, without consuming the
    leading whitespace, so long as the empty string satisfies pat.
    """
    need_outer_space = kwargs.get('need_outer_space', True)
    permit_inner_space = kwargs.get('permit_inner_space', False)
    permit_empty_words = kwargs.get('permit_empty_words', True)

    space = pp(remove(" "))
    if not need_outer_space:
        space = qq(space)

    r = insert(" ")
    for pat in args:
        pat = Fst.convert(pat)

        # pat may have already been wrapped in word() and have a space at the start.
        # Hence the unword() option.
        pat = pat | unword(pat)

        if not permit_inner_space:
            pat = ss(g.word_sym_fst) >> pat

        if permit_empty_words:
            empty_p = empty() >> pat
        else:
            empty_p = failure()
        r = r + (space + pat | empty_p)

    return r.optimize()


def word_list_from_file(fname):
    """
    Contents of fname should be a list of words, each on its own line.
    Return an FST that matches any of these words.
    """
    with codecs.open(fname) as f:
        r = Fst.union_seq([ln.strip().decode('utf-8') for ln in f])
    return r.optimize()


def replacement_list_from_file(fname):
    """
    Contents of fname should be a list of replacements, each on its own line,
    source and target separated by tab.
    Return an FST that replaces any of the sources with the corresponding target.
    """
    def do_line(ln):
        ln = ln.strip().decode('utf-8')
        if ln == '':
            return failure()
        [source, target] = ln.split('\t')
        return replace(source, target)

    with codecs.open(fname) as f:
        r = Fst.union_seq(do_line(ln) for ln in f)
    return r.optimize()


def replacement_list_from_file_map(fname, target_compute):
    """
    Contents of fname should be a list of words, each on its own line.
    The target replacement is build by calling a mapper function
    (this may be ROT13 encoding, for example, or as simple as returning constant "censored")
    Return an FST that replaces any of the sources with the corresponding computed target.
    """
    def do_line(ln):
        source = ln.strip().decode('utf-8')
        if source == '':
            return failure()
        target = target_compute(source)
        return replace(source, target)

    with codecs.open(fname) as f:
        r = Fst.union_seq(do_line(ln) for ln in f)
    return r.optimize()


def cost(f, w=1.0):
    """
    Match whatever f specifies, but assign it an additional cost w.
    """
    f = Fst.convert(f)
    aux = fst.Acceptor(g.symtab)
    aux.add_arc(0, 1, 'ε', w)
    aux[1].final = True
    r = Fst(aux) + f
    return r.optimize()


def words_with_unigram_logprobs(dictionary, char_coalescer=None):
    """
    Match any of the words in `dictionary` (a `collections.Counter` instance)
    paying the cost of negative unigram logprobs derived from its frequencies.
    If `char_coalescer` is non-None, a word's character is represented by the acceptor that `char_coalescer(c)` returns.
    """
    assert isinstance(dictionary, collections.Counter)
    r = failure()
    total = sum(dictionary.itervalues())
    assert total != 0
    for word, count in dictionary.iteritems():
        loss = -math.log(float(count) / total)
        if char_coalescer is not None:
            word_fst = empty()
            for char in word:
                word_fst += char_coalescer(char)
        else:
            word_fst = Fst(word)
        r |= cost(word_fst.optimize(), loss)
    return r.optimize()


# Pattern matches s or any case variant thereof
def anycase(s):
    s = unicode(s)
    res = empty()
    for c in s:
        res += anyof([c.lower(), c.upper()])
    return res.optimize()


# Convert only words that fit a pattern, leave the rest as it is
# The small penalty for f is present so that longer f-matches are preferred.
def convert_words(f, f_cost=0.0001, no_match_cost=1.0, **kwargs):
    r = (ss(cost(word(f, **kwargs), f_cost) |
            g.direct_transcription_word |
            g.read_by_char_word |
            g.chem_word |
            cost(word(pp(g.any_symb)), no_match_cost)) +
         # Allow some spaces at the end
         ss(" "))
    return r.optimize()


# Same, but don't restrict ourselves to words
def convert_symbols(f, f_cost=0.0001, no_match_cost=1):
    r = ss(cost(f, f_cost) |
           g.direct_transcription_word |
           g.read_by_char_word |
           g.chem_word |
           cost(g.any_symb, no_match_cost))
    return r.optimize()


def make_unconditional_replacements(replacements_file_path):
    """
    Reads linewise tab separated replacements from file and create corresponding FST.
    """
    r = empty()
    with open(replacements_file_path) as rf:
        for ln in rf:
            # Spaces at the left edge may be meaningful
            words = ln.rstrip().split('\t')
            if len(words) == 0:
                continue
            w_from = words[0]
            w_to = " ".join(words[1:])
            r = r | replace(w_from, " " + w_to + " ")
    return r.optimize()


def fst_save(f, fname_base):
    """
    Write an Fst to file 'fname_base.fst'
    """
    f.write(fname_base + "_vector.fst")
    fstconvert_path = os.environ.get('FSTCONVERT_PATH', 'fstconvert')
    r = os.system(fstconvert_path + " --fst_type=const --fst_align=true " + fname_base + "_vector.fst " + fname_base + ".fst")
    if r != 0:
        print >>sys.stderr, "Bad return code from fstconvert:", r
        return
    os.remove(fname_base + "_vector.fst")


def fst_syms_save(fname_base):
    """
    Write the packages symbol table to 'fname_base.sym'
    """
    g.symtab.write(fname_base + ".sym")


def fst_render(fst, name='noname'):
    """
    Render the FST graph to a temporary SVG file and open the associated viewer for it.
    Requires Graphviz.
    """
    with tempfile.NamedTemporaryFile(prefix='fst-', suffix='-' + name + '.dot') as graph:
        graph.write(fst.f.draw())
        graph.flush()
        subprocess.check_call(['dot', '-Tsvg', '-O', graph.name])

        viewer = {'Linux': 'xdg-open', 'Darwin': 'open'}
        subprocess.Popen(
            [viewer[platform.system()], graph.name + '.svg'],
            stdin=None, stdout=None, stderr=None, close_fds=True
        )


def fst_render_label(label_id):
    """
    Return the UTF-8 string corresponding to the label_id in the symbol table,
    providing an empty string for ε and surrounding nonterminals with [].
    """
    result = g.symtab.find(label_id)
    if result == u'ε':
        result = u''
    if len(result) > 1:
        result = u'[{}]'.format(result)
    return result.encode('utf-8')


class EmptyTransducerException(Exception):
    pass


def fst_random_path(fst, max_length=1000, final_exit_probability=0.8):
    """
    Walk a random path of length at most @max_length through the FST (the sampling ignores weights).
    Return the rendered in/out strings along this path once a final state is reached with @final_exit_probability,
    or None on failing to find one.
    """
    transducer = fst.f

    in_result = out_result = ''

    states = list(transducer.states)

    if not states:
        raise EmptyTransducerException()

    state = states[transducer.start]

    for _ in xrange(max_length):
        arcs = list(state.arcs)

        if state.final and (not arcs or random.uniform(0, 1) < final_exit_probability):
            return in_result, out_result

        if not state.final and not arcs:
            raise Exception('Non-final state with no outgoing edges reached by {}/{}'.format(in_result, out_result))

        arc = random.choice(arcs)
        in_label = arc.ilabel
        out_label = arc.olabel

        in_result += fst_render_label(in_label)
        out_result += fst_render_label(out_label)

        state = states[arc.nextstate]

    return None


def fst_random_sample(fst, samples=2000, compress_spaces=True, **kwargs):
    """
    Make @samples random walks with @fst_random_sample and collect the distinct ones.
    @compress_spaces is useful to compress repeated whitespace in the paths.
    """
    def do_compress_space(string):
        return ' '.join(string.split())

    results = set()
    fst = fst.optimize()
    for _ in xrange(samples):
        generated = fst_random_path(fst, **kwargs)
        if generated is not None:
            results.add(tuple(map(do_compress_space, generated)) if compress_spaces else generated)
    return sorted(results)


def fst_print_random_sample(*args, **kwargs):
    """
    A wrapper to @fst_random_sample that just prints the results.
    """
    try:
        for input, output in fst_random_sample(*args, **kwargs):
            print '{%s} => {%s}' % (input, output)
    except EmptyTransducerException:
        print '[Empty transducer a.k.a. failure()]'


def any_name_from_file(file_name):
    fst = failure()
    possible_replacements = {"ё": "е", "-": " "}
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


class FSTSequenceSaver(object):
    def __init__(self, target_path):
        self._target_path = target_path
        self._sequence_file = None

    def __enter__(self):
        shutil.rmtree(self._target_path, ignore_errors=True)
        os.mkdir(self._target_path)

        fst_syms_save(os.path.join(self._target_path, 'symbols'))

        self._sequence_file = open(os.path.join(self._target_path, 'sequence.txt'), 'w')
        return self

    def add(self, fst, name):
        assert self._sequence_file is not None

        fst_save(fst, os.path.join(self._target_path, name))
        print >> self._sequence_file, name

    def __exit__(self, *args):
        self._sequence_file.close()

        with open(os.path.join(self._target_path, 'flags.txt'), 'w') as flags:
            print >> flags, 'report-intermediate false'
