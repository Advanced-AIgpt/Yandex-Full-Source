# coding: utf-8

import collections
import copy
import logging
import re

from liblemmer_python_binding import AnalyzeWord
from yandex_inflector_python import Inflector as ya_inflector, FioInflector as ya_fio_inflector
from yandex_inflector_python import Pluralize
from yandex_inflector_python import Singularize

from nlu_service.entities.sample import Sample
from nlu_service.utils.string import ensure_unicode
from nlu_service.services.vins_normalizer import NormalizeSampleProcessor


logger = logging.getLogger(__name__)


GRAM_POS = {
    'NOUN': 'S',
    'ADJF': 'A',
    'ADJS': 'A',
    'COMP': 'ADV',
    'VERB': 'V',
    'INFN': 'inf',
    'PRTF': 'partcp',
    'PRTS': 'partcp',
    'GRND': 'ger',
    'NUMR': 'NUM',
    'ADVB': 'ADV',
    'NPRO': 'SPRO',
    'PRED': 'ADVPRO',
    'PREP': 'PR',
    'CONJ': 'CONJ',
    'PRCL': 'PART',
    'INTJ': 'INTJ',
}

GRAM_CASE = {
    'nomn': 'nom',
    'gent': 'gen',
    'datv': 'dat',
    'accs': 'acc',
    'ablt': 'ins',
    'loct': 'abl',
    'voct': 'voc',
    'loc2': 'loc'
}

GRAM_NUMBER = {
    'sing': 'sg',
    'plur': 'pl',
}

GRAM_GENDER = {
    'masc': 'm',
    'femn': 'f',
    'neut': 'n'
}

GRAM_POS_REV = {v: x for x, v in GRAM_POS.iteritems()}
GRAM_CASE_REV = {v: x for x, v in GRAM_CASE.iteritems()}
GRAM_NUMBER_REV = {v: x for x, v in GRAM_NUMBER.iteritems()}
GRAM_GENDER_REV = {v: x for x, v in GRAM_GENDER.iteritems()}

GRAM_ALL = {}
GRAM_ALL.update(GRAM_POS)
GRAM_ALL.update(GRAM_CASE)
GRAM_ALL.update(GRAM_NUMBER)
GRAM_ALL.update(GRAM_GENDER)

inflect_cases = ['nomn', 'gent', 'datv', 'accs', 'ablt', 'loct']
inflect_genders = ['masc', 'femn', 'neut', 'plur']
inflect_numbers = ['sing', 'plur']


class Tag(object):
    def __init__(self, lex_grams, flex_grams):
        self.common = set()
        self.POS = None
        self.case = None
        self.number = None
        self.gender = None
        self.lex_grams = self._gram_set(lex_grams)
        self.flex_grams = self._gram_set(flex_grams)
        self._inflect(self.lex_grams | self.flex_grams)

    def _gram_set(self, grams):
        if isinstance(grams, tuple):
            gram_set = set(grams)
        elif isinstance(grams, list):
            gram_set = set(grams)
        elif isinstance(grams, set):
            gram_set = grams.copy()
        else:
            gram_set = {grams}
        return {ensure_unicode(gr) for gr in gram_set}

    def __contains__(self, key):
        if isinstance(key, set):
            return key.issubset(self.common)
        elif isinstance(key, list):
            return set(key).issubset(self.common)
        else:
            return key in self.common

    def __str__(self):
        return ', '.join(self.common)

    def _inflect(self, grams):
        for gr in grams:
            if gr in GRAM_POS:
                self.POS = gr
            elif gr in GRAM_POS_REV:
                self.POS = GRAM_POS_REV[gr]
                if self.POS == 'ADJF' and 'brev' in grams:
                    self.POS = 'ADJS'
                elif self.POS == 'PRTF' and 'brev' in grams:
                    self.POS = 'PRTS'
            elif gr in GRAM_CASE:
                self.case = gr
            elif gr in GRAM_CASE_REV:
                self.case = GRAM_CASE_REV[gr]
            elif gr in GRAM_NUMBER:
                self.number = gr
            elif gr in GRAM_NUMBER_REV:
                self.number = GRAM_NUMBER_REV[gr]
            elif gr in GRAM_GENDER:
                self.gender = gr
            elif gr in GRAM_GENDER_REV:
                self.gender = GRAM_GENDER_REV[gr]
            elif gr in self.flex_grams:
                self.common.add(gr)

    def flexgram(self):
        return self.common | {self.case, self.gender, self.number}


class FakeInfo(object):
    def __init__(self, word):
        self.LexicalFeature = set()
        self.FormFeature = [set()]
        self.Lemma = word
        self.Form = word


