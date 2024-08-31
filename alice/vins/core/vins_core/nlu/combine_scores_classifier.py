# coding: utf-8
from __future__ import unicode_literals

from vins_core.nlu.token_classifier import create_token_classifier
from vins_core.nlu.classifier import Classifier
from vins_core.utils.misc import multiply_dicts, best_score_dicts

from operator import mul
from collections import defaultdict
from copy import deepcopy


class CombineScoresClassifier(Classifier):
    def __init__(self, method, name=None, model='combine_scores', select_features=(), classifiers=None, **kwargs):
        """
        :param classifiers: list of TokenClassifier objects
        :param method: method used to combine scores - 'multiply' or 'best_score'
        """
        super(CombineScoresClassifier, self).__init__(model=model, name=name, select_features=select_features, **kwargs)

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
            self._classifiers.append(create_token_classifier(
                model=classifier_config['model'],
                name=classifier_config.get('name', None),
                model_file=classifier_config.get('model_file', None),
                select_features=classifier_config.get('features', ()),
                **params))

    def train(self, intent_to_features=None, **kwargs):
        for classifier in self._classifiers:
            classifier.train(intent_to_features, **kwargs)
        return self

    @property
    def trained(self):
        return all(classifier.trained for classifier in self._classifiers)

    def load(self, archive, name, **kwargs):
        self._name = name
        for config, classifier in zip(self._classifiers_configs, self._classifiers):
            classifier.load(archive, config['name'], **kwargs)

    def save(self, archive, name):
        for classifier in self._classifiers:
            classifier.save(archive, classifier.name)

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

    def add_classifier(self, classifier):
        self._classifiers.append(classifier)

    def update_classifier(self, updated_classifier, updated_classifier_config=None):
        for i, classifier in enumerate(self._classifiers):
            if updated_classifier.name in classifier.trainable_classifiers:
                # If updated classifier found in trainable_classifiers and name of current classifier equals name of
                # updated classifier we update current classifier. In case names are different, updated classifier
                # located inside current, so we call method update classifier on current classifier.
                if updated_classifier.name == classifier.name:
                    self._classifiers[i] = updated_classifier
                    if updated_classifier_config:
                        self._classifiers_configs[i] = updated_classifier_config
                else:
                    classifier.update_classifier(updated_classifier, updated_classifier_config)
                break

    @property
    def trainable_classifiers(self):
        result = super(CombineScoresClassifier, self).trainable_classifiers
        for classifier in self._classifiers:
            result.update(classifier.trainable_classifiers)
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
