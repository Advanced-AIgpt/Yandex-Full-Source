# -*- coding: utf-8 -*-

from __future__ import unicode_literals

import re
import attr
import six

from text_processing import Tokenizer as CatboostTokenizer


SENT_RE = re.compile(r'[^\.?!…]+[\.?!…]*["»“]*')

_LAST_WORD = re.compile(r'(?:\b|\d)([a-zа-я]+)\.$', re.IGNORECASE)
_FIRST_WORD = re.compile(r'^\W*(\w+)')
_ENDS_WITH_ONE_LETTER_LAT_AND_DOT = re.compile(r'(\d|\W|\b)([a-zA-Z])\.$')
_HAS_DOT_INSIDE = re.compile(r'[\w]+\.[\w]+\.$', re.IGNORECASE)
_INITIALS = re.compile(r'(\W|\b)([A-ZА-Я]{1})\.$')
_ONLY_RUS_CONSONANTS = re.compile(r'^[бвгджзйклмнпрстфхцчшщ]{1,4}$', re.IGNORECASE)
_STARTS_WITH_EMPTYNESS = re.compile(r'^\s+')
_ENDS_WITH_EMOTION = re.compile(r'[!?…]|\.{2,}\s?[)"«»,“]?$')
_STARTS_WITH_LOWER = re.compile(r'^\s*[–-—-("«]?\s*[a-zа-я]')
_STARTS_WITH_DIGIT = re.compile(r'^\s*\d')
_NUMERATION = re.compile(r'^\W*[IVXMCL\d]+\.$')
_PAIRED_SHORTENING_IN_THE_END = re.compile(r'\b(\w+)\. (\w+)\.\W*$')

_JOIN = 0
_MAYBE = 1
_SPLIT = 2

JOINING_SHORTENINGS = {'mr', 'mrs', 'ms', 'dr', 'vs', 'англ', 'итал', 'греч', 'евр', 'араб', 'яп', 'слав', 'кит',
                       'тел', 'св', 'ул', 'устар', 'им', 'г', 'см', 'д', 'стр', 'корп', 'пл', 'пер', 'сокр', 'рис'}
SHORTENINGS = {'co', 'corp', 'inc', 'авт', 'адм', 'барр', 'внутр', 'га', 'дифф', 'дол', 'долл', 'зав', 'зам', 'искл',
               'коп', 'корп', 'куб', 'лат', 'мин', 'о', 'обл', 'обр', 'прим', 'проц', 'р', 'ред', 'руб', 'рус', 'русск',
               'сан', 'сек', 'тыс', 'эт', 'яз', 'гос', 'мн', 'жен', 'муж', 'накл', 'повел', 'букв', 'шутл', 'ед'}

PAIRED_SHORTENINGS = {('и', 'о'), ('т', 'е'), ('т', 'к'), ('т', 'п'), ('т', 'д'), ('у', 'е'), ('н', 'э')}

_TOKENIZER = CatboostTokenizer(
    separator_type='BySense',
    token_types=['Word', 'Number', 'Punctuation', 'SentenceBreak', 'Unknown'],
    languages=['ru', 'en']
)


@attr.s(frozen=True)
class Substring(object):
    start = attr.ib()
    stop = attr.ib()
    text = attr.ib()


def _regex_split_separators(text):
    return [x.strip() for x in SENT_RE.findall(text)]


def _is_sentence_end(left, right,
                     shortenings,
                     joining_shortenings,
                     paired_shortenings):
    if not _STARTS_WITH_EMPTYNESS.match(right):
        return _JOIN

    if _HAS_DOT_INSIDE.search(left):
        return _JOIN

    left_last_word = _LAST_WORD.search(left)
    lw = ' '
    if left_last_word:
        lw = left_last_word.group(1)

        if lw.lower() in joining_shortenings:
            return _JOIN

        if _ONLY_RUS_CONSONANTS.search(lw) and lw[-1].islower():
            return _MAYBE

    pse = _PAIRED_SHORTENING_IN_THE_END.search(left)
    if pse:
        s1, s2 = pse.groups()
        if (s1, s2) in paired_shortenings:
            return _MAYBE

    right_first_word = _FIRST_WORD.match(right)
    if right_first_word:
        rw = right_first_word.group(1)
        if (lw, rw) in paired_shortenings:
            return _MAYBE

    if _ENDS_WITH_EMOTION.search(left) and _STARTS_WITH_LOWER.match(right):
        return _JOIN

    initials = _INITIALS.search(left)
    if initials:
        border, _ = initials.groups()
        if (border or ' ') not in "°'":
            return _JOIN

    if lw.lower() in shortenings:
        return _MAYBE

    last_letter = _ENDS_WITH_ONE_LETTER_LAT_AND_DOT.search(left)
    if last_letter:
        border, _ = last_letter.groups()
        if (border or ' ') not in "°'":
            return _MAYBE
    if _NUMERATION.match(left):
        return _JOIN
    return _SPLIT


def _align_substrings(text, substrings):
    start_index = 0
    for substring in substrings:
        start_index = text.index(substring, start_index)
        if len(substring) > 0:
            yield Substring(start_index, start_index + len(substring), text[start_index: start_index + len(substring)])
        start_index += len(substring)


def sentenize(original_text,
              shortenings=SHORTENINGS,
              joining_shortenings=JOINING_SHORTENINGS,
              paired_shortenings=PAIRED_SHORTENINGS):
    text = six.ensure_text(original_text)
    sents = _regex_split_separators(text)
    si = 0
    processed_index = 0
    sent_start = 0
    sentences = []
    while si < len(sents):
        s = sents[si]
        span_start = text[processed_index:].index(s) + processed_index
        span_end = span_start + len(s)
        processed_index += len(s)

        si += 1

        send = _is_sentence_end(text[sent_start: span_end], text[span_end:],
                                shortenings, joining_shortenings, paired_shortenings)
        if send == _JOIN:
            continue

        if send == _MAYBE:
            if _STARTS_WITH_LOWER.match(text[span_end:]):
                continue
            if _STARTS_WITH_DIGIT.match(text[span_end:]):
                continue

        sentence = text[sent_start: span_end].strip()
        if sentence:
            sentences.append(sentence)
        sent_start = span_end
        processed_index = span_end

    if sent_start != len(text):
        sentence = text[sent_start:].strip()
        if sentence:
            sentences.append(sentence)

    if isinstance(original_text, six.binary_type):
        sentences = [six.ensure_binary(sent) for sent in sentences]

    for substring in _align_substrings(original_text, sentences):
        yield substring


def tokenize(original_text, splits=None):
    text = original_text

    splits = splits or []
    for split_from, split_to in splits:
        text = split_from.sub(split_to, text)

    tokens = _TOKENIZER.tokenize(text)
    if isinstance(original_text, six.text_type):
        tokens = [six.ensure_text(token) for token in tokens]

    for substring in _align_substrings(original_text, tokens):
        yield substring