class Parse(object):
    def __init__(self, info, inflector):
        self._info = info
        self._inflector = inflector
        flexgram = info.FormFeature[0] if len(info.FormFeature) > 0 else []
        self.tag = Tag(info.LexicalFeature, flexgram)
        self.normal_form = info.Lemma
        self.word = info.Form
        self._paradigm = None

    def inflect(self, grams):
        word = self._inflector.inflect(self.word, grams)
        if not word:
            return copy.copy(self)
        else:
            result = copy.copy(self)
            result.word = word
            result.tag = Tag(self.tag.lex_grams, grams)
            return result

    def make_agree_with_number(self, number):
        word = self._inflector.pluralize(self.word, number)
        if not word:
            return copy.copy(self)
        else:
            result = copy.copy(self)
            result.word = word
            result.tag = Tag(self.tag.lex_grams, self._agree_gram(number))
            return result

    def _agree_gram_21(self):
        if self.tag.POS == 'NOUN' and self.tag.case not in ('nomn', 'accs'):
            return {'sing', self.tag.case}
        elif self.tag.case == 'nomn':
            return {'sing', 'nomn'}
        else:
            return {'sing', 'accs'}

    def _agree_gram_25(self):
        if self.tag.POS == 'NOUN' and self.tag.case not in ('nomn', 'accs'):
            return {'plur', self.tag.case}
        elif self.tag.POS == 'NOUN':
            return {'sing', 'gent'}
        elif self.tag.POS in ('ADJF', 'PRTF') and self.tag.gender == 'femn':
            return {'plur', 'nomn'}
        else:
            return {'plur', 'gent'}

    def _agree_gram_13(self):
        if self.tag.POS == 'NOUN' and self.tag.case not in ('nomn', 'accs'):
            return {'plur', self.tag.case}
        else:
            return {'plur', 'gent'}

    def _agree_gram(self, number):
        if self.tag.POS not in ('NOUN', 'ADJF', 'PRTF'):
            return set()
        elif (number % 10 == 1) and (number % 100 != 11):
            return self._agree_gram_21()
        elif (number % 10 >= 2) and (number % 10 <= 4) and (number % 100 < 10 or number % 100 >= 20):
            return self._agree_gram_25()
        else:
            return self._agree_gram_13()


class Inflector(object):
    _LANGS_LEMMER_TO_INFLECTOR = {
        'ru': b'rus',
        'en': b'eng',
        'tr': b'tur',
        'uk': b'ukr',
        'fr': b'fra'
    }

    def __init__(self, lang):
        if lang in self._LANGS_LEMMER_TO_INFLECTOR:
            self._lang = self._LANGS_LEMMER_TO_INFLECTOR[lang]
        else:
            self._lang = ensure_unicode(lang)

        if self._lang == 'mis' or self._lang == 'unk':  # do nothing if language is unknown
            self._inflector = None
            self._fio_inflector = None
        else:
            self._inflector = ya_inflector(self._lang)
            if self._lang == 'rus':
                self._fio_inflector = ya_fio_inflector(self._lang)
            else:
                # in English, fio inflection is not supported by the underlying library
                self._fio_inflector = self._inflector

    def inflect(self, words, grams, fio=False):
        """
        Inflect single word or whole phrase.
        :param words: some named entity that should be inflected.
        :param grams: grammems. You could use yandex format (https://lemmer.viewer.yandex-team.ru/lemmer.py?help=1) or
        pymorphy (pymorphy2.readthedocs.io/en/latest/user/grammemes.html) If you specify pymorphy grammemes,
        code will translate them to yandex format.
        :param fio: whether to use the inflector for personal names
        :return: inflected words
        """
        if self._inflector is None:
            return words
        target_grams = {GRAM_ALL.get(gr, gr) for gr in grams}

        inflector = self._fio_inflector if fio else self._inflector

        result = inflector.Inflect(words, (','.join(target_grams)).encode("utf-8"))

        if not result:
            logger.debug('Unable to inflect \'%s\'' % words)
            # avoid to return empty string (inflector returns '' when words do not correspond to language)
            return words
        return result

    def pluralize(self, words, number, case='nom'):
        """
        produce plural form of phrase by given number and case
        pluralize('калачик', '25') == 'калачиков'
        pluralize('калачик', '25', 'dat') == 'калачикам'
        """

        case = GRAM_ALL.get(case, case)

        if isinstance(number, float) and not float.is_integer(number):
            res = self.inflect(words, {'gen', 'sg'})
        else:
            number = int(number)
            if number == 0 or number >= 1000 and number % 1000 == 0:
                res = self.inflect(words, {'gen', 'pl'})
            else:
                numstr = unicode(number)
                if numstr[-1] == '1' and (len(numstr) == 1 or numstr[-2] != '1'):
                    res = self.inflect(words, {case, 'sg'})
                elif case == 'nom' or case == 'acc':
                    res = Pluralize(words, number, self._lang)
                else:
                    res = self.inflect(words, {case, 'pl'})

        if not res:
            logger.debug('Unable to pluralize \'%s\'' % words)
            # avoid to return empty string (Pluralize returns '' when words do not correspond to language)
            return words
        return res

    def singularize(self, words, number):
        """
        produce singular form of named entity by given number
        singular('калачиков', '25') == 'калачик'
        such function may be useful for cases when we need disambiguate fio 'калачиков' and object 'калачик'
        """
        return Singularize(words, number, self._lang)


