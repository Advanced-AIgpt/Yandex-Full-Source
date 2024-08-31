# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import logging
import attr
import re
import copy

from operator import attrgetter
from collections import defaultdict

from vins_core.common.annotations.ner import NerAnnotation
from vins_core.common.annotations.flag import FlagAnnotation
from vins_core.nlu.base_nlu import BaseNLU, FeatureExtractorResult, IntentInfo
from vins_core.dm.session import Session
from vins_core.dm.request import create_request
from vins_core.nlu.combine_scores_classifier import CombineScoresClassifier
from vins_core.nlu.combine_scores_tagger import CombineScoresTokenTagger
from vins_core.nlu.exact_match_classifier import ExactMatchClassifier
from vins_core.nlu.exact_match_token_tagger import ExactMatchTokenTagger
from vins_core.nlu.granet_classifier import GranetClassifier
from vins_core.nlu.granet_token_tagger import GranetTokenTagger
from vins_core.nlu.irrelevant_classifier import IrrelevantClassifier
from vins_core.nlu.protocol_semantic_frame_classifier import ProtocolSemanticFrameClassifier
from vins_core.nlu.protocol_semantic_frame_tagger import ProtocolSemanticFrameTagger
from vins_core.nlu.dirty_lang_classifier import DirtyLangClassifier
from vins_core.nlu.lookup_classifier import (
    DataLookupTokenClassifier, YTLookupTokenClassifier, S3LookupTokenClassifier, FileLookupTokenClassifier
)
from vins_core.utils.misc import dict_shuffle_split
from vins_core.utils.metrics import sensors
from vins_core.nlu.anaphora.factory import AnaphoraResolverFactory
from vins_core.nlu.token_classifier import register_token_classifier_type
from vins_core.nlu.token_tagger import create_token_tagger, register_token_tagger_type
from vins_core.nlu.features_extractor import FeaturesExtractorFactory
from vins_core.nlu.classifiers_cascade import ClassifiersCascade
from vins_core.nlu.intent_candidate import IntentCandidate
from vins_core.logger.utils import dump_object_sequence
from vins_core.common.slots_map_utils import tags_to_slots

# TODO(smirnovpavel): uncomment when no training will be in compile_app_model.py
# from vins_core.nlu.nontrainable_token_classifier import (
#     register_token_classifier_type as register_nontrainable_token_classifier_type)
# from vins_core.nlu.nontrainable_combine_scores_classifier import (
#    NontrainableCombineScoresClassifier)

logger = logging.getLogger(__name__)


_DEFAULT_FALLBACK_THRESHOLD = 0.8


