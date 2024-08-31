# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import logging

import vins_tools.nlu.ner.normbase as gn
from vins_tools.nlu.ner.fst_base import NluFstBaseConstructor
from vins_tools.nlu.ner.fst_utils import put_cases_cartesian_numbers, put_cases, fstring, is_single

EXAMPLE_FILE = './data/example'

logger = logging.getLogger(__name__)


def extract_weight(e):
    if isinstance(e, (list, tuple)) and len(e) == 2:
        name, weight = e
        if isinstance(name, basestring) and (weight is None or isinstance(weight, float)):
            return (name, weight) if weight is not None and weight != 1.0 else (name, None)
    elif isinstance(e, basestring):
        return e, None

    raise ValueError('Incorrect synonym format: %s, must be <basestring> or (<basestring>, <float>|None)' % e)


class NluFstCustomConstructor(NluFstBaseConstructor):
    def __init__(self, data=None, **kwargs):
        super(NluFstCustomConstructor, self).__init__(**kwargs)

        # due to the vins-api pipeline,
        # using the file content instead of the filename here
        self.data = data

    def create(self):
        super(NluFstCustomConstructor, self).create()

        self._entities = self._load_entities(self.data)
        if self._entities is not None:
            self.fentity = self.entity()
            self.fsts = [
                self.ftag + self.fentity
            ]

    # changed it to json
    def _load_entities(self, content):
        # converting to the old format
        return [
            (canon, synonyms)
            for canon, synonyms in content.iteritems()
        ]

    def entity(self):
        f = []
        for e_canon, synonyms in self._entities:
            for synonym in synonyms:
                e, weight = extract_weight(synonym)

                en = self.normalizer(e)
                self.maps[en] = e_canon
                if weight is not None:
                    self.weights[e_canon][en] = weight

                f.append(fstring(en))
        return gn.Fst.union_seq(f)


class NluFstCustomHierarchyConstructor(NluFstCustomConstructor):
    def entity(self):
        f, fs = [], []
        subtype = ''
        self.maps[subtype] = {}
        for e_canon, synonyms in self._entities:
            for synonym in synonyms:
                e, weight = extract_weight(synonym)
                en = self.normalizer(e)
                self.maps[subtype][en] = e_canon
                if weight is not None:
                    self.weights[subtype + '_' + e_canon][en] = weight
                fs.append(fstring(en))
        return gn.Fst.union_seq(f)


class NluFstCustomMorphyConstructor(NluFstCustomConstructor):
    def __init__(self, inflect_numbers=False, **kwargs):
        """
        :param inflect_number: do inflect entities for number during fst building. If set to true object will act as
               follows: if canonical entity name is singular then both numbers will be used during inflection; otherwise
               inflection for cases only will be performed

        :type inflect_number: bool
        """
        super(NluFstCustomMorphyConstructor, self).__init__(**kwargs)
        self._inflect_numbers = inflect_numbers

    def entity(self):
        f = []
        for e_canon, synonyms in self._entities:
            for synonym in synonyms:
                e, weight = extract_weight(synonym)

                en = self.normalizer(e)
                if len(en) == 0:
                    continue

                if self._inflect_numbers and is_single(en):
                    inflected_forms = put_cases_cartesian_numbers(en)
                else:
                    inflected_forms = put_cases(en)

                logger.debug('Custom entity key="%s", value="%s". Inflected values: "%s"',
                             e_canon, en, ', '.join(inflected_forms))

                for emorph in [en] + inflected_forms:
                    if emorph in self.maps and self.maps[emorph] != e_canon:
                        logger.warn(
                            "Multiple values for key %s detected, fst_name is %s, "
                            "previous value is %s, new value is %s",
                            emorph, self.fst_name,
                            self.maps[emorph], e_canon
                        )
                    self.maps[emorph] = e_canon
                    if weight is not None:
                        self.weights[e_canon][emorph] = weight
                    f.append(fstring(emorph))
        return gn.Fst.union_seq(f)


def build_custom_entity_parser(entity_name, entity_samples, entity_inflect_info):
    input_data = {}
    for entity_canon, entity_synonims in entity_samples.iteritems():
        input_data[entity_canon] = [(sample.text, sample.weight) for sample in entity_synonims]

    if entity_inflect_info.inflect:
        custom_entity_parser = NluFstCustomMorphyConstructor(
            fst_name=entity_name,
            data=input_data,
            inflect_numbers=entity_inflect_info.inflect_numbers
        )
    elif entity_inflect_info.inflect_numbers:
        raise ValueError("Can't inflect numbers for uninflectable custom_entity %s", entity_name)
    else:
        custom_entity_parser = NluFstCustomConstructor(
            fst_name=entity_name,
            data=input_data
        )

    custom_entity_parser.compile()

    logger.info("Custom entity %s has been compiled", entity_name)

    return custom_entity_parser