class Lemmer(object):
    DEFAULT_LANGS = ['ru', 'en']

    def __init__(self, langs=None):
        if langs is None:
            self._langs = self.DEFAULT_LANGS
        else:
            self._langs = langs
        assert len(self._langs) > 0
        self._inflectors = {lang: Inflector(lang) for lang in self._langs}
        self._inflectors['unk'] = Inflector('unk')

    def parse(self, word, **kwargs):

        analyze_result = AnalyzeWord(word, langs=self._langs, split=True, **kwargs)
        if len(analyze_result) == 0:
            return [Parse(FakeInfo(word), self._inflectors[self._langs[0]])]
        else:
            result = []
            for x in sorted(analyze_result, key=self._cmp_lemms, reverse=True):
                lemm_lang = x.Language if x.Language in self._inflectors else 'unk'
                result.append(Parse(x, self._inflectors[lemm_lang]))
            return result

    @staticmethod
    def _cmp_lemms(x):
        return (x.Last - x.First) * x.Weight


rumorph = Lemmer(['ru', 'en'])
ruinflector = Inflector('ru')

def is_single(phrase):
    """ returns true if all parse variants of all words to which number category is applicable are singular
        if no "numberable" words detected also returns false
    :param phrase: input phrase
    :type phrase: unicode
    :rtype: bool
    """
    single_found = False
    for word in phrase.split():
        result = rumorph.parse(word)
        for x in result:
            if x.tag.number is None:
                continue
            if x.tag.number == 'sing':
                single_found = True
            else:
                return False
    return single_found


def _extract_weight(e):
    if isinstance(e, (list, tuple)) and len(e) == 2:
        name, weight = e
        if isinstance(name, basestring) and (weight is None or isinstance(weight, float)):
            return (name, weight) if weight is not None and weight != 1.0 else (name, None)
    elif isinstance(e, basestring):
        return e, None

    raise ValueError('Incorrect synonym format: %s, must be <basestring> or (<basestring>, <float>|None)' % e)


def _put_cases(phrase, cases, fio=False):
    return [ruinflector.inflect(phrase, case, fio=fio) for case in cases]


def put_cases(phrase, plur=False, fio=False, list_=""" inflects phrase for all cases

    :param phrase: phrase to be inflected
    :type phrase: unicode
    :param plur: whether to use plural form
    :param fio: whether to use the special inflector for personal names
    :return: list of wordforms
    :rtype: list
    """):
    return _put_cases(phrase, (({c, 'plur'} if plur else {c}) for c in inflect_cases), fio)


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


def default_normalization(text):
    return re.sub('\s+', ' ', text).strip()


SAMPLE_PROCESSOR = NormalizeSampleProcessor('normalizer_ru')


def inflect(e_canon, synonyms, _inflect_numbers=True):
    f = []
    for synonym in synonyms:
        processed_synonym = ensure_unicode(synonym)
        sample = Sample.from_string(processed_synonym)
        normalized_sample = SAMPLE_PROCESSOR._process(sample)
        e = ensure_unicode(normalized_sample.text)

        en = default_normalization(e)
        if len(en) == 0:
            continue

        if _inflect_numbers and is_single(en):
            inflected_forms = put_cases_cartesian_numbers(en)
        else:
            inflected_forms = put_cases(en)
        logger.debug('Custom entity key="%s", value="%s". Inflected values: "%s"',
                     e_canon, en, ', '.join(inflected_forms))
        known_forms = set()
        for emorph in [en] + inflected_forms:
            if emorph not in known_forms:
                f.append(emorph)
                known_forms.add(emorph)
    return f


def inflect_batch(data, _inflect_numbers=True):
    inflected_to_canon = collections.defaultdict(set)
    for e_canon, synonyms in data.iteritems():
        for inflected_form in inflect(e_canon, synonyms):
            inflected_to_canon[inflected_form].add(e_canon)
    batch_result = {}
    for inflected, canon_set in inflected_to_canon.iteritems():
        values = [
            {
                'id': canon,
                'activationPhrases': data[canon]
            }
            for canon in canon_set
        ]
        values.sort(
            key=lambda skill_info: inflected in skill_info['activationPhrases'],
            reverse=True,
        )
        batch_result[inflected] = values
    return batch_result
