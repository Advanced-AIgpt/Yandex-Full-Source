from __future__ import unicode_literals

import cPickle as pickle
import codecs
import logging
import numbers
import os
import numpy as np
import pandas as pd
import re

from collections import Mapping
from itertools import izip
from operator import attrgetter
from fasteners.process_lock import InterProcessLock
from sklearn.model_selection import PredefinedSplit
from sklearn.metrics import classification_report, f1_score, precision_recall_fscore_support
from sklearn.model_selection import GridSearchCV, cross_val_score, StratifiedKFold
from sklearn.preprocessing import LabelEncoder

from vins_core.nlu.classifier import Classifier
from neural.nn_classifier import (
    RNNClassifierModel, CNNClassifierModel,
    ResidualCNNClassifierModel, ResidualRNNClassifierModel
)
from vins_core.utils.iter import first_of
from vins_core.utils.misc import dict_zip
from vins_core.nlu.features.post_processor.selector import SelectorFeaturesPostProcessor
from vins_core.nlu.features.post_processor.char_indexer import CharIndexerFeaturesPostProcessor
from vins_core.nlu.features.post_processor.vectorizer import (
    VectorizerFeaturesPostProcessor
)
from vins_core.nlu.sklearn_utils import make_pipeline
from vins_core.utils.data import get_resource_full_path, TarArchive

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
logger = logging.getLogger(__name__)


class TokenClassifierScorer(object):

    def __init__(self, label_encoder, output_file=None, output_data='errors',
                 intent_mapper=None, average='micro', **kwargs):
        assert output_data in ('report', 'errors')
        self._label_encoder = label_encoder
        self._output_file = output_file
        self._output_data = output_data
        self._intent_mapper = intent_mapper
        self._average = average

    def _get_labels_probs(self, estimator, X, y):
        p_pred = estimator.predict_proba(X)
        y_pred = np.argmax(p_pred, axis=-1)
        true_labels = self._label_encoder.inverse_transform(y)
        pred_labels = self._label_encoder.inverse_transform(y_pred)
        return self._map_intent_names(true_labels), self._map_intent_names(pred_labels), p_pred

    def _map_intent_names(self, intent_names):
        if not self._intent_mapper:
            return intent_names
        new_intent_names = []
        for intent_name in intent_names:
            new_intent_names.append(first_of((
                new_intent for intent_pattern, new_intent in self._intent_mapper.iteritems()
                if re.match(intent_pattern, intent_name, re.U)
            ), default=intent_name))
        return new_intent_names

    @classmethod
    def _get_score(cls, true_labels, pred_labels, average):
        if average == 'wmacro':
            p, r, f, s = precision_recall_fscore_support(true_labels, pred_labels, average=None)
            return np.mean(f[np.where(s > 0)])
        else:
            return f1_score(true_labels, pred_labels, average=average)

    def __call__(self, estimator, X, y):
        true_labels, pred_labels, probs = self._get_labels_probs(estimator, X, y)
        score = self._get_score(true_labels, pred_labels, self._average)
        logger.info('Num samples: {0}, score: {1}'.format(len(true_labels), score))
        if self._output_file:
            errors = []
            samples = map(attrgetter('sample'), X)
            for true_label, pred_label, sample, probs in izip(true_labels, pred_labels, samples, probs):
                if true_label != pred_label:
                    max_scores = sorted(probs, reverse=True)[:5]
                    errors.append((sample.text, true_label, pred_label, max_scores))
            with InterProcessLock(self._output_file + '.lock'):
                with codecs.open(self._output_file, mode='a', encoding='utf-8') as fout:
                    if self._output_data == 'errors':
                        for text, true_label, pred_label, p in errors:
                            fout.write('%s\t%s\t%s\t%s\n' % (text, true_label, pred_label, ';'.join(map(str, p))))
                    elif self._output_data == 'report':
                        fout.write(classification_report(y_true=true_labels, y_pred=pred_labels) + '\n')
        return score


