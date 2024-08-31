# -*- coding: utf-8 -*-
from __future__ import unicode_literals
from alice.nlu.py_libs import custom_entities
from vins_core.common import entity as m_entity
from vins_core.ner import ner_mixin

import os.path


class CustomEntityParser(ner_mixin.NluNerMixin):
    DATA_FILE = 'custom_entities.trie'

    def __init__(self, fst_name, fst_path, entity_names):
        self.fst_name = fst_name
        self._searcher = custom_entities.CustomEntitySearcher(os.path.join(fst_path, self.DATA_FILE))
        self._entity_names = None
        if entity_names is not None:
            self._entity_names = frozenset(entity_names)

    def parse(self, sample):
        result = []
        tokens = sample.text.split()
        for entity in self._searcher.search([x.encode('utf-8') for x in tokens]):
            if self._entity_names is None or entity.type in self._entity_names:
                result.append(m_entity.Entity(
                    start=entity.begin,
                    end=entity.end,
                    type=entity.type.decode('utf-8').upper(),
                    value=entity.value.decode('utf-8'),
                    substr=' '.join(tokens[entity.begin:entity.end])
                ))
        return result
