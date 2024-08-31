# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import logging
import attr
import random

from operator import itemgetter
from collections import OrderedDict

from vins_core.nlu.flow_nlu_factory.transition_model import TransitionModel
from vins_core.nlu.intent_candidate import IntentCandidate
from vins_core.nlu.nlu_utils import should_skip_classifier
from vins_core.nlu.nontrainable_token_classifier import create_nontrainable_token_classifier
from vins_core.nlu.token_classifier import create_token_classifier
from vins_core.nlu.nlu_utils import (is_allowed_app, is_allowed_device_state, is_allowed_prev_intent,
                                     is_allowed_experiment, is_allowed_active_slot, is_allowed_request)
from vins_core.logger.utils import dump_sequence, dump_object_sequence
from vins_core.utils.misc import EPS
from vins_core.utils.metrics import sensors

logger = logging.getLogger(__name__)


class ClassifiersCascade(object):
    def __init__(self, intent_classifiers, intent_infos, default_fallback_threshold,
                 exact_matched_intents, is_fallback_cascade, request_filters_dict=None):

        self._intent_classifiers_config = intent_classifiers or []
        self._intent_infos = intent_infos
        self._default_fallback_threshold = default_fallback_threshold
        self._exact_matched_intents = exact_matched_intents
        self._is_fallback_cascade = is_fallback_cascade

        self._classifiers_pool = {}
        self._classifiers_cascade = OrderedDict()

        self._model_configs = {
            config['name']: config for config in self._iterate_classifiers_configs(self._intent_classifiers_config)
        }

        self._transition_model = None
        self._fallback_intents = set()
        self._total_fallback_intent = None
        self._request_filters_dict = request_filters_dict

    @property
    def transition_model(self):
        return self._transition_model

    def set_transition_model(self, transition_model):
        assert isinstance(transition_model, TransitionModel)
        self._transition_model = transition_model

    @property
    def classifiers_cascade(self):
        return self._classifiers_cascade

    def add_fallback_intent(self, intent_name):
        self._fallback_intents.add(intent_name)

    def add_total_fallback_intent(self, intent_name):
        if self._total_fallback_intent:
            raise ValueError(
                'You have specified "%s" and "%s" both as total fallback intents, '
                'but only one total fallback intent is expected.' % (self._total_fallback_intent, intent_name)
            )
        self._total_fallback_intent = intent_name

    @property
    def initialized(self):
        return bool(self._classifiers_pool) or self._is_fallback_cascade

    @staticmethod
    def _iterate_classifiers_configs(classifiers_configs):
        error_string = 'Parameter "name" is required for all classifiers, but is missing in the config {}.'
        for config in classifiers_configs:
            assert 'name' in config, error_string.format(config)
            yield config
            if 'params' in config and 'classifiers' in config['params']:
                for internal_config in ClassifiersCascade._iterate_classifiers_configs(config['params']['classifiers']):
                    assert 'name' in internal_config, error_string.format(internal_config)
                    yield internal_config

    def create_intent_classifier(self, classifier_name, samples_extractor, features_extractor=None,
                                 archive=None, default_name='intent_classifier', **kwargs):
        if classifier_name not in self._model_configs:
            raise ValueError('Classifier config not found for %s', classifier_name)

        return self._create_intent_classifier(
            self._model_configs[classifier_name], samples_extractor, features_extractor, archive, default_name, **kwargs
        )

    def _create_intent_classifier(self, config, samples_extractor, features_extractor=None,
                                  archive=None, default_name='intent_classifier', **kwargs):
        classifier_name = config.get('name', default_name)
        fallback_threshold = config.get('fallback_threshold', self._default_fallback_threshold)
        logger.debug('Creating intent classifier "%s" (model="%s") with the following params: %r',
                     classifier_name, config['model'], config.get('params', {}))
        params = config.get('params', {})
        params.update(kwargs)
        features_extractor_info = features_extractor.classifiers_info if features_extractor else None
        params_to_create_classifier = {
            'name': classifier_name,
            'model': config['model'],
            'archive': archive,
            'select_features': config.get('features', ()),
            'samples_extractor': samples_extractor,
            'intent_infos': self._intent_infos,
            'features_extractor_info': features_extractor_info,
            'fallback_threshold': fallback_threshold
        }
        params_to_create_classifier.update(params)

        classifier = None
        if archive is not None or 'model_file' in params_to_create_classifier:
            classifier = create_nontrainable_token_classifier(**params_to_create_classifier)
        if classifier is None:
            classifier = create_token_classifier(**params_to_create_classifier)

            if archive is not None:
                try:
                    logger.debug('Loading from archive...')
                    classifier.load(archive, classifier_name)
                except Exception as e:
                    logger.exception(
                        'Data for %s is not loaded. Reason: %s',
                        classifier_name,
                        e.message
                    )
            else:
                logger.warning('Archive for classifier {} is None'.format(classifier_name))

        return classifier

    def add_classifier(self, classifier, config=None):
        """Add a new classifier to NLU. Classifier and all internal classifiers are inserted to classifiers pool
           and classifier added to classifiers cascade list.
        Args:
            classifier (Classifier): Classifier object.
            config (dict): Optional classifier config.
        """
        logger.info('Adding classifier {} = {}\n'.format(classifier.name, classifier))
        self._classifiers_pool.update(classifier.trainable_classifiers)
        self._classifiers_cascade[classifier.name] = classifier

        if config:
            self._model_configs.update({config['name']: config
                                        for config in ClassifiersCascade._iterate_classifiers_configs([config])})

    def update_classifier(self, updated_classifier, config=None):
        """Update an existing classifier.
        Args:
            updated_classifier (Classifier): Classifier object.
            config (dict): Optional classifier config.
        """
        logger.info('Updating classifier {} =  {}\n'.format(updated_classifier.name, updated_classifier))

        self._classifiers_pool.update(updated_classifier.trainable_classifiers)
        for _, classifier in self._classifiers_cascade.items():
            if classifier.name == updated_classifier.name:
                self._classifiers_cascade[updated_classifier.name] = updated_classifier
                break
            elif updated_classifier.name in classifier.trainable_classifiers:
                classifier.update_classifier(updated_classifier, config)
                break

        if config is not None:
            self._model_configs[updated_classifier.name] = config

    def get_classifier(self, name):
        return self._classifiers_pool[name]

    def has_classifier(self, name):
        return name in self._classifiers_pool

    def _get_skip_classifiers_list(self, classifier, req_info):
        if classifier.name in self._model_configs:
            classifier_config = self._model_configs[classifier.name]
            return set(config['name'] for config in ClassifiersCascade._iterate_classifiers_configs([classifier_config])
                       if should_skip_classifier(config['name'], config, req_info))
        return ()

    def compute_likelihoods(self, classifier, feature, req_info):
        return classifier(
            feature,
            skip_classifiers=self._get_skip_classifiers_list(classifier, req_info),
            req_info=req_info,
        )

    @staticmethod
    def _prepare_scores_to_dump(scores):
        sorted_scores = sorted(scores, key=itemgetter(1), reverse=True)
        result = []
        for name, score in sorted_scores:
            if score == 0.0:
                break
            result.append({'name': name, 'score': score})
        return result

    @sensors.with_timer('nlu_predict_intents_time')
    def predict_intents(self, feature, session, req_info=None, **kwargs):
        transition_scores_cache = {}
        if req_info and req_info.experiments['sorting_transition_model'] is not None:
            intents = self._predict_intents_new(feature, session, req_info, transition_scores_cache)
        else:
            intents = self._predict_intents_old(feature, session, req_info, transition_scores_cache)
        return self._get_scenarios(
            intents, req_info=req_info, session=session, transition_scores_cache=transition_scores_cache
        )

    def _predict_intents_old(self, feature, session, req_info=None, transition_scores_cache=None):
        """ At first, we get answers from all classifiers.
            Then we multiplies all likelihoods for each intent.
            And return intent with best confidence or None
        """
        logger.info('Using old predict_intents')
        DUMP_SEQUENCE_SIZE = 10
        for level, classifier in enumerate(self._classifiers_cascade.itervalues()):
            likelihoods = self.compute_likelihoods(classifier, feature, req_info)
            if likelihoods is None:
                continue

            log_fn = logger.debug
            if level == 12:
                log_fn = logger.info

            log_fn(
                'Likelihoods @level %d: %s\n%s',
                level, classifier.name,
                dump_sequence(
                    self._prepare_scores_to_dump(likelihoods.iteritems())[:DUMP_SEQUENCE_SIZE],
                    ['name', 'score']
                )
            )

            posteriors = self.compute_posteriors(
                likelihoods, session, req_info=req_info,
                normalize=classifier.need_normalize, transition_scores_cache=transition_scores_cache
            )
            log_fn(
                'Posteriors @level %d: %s\n%s',
                level, classifier.name,
                dump_sequence(
                    self._prepare_scores_to_dump(posteriors.iteritems())[:DUMP_SEQUENCE_SIZE],
                    ['name', 'score']
                )
            )
            intent_candidates = sorted(posteriors.iteritems(), key=itemgetter(1), reverse=True)

            if (
                req_info and
                req_info.experiments['worse_classification'] is not None and
                classifier.name == "scenarios_combined" and
                len(intent_candidates) > 1 and
                intent_candidates[1][1] > 0 and
                random.randint(0, 9) == 0
            ):
                intent_candidates = intent_candidates[1:]

            classifier_fixlist_score = classifier.fixlist_score(self._get_skip_classifiers_list(classifier, req_info))
            result_candidates = []
            for intent_candidate_name, score in intent_candidates:
                if self._is_fallback_cascade and intent_candidate_name in self._fallback_intents:
                    break

                intent_threshold = None
                if intent_candidate_name in self._intent_infos:
                    intent_threshold = self._intent_infos[intent_candidate_name].fallback_threshold

                if intent_threshold is None:
                    intent_threshold = classifier.fallback_threshold
                if score > intent_threshold:
                    # score can be higher if some boost was applied
                    is_in_fixlist = bool(score >= classifier_fixlist_score)
                    candidate = IntentCandidate(
                        name=intent_candidate_name, score=score, is_in_fixlist=is_in_fixlist,
                        is_fallback=intent_candidate_name in self._fallback_intents
                    )
                    result_candidates.append(candidate)

            self._add_forced_candidates(result_candidates, posteriors, classifier, req_info)

            if len(result_candidates) == 0 or all(candidate.is_fallback for candidate in result_candidates):
                continue

            log_fn(
                'Predicted intents:\n%s',
                dump_object_sequence(result_candidates[:DUMP_SEQUENCE_SIZE], ['name', 'score'])
            )

            return result_candidates

        if self._is_fallback_cascade:
            return [IntentCandidate(name=self._total_fallback_intent, score=1.0, is_fallback=True)]
        return []

    def compute_posteriors(
        self,
        likelihoods, session, req_info=None, normalize=True,
        transition_scores_cache=None
    ):
        posteriors = {}

        for intent_name, likelihood in likelihoods.iteritems():
            if intent_name not in self._intent_infos:
                logger.warning('Unknown intent %s not in intent_infos', intent_name)
                continue

            score = likelihood * self._intent_infos[intent_name].prior
            if score != 0.0:
                score *= self._get_transition_model_score(intent_name, session, req_info, transition_scores_cache)

            posteriors[intent_name] = score

        if normalize:
            total_score = max(sum(posteriors.itervalues()), EPS)
            for intent in posteriors:
                posteriors[intent] /= total_score

        return posteriors

    def _add_forced_candidates(self, intent_candidates, intent_scores, classifier, req_info):
        if req_info is None or req_info.experiments['force_intents'] is None:
            return

        if classifier.name == 'scenarios_combined':
            # If intent_candidates list is empty, then other isn't fake (it shouldn't be filtered out before reranking)
            is_other_can_be_fake = (len(intent_candidates) > 0)

            forced_intents = [
                'personal_assistant.scenarios.music_play',
                'personal_assistant.scenarios.video_play',
                'personal_assistant.scenarios.other',
            ]

            candidates_to_add = []
            for intent in forced_intents:
                if all(candidate.name != intent for candidate in intent_candidates):
                    is_fake_intent = (intent_scores[intent] == 0.)

                    if intent == 'personal_assistant.scenarios.other':
                        is_fake_intent = is_fake_intent and is_other_can_be_fake

                    candidates_to_add.append(IntentCandidate(
                        name=intent, score=intent_scores[intent], is_fallback=intent in self._fallback_intents,
                        is_fake=is_fake_intent
                    ))
                    logger.info('Forcing intent %s', intent)

            candidates_to_add = sorted(candidates_to_add, key=lambda candidate: candidate.score, reverse=True)
            intent_candidates.extend(candidates_to_add)

    def _predict_intents_new(self, feature, session, req_info=None, transition_scores_cache=None):
        """ Sorts intents by classifiers' scores and transition model
        All intents are divided into 4 groups by their priorities:
        1. intents with active slot boost and good enough classifier score are sorted by classifier score
        2. intents from train (with 1. classifier score) are sorted by boost
        3. intents with classifier score higher then threshold and non-zero boost are sorted by pair (boost, classifier)
        4. all other intents doesn't pass the classifier
        """
        logger.info('Using new predict_intents')

        if transition_scores_cache is None:
            transition_scores_cache = {}
        for intent_name in self._intent_infos:
            transition_scores_cache[intent_name] = self._get_transition_model_score(
                intent_name, session, req_info, scores_cache=transition_scores_cache
            )

        active_rules_intent_names = {}
        if self._transition_model:
            active_rules_intent_names = {
                intent_name for intent_name in self._intent_infos if transition_scores_cache[intent_name] >= 1.3
            }

        if active_rules_intent_names:
            logger.info('Active rule intents: %s', ' '.join(active_rules_intent_names))

        for level, classifier in enumerate(self._classifiers_cascade.itervalues()):
            likelihoods = self.compute_likelihoods(classifier, feature, req_info)
            if likelihoods is None:
                continue

            fixlist_score = classifier.fixlist_score(self._get_skip_classifiers_list(classifier, req_info))

            logger.info(self._print_scores_with_transition_model(
                'Likelihoods @level %d: ' % level + classifier.name, likelihoods, transition_scores_cache
            ))

            if classifier.need_normalize:
                total_score = max(sum(likelihoods.itervalues()), EPS)
                for intent in likelihoods:
                    likelihoods[intent] /= total_score

            candidates = self._apply_transition_model(
                likelihoods, transition_scores_cache, session, req_info,
                active_rules_intent_names, classifier.fallback_threshold, fixlist_score
            )

            if candidates:
                return candidates

        if self._is_fallback_cascade:
            return [IntentCandidate(name=self._total_fallback_intent, score=1.0, is_fallback=True)]
        return []

    @staticmethod
    def _print_scores_with_transition_model(name, scores, transition_scores_cache, count=10):
        s = ['"%s":\n' % name]
        sorted_scores = sorted(scores.iteritems(), key=itemgetter(1), reverse=True)
        for intent_name, intent_score in sorted_scores:
            if intent_score == 0.0:
                break

            if transition_scores_cache.get(intent_name, 0) == 0.:
                continue

            s.append('{}:\t{:g}\t{:g}\n'.format(intent_name, intent_score, transition_scores_cache[intent_name]))

            if len(s) - 1 == count:
                break
        s.append('\n')
        return ''.join(s)

    def _get_intent_threshold(self, intent, default_threshold):
        if intent in self._intent_infos and self._intent_infos[intent].fallback_threshold is not None:
            return self._intent_infos[intent].fallback_threshold
        return default_threshold

    def _apply_transition_model(self, likelihoods, transition_scores_cache, session, req_info,
                                active_rules_intents, default_threshold, fixlist_score):
        best_non_fallback_score, best_fallback_score = self._find_best_likelihoods(
            likelihoods, transition_scores_cache, session, req_info, default_threshold
        )

        is_fallback_better = self._is_fallback_intent_better(best_non_fallback_score, best_fallback_score)
        if is_fallback_better:
            logger.info('Fallback intent is better: %s > %s', best_fallback_score, best_non_fallback_score)

        return self._collect_candidates_by_transition_model(
            likelihoods, transition_scores_cache, session, req_info, active_rules_intents, is_fallback_better,
            default_threshold, fixlist_score, best_non_fallback_score
        )

    @staticmethod
    def _is_fallback_intent_better(best_non_fallback_score, best_fallback_score):
        if best_non_fallback_score is None:
            return True
        return best_fallback_score is not None and best_fallback_score > best_non_fallback_score

    def _collect_candidates_by_transition_model(
        self, likelihoods, transition_scores_cache, session, req_info, active_rules_intents,
        is_fallback_better, default_threshold, fixlist_score, best_non_fallback_score, adaptive_threshold_coef=0.94
    ):
        candidates = []

        if not is_fallback_better:
            adaptive_threshold = adaptive_threshold_coef * best_non_fallback_score
        else:
            # do not collect candidates except active slot ones
            adaptive_threshold = None

        logger.info('Collecting candidates by transition model, fixlist score = %s, adaptive threshold = %s, %s',
                    fixlist_score, adaptive_threshold,
                    ('active slot candidates {}'.format(active_rules_intents) if active_rules_intents else ''))

        for intent, score in likelihoods.iteritems():
            if self._is_active_slot_candidate(intent, score, active_rules_intents):
                candidates.append(IntentCandidate(name=intent, score=score, is_active_slot=True))
                logger.error('Found active slot intent %s with score %s', intent, score)
                continue

            if intent not in transition_scores_cache:
                transition_scores_cache[intent] = self._get_transition_model_score(intent, session, req_info)
            transition_score = transition_scores_cache[intent]

            if is_fallback_better or transition_score == 0.:
                continue

            candidate = IntentCandidate(
                name=intent, score=score, transition_model_score=transition_score,
                has_priority_boost=self._transition_model.is_priority_boost(transition_score),
                is_in_fixlist=True, is_fallback=intent in self._fallback_intents
            )
            if self._is_fixlist_candidate(intent, score, fixlist_score):
                candidates.append(attr.evolve(candidate, is_in_fixlist=True))
                logger.info('Found train-set intent %s with score %s and transition score %s',
                            intent, score, transition_score)
            elif self._is_good_candidate(intent, score, default_threshold, adaptive_threshold):
                candidates.append(candidate)
                logger.info('Found good intent %s with score %s and transition score %s',
                            intent, score, transition_score)

        return candidates

    def _find_best_likelihoods(self, likelihoods, transition_scores, session, req_info, default_threshold):
        best_fallback_score, best_non_fallback_score = None, None

        for intent, score in likelihoods.iteritems():
            if score <= self._get_intent_threshold(intent, default_threshold):
                continue

            if intent not in transition_scores:
                transition_scores[intent] = self._get_transition_model_score(intent, session, req_info)

            if transition_scores[intent] == 0.:
                continue

            if intent not in self._fallback_intents:
                best_non_fallback_score = score if not best_non_fallback_score else max(score, best_non_fallback_score)
            else:
                best_fallback_score = score if not best_fallback_score else max(score, best_fallback_score)

        return best_non_fallback_score, best_fallback_score

    @staticmethod
    def _is_active_slot_candidate(intent, score, active_rules_intents, score_threshold=0.25):
        return score > score_threshold and intent in active_rules_intents

    def _is_fixlist_candidate(self, intent, score, fixlist_score):
        return fixlist_score is not None and score == fixlist_score and intent not in self._fallback_intents

    def _is_good_candidate(self, intent, score, default_threshold, adaptive_threshold):
        intent_threshold = self._get_intent_threshold(intent, default_threshold)
        return score > max(intent_threshold, adaptive_threshold) and intent not in self._fallback_intents

    def _get_transition_model_score(self, intent, session, req_info, scores_cache=None):
        if scores_cache is not None:
            if intent in scores_cache:
                return scores_cache[intent]
        if self._transition_model is not None:
            score = self._transition_model(intent, session, req_info)
        else:
            score = 1.
        if scores_cache is not None:
            scores_cache[intent] = score
        return score

    def load(self, sample_extractor, nlu_sources_data, feature_extractor=None, archive=None):
        self._classifiers_cascade = OrderedDict()
        for i, classifier_cfg in enumerate(self._intent_classifiers_config):
            classifier = self._create_intent_classifier(
                classifier_cfg, sample_extractor, feature_extractor, archive,
                default_name='intent_classifier_%d' % i, nlu_sources_data=nlu_sources_data,
                exact_matched_intents=self._exact_matched_intents
            )
            self.add_classifier(classifier, classifier_cfg)

    def save(self, archive):
        for classifier in self._classifiers_cascade.itervalues():
            logger.debug('Saving "%s" classifier', classifier.name)
            classifier.save(archive, classifier.name)

    def validate(self):
        # validate classifiers
        registered_intents = set(self._intent_infos.keys())
        for classifier_name, classifier in self._classifiers_pool.iteritems():
            if not set(classifier.classes).issubset(registered_intents):
                logger.error(
                    'NLU validation failed: classifier %s contains some labels '
                    'that are not registered as valid intent labels: %r',
                    classifier_name, set(classifier.classes).difference(registered_intents)
                )
        # validate total fallback intent
        if not self._total_fallback_intent and self._is_fallback_cascade:
            raise ValueError(
                'Total fallback intent is not specified. '
                'At least one intent config should expose "total_fallback"=true flag'
            )

    def _get_scenarios(self, intents, req_info=None, session=None, transition_scores_cache=None):
        def is_allowed(config):
            return all((
                is_allowed_app(config, req_info),
                is_allowed_device_state(config, req_info),
                is_allowed_prev_intent(config, session),
                is_allowed_experiment(config, req_info),
                is_allowed_active_slot(config, session),
                is_allowed_request(config, self._request_filters_dict, req_info)
            ))
        # todo: if multiple scenarios are available, maybe rank them by transition model scores
        out = []
        for intent in intents:
            scenario_name = intent.name
            if req_info and intent.name in self._intent_infos and self._intent_infos[intent.name].scenarios is not None:
                logger.info('Checking intent switches ("scenarios") for raw intent %s...', intent.name)
                for scenario in self._intent_infos[intent.name].scenarios:
                    transition_score = self._get_transition_model_score(
                        scenario['name'], session, req_info, transition_scores_cache
                    )
                    if transition_score <= 0:
                        logger.info('Scenario "{}" skipped because of zero transition score'.format(scenario['name']))
                        continue
                    if (('context' not in scenario or is_allowed(scenario['context'])) and
                            ('not_context' not in scenario or not is_allowed(scenario['not_context']))):
                        scenario_name = scenario['name']
                        logger.info('Intent "%s" triggers scenario "%s"', intent.name, scenario_name)
                        break
                if intent.name == scenario_name:
                    logger.info('Raw intent %s triggers itself', scenario_name)

            out.append(attr.evolve(intent, name=scenario_name, name_for_reranker=scenario_name))
        return out