class TokenClassifier(Classifier):
    _MODEL_DATA_FILE = 'model_data.pkl'

    def __init__(self, random_state=42, select_features=(), allow_singletons=False, **kwargs):
        """
        Compiles pipeline for feature extraction followed by estimator
        Typical pipeline starts with FeaturesExtractor object and ends with valid sklearn classifier
        In any case every step should be valid sklearn pipeline object
        (see e.g. http://scikit-learn.org/stable/modules/pipeline.html#pipeline)
        Definition of pipeline wrapped into _compile function, for example:
            self._compile([
                FeaturesExtractor(NGramFeatureExtractor(1), NGramFeatureExtractor(2)),
                DictVectorizer(),
                SVC()
            ])
        :return:
        """
        super(TokenClassifier, self).__init__(**kwargs)
        self.random_state = random_state
        self._features = select_features
        self._allow_singletons = allow_singletons

        self._label_encoder = LabelEncoder()
        self._pipeline = []
        self._model = None

        if self._features:
            self._pipeline.append(SelectorFeaturesPostProcessor(self._features))

    @classmethod
    def from_dict(cls, d):
        clf = cls()
        clf._model = d.get('model')
        clf._label_encoder = d.get('label_encoder', LabelEncoder())
        return clf

    def _make_pipeline(self, *steps):
        pipeline_steps = []
        for step in steps:
            if isinstance(step, (list, tuple)):
                pipeline_steps.extend(step)
            else:
                pipeline_steps.append(step)
        return make_pipeline(*pipeline_steps)

    def _get_fit_params(self, kwargs):
        return {}

    def train(self, intent_to_features, reset_model=True, **kwargs):
        """
        Training pipelined classifier
        :param intent_to_samples: mappable object where key are any hashable label and values are list of Sample
        :param reset_model: whether to reset input/output indexers
        :return: self
        """
        x = self.get_input(intent_to_features, reset_model=reset_model, **kwargs)
        y = self.get_output(intent_to_features, reset_model=reset_model, **kwargs)
        if len(self._label_encoder.classes_) > 1 or self._allow_singletons:
            self._model = self._make_pipeline(*self._pipeline)
            self._model.fit(x, y, **self._get_fit_params(kwargs))
        return self

    def transform_to_final_estimator(self, features, labels):
        x = self.get_input(features, reset_model=False)
        y = self.get_output(labels, reset_model=False)
        for _, transformer in self._model.steps[:-1]:
            x = transformer.transform(x)
        return x, y

    def _process(self, feature, skip_classifiers=(), **kwargs):
        if feature:
            scores = self.predict([feature], **kwargs)[0]
        else:
            scores = [0] * len(self._label_encoder.classes_)
        return dict_zip(keys=self._label_encoder.classes_, values=scores)

    def replace_labels(self, label_maps):
        self._label_encoder.classes_ = [
            label_maps.get(class_, class_)
            for class_ in self._label_encoder.classes_
        ]

    @property
    def label_encoder(self):
        return self._label_encoder

    @property
    def classes(self):
        if hasattr(self._label_encoder, 'classes_'):
            return self._label_encoder.classes_
        return []

    def predict(self, features, **kwargs):
        error_message = 'Classifier {} has not been trained or loaded and has no class labels.'
        assert hasattr(self._label_encoder, 'classes_'), error_message.format(self.name)
        if len(self._label_encoder.classes_) == 1 and not self._allow_singletons:
            return [[1.]] * len(features)
        x = self.get_input(features, reset_model=False, **kwargs)
        return self._model.predict_proba(x)

    def step_params(self, steps_params):
        """
        Helper intended to make input paramaters compatible with sklearn pipeline
        sklearn follows special convention for runtime pipeline params:
        http://scikit-learn.org/stable/modules/generated/sklearn.pipeline.Pipeline.html#sklearn.pipeline.Pipeline.fit_transform
        :param steps_params: could be one of:
                    - dict: { parameter: value } (if there's no pipeline step with any parameter, exception is raised)
                    - list of tuples: (pipeline step, dict { parameter: value })
        :return: dict of params
        """
        p = {}
        if isinstance(steps_params, dict):
            steps_params = [(None, steps_params)]
        for step, params in steps_params:
            for param, value in params.iteritems():
                if step is None:
                    # try to find corresponding pipeline step
                    # that matches all specified parameters
                    target_step = first_of(_step for _step in self._pipeline if hasattr(_step, param))
                    if target_step is None:
                        raise StopIteration('Invalid parameter specified: %s' % param)
                else:
                    target_step = step
                step_name = type(target_step).__name__.lower()
                p['%s__%s' % (step_name, param)] = value
        return p

    def save(self, archive, name):
        tmp = archive.get_tmp_dir()
        data = {
            'transformers': [step for _, step in self._model.steps[:-1]],
            'label_encoder': self._label_encoder
        }

        with open(os.path.join(tmp, self._MODEL_DATA_FILE), 'wb') as f:
            pickle.dump(data, f, pickle.HIGHEST_PROTOCOL)

        self.final_estimator.save_to_dir(tmp, min_conversion_size_bytes=100)

        for file_name in os.listdir(tmp):
            archive.add(os.path.join(name, file_name), os.path.join(tmp, file_name))

    def load(self, archive, name, **kwargs):
        self._name = name
        data = pickle.load(archive.get_by_name(name))
        self._model = data['model']
        self._label_encoder = data['label_encoder']

    def get_input(self, features, reset_model, **kwargs):
        if isinstance(features, Mapping):  # intent-to-samples map is given
            features = sum(features.itervalues(), [])
        return features

    def get_output(self, intents, reset_model, **kwargs):
        if isinstance(intents, Mapping):
            intents = sum(([intent] * len(features) for intent, features in intents.iteritems()), [])
        if reset_model:
            y = self._label_encoder.fit_transform(intents)
        else:
            y = self._label_encoder.transform(intents) if intents else []
        return y

    def _get_cv(self, cv=3, x=(), y=(), x_val=(), y_val=()):
        if len(x_val):
            cv = PredefinedSplit([-1] * len(x) + [1] * len(x_val))
        elif isinstance(cv, numbers.Integral):
            cv = StratifiedKFold(n_splits=int(cv), shuffle=True, random_state=self.random_state)
        return cv

    def crossvalidation(self, intent_to_features, cv=3, n_jobs=1, verbose=2, average='micro', output_file=None,
                        custom_scorer=None, **kwargs):
        reset_model = True
        input = self.get_input(intent_to_features, reset_model, **kwargs)
        output = self.get_output(intent_to_features, reset_model, **kwargs)
        if custom_scorer:
            scorer = custom_scorer
        else:
            scorer = TokenClassifierScorer(self._label_encoder, average=average, output_file=output_file)
        cross_val_scores = cross_val_score(
            estimator=self._make_pipeline(*self._pipeline),
            X=input,
            y=output,
            cv=self._get_cv(cv), n_jobs=n_jobs, verbose=verbose,
            scoring=scorer
        )
        return np.mean(cross_val_scores), np.std(cross_val_scores, ddof=1)

    def gridsearch(self, intent_to_features, param_grid, n_jobs=1, cv=3, refit=True,
                   verbose=2, average='micro', validation_data=None, intent_mapper=None, output_file=None,
                   custom_scorer=None, **kwargs):
            """
            Makes grid search over parameters grid, and stores best-scoring model
            :param samples: input samples
            :param param_grid: dict where keys are paramater names, and values are list of parameter values
            :param n_jobs: number of parallel processes
            :param cv: number of cross-validation folds if validation_data=None
            :param refit: if True, rebuilds inner model with best parameter set
            :param verbose: verbosity level
            :param average: f-score averaging method ('micro', 'macro')
            :param validation_data: specify fixed set validation data
            :param kwargs: input, output, fitting-stage optional parameters
            :return:
            """
            reset_model = True  # model always reset
            x = self.get_input(intent_to_features, reset_model, **kwargs)
            y = self.get_output(intent_to_features, reset_model, **kwargs)
            if validation_data:
                x_val = self.get_input(validation_data, reset_model=False, **kwargs)
                y_val = self.get_output(validation_data, reset_model=False, **kwargs)
            else:
                x_val, y_val = [], []
            if not custom_scorer:
                scorer = TokenClassifierScorer(
                    self._label_encoder,
                    average=average,
                    intent_mapper=intent_mapper,
                    output_file=output_file
                )
            else:
                scorer = custom_scorer
            gs = GridSearchCV(
                estimator=self._make_pipeline(*self._pipeline),
                param_grid=self.step_params(param_grid),
                scoring=scorer,
                n_jobs=n_jobs, verbose=verbose, refit=refit,
                cv=self._get_cv(cv, x, y, x_val, y_val),
                return_train_score=False,
                pre_dispatch=n_jobs
            )
            gs.fit(x + x_val, np.hstack((y, y_val)).astype('int'))
            if refit:
                self._model = gs.best_estimator_
            return pd.DataFrame(gs.cv_results_)  # TODO: squeeze back parameter names

    @property
    def final_estimator(self):
        if self._model:
            return self._model._final_estimator
        else:
            raise AttributeError(
                'The model %s has not final estimator'
                ' because it is not trained / loaded' % self.__class__.__name__
            )

    @property
    def trained(self):
        return bool(self._model)

    @property
    def default_score(self):
        return 0


