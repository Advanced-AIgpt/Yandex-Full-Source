# coding: utf-8

from itertools import permutations

import vins_tools.nlu.ner.normbase as gn

from vins_core.utils.lemmer import Lemmer, Inflector

morph = {
    'ru': Lemmer(['ru', 'en']),
    'tr': Lemmer(['tr', 'en'])
}
inflector = {
    'ru': Inflector('ru'),
    'tr': Inflector('tr')
}
inflect_cases = ['nomn', 'gent', 'datv', 'accs', 'ablt', 'loct']
inflect_genders = ['masc', 'femn', 'neut', 'plur']
inflect_numbers = ['sing', 'plur']


def num2text(numbers, leading_zeros=False):
    arr = [unicode(i) for i in numbers]
    if leading_zeros:
        max_size = len(max(arr, key=lambda c: len(c)))
        arr = ['0' * (max_size - len(c)) + c for c in arr]
    return arr


def is_single(phrase, lang='ru'):
    """ returns true if all parse variants of all words to which number category is applicable are singular
        if no "numberable" words detected also returns false
    :param phrase: input phrase
    :type phrase: unicode
    :rtype: bool
    """
    single_found = False
    for word in phrase.split():
        result = morph[lang].parse(word)
        for x in result:
            if x.tag.number is None:
                continue
            if x.tag.number == 'sing':
                single_found = True
            else:
                return False
    return single_found


def _put_cases(phrase, cases, fio=False, lang='ru'):
    return [inflector[lang].inflect(phrase, case, fio=fio) for case in cases]


def put_cases(phrase, plur=False, fio=False, lang='ru'):
    """ inflects phrase for all cases

    :param phrase: phrase to be inflected
    :type phrase: unicode
    :param plur: whether to use plural form
    :param fio: whether to use the special inflector for personal names
    :return: list of wordforms
    :rtype: list
    """
    return _put_cases(phrase, (({c, 'plur'} if plur else {c}) for c in inflect_cases), fio, lang=lang)


def put_genders_cases(phrase, plur=True, fio=False):
    """ inflects phrase for all genders and plural form
    :param phrase: phrase to be inflected
    :type phrase: unicode
    :param plur: whether to add
    :param fio: whether to use the special inflector for personal names
    :return: list of wordforms
    :rtype: list
    """
    return _put_cases(phrase, (({gend, case} if gend != 'plur' or plur else {})
                               for gend in inflect_genders
                               for case in inflect_cases
                               ), fio=fio)


def put_cases_cartesian_numbers(phrase):
    """ inflects phrase for all cases and numbers.
    :param phrase: phrase to be inflected
    :type phrase: unicode
    :return: list of wordforms
    :rtype: list
    """

    return _put_cases(phrase, ({c, n} for c in inflect_cases for n in inflect_numbers))


def put_case(word, case='nomn', lang='ru'):
    return inflector[lang].inflect(word, set(case.split(',')))


def fst_cases_num(word):
    return gn.Fst.union_seq(put_cases_cartesian_numbers(word)) | gn.Fst(word)


def _add_spaces(x, y):
    return x + gn.pp(' ') + y


def fstring(s):
    words = s.split()

    f = reduce(
        _add_spaces,
        words[1:],
        gn.Fst(words[0])
    )
    return f


def fst_cases(word, plur=False, lang='ru'):
    result = gn.Fst.union_seq([w for w in put_cases(word, plur, lang=lang)])
    return result if plur else result | gn.Fst(word)


def fcopy(arr, prefix='', suffix=''):
    return gn.Fst.union_seq([
        gn.Fst(unicode(x)) + gn.insert(prefix + unicode(x) + suffix)
        for x in arr
    ])


def fst_alt(prefix, f, suffix):
    return (prefix + f) | (f + suffix)


def fjoin(fst_arr, fsep=None):
    if fsep is None:
        fsep = gn.pp(' ')
    if not fst_arr:
        return gn.empty()
    f = [fst_arr[0]]
    for ff in fst_arr[1:]:
        f.append(fsep + ff)
    return sum(f, gn.empty())


def fallperm(fst_arr, fsep):
    K = len(fst_arr)
    f = []
    for k in xrange(1, K + 1):
        for fs in permutations(fst_arr, k):
            f.append(fjoin(fs, fsep))
    return gn.Fst.union_seq(f)


def ordinal_suffix():
    return gn.qq(gn.pp(' ') | '-') + gn.Fst.union_seq([
        'ый', 'ой', 'ий', 'им', 'ом',
        'го', 'му', 'ым', 'ою', 'ем',
        'е', 'я', 'й', 'ю',
        'st', 'nd', 'rd', 'th'
    ])