class FlowNLU(BaseNLU):
    def __init__(
        self, intent_classifiers, fallback_intent_classifiers=[], utterance_tagger=None,
        default_fallback_threshold=_DEFAULT_FALLBACK_THRESHOLD, feature_extractors=None,
        anaphora_config=None, rng_seed=None, exact_matched_intents=None, **kwargs
    ):
        """
        :param max_like: if True, only maximum likelihood intent is returned
        :return:
        """
        super(FlowNLU, self).__init__(**kwargs)
        self._utterance_tagger_config = utterance_tagger
        self._feature_extractors_config = feature_extractors or []

        self._anaphora_intents_regexp = None
        self._anaphora_resolver = None
        if anaphora_config:
            anaphora_intents = anaphora_config.get('intents')
            if anaphora_intents:
                self._anaphora_intents_regexp = re.compile('^(?:{})$'.format('|'.join(anaphora_intents)))
            if 'matcher' in anaphora_config:
                self._anaphora_resolver = self._create_anaphora_resolver(anaphora_config)

        self._tagger = None
        self._transition_model = None
        self._features_extractor = None
        self._default_fallback_threshold = default_fallback_threshold
        self._total_fallback_intent = None
        self._trained = False
        self._rng_seed = rng_seed
        self._exact_matched_intents = exact_matched_intents or set()

        self._intent_classifiers_cascade = ClassifiersCascade(
            intent_classifiers, self._intent_infos, default_fallback_threshold,
            exact_matched_intents, is_fallback_cascade=False, request_filters_dict=kwargs.get('request_filters_dict')
        )
        self._fallback_intent_classifiers_cascade = ClassifiersCascade(
            fallback_intent_classifiers, self._intent_infos, default_fallback_threshold,
            exact_matched_intents, is_fallback_cascade=True
        )

    @property
    def transition_model(self):
        return self._intent_classifiers_cascade.transition_model

    def set_transition_model(self, transition_model):
        super(FlowNLU, self).set_transition_model(transition_model)
        self._intent_classifiers_cascade.set_transition_model(transition_model)
        self._fallback_intent_classifiers_cascade.set_transition_model(transition_model)

    @property
    def anaphora_resolver(self):
        return self._anaphora_resolver

    @property
    def classifiers_cascade(self):
        return self._intent_classifiers_cascade.classifiers_cascade

    def add_intent(
        self,
        intent_name,
        prior=1.0,
        fallback_threshold=0,
        tagger_data_keys=None,
        trainable_classifiers=(),
        fallback=False,
        total_fallback=False,
        positive_sampling=True,
        negative_sampling_from=None,
        scenarios=None,
        utterance_tagger=None,
        negative_sampling_for=None,
    ):
        """Add a new intent to NLU.

        Args:
            intent_name (str or unicode): The name of the intent being added.
            prior (float): Constant likelihood multiplier.
            fallback_threshold (float): Fallback threshold.
            tagger_data_keys (set of data keys): List of keys matching data that used to train taggers (assuming
                data was added by add_input_data() ).
            trainable_classifiers (list of string): list of trainable classifiers
                that provide likelihoods for this intent
            fallback (bool): if True, winning this intent triggers next cascade level
            total_fallback (bool): when no intents chosen on inference, use this one
            positive_sampling (bool): whether to use pairwise samples from this intent as positive for metric learning
            negative_sampling_from (list or None): names of intents to use as negative examples for this one
                                                    (all the rest by default)
            negative_sampling_for (list or None): names of intents to use this intent samples as negative samples for
                                                    (all the rest by default)
            scenarios (list or None): list of dicts like {"name": scenario, "context": {...}},
                                      with scenario to be triggered as well as optional triggering context
                                     (if scenarios is None or contexts are not matched,
                                     current intent triggers scenario of the same name)
        """

        anaphora_allowed = (self._anaphora_intents_regexp and
                            self._anaphora_intents_regexp.match(intent_name) is not None)
        self._intent_infos[intent_name] = IntentInfo(
            prior,
            fallback_threshold,
            tagger_data_keys or set(),
            trainable_classifiers or [],
            anaphora_allowed,
            positive_sampling,
            negative_sampling_from,
            negative_sampling_for,
            scenarios,
            utterance_tagger
        )

        if fallback:
            self._intent_classifiers_cascade.add_fallback_intent(intent_name)
            self._fallback_intent_classifiers_cascade.add_fallback_intent(intent_name)
        if total_fallback:
            self._intent_classifiers_cascade.add_total_fallback_intent(intent_name)
            self._fallback_intent_classifiers_cascade.add_total_fallback_intent(intent_name)

    @property
    def initialized(self):
        return (super(FlowNLU, self).initialized
                and self._intent_classifiers_cascade.initialized
                and self._fallback_intent_classifiers_cascade.initialized
                and bool(self._tagger))

    @staticmethod
    def create_features_extractor(feature_extractors_config, parser=None):
        factory = FeaturesExtractorFactory()

        if parser is not None:
            factory.register_parser(parser)

        for cfg in feature_extractors_config:
            feature_params = copy.deepcopy(cfg)
            feature_params.pop('id')
            factory.add(cfg['id'], **feature_params)

        features_extractor = factory.create_extractor()

        return features_extractor

    def _create_features_extractor(self):
        self._features_extractor = self.create_features_extractor(self._feature_extractors_config, self._get_parser())

    def extract_features_on_train(
        self, classifiers, feature_cache, validation, taggers,
        intents_to_train_classifiers=None, intents_to_train_tagger=None, filter_features=False
    ):
        logger.info('Creating features extractor...')
        classifiers_for_feature_extractor = None if validation else classifiers
        trainable_classifiers = set(sum(map(attrgetter('trainable_classifiers'), self._intent_infos.itervalues()), []))
        self._create_features_extractor()
        logger.info('Extract features...')
        return self._extract_features_on_train(
            feature_cache=feature_cache,
            input_data=self.nlu_sources_data,
            trainable_classifiers=classifiers_for_feature_extractor or list(trainable_classifiers),
            train_taggers=taggers,
            intents_to_train_classifiers=intents_to_train_classifiers,
            intents_to_train_tagger=intents_to_train_tagger,
            filter_features=filter_features,
        )

    def train(
        self, classifiers=None, taggers=True, feature_cache=None, validation=None, validate_intent=None,
        intents_to_train_tagger=None, intents_to_train_classifiers=None, filter_features=False, **kwargs
    ):
        if not self.initialized:
            raise RuntimeError('{0} is not initialized.'
                               ' Run {0}.load() before actual computations'.format(self.__class__.__name__))

        sample_features = self.extract_features_on_train(
            classifiers=classifiers,
            feature_cache=feature_cache,
            validation=validation,
            taggers=taggers,
            intents_to_train_classifiers=intents_to_train_classifiers,
            intents_to_train_tagger=intents_to_train_tagger,
            filter_features=filter_features,
        )

        logger.info('Start training intent classifiers')
        classifiers_valid_result = self._train_intent_classifiers(
            sample_features, validation=validation, validate_intent=validate_intent, classifiers=classifiers,
            intents_to_train_classifiers=intents_to_train_classifiers, **kwargs
        )
        if taggers:
            logger.info('Start training taggers')
            taggers_valid_result = self._train_utterance_tagger(
                sample_features, validation=validation, validate_intent=validate_intent,
                intents_to_train_tagger=intents_to_train_tagger
            )
        else:
            taggers_valid_result = None
        self._trained = True
        return {
            'classifiers_validation_result': classifiers_valid_result,
            'taggers_validation_result': taggers_valid_result
        }

    def add_classifier(self, classifier, is_fallback_intent_classifier=False, config=None):
        """Add a new classifier to NLU. Classifier and all internal classifiers are inserted to classifiers pool
           and classifier added to classifiers cascade list.
        Args:
            classifier (Classifier): Classifier object.
            config (dict): Optional classifier config.
        """
        if not is_fallback_intent_classifier:
            self._intent_classifiers_cascade.add_classifier(classifier, config)
        else:
            self._fallback_intent_classifiers_cascade.add_classifier(classifier, config)

    def get_classifier(self, name):
        if self._intent_classifiers_cascade.has_classifier(name):
            return self._intent_classifiers_cascade.get_classifier(name)
        return self._fallback_intent_classifiers_cascade.get_classifier(name)

    def has_classifier(self, name):
        return (self._intent_classifiers_cascade.has_classifier(name)
                or self._fallback_intent_classifiers_cascade.has_classifier(name))

    def create_train_data_for_intent_classifier(self, sample_features, classifiers=None, validation=None,
                                                intents_to_train_classifiers=None):
        train_data = defaultdict(lambda: defaultdict(list))
        intents_for_classifier = set()
        if intents_to_train_classifiers is not None:
            pattern = re.compile(intents_to_train_classifiers)
            for intent_name in self._intent_infos.keys():
                if re.match(pattern, intent_name):
                    intents_for_classifier.add(intent_name)
        for intent_name, intent_info in self._intent_infos.iteritems():
            for result in sample_features.get(intent_name, ()):
                if not isinstance(result, FeatureExtractorResult):
                    continue
                for classifier in result.item.trainable_classifiers:
                    if classifiers is not None and classifier not in classifiers:
                        continue
                    # if the classifier does not support partial update, the error will be raised elsewhere
                    if intents_to_train_classifiers is not None and intent_name not in intents_for_classifier:
                        continue
                    train_data[classifier][intent_name].append(result.sample_features)

        if not train_data:
            logger.warning('Training data is empty')
        if validation:
            valid_data = {}
            for classifier in train_data:
                train_data[classifier], valid_data[classifier] = dict_shuffle_split(
                    train_data[classifier], validation, self._rng_seed)
        else:
            valid_data = None
        return train_data, valid_data

    def _train_intent_classifiers(self, sample_features, validation, classifiers=None, validate_intent=None,
                                  intents_to_train_classifiers=None, **kwargs):
        train_data, valid_data = self.create_train_data_for_intent_classifier(
            sample_features, classifiers, validation, intents_to_train_classifiers
        )
        for classifier_name, classifier_train_data in train_data.iteritems():
            logger.info('Start training intent classifier %s', classifier_name)

            if self._intent_classifiers_cascade.has_classifier(classifier_name):
                cascade = self._intent_classifiers_cascade
            elif self._fallback_intent_classifiers_cascade.has_classifier(classifier_name):
                cascade = self._fallback_intent_classifiers_cascade
            else:
                raise ValueError('Classifier config not found for %s', classifier_name)

            if intents_to_train_classifiers is None:
                logger.info('Creating a new classifier instance')
                updated_classifier = cascade.create_intent_classifier(
                    classifier_name, samples_extractor=self._samples_extractor, nlu_sources_data=self.nlu_sources_data,
                    exact_matched_intents=self._exact_matched_intents, features_extractor=self._features_extractor,
                    **kwargs)
            elif cascade.has_classifier(classifier_name):
                logger.info('Getting the old classifier instance')
                updated_classifier = cascade.get_classifier(classifier_name)
            else:
                raise ValueError('Classifier {} does not exist, so it cannot be updated.'.format(classifier_name))

            updated_classifier.train(
                intent_to_features=classifier_train_data,
                transition_model=self._transition_model and self._transition_model.model,
                intent_infos=self._intent_infos,
                update_only=intents_to_train_classifiers is not None
            )

            cascade.update_classifier(updated_classifier)

        if valid_data:
            result, accuracy = self._validate_intent_classifiers(valid_data, validate_intent)
            logger.info('Classification accuracy on %.0f%% validation set: %.4f', 100 * (1 - validation), accuracy)
            return result

    def _validate_intent_classifiers(self, validation_data, validate_intent):
        errors, total_items = 0, 0.0
        result = []
        for classifier, classifier_data in validation_data.iteritems():
            for intent_name, intent_data in classifier_data.iteritems():
                if validate_intent and not re.match(validate_intent, intent_name):
                    continue
                for sample_features in intent_data:
                    # TODO: setup context in session
                    session = Session('app_id', 'uuid')
                    req_info = create_request('uuid')
                    intents = self._predict_intents(sample_features, session, req_info)
                    intents = self.handle_fallback_intents(intents, sample_features, session, req_info, max_intents=1)
                    semantic_frames = self._get_slotsmap_list(
                        entities=[],
                        intent_candidate=IntentCandidate(name=intents[0].name, score=intents[0].score),
                        confidence=intents[0].score,
                        slots_list=[],
                        entities_list=[],
                        score_list=[]
                    )
                    result.append((sample_features, intent_name, semantic_frames))
                    if intents[0].name != intent_name:
                        errors += 1
                    total_items += 1.0
        return result, 1 - errors / total_items

    def _create_utterance_tagger(self, archive=None):
        if self._utterance_tagger_config is None:
            return None

        cfg = self._utterance_tagger_config.copy()
        params = cfg.get('params', {})
        cfg.update(**params)

        tagger = create_token_tagger(
            nlu_sources_data=self.nlu_sources_data,
            intent_infos=self._intent_infos,
            samples_extractor=self._samples_extractor,
            exact_matched_intents=self._exact_matched_intents,
            **cfg
        )
        if archive is not None:
            logger.debug('Loading utterance tagger')
            tagger.load(archive, 'tagger')
        return tagger

    def _create_anaphora_resolver(self, anaphora_config):
        factory = AnaphoraResolverFactory()
        factory.set_samples_extractor(self.samples_extractor)
        return factory.create_resolver(anaphora_config)

    def _create_train_data_for_utterance_tagger(self, sample_features, validation=None):
        train_data = defaultdict(list)
        for intent_name, intent_info in self._intent_infos.iteritems():
            if not intent_info.tagger_data_keys:
                continue
            for key in intent_info.tagger_data_keys:
                for result in sample_features.get(key, ()):
                    if not isinstance(result, FeatureExtractorResult):
                        continue
                    if result.item.can_use_to_train_tagger:
                        train_data[intent_name].append(result.sample_features)
        if validation:
            train_data, valid_data = dict_shuffle_split(train_data, validation, self._rng_seed)
        else:
            valid_data = None
        return train_data, valid_data

    def _train_utterance_tagger(
        self, sample_features, validation, recall_at=None, validate_intent=None, intents_to_train_tagger=None
    ):
        train_data, valid_data = self._create_train_data_for_utterance_tagger(sample_features, validation=validation)
        if not intents_to_train_tagger:
            # train taggers from scratch
            self._tagger = self._create_utterance_tagger().train(train_data)
        else:
            # retrain selected intents
            self._tagger = self._tagger.train(train_data, intents_to_train=intents_to_train_tagger)
        result = []
        if valid_data:
            errors, total_items = 0, 0.0
            for intent_name, intent_data in valid_data.iteritems():
                if validate_intent and not re.match(validate_intent, intent_name):
                    continue
                for sample_features in intent_data:
                    tags_lists, score_lists = self._tagger.predict([sample_features], intent=intent_name)
                    tags_list, score_list = tags_lists[0], score_lists[0]
                    slots_list = [tags_to_slots(sample_features.sample.tokens, tags, [])[0] for tags in tags_list]
                    entities_list = [[]] * len(tags_list)

                    slots_map_list = self._get_slotsmap_list(
                        entities=[],
                        intent_candidate=IntentCandidate(name=intent_name, score=1.0),
                        confidence=1.0,
                        slots_list=slots_list,
                        entities_list=entities_list,
                        score_list=score_list
                    )
                    result.append((sample_features, slots_map_list))
                    check_list = tags_list[:recall_at] if recall_at else tags_list
                    if not any(tags == sample_features.sample.tags for tags in check_list):
                        errors += 1
                    total_items += 1.0
            logger.info('Tagging accuracy on %.0f%% validation set: %.4f', 100 * (1 - validation),
                        1 - errors / total_items)
        return result

    def handle_fallback_intents(self, intent_candidates, feature, session, req_info, max_intents):
        if not any(candidate.is_fallback for candidate in intent_candidates) and len(intent_candidates) != 0:
            return intent_candidates

        fallback_intent_candidate = self._predict_fallback_intent(
            feature, session=session, req_info=req_info
        )

        if len(intent_candidates) == 0:
            return [fallback_intent_candidate]

        result_candidates = []
        for candidate in intent_candidates[:max_intents]:
            if candidate.is_fallback:
                candidate = attr.evolve(candidate, name=fallback_intent_candidate.name,
                                        fallback_score=fallback_intent_candidate.score)
            result_candidates.append(candidate)

        return result_candidates

    @staticmethod
    def _get_intents_above_fallback(intent_candidates):
        result_candidates = []
        for candidate in intent_candidates:
            if candidate.is_fallback:
                break
            result_candidates.append(candidate)

        return result_candidates

    @sensors.with_timer('nlu_predict_intents_time')
    def _predict_intents(self, feature, session, req_info=None, **kwargs):
        return self._intent_classifiers_cascade.predict_intents(feature, session, req_info, **kwargs)

    @sensors.with_timer('nlu_predict_fallback_intent_time')
    def _predict_fallback_intent(self, feature, session, req_info=None, **kwargs):
        fallback_intent_candidates = self._fallback_intent_classifiers_cascade.predict_intents(
            feature, session, req_info, **kwargs
        )

        logger.debug(
            'Fallback candidate intents:\n%s',
            dump_object_sequence(fallback_intent_candidates, ['name', 'score'])
        )

        assert fallback_intent_candidates

        return fallback_intent_candidates[0]

    @sensors.with_timer('nlu_predict_tags_time')
    def _predict_tags(self, sample, feature, entities, intent_name, **kwargs):
        logger.debug('Tagging intent %s', intent_name)
        batch_slots_lists, batch_entities_lists, batch_score_lists = self._tagger.predict_slots(
            [sample], [feature], [entities], intent=intent_name, **kwargs
        )
        if batch_slots_lists:
            return batch_slots_lists[0], batch_entities_lists[0], batch_score_lists[0]
        else:
            return [], [], []

    def save(self, archive):
        super(FlowNLU, self).save(archive)

        logger.info('Saving classifiers')
        with archive.nested('classifiers') as arch:
            self._intent_classifiers_cascade.save(arch)
            self._fallback_intent_classifiers_cascade.save(arch)

        logger.info('Saving tagger')
        with archive.nested('tagger') as arch:
            self._tagger.save(arch, name=self._tagger.name)

    def load(self, archive=None):
        super(FlowNLU, self).load(archive)

        self._create_features_extractor()

        if archive:
            with archive.nested('classifiers') as arch:
                self._intent_classifiers_cascade.load(
                    self._samples_extractor, self._nlu_sources_data, self._features_extractor, arch
                )
                self._fallback_intent_classifiers_cascade.load(
                    self._samples_extractor, self._nlu_sources_data, self._features_extractor, arch
                )
            with archive.nested('tagger') as arch:
                self._tagger = self._create_utterance_tagger(arch)
            self._trained = True
        else:
            self._intent_classifiers_cascade.load(
                self._samples_extractor, self._nlu_sources_data, self._features_extractor
            )
            self._fallback_intent_classifiers_cascade.load(
                self._samples_extractor, self._nlu_sources_data, self._features_extractor
            )
            self._tagger = self._create_utterance_tagger()

        return self

    def validate(self):
        self._intent_classifiers_cascade.validate()
        self._fallback_intent_classifiers_cascade.validate()

    def warm_up_taggers(self):
        from vins_core.common.sample import Sample
        from vins_core.nlu.features.base import SampleFeatures
        import numpy as np
        sample = Sample.from_string(u'китик')
        features = SampleFeatures(sample, dense_seq={'alice_requests_emb': np.zeros((1, 300))})
        for intent in self.intent_infos.keys():
            self._tagger.predict([features], batch_samples=[sample], intent=intent)

    @property
    def trained(self):
        return self._trained

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
        has_anaphora_allowed_intent = False
        for intent in intent_candidates:
            if intent.name in self._intent_infos and self._intent_infos[intent.name].anaphora_allowed:
                has_anaphora_allowed_intent = True
                break

        logger.debug('Original utterance = "%s"', sample.text)
        if has_anaphora_allowed_intent:
            resolved_string = self._get_anaphora_resolved_string(sample, session)
            if resolved_string:
                logger.debug('Anaphora resolved utterance = "%s"', resolved_string)
                # update the old sample with the anaphora flag
                sample.annotations['anaphora_resolved_flag'] = FlagAnnotation(value=True, value_type='bool')
                sample = self._samples_extractor(
                    [resolved_string],
                    session,
                    filter_errors=True,
                    app_id=req_info.app_info.app_id or '',
                    request_id=req_info.request_id
                )[0]
                # update the new sample with the anaphora flag
                sample.annotations['anaphora_resolved_flag'] = FlagAnnotation(value=True, value_type='bool')
                sample_entities = self._extract_entities(sample, req_info=req_info, **kwargs)
                sample.annotations['ner'] = NerAnnotation(entities=sample_entities)
                sample_features = self._extract_features_on_handle(sample)

        return sample, sample_features, sample_entities

    def _get_anaphora_resolved_string(self, sample, session):
        anaphora_substitutor_response = self._get_anaphora_substitutor_response(sample)
        if anaphora_substitutor_response:
            logger.info('Using begemot anaphora resolver')
            return self._extract_resolved_string_from_anaphora_substitutor(anaphora_substitutor_response)

        logger.info('Using vins anaphora resolver')
        return self._anaphora_resolver(sample, session)

    @staticmethod
    def _get_anaphora_substitutor_response(sample):
        wizard_annotation = sample.annotations.get('wizard')
        if wizard_annotation:
            return wizard_annotation.rules.get('AliceAnaphoraSubstitutor')
        return None

    @staticmethod
    def _extract_resolved_string_from_anaphora_substitutor(anaphora_substitutor_response):
        substitutions = anaphora_substitutor_response.get('Substitution', [])
        if substitutions and substitutions[0].get('IsRewritten', False):
            return substitutions[0].get('RewrittenRequest')
        return None


