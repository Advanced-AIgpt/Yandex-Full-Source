# -*- coding: utf-8 -*-

from __future__ import unicode_literals

import attr
import copy
import logging
import re

from itertools import izip

from vins_core.common.annotations.ner import NerAnnotation
from vins_core.nlu.samples_extractor import SamplesExtractor
from vins_core.dm.formats import NluSourceItem
from vins_core.dm.nlu_sources import NLUSource
from vins_core.nlu.features.cache import factory
from vins_core.nlu.features.cache.picklecache import FeatureExtractorResult
from vins_core.nlu.flow_nlu_factory.transition_model import TransitionModel
from vins_core.nlu.intent_candidate import IntentCandidate
from vins_core.nlu.nlu_data_cache import NluDataCache
from vins_core.nlu.features.base import IntentScore

from vins_core.utils.misc import call_once_on_dict, parallel, ParallelItemError
from vins_core.utils.decorators import time_info
from vins_core.utils.metrics import sensors
from vins_core.nlu.text_to_source import create_text_to_source_from_features_extractor_results
from vins_core.logger.utils import dump_object_sequence


logger = logging.getLogger(__name__)


@attr.s
class NLUHandleResult(object):
    semantic_frames = attr.ib()
    sample_features = attr.ib()


@attr.s
class IntentInfo(object):
    prior = attr.ib()
    fallback_threshold = attr.ib()
    tagger_data_keys = attr.ib()
    trainable_classifiers = attr.ib()
    anaphora_allowed = attr.ib()
    positive_sampling = attr.ib(default=True)
    negative_sampling_from = attr.ib(default=None)
    negative_sampling_for = attr.ib(default=None)
    scenarios = attr.ib(default=None)
    utterance_tagger = attr.ib(default=None)


class FeatureExtractorFromItem(object):
    def __init__(self, samples_extractor, features_extractor, feature_cache, trainable_classifiers, train_taggers=True):
        """
        Run sample and feature extractors at once, optionally using feature cache
        :param samples_extractor:
        :param features_extractor:
        :param feature_cache:
        """
        self._samples_extractor = samples_extractor
        self._features_extractor = features_extractor
        self._feature_cache = feature_cache
        self._trainable_classifiers = trainable_classifiers
        self._train_taggers = train_taggers

    def _is_processing(self, item):
        item_needed_for_classifiers = any(clf in self._trainable_classifiers for clf in item.trainable_classifiers)
        item_needed_for_taggers = self._train_taggers and item.can_use_to_train_tagger
        return item_needed_for_classifiers or item_needed_for_taggers

    def _call_one(self, item, **kwargs):
        if not self._is_processing(item):
            return None
        if self._feature_cache and item in self._feature_cache:
            return self._feature_cache[item]

        sample = self._samples_extractor.process_item(item, session=None, is_inference=False)
        if isinstance(sample, ParallelItemError):
            return sample
        sample_features = self._features_extractor.get_sample_features(sample, response=None)
        return FeatureExtractorResult(item, sample_features)

    def __call__(self, items, **kwargs):
        outputs = parallel(function=self._call_one, items=items)
        if self._feature_cache:
            outputs = self._feature_cache.update(items, outputs)
        return outputs


