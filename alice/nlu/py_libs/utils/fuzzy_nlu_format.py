# -*- coding: utf-8 -*-
import logging

import numpy as np
import re
import attr

from itertools import izip
from alice.nlu.py_libs.utils.strings import smart_unicode
from alice.nlu.py_libs.utils.sampling import isample_tuples_from_groups
from alice.nlu.py_libs.utils.lemmer import Inflector

logger = logging.getLogger(__name__)
_inflector = Inflector('ru')  # TODO: fix such hardcoded lang=ru and replace it to usage of project settings

SLOT_NAME_FORMAT = r'\+?[\w\-_\.]+'

slot_patterns = (
    re.compile(r'''('([^']+)'\(({0})\))'''.format(SLOT_NAME_FORMAT), re.U),
    re.compile(r'''("([^"]+)"\(({0})\))'''.format(SLOT_NAME_FORMAT), re.U),
)

template_pattern = re.compile(
    (
        r'''((^|[^@])@(?P<ats>(@@)*)(?P<template>{0})(\(((?P<number>[0-9]+)|(?P<all>all))?(:(?P<grams>[a-z0-9_,]+))?\))?)'''
    ).format(SLOT_NAME_FORMAT), re.U
)


@attr.s(slots=True, frozen=True, cmp=True)
class Slot(object):

    name = attr.ib()
    begin = attr.ib()
    end = attr.ib()
    is_continuation = attr.ib(default=False, converter=bool)
    options = attr.ib(default=(), converter=tuple)


def _to_tuple(data):
    if data is None:
        return ()
    return tuple(data)


@attr.s(slots=True, frozen=True, cmp=True)
class NluSourceItem(object):
    text = attr.ib(converter=smart_unicode)
    original_text = attr.ib(default=None)
    slots = attr.ib(default=(), converter=tuple)
    trainable_classifiers = attr.ib(default=attr.Factory(tuple), converter=_to_tuple)
    can_use_to_train_tagger = attr.ib(default=True)
    source_path = attr.ib(default=None)
    extra = attr.ib(default=None)

    @property
    def tokens(self):
        return self.text.split()

    def __bool__(self):
        return bool(self.text)

    def __nonzero__(self):
        return self.__bool__()


@attr.s(frozen=True)
class NluWeightedString(object):
    string = attr.ib(converter=smart_unicode)
    weight = attr.ib(converter=float)


class NluSourceItems(object):
    __slots__ = 'name', 'items'

    def __init__(self, name, items):
        self.name = name
        self.items = items

    def __hash__(self):
        return hash((
            self.name,
            tuple(self.items),
        ))

    def append(self, other):
        self.items.extend(other.items)


class FuzzyNLUTemplate(object):
    def __init__(self, data):
        self.data = data


class FuzzyNLUFormat(object):
    _TEMPLATE_LINES_LIMIT = 10000

    @classmethod
    def parse_one(cls, utterance, original_utterance=None, trainable_classifiers=(),
                  can_use_to_train_tagger=True, source_path=None, extra=None):
        text = utterance.replace('@@', '@')
        original_text = original_utterance or text
        slots = []
        seen_before = set()
        for pattern in slot_patterns:
            matched = pattern.search(text)
            while matched:
                # 'льва тостого 16'(location_to), льва тостого 16, location_to
                _, value, name = matched.groups()
                if name.startswith('+'):
                    name = name[1:]
                    # make sure that the first occurence of tag is not a continuation, and starts with -B
                    # todo: handle the unusual case when two slot parts are expressed by different slot_patterns
                    is_continuation = name in seen_before
                else:
                    is_continuation = False
                seen_before.add(name)
                begin = matched.start()
                end = matched.end()
                slots.append(Slot(name=name, begin=begin, end=begin + len(value), is_continuation=is_continuation))
                text = text[:begin] + value + text[end:]
                matched = pattern.search(text)
        return NluSourceItem(
            text=text, original_text=original_text, slots=slots,
            trainable_classifiers=trainable_classifiers, can_use_to_train_tagger=can_use_to_train_tagger,
            source_path=source_path, extra=extra
        )

    @classmethod
    def parse_iter(
        cls, utterances, name='', templates=None, rng=None, trainable_classifiers=(), can_use_to_train_tagger=True,
        source_path=None
    ):
        rng = rng or np.random.RandomState(42)
        items = []
        if templates is None:
            templates = {}
        for utterance in utterances:
            utterance = smart_unicode(utterance).strip()
            if not utterance or utterance.startswith('#'):
                continue
            utterance, extra = cls._parse_extra(utterance)
            for generated_utterance in cls._generate(utterance, templates, rng):
                items.append(cls.parse_one(
                    utterance=generated_utterance,
                    original_utterance=utterance,
                    trainable_classifiers=trainable_classifiers,
                    can_use_to_train_tagger=can_use_to_train_tagger,
                    source_path=source_path,
                    extra=extra
                ))

        return NluSourceItems(name=name, items=items)

    @classmethod
    def _generate(cls, utterance, templates, rng):
        text_parts = []
        pos = 0
        num_samples = 1
        for matched in template_pattern.finditer(utterance):
            group_dict = matched.groupdict()
            if 'template' not in group_dict:
                raise ValueError("Cannot parse '%s': template name" % utterance)
            grams = group_dict.get('grams', None)

            template = group_dict['template']
            if template not in templates:
                raise ValueError('No %r in templates' % template)

            if not group_dict.get('number') and not group_dict.get('all'):
                raise ValueError('Either a number of template instances or `all` keyword is expected after "(" in "%s" for utterance "%s".',
                                 matched.group(), utterance)

            if group_dict.get('number'):
                number = int(group_dict.get('number'))

            if group_dict.get('all'):
                number = cls._TEMPLATE_LINES_LIMIT

            number = min(number, len(templates[template].data))

            num_samples *= number
            if num_samples > cls._TEMPLATE_LINES_LIMIT:
                raise RuntimeError("Too many lines generated from NLU template '%s'. A template cannot generate more than %d lines." % (utterance, cls._TEMPLATE_LINES_LIMIT))  # noqa

            ats = group_dict.get('ats', '')

            begin = matched.start()
            if begin > 0 or utterance[begin] != '@':
                begin += 1
            end = matched.end()
            text_parts.append([utterance[pos:begin] + ats])
            text_parts.append(map(cls._inflect(grams), templates[template].data))
            pos = end
        text_parts.append([utterance[pos:]])

        part_lens = map(len, text_parts)
        for ids_in_parts in isample_tuples_from_groups(num_samples, part_lens, rng):
            yield ''.join(part[i] for part, i in izip(text_parts, ids_in_parts))

    @staticmethod
    def _inflect(grams):
        def do_nothing(phrase):
            return phrase

        def inflect_grams(phrase):
            return _inflector.inflect(phrase, set(grams.split(',')))

        if grams:
            return inflect_grams
        else:
            return do_nothing

    @staticmethod
    def _parse_extra(utterance):
        """
        Return tuple of
        - original utterance without extra-information
        - extra-information for source nlu-utterance (as original string)

        The extra information may be added to the end of line like this:
        'Включи'(action_request) музыку на `кухне`(location)\t{'rooms':['кухне']}
        """
        if '\t' not in utterance:
            return utterance, None

        utterance, extra_str = utterance.split('\t')
        return utterance, extra_str
