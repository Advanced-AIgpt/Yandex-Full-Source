# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from collections import defaultdict
from collections import Mapping


class NluDataCache(Mapping):
    def __init__(self, lazy=True):
        self._intents_nlu_sources = defaultdict(list)
        self._intents_nlu_items = defaultdict(list)
        self._loaded = set()
        self._all_loaded = False
        self._lazy = lazy

    def add(self, intent_name, nlu_items):
        self._intents_nlu_items[intent_name].extend(nlu_items)

    def add_source(self, intent_name, nlu_source):
        self._all_loaded = False
        if not self._lazy:
            self._intents_nlu_items[intent_name].extend(nlu_source.load())
            self._loaded.add(intent_name)
        else:
            self._intents_nlu_sources[intent_name].append(nlu_source)
            self._loaded.discard(intent_name)

    def load_all(self):
        if self._all_loaded:
            return
        for intent_name in self._intents_nlu_sources:
            self.load(intent_name)
        self._all_loaded = True

    def load(self, intent_name):
        if intent_name not in self._intents_nlu_sources:
            if intent_name not in self._intents_nlu_items:
                raise ValueError("Unknown intent_name %s" % intent_name)
            else:
                return

        if intent_name not in self._loaded:
            for nlu_source in self._intents_nlu_sources[intent_name]:
                self._intents_nlu_items[intent_name].extend(nlu_source.load())
            self._intents_nlu_sources[intent_name] = []
            self._loaded.add(intent_name)

    def __getitem__(self, intent_name):
        if self._lazy:
            self.load(intent_name)
        return self._intents_nlu_items[intent_name]

    def __setitem__(self, intent_name, items):
        self._intents_nlu_items[intent_name] = items

    def __iter__(self):
        if self._lazy:
            self.load_all()
        return iter(self._intents_nlu_items)

    def __len__(self):
        return len(self._intents_nlu_items)

    def items(self):
        if self._lazy:
            self.load_all()
        return self._intents_nlu_items.items()