class BaseNLU(object):
    """Base class for NLU module.

    Interface:
    Setting up methods:
        - add_intent(intent_name, predictors, **kwargs) -- add an intent with specified predictors.
        - add_input_data(intent_name, data, can_use_to_train_tagger=True) -- add labeled data
            for intent classifier and tagger.
        - add_entity(entity_name, entity_samples) -- add custom entity for tagger.
        - train() -- train classifiers and taggers using labeled data and custom entities added.

    Prediction:
        - predict_intents(sample, session, **kwargs) -- return list of (intent_name, confidence) pairs
        - predict_tags(sample, tagger_id, **kwargs) -- return N-best list with predicted slot sequences.

    Handling:
        - handle(sample, session, **kwargs) -- return tuple with nlu top frames with slots and intent names, and sample
        features.

    """

    EMPTY_SLOTS_MAP = {
        'intent_candidate': None,
        'confidence': None,
        'slots': {},
        'entities': [],
        'tagger_score': None,
    }

    def __init__(self, fst_parser_factory, fst_parsers, samples_extractor=None, **kwargs):
        self._fst_parser_factory = fst_parser_factory
        self._fst_parsers = fst_parsers

        self._custom_entity_parsers = {}
        self._parser = None

        self._nlu_sources_data = NluDataCache()
        self._features_extractor = None
        self._samples_extractor = SamplesExtractor.from_config(samples_extractor)

        self._intent_infos = {}

    @property
    def intent_infos(self):
        return self._intent_infos

    @property
    def features_extractor(self):
        return self._features_extractor

    @property
    def samples_extractor(self):
        return self._samples_extractor

    @property
    def nlu_sources_data(self):
        return self._nlu_sources_data

    @property
    def transition_model(self):
        return self._transition_model

    @property
    def reranker(self):
        return self._reranker

    def set_transition_model(self, transition_model):
        assert isinstance(transition_model, TransitionModel)
        self._transition_model = transition_model

    def add_input_data(self, intent_name, data):
        """
        :param intent_name: The name of an intent
        :param data: List of Sample instances.
        :return:
        """
        if not data:
            logger.warning('Trying to add empty NLU data chunk for intent %s, skipped.', intent_name)
            return
        if isinstance(data, NLUSource):
            self._nlu_sources_data.add_source(intent_name, data)
        elif isinstance(data, list) and all(isinstance(item, NluSourceItem) for item in data):
            self._nlu_sources_data.add(intent_name, data)
        else:
            raise ValueError('NLUSource input data type is expected.')

    def get_custom_entity_parsers(self):
        return self._custom_entity_parsers

    def set_entity_parsers(self, entity_parsers):
        self._custom_entity_parsers = entity_parsers
        self._parser = None  # Parser needs to be rebuilt

    def add_intent(self, intent_name, prior=1.0, **kwargs):
        raise NotImplementedError

    @property
    def initialized(self):
        return bool(self._features_extractor)

    def train(self):
        """
        Train NLU using data from self.input_data and create classifier and slot filler model(s).
        """
        raise NotImplementedError

    def _predict_intents(self, sample, session, req_info, **kwargs):
        """Predict intents of ``sample`` and return intents and confidences.
        Args:
            sample (vins_core.common.sample.Sample): Sample to classify.
            session (vins_core.dm.session.Session): User session. Can be ``None`` if do not exist.
        Returns:
            iterable of pairs (intent, confidence)
        """
        raise NotImplementedError

    def _predict_tags(self, sample, feature, entities, tagger_id, **kwargs):
        """Run trained tagger with ``tagger_id`` on ``sample`` and return N-best label sequences / scores.
        Args:
            sample (vins_core.common.sample.Sample): Sample to tag.
            session (vins_core.dm.session.Session): User session. Can be ``None`` if do not exist.
            tagger_id (str or unicode): Tagger id if

        :param tokens: tokens
        :param tagger_id: choose associated tagger if any
        :return: list of list of labels, list of path scores
        """
        raise NotImplementedError

    def save(self, archive):
        pass

    def load(self, archive=None):
        pass

    def update_custom_entities(self, archive):
        for entity_name in archive.list():
            if entity_name in self._custom_entity_parsers:
                self._custom_entity_parsers[entity_name].reload_from_archive(entity_name, archive)
            else:
                logger.warning('Could not update unknown custom entity %s, skipped', entity_name)

    @time_info('extract_features')
    def _extract_features_on_train(
        self, feature_cache, input_data, trainable_classifiers, train_taggers,
        intents_to_train_classifiers, intents_to_train_tagger, filter_features,
    ):
        if not self._samples_extractor:
            raise ValueError('Samples extractor is not loaded.')
        if not self._features_extractor:
            raise ValueError('Features extractor is not loaded.'
                             ' Please ensure running %s.make_features_extractor() before calling train() or handle()' %
                             self.__class__.__name__)

        def intent_filter(intent_name):
            if not filter_features:
                return True
            if intents_to_train_classifiers:
                if re.match(intents_to_train_classifiers, intent_name):
                    return True
            if intents_to_train_tagger:
                if re.match(intents_to_train_tagger, intent_name):
                    return True
            logger.info('Skipping samples from intent {}'.format(intent_name))
            return False

        sample_features_cache = None
        if feature_cache is not None:
            sample_features_cache = factory.FeatureCacheFactory.create(feature_cache)
            sample_features_cache.check_consistency(self.features_extractor, self._custom_entity_parsers.keys())

        if sample_features_cache and not sample_features_cache.UPDATABLE:
            def _get_items_from_cache(items):
                return [
                    sample_features_cache[item] for item in items
                    if item in sample_features_cache
                ]
            output = {}
            for key, items in input_data.iteritems():
                cached_items = _get_items_from_cache(items)
                logger.info(
                    'Feature extraction for intent %s, total items %d, fetched from yt-cache items %d',
                    key, len(items), len(cached_items)
                )
                if cached_items:
                    output[key] = cached_items
        else:
            output = call_once_on_dict(
                function=FeatureExtractorFromItem(
                    samples_extractor=self._samples_extractor,
                    features_extractor=self._features_extractor,
                    feature_cache=sample_features_cache,
                    trainable_classifiers=trainable_classifiers,
                    train_taggers=train_taggers
                ),
                mappable=input_data,
                filter_errors=False,
                key_filter=intent_filter,
            )

        if sample_features_cache and hasattr(sample_features_cache, 'cache_file'):
            create_text_to_source_from_features_extractor_results(
                output, sample_features_cache.cache_file + '.text-to-source')

        return output

    def _extract_features_on_handle(self, sample, response=None):
        if not self._features_extractor:
            raise ValueError('Features extractor is not loaded.'
                             ' Please ensure running %s.make_features_extractor() before calling train() or handle()' %
                             self.__class__.__name__)
        return self._features_extractor.get_sample_features(sample, response)

    def _get_slotsmap_list(self, entities, intent_candidate, confidence, slots_list, entities_list, score_list):
        slots_map = copy.deepcopy(self.EMPTY_SLOTS_MAP)
        slots_map['intent_name'] = intent_candidate.name
        slots_map['confidence'] = confidence
        slots_map['intent_candidate'] = intent_candidate

        if not slots_list:
            slots_map['entities'] = entities
            return [slots_map]

        slotsmap_list = []
        for slots, entities, score in izip(slots_list, entities_list, score_list):
            slots_map_nbest = copy.deepcopy(slots_map)
            slots_map_nbest['tagger_score'] = score
            slots_map_nbest['slots'], slots_map_nbest['entities'] = slots, entities
            slotsmap_list.append(slots_map_nbest)
        return slotsmap_list

    def _build_parser(self):
        return self._fst_parser_factory.create_parser(
            self._fst_parsers,
            additional_parsers=self._custom_entity_parsers.values()
        )

    def _get_parser(self):
        if self._parser is None:
            self._parser = self._build_parser()
        return self._parser

    def _extract_entities(self, sample, **kwargs):
        parser = self._get_parser()
        entities = parser(sample, **kwargs)
        logger.debug('Extracted entities:')
        for entity in entities:
            logger.debug(
                'TEXT:%s, VALUE:%s, TYPE:%s',
                ' '.join(sample.tokens[entity.start:entity.end]),
                entity.value,
                entity.type
            )
        return entities

    def _extend_slots_map_list(self, slots_map_list, sample,
                               sample_features, entities, intent_candidate, confidence, **kwargs):
        slots_list, entities_list, score_list = self._predict_tags(
            sample, sample_features, entities, intent_candidate.name, **kwargs
        )

        slots_map_list.extend(self._get_slotsmap_list(
            entities, intent_candidate, confidence, slots_list, entities_list, score_list))

    def _update_sample_for_tagger(
        self,
        sample,
        sample_features,
        sample_entities,
        intent_candidates,
        session,
        req_info,
        **kwargs
    ):
        return sample, sample_features, sample_entities

    @sensors.with_timer('nlu_handle_time')
    def handle(self, sample, session, response=None, force_intent=None, req_info=None, max_intents=1,
               only_sample_features=False, **kwargs):
        if not self.initialized:
            raise RuntimeError('{0} is not initialized.'
                               ' Run {0}.load() before actual computations'.format(self.__class__.__name__))

        with sensors.timer('nlu_extract_entities_time'):
            sample_entities = self._extract_entities(sample, req_info=req_info, **kwargs)
        sample.annotations['ner'] = NerAnnotation(entities=sample_entities)

        with sensors.timer('nlu_extract_features_time'):
            sample_features = self._extract_features_on_handle(sample, response=response)
        if only_sample_features:
            return NLUHandleResult(semantic_frames=None, sample_features=sample_features)

        if force_intent is None:
            force_intent = req_info and req_info.experiments['force_intent']

        if force_intent:
            if force_intent in self._intent_infos:
                intent_candidates = [IntentCandidate(name=force_intent, score=1.0)]
            else:
                raise ValueError('Invalid force_intent experiment.'
                                 ' Cannot force non-existent intent "{}"'.format(force_intent))
        else:
            intent_candidates = self._predict_intents(sample_features, session=session, req_info=req_info, **kwargs)

        if sample_features is not None:
            intent_scores = [IntentScore(name=intent_candidate.name, score=intent_candidate.score)
                             for intent_candidate in intent_candidates]
            sample_features.add_classification_scores('predict_intents', intent_scores)

        intent_candidates = self.handle_fallback_intents(
            intent_candidates, sample_features, session, req_info, max_intents
        )
        intent_scores = [IntentScore(name=intent_candidate.name, score=intent_candidate.score)
                         for intent_candidate in intent_candidates]
        sample_features.add_classification_scores('handle_fallback_intents', intent_scores)

        intent_candidates = intent_candidates[:max_intents]
        if sample_features is not None:
            intent_scores = [IntentScore(name=intent_candidate.name, score=intent_candidate.score)
                             for intent_candidate in intent_candidates]
            sample_features.add_classification_scores('max_intents', intent_scores)

        logger.debug(
            'Result candidate intents:\n%s',
            dump_object_sequence(intent_candidates, ['name', 'score'])
        )

        updated_sample, updated_sample_features, updated_sample_entities = self._update_sample_for_tagger(
            sample, sample_features, sample_entities, intent_candidates, session, req_info=req_info, **kwargs
        )

        slots_map_list = []
        for intent_candidate in intent_candidates:
            intent_name = intent_candidate.name
            if intent_name in self._intent_infos and self._intent_infos[intent_name].anaphora_allowed:
                self._extend_slots_map_list(
                    slots_map_list,
                    updated_sample, updated_sample_features, updated_sample_entities,
                    intent_candidate,
                    intent_candidate.score,
                    req_info=req_info,
                    **kwargs
                )
            else:
                self._extend_slots_map_list(
                    slots_map_list,
                    sample, sample_features, sample_entities,
                    intent_candidate,
                    intent_candidate.score,
                    req_info=req_info,
                    **kwargs
                )

        if kwargs.get('return_normalized_utt', False):
            for sm in slots_map_list:
                sm['normalized_utt'] = ' '.join(sample.tokens)

        return NLUHandleResult(semantic_frames=slots_map_list, sample_features=sample_features)

    def handle_fallback_intents(self, intent_candidates, sample_features, session, req_info, max_intents):
        raise NotImplementedError
