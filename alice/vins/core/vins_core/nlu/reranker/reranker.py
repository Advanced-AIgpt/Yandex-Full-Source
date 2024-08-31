# -*- coding: utf-8 -*-

import abc
import logging
import numpy as np

from collections import OrderedDict
from itertools import izip
from attr import evolve

from vins_core.nlu.reranker.catboost_model import CatboostModel

# imports used only for typing
from typing import List, NoReturn, Tuple, AnyStr  # noqa: UnusedImport
from vins_core.nlu.features.base import SampleFeatures  # noqa: UnusedImport
from vins_core.dm.form_filler.form_candidate import FormCandidate  # noqa: UnusedImport
from vins_core.dm.request import ReqInfo  # noqa: UnusedImport
from vins_core.nlu.reranker.factor_calcer import FactorCalcer  # noqa: UnusedImport

logger = logging.getLogger(__name__)


class BaseReranker(object):
    __metaclass__ = abc.ABCMeta

    @abc.abstractmethod
    def __call__(self, sample_features, candidates, req_info=None):
        # type: (SampleFeatures, List[FormCandidate], ReqInfo) -> List[FormCandidate]
        raise NotImplementedError

    @classmethod
    @abc.abstractmethod
    def from_config(cls, config, factor_calcer):
        raise NotImplementedError


class ExperimentBasedReranker(BaseReranker):
    NAME = 'experiment_based'

    def __init__(self, experiment_to_reranker, default_reranker):
        assert isinstance(experiment_to_reranker, OrderedDict), 'Rerankers should be ordered by their priorities'

        self._experiment_to_reranker = experiment_to_reranker
        self._default_reranker = default_reranker

    def __call__(self, sample_features, candidates, req_info=None):
        # type: (SampleFeatures, List[FormCandidate], ReqInfo) -> List[FormCandidate]

        if req_info:
            # If multiple experiments exists then the first reranker with appropriate experiment would be used
            # Rerankers are expected to be sorted by their priorities in the config
            for experiment, reranker in self._experiment_to_reranker.iteritems():
                if req_info.experiments[experiment] is not None:
                    return reranker(sample_features, candidates, req_info)

        return self._default_reranker(sample_features, candidates, req_info)

    @classmethod
    def from_config(cls, config, factor_calcer):
        inner_models = config.get('inner_models', [])
        experiment_to_reranker = OrderedDict()
        default_reranker = None
        for inner_model_config in inner_models:
            reranker = create_reranker(inner_model_config['model'], inner_model_config.get('params', {}), factor_calcer)
            experiment = inner_model_config.get('experiment', None)
            if experiment:
                if experiment not in experiment_to_reranker:
                    experiment_to_reranker[experiment] = reranker
                else:
                    logger.warning('Multiple rerankers with %s experiment are found, only first is used', experiment)
            else:
                assert not default_reranker, 'Only one default reranker is supported'
                default_reranker = reranker

        return cls(experiment_to_reranker, default_reranker)


class DescendingByScoresReranker(BaseReranker):
    NAME = 'descending_by_scores'

    def __call__(self, sample_features, candidates, req_info=None):
        # type: (SampleFeatures, List[FormCandidate], ReqInfo) -> List[FormCandidate]
        return sorted(candidates, key=lambda candidate: candidate.intent.score, reverse=True)

    @classmethod
    def from_config(cls, config, factor_calcer):
        return cls()


class DescendingByScoresAndTransitionModelReranker(BaseReranker):
    NAME = 'descending_by_scores_and_transition_model'

    def __call__(self, sample_features, candidates, req_info=None):
        # type: (SampleFeatures, List[FormCandidate], ReqInfo) -> List[FormCandidate]
        return sorted(candidates, cmp=self._comparator_with_transition_scores)

    @staticmethod
    def _comparator_with_transition_scores(first_candidate, second_candidate):
        # type: (FormCandidate, FormCandidate) -> int
        """
        Compares candidates first by transition model score, than by classifiers score
        Takes into account priority boosts (transition scores like 1.000001) only when classifiers scores are equal
        Otherwise, considers such transition model scores equal to 1
        to sort by transition model scores only when significantly different
        """

        if first_candidate.intent.score == second_candidate.intent.score:
            if first_candidate.intent.transition_model_score < second_candidate.intent.transition_model_score:
                return 1
            elif first_candidate.intent.transition_model_score == second_candidate.intent.transition_model_score:
                return 0
            return -1

        # ellipsis intents should have same priority as the external ones
        first_transition_score = first_candidate.intent.transition_model_score
        if first_candidate.intent.has_priority_boost:
            first_transition_score = 1.
        second_transition_score = second_candidate.intent.transition_model_score
        if second_candidate.intent.has_priority_boost:
            second_transition_score = 1.

        if ((first_transition_score, first_candidate.intent.score) <
                (second_transition_score, second_candidate.intent.score)):
            return 1
        # the equal pairs are already processed in the first condition
        return -1

    @classmethod
    def from_config(cls, config, factor_calcer):
        return cls()


