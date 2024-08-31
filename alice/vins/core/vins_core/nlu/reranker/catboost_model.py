# -*- coding: utf-8 -*-

import cPickle as pickle

from catboost import Pool, CatBoost

from vins_core.utils.data import get_resource_full_path

# Imports used only for typing
from typing import NoReturn, Dict, List  # noqa: UnusedImport


class CatboostModel(object):
    def __init__(self, model, cat_features):
        # type: (CatBoost, List[int]) -> NoReturn

        self._model = model
        self._cat_features = cat_features

    def predict(self, factors):
        # type: (np.ndarray) -> np.ndarray

        data = Pool(factors, cat_features=self._cat_features)
        return self._model.predict(data, thread_count=1)

    @classmethod
    def from_config(cls, config):
        # type: (Dict) -> NoReturn

        model_path = get_resource_full_path(config['model_resource'])
        desc_path = get_resource_full_path(config['desc_resource'])

        model = CatBoost()
        model.load_model(model_path)

        with open(desc_path, 'rb') as f:
            _, cat_features = pickle.load(f)

        return cls(model, cat_features)