register_token_classifier_type(DataLookupTokenClassifier, 'data_lookup')
register_token_classifier_type(FileLookupTokenClassifier, 'file_lookup')
register_token_classifier_type(YTLookupTokenClassifier, 'yt_lookup')
register_token_classifier_type(S3LookupTokenClassifier, 's3_lookup')
register_token_classifier_type(ExactMatchClassifier, 'nlu_exact_matching')
register_token_classifier_type(CombineScoresClassifier, 'combine_scores')
register_token_classifier_type(DirtyLangClassifier, 'dirty_lang')
register_token_classifier_type(GranetClassifier, 'granet')
register_token_classifier_type(ProtocolSemanticFrameClassifier, 'protocol_semantic_frame')
register_token_classifier_type(IrrelevantClassifier, 'irrelevant')

# TODO(smirnovpavel): uncomment when no training will be in compile_app_model.py
# register_nontrainable_token_classifier_type(NontrainableCombineScoresClassifier, 'combine_scores')

register_token_tagger_type(CombineScoresTokenTagger, 'combine_scores')
register_token_tagger_type(ExactMatchTokenTagger, 'nlu_exact_matching')
register_token_tagger_type(GranetTokenTagger, 'granet')
register_token_tagger_type(ProtocolSemanticFrameTagger, 'protocol_semantic_frame')
