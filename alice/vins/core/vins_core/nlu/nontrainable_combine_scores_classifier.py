# coding: utf-8
from __future__ import unicode_literals

from vins_core.nlu.token_classifier import create_token_classifier
from vins_core.nlu.nontrainable_token_classifier import create_nontrainable_token_classifier
from vins_core.nlu.classifier import Classifier
from vins_core.utils.misc import multiply_dicts, best_score_dicts

from operator import mul
from collections import defaultdict
from copy import deepcopy


class NontrainableCombineScoresClassifier(Classifier):
    def __init__(self, method, name=None, model='combine_scores', select_features=(), classifiers=None, **kwargs):
        """
        :param classifiers: list of TokenClassifier objects
        :param method: method used to combine scores - 'multiply' or 'best_score'
        """
        super(NontrainableCombineScoresClassifier, self).__init__(
            model=model,
            name=name,
            select_features=select_features,
            **kwargs
        )

        self._classifiers_configs = classifiers or []
        self._classifiers = []

        if method == 'multiply':
            self._op = multiply_dicts
            self._reduce_op = mul
        elif method == 'best_score':
            self._op = best_score_dicts
            self._reduce_op = max
        else:
            raise ValueError('Unknown method for combine classifiers scores: %s' % method)

        for classifier_config in classifiers:
            params = deepcopy(classifier_config.get('params', {}))
            params.update(kwargs)
            self._classifiers.append(self._create_token_classifier(
                model=classifier_config['model'],
                name=classifier_config.get('name', None),
                model_file=classifier_config.get('model_file', None),
                select_features=classifier_config.get('features', ()),
                **params))

    def _create_token_classifier(self, **kwargs):
        archive = kwargs.get('archive', None)
        model_file = kwargs.get('model_file', None)
        name = kwargs.get('name', None)
        classifier = None
        if archive is not None or model_file is not None:
            classifier = create_nontrainable_token_classifier(**kwargs)
        if classifier is None:
            classifier = create_token_classifier(**kwargs)
            if archive is not None:
                classifier.load(archive, name)
        return classifier

    def _process(self, feature, skip_classifiers=(), **kwargs):
        likelihoods = []
        default_score = 1
        for classifier in self._classifiers:
            if classifier.name in skip_classifiers:
                continue
            classifier_result = defaultdict(lambda x=classifier.default_score: x)
            classifier_result.update(classifier(feature, skip_classifiers=skip_classifiers, **kwargs))
            likelihoods.append(classifier_result)
            default_score *= classifier.default_score
        if not likelihoods:
            return {}
        else:
            result = defaultdict(lambda: default_score)
            result.update(self._op(likelihoods))
            return result

    @property
    def need_normalize(self):
        return all(classifier.need_normalize for classifier in self._classifiers)

    @property
    def features(self):
        result = set()
        for classifier in self._classifiers:
            result.update(classifier.features)
        return result

    @classmethod
    def _validate(cls, predictions):
        return True

    @property
    def default_score(self):
        return 1.0

    @property
    def classes(self):
        result = []
        for classifier in self._classifiers:
            result.extend(classifier.classes)
        return result

    def fixlist_score(self, skip_classifiers=()):
        fixlist_score = None
        for classifier in self._classifiers:
            if classifier.name in skip_classifiers:
                continue
            classifier_fixlist_score = classifier.fixlist_score(skip_classifiers)
            if classifier_fixlist_score is None:
                return None

            if fixlist_score is None:
                fixlist_score = classifier_fixlist_score
            else:
                fixlist_score = self._reduce_op(fixlist_score, classifier_fixlist_score)
        return fixlist_score