class CatboostReranker(BaseReranker):
    NAME = 'catboost'

    def __init__(self, model, factor_calcer, used_factors, known_intents):
        # type: (CatboostModel, FactorCalcer, List[AnyStr], List[AnyStr]) -> NoReturn

        self._model = model
        self._factor_calcer = factor_calcer
        self._used_factors = used_factors
        self._known_intents = set(known_intents)

    def __call__(self, sample_features, candidates, req_info=None):
        # type: (SampleFeatures, List[FormCandidate], ReqInfo) -> List[FormCandidate]

        # only known intents should be reranked. Others stay on their initial positions
        indices_for_reranking = [
            ind for ind, candidate in enumerate(candidates)
            if candidate.intent.name_for_reranker in self._known_intents
        ]
        if len(indices_for_reranking) <= 1:
            return candidates

        candidates = np.array(candidates)
        candidates[indices_for_reranking] = self._sort_candidates(sample_features, candidates[indices_for_reranking])

        return list(candidates)

    def _sort_candidates(self, sample_features, candidates):
        factor_values = self._factor_calcer(sample_features, candidates, self._used_factors)
        candidate_scores = self._model.predict(factor_values)

        candidates = [evolve(candidate, intent=evolve(candidate.intent, score=score))
                      for candidate, score in izip(candidates, candidate_scores)]

        return sorted(candidates, key=lambda candidate: candidate.intent.score, reverse=True)

    @classmethod
    def from_config(cls, config, factor_calcer):
        model = CatboostModel.from_config(config)

        return cls(model, factor_calcer, config.get('used_factors', []), config.get('known_intents', []))


class CatboostIndependentReranker(BaseReranker):
    NAME = 'independent_catboost'

    def __init__(self, models, factor_calcer, used_factors, threshold):
        self._models = models
        self._factor_calcer = factor_calcer
        self._used_factors = used_factors
        self._threshold = threshold

    def __call__(self, sample_features, candidates, req_info=None):
        # type: (SampleFeatures, List[FormCandidate], ReqInfo) -> List[FormCandidate]

        # only known intents should be reranked. Others stay on their initial positions
        indices_for_reranking = [
            ind for ind, candidate in enumerate(candidates)
            if candidate.intent.name_for_reranker in self._models
        ]
        if len(indices_for_reranking) <= 1:
            return candidates

        candidates = np.array(candidates)
        candidates_after_reranker = self._sort_candidates(sample_features, candidates[indices_for_reranking])
        if self._threshold is not None and candidates_after_reranker[0].score < self._threshold:
            return candidates
        candidates[indices_for_reranking] = candidates_after_reranker

        return list(candidates)

    def _sort_candidates(self, sample_features, candidates):
        factor_values = self._factor_calcer(sample_features, candidates, self._used_factors)
        candidate_scores = []
        for factors, candidate in zip(factor_values, candidates):
            candidate_scores.append(
                self._models[candidate.intent.name_for_reranker].predict(factors.reshape((1, -1)))[0])
        candidate_scores = np.array(candidate_scores)
        candidates = [evolve(candidate, intent=evolve(candidate.intent, score=score))
                      for candidate, score in izip(candidates, candidate_scores)]
        return sorted(candidates, key=lambda candidate: candidate.intent.score, reverse=True)

    @classmethod
    def from_config(cls, config, factor_calcer):
        models = dict()
        for intent in config.get('models', []):
            models[intent] = CatboostModel.from_config(config['models'][intent])
        used_factors = config.get('used_factors', [])
        threshold = config.get('threshold')
        return cls(models, factor_calcer, used_factors, threshold)


class ExternalModelReranker(BaseReranker):
    NAME = 'external_reranker'

    def __init__(self, intent_flags, threshold):
        self._intent_flags = intent_flags
        self._threshold = threshold

    def __call__(self, sample_features, candidates, req_info=None):
        # type: (SampleFeatures, List[FormCandidate], ReqInfo) -> List[FormCandidate]

        # only known intents should be reranked. Others stay on their initial positions
        indices_for_reranking = [
            ind for ind, candidate in enumerate(candidates)
            if candidate.intent.name_for_reranker in self._intent_flags
        ]
        if len(indices_for_reranking) <= 1 or req_info is None:
            return candidates
        candidates = np.array(candidates)
        candidates_after_reranker = self._sort_candidates(candidates[indices_for_reranking], req_info)
        if self._threshold and candidates_after_reranker[0].score < self._threshold:
            return candidates
        candidates[indices_for_reranking] = candidates_after_reranker

        return list(candidates)

    def _sort_candidates(self, candidates, req_info):
        candidate_scores = []
        for candidate in candidates:
            candidate_experiment = self._intent_flags[candidate.intent.name_for_reranker]
            current_score = req_info.experiments[candidate_experiment]
            try:
                current_score = float(current_score)
            except (ValueError, TypeError):
                logger.warning("Invalid value of flag %s", candidate_experiment)
                current_score = -np.inf
            candidate_scores.append(current_score)
        candidate_scores = np.array(candidate_scores)
        candidates = [evolve(candidate, intent=evolve(candidate.intent, score=score))
                      for candidate, score in izip(candidates, candidate_scores)]
        return sorted(candidates, key=lambda candidate: candidate.intent.score, reverse=True)

    @classmethod
    def from_config(cls, config, factor_calcer):
        intent_flags = config.get('intent_flags', {})
        threshold = config.get('threshold')
        return cls(intent_flags, threshold)


