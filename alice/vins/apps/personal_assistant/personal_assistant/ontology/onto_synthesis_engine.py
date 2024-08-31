# coding: utf-8
from __future__ import unicode_literals

import logging
import numpy

from operator import attrgetter
from personal_assistant.ontology.onto import Entity

logger = logging.getLogger(__name__)


class SynthesisInfo(object):
    def __init__(self, phrases):
        """
        :param phrases: all phrases (from ontology)
        :type phrases: list[PhraseInfo]
        """
        self.nlg_texts = []
        self.nlg_probs = []
        nlg_sum = 0.0
        self.nlu_texts = []
        self.nlu_probs = []
        nlu_sum = 0.0
        for p in phrases:
            nlu_sum += p.weight
            if p.use_for_nlg:
                nlg_sum += p.weight

        for p in sorted(phrases, key=attrgetter('text')):
            self.nlu_texts.append(p.text)
            self.nlu_probs.append(p.weight / nlu_sum)
            if p.use_for_nlg:
                self.nlg_texts.append(p.text)
                self.nlg_probs.append(p.weight / nlg_sum)


class OntologicalSynthesisEngine(object):
    def __init__(self, ontology):
        """
        Construct synthesis engine as wrapper around some ontology
        :param ontology:
        :type ontology: Ontology
        """
        self.ontology = ontology
        self._synthesis_info_cache = {}

    def random_phrase(self, instance):
        info = self._get_nlu_synthesis_info(instance)
        return numpy.random.choice(info.nlg_texts, 1, p=info.nlg_probs)[0]

    def random_nlu_phrase(self, instance):
        info = self._get_nlu_synthesis_info(instance)
        return numpy.random.choice(info.nlu_texts, 1, p=info.nlu_probs)[0]

    def _get_nlu_synthesis_info(self, instance):
        if not isinstance(instance, Entity.Instance):
            instance = self.ontology[instance]
            if instance is None:
                raise LookupError('Instance %s not found' % instance)

        if instance.name in self._synthesis_info_cache:
            return self._synthesis_info_cache[instance.name]

        info = SynthesisInfo(instance.phrases)
        self._synthesis_info_cache[instance.name] = info
        return info
