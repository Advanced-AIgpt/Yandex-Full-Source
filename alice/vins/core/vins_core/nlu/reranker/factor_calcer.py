# -*- coding: utf-8 -*-

import abc
import attr
import logging

# Imports used only for typing
import numpy as np  # noqa: UnusedImport
from typing import List, AnyStr, Dict, NoReturn  # noqa: UnusedImport
from vins_core.nlu.features.base import SampleFeatures  # noqa: UnusedImport
from vins_core.dm.form_filler.form_candidate import FormCandidate  # noqa: UnusedImport

logger = logging.getLogger()


class Factor(object):
    __metaclass__ = abc.ABCMeta

    @abc.abstractmethod
    def append_factor_values(self, context, factor_values):
        # type: (FactorCalcerContext, List[np.ndarray]) -> NoReturn
        raise NotImplementedError

    @abc.abstractmethod
    def get_factor_value_names(self):
        # type: () -> List[AnyStr]
        raise NotImplementedError

    @classmethod
    @abc.abstractmethod
    def from_config(cls, config):
        # type: (Dict) -> Factor
        raise NotImplementedError


@attr.s
class FactorCalcerContext(object):
    sample_features = attr.ib()  # type: SampleFeatures
    classes = attr.ib(default=attr.Factory(list))  # type: List[AnyStr]
    original_classes = attr.ib(default=attr.Factory(list))  # type: List[AnyStr]
    scenarios_scores = attr.ib(default=attr.Factory(list))


class FactorCalcer(object):
    def __init__(self, factors):
        # type: (Dict[AnyStr, Factor]) -> NoReturn

        self._factors = factors

    def __call__(self, sample_features, candidates, required_factors):
        # type: (SampleFeatures, List[FormCandidate], List[AnyStr]) -> np.ndarray

        context = FactorCalcerContext(sample_features)
        context.classes = [candidate.intent.name_for_reranker for candidate in candidates]
        context.original_classes = [candidate.intent.name for candidate in candidates]
        context.scenarios_scores = [candidate.intent.score for candidate in candidates]
        factor_values = []
        for factor_name in required_factors:
            self._factors[factor_name].append_factor_values(context, factor_values)

        return np.concatenate(factor_values, axis=-1)

    def get_factor(self, name):
        # type: (AnyStr) -> Factor
        return self._factors[name]

    @classmethod
    def from_config(cls, config):
        return cls(factors={
            factor_name: create_factor(factor_config['model'], factor_config.get('params', {}))
            for factor_name, factor_config in config.get('factors', {}).iteritems()
        })


def register_factor_type(cls_type, model_name):
    _factor_factories[model_name] = cls_type


def create_factor(model_name, config):
    assert model_name in _factor_factories, 'Factor {} is not registered'.format(model_name)
    return _factor_factories[model_name].from_config(config)


_factor_factories = {}