class CombinedReranker(BaseReranker):
    NAME = 'combined'

    def __init__(self, active_slot_reranker, fixlist_reranker, passed_candidates_reranker,
                 apply_to_first_non_empty_group_only=False):
        # type: (BaseReranker, BaseReranker, BaseReranker, bool) -> NoReturn

        self._active_slot_reranker = active_slot_reranker
        self._fixlist_reranker = fixlist_reranker
        self._passed_candidates_reranker = passed_candidates_reranker
        self._apply_to_first_non_empty_group_only = apply_to_first_non_empty_group_only

    def __call__(self, sample_features, candidates, req_info=None):
        # type: (SampleFeatures, List[FormCandidate], ReqInfo) -> List[FormCandidate]

        active_slot_candidates, fixlist_candidates, passed_candidates = self._split_candidates(candidates)

        active_slot_candidates = self._active_slot_reranker(sample_features, active_slot_candidates, req_info)
        if active_slot_candidates and self._apply_to_first_non_empty_group_only:
            return active_slot_candidates + fixlist_candidates + passed_candidates

        fixlist_candidates = self._fixlist_reranker(sample_features, fixlist_candidates, req_info)
        if fixlist_candidates and self._apply_to_first_non_empty_group_only:
            return active_slot_candidates + fixlist_candidates + passed_candidates

        passed_candidates = self._passed_candidates_reranker(sample_features, passed_candidates, req_info)
        return active_slot_candidates + fixlist_candidates + passed_candidates

    def _split_candidates(self, candidates):
        # type: (List[FormCandidate]) -> Tuple[List[FormCandidate], List[FormCandidate], List[FormCandidate]]

        active_slot_candidates, fixlist_candidates, passed_candidates = [], [], []
        for candidate in candidates:
            if candidate.intent.is_active_slot:
                active_slot_candidates.append(candidate)
            elif candidate.intent.is_in_fixlist:
                fixlist_candidates.append(candidate)
            else:
                passed_candidates.append(candidate)
        return active_slot_candidates, fixlist_candidates, passed_candidates

    @classmethod
    def from_config(cls, config, factor_calcer):
        active_slot_reranker = create_reranker(
            model_name=config['active_slot_candidates_reranker']['model'],
            config=config['active_slot_candidates_reranker'].get('params', {}),
            factor_calcer=factor_calcer
        )

        fixlist_reranker = create_reranker(
            model_name=config['fixlist_candidates_reranker']['model'],
            config=config['fixlist_candidates_reranker'].get('params', {}),
            factor_calcer=factor_calcer
        )

        passed_candidates_reranker = create_reranker(
            model_name=config['passed_candidates_reranker']['model'],
            config=config['passed_candidates_reranker'].get('params', {}),
            factor_calcer=factor_calcer
        )

        return cls(
            active_slot_reranker=active_slot_reranker,
            fixlist_reranker=fixlist_reranker,
            passed_candidates_reranker=passed_candidates_reranker,
            apply_to_first_non_empty_group_only=config.get('apply_to_first_non_empty_group_only', False)
        )


def register_reranker_type(cls_type, model_name):
    _reranker_factories[model_name] = cls_type


def create_reranker(model_name, config, factor_calcer):
    assert model_name in _reranker_factories, 'Reranker {} is not registered'.format(model_name)
    return _reranker_factories[model_name].from_config(config, factor_calcer)


def create_default_reranker():
    return CombinedReranker(
        active_slot_reranker=DescendingByScoresReranker(),
        fixlist_reranker=DescendingByScoresReranker(),
        passed_candidates_reranker=DescendingByScoresReranker(),
    )


_reranker_factories = {}

register_reranker_type(CombinedReranker, CombinedReranker.NAME)
register_reranker_type(ExperimentBasedReranker, ExperimentBasedReranker.NAME)
register_reranker_type(DescendingByScoresReranker, DescendingByScoresReranker.NAME)
register_reranker_type(DescendingByScoresAndTransitionModelReranker, DescendingByScoresAndTransitionModelReranker.NAME)
register_reranker_type(CatboostReranker, CatboostReranker.NAME)
register_reranker_type(CatboostIndependentReranker, CatboostIndependentReranker.NAME)
register_reranker_type(ExternalModelReranker, ExternalModelReranker.NAME)