class SequentialTokenClassifier(TokenClassifier):

    def __init__(self, sparse=True, vectorizer=True, **kwargs):
        super(SequentialTokenClassifier, self).__init__(**kwargs)
        self.sparse = sparse
        if vectorizer:
            self._pipeline.append(VectorizerFeaturesPostProcessor(sparse=self.sparse, sequential=True))


class NNTokenClassifier(SequentialTokenClassifier):

    def __init__(self, **kwargs):
        super(NNTokenClassifier, self).__init__(**kwargs)
        if kwargs.get('model') == 'rnn':
            self._pipeline.append(RNNClassifierModel(**kwargs))
        elif kwargs.get('model') == 'cnn':
            self._pipeline.append(CNNClassifierModel(**kwargs))
        elif kwargs.get('model') == 'res-rnn':
            self._pipeline.append(ResidualRNNClassifierModel(**kwargs))
        elif kwargs.get('model') == 'res-cnn':
            self._pipeline.append(ResidualCNNClassifierModel(**kwargs))

    def save_progress(self, archive, name):
        self.final_estimator.save(archive, name)
        super(NNTokenClassifier, self).save(archive, name)

    def load(self, archive, name, **kwargs):
        super(NNTokenClassifier, self).load(archive, name, **kwargs)
        self.final_estimator.load(archive, name)


