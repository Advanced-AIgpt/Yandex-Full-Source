# coding: utf-8

from copy import deepcopy
from vins_core.nlu.token_classifier import create_token_classifier
from vins_core.nlu.neural.metric_learning.metric_learning import TrainMode


def metric_learning_with_index(train_data, params):
    params = deepcopy(params)

    params['metric_learning'] = TrainMode.METRIC_LEARNING_FROM_SCRATCH

    knn = create_token_classifier('knn', **params)
    knn.train(train_data)

    params['metric_learning'] = TrainMode.NO_METRIC_LEARNING
    knn = create_token_classifier('knn', **params)
    knn.train(train_data)

    return knn