class CharCNNTokenClassifier(TokenClassifier):

    def __init__(self, nb_epoch=20, **kwargs):
        super(CharCNNTokenClassifier, self).__init__(**kwargs)

        self._pipeline = [
            CharIndexerFeaturesPostProcessor(),
            CNNClassifierModel(
                maxlen=100,
                input_projection_dim=100,
                encoder_dim=256,
                ngram_range=(2, 10, 2),
                fc_layers=(512,),
                nb_epoch=nb_epoch
            )
        ]

    def save_progress(self, archive, name):
        self.final_estimator.save(archive, name)
        super(CharCNNTokenClassifier, self).save(archive, name)

    def load(self, archive, name, **kwargs):
        super(CharCNNTokenClassifier, self).load(archive, name, **kwargs)
        self.final_estimator.load(archive, name)


def create_token_classifier(model, model_file=None, name=None, **kwargs):
    if model not in _token_classifier_factories:
        return None
    clf = _token_classifier_factories[model](model=model, name=name, **kwargs)
    if model_file:
        model_file = get_resource_full_path(model_file)
        with TarArchive(model_file) as archive:
            clf.load(archive, name or model)
    return clf


def register_token_classifier_type(cls_type, name):
    _token_classifier_factories[name] = cls_type


_token_classifier_factories = {}

register_token_classifier_type(CharCNNTokenClassifier, 'charcnn')
register_token_classifier_type(NNTokenClassifier, 'rnn')
register_token_classifier_type(NNTokenClassifier, 'cnn')
