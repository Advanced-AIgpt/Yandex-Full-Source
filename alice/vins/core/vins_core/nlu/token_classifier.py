# coding: utf-8
from __future__ import unicode_literals

import cPickle as pickle
import codecs
import logging
import numbers
import re
from collections import Mapping, defaultdict
from itertools import izip
from functools import partial
from operator import attrgetter
from json import dumps

import attr
import numpy as np
import pandas as pd
import scipy.sparse
import tempfile
from fasteners.process_lock import InterProcessLock
from hnsw import EVectorComponentType, EDistance, Pool, Hnsw
from marisa_trie import BytesTrie
from sklearn.base import BaseEstimator, ClassifierMixin
from sklearn.ensemble import GradientBoostingClassifier
from sklearn.linear_model import LogisticRegression
from sklearn.metrics import classification_report, f1_score, precision_recall_fscore_support
from sklearn.model_selection import GridSearchCV, cross_val_score, StratifiedKFold, PredefinedSplit
from sklearn.preprocessing import LabelEncoder

from vins_core.common.knn_cache import get_knn_cache
from vins_core.nlu import knn
from vins_core.nlu.classifier import Classifier
from vins_core.nlu.features.post_processor.selector import SelectorFeaturesPostProcessor
from vins_core.nlu.features.post_processor.std_normalizer import StdNormalizerFeaturesPostProcessor
from vins_core.nlu.features.post_processor.vectorizer import VectorizerFeaturesPostProcessor
from vins_core.nlu.neural.metric_learning.metric_learning import MetricLearningFeaturesPostProcessor, TrainMode
from vins_core.nlu.sklearn_utils import encode_labels, make_pipeline
from vins_core.utils.config import get_setting, get_bool_setting
from vins_core.utils.data import load_data_from_file
from vins_core.utils.decorators import time_info
from vins_core.utils.iter import first_of
from vins_core.utils.metrics import sensors
from vins_core.utils.misc import dict_zip


logger = logging.getLogger(__name__)
features_logger = logging.getLogger('features')


def _get_knn_cache_key(salt, input_vectors, input_texts):
    text_cache_key = '#'.join(input_texts)
    vectors_cache_key = input_vectors.tostring()
    return b'{}##{}##{}'.format(salt.encode('utf-8'), text_cache_key.encode('utf-8'), vectors_cache_key)


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
    is_updatable = False

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

    def train(self, intent_to_features, reset_model=True, update_only=False, **kwargs):
        """
        Training pipelined classifier
        :param intent_to_samples: mappable object where key are any hashable label and values are list of Sample
        :param reset_model: whether to reset input/output indexers
        :return: self
        """
        x = self.get_input(intent_to_features, reset_model=reset_model, update_only=update_only, **kwargs)
        y = self.get_output(intent_to_features, reset_model=reset_model, update_only=update_only, **kwargs)
        if len(self._label_encoder.classes_) > 1 or self._allow_singletons:
            if update_only:
                if not getattr(self, 'is_updatable', False):
                    raise ValueError('Argument "update_only" is set, but classifier {} is not updatable'.format(self))
                x_transformed = x
                fit_params = self._get_fit_params(kwargs)
                params_by_step = defaultdict(dict)
                for key, value in fit_params.iteritems():
                    step_name, param_name = key.split('__', 1)
                    params_by_step[step_name][param_name] = value
                for step_id, (step_name, step) in enumerate(self._model.steps):
                    if getattr(step, 'is_updatable', False):
                        logger.info('Partially updating the classifier step {}'.format(step_name))
                        step_params = params_by_step.get(step_name, {})
                        step_params['update_only'] = True
                        step.fit(x_transformed, y, **step_params)
                        if step_id < len(self._model.steps) - 1:
                            x_transformed = step.transform(x_transformed)
                    else:
                        logger.info('Skipping partial update of the classifier step {}'.format(step_name))
                        x_transformed = step.transform(x_transformed)
                logger.info('Partial update has been successful')
            else:
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

    @property
    def label_encoder(self):
        return self._label_encoder

    @property
    def classes(self):
        if hasattr(self._label_encoder, 'classes_'):
            return self._label_encoder.classes_
        return []

    def predict(self, features, req_info=None, **kwargs):
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
        tmp = archive.get_tmp_file()
        data = {
            'model': self._model,
            'label_encoder': self._label_encoder
        }

        with open(tmp, 'wb') as f:
            pickle.dump(data, f, pickle.HIGHEST_PROTOCOL)

        archive.add(name, tmp)

    def load(self, archive, name, **kwargs):
        self._name = name
        data = pickle.load(archive.get_by_name(name))
        self._model = data['model']
        self._label_encoder = data['label_encoder']
        # need to reconstruct the pipeline - for updates
        self._pipeline = [step[1] for step in self._model.steps]

    def get_input(self, features, reset_model, **kwargs):
        if isinstance(features, Mapping):  # intent-to-samples map is given

            features = sum(features.itervalues(), [])
        return features

    def get_output(self, intents, reset_model, update_only=False, **kwargs):
        if isinstance(intents, Mapping):
            intents = sum(([intent] * len(features) for intent, features in intents.iteritems()), [])
        if update_only:
            new_classes = set(intents).difference(self._label_encoder.classes_)
            logger.debug('New classes for classifier update are {} of {}'.format(new_classes, len(set(intents))))
            if new_classes:
                self._label_encoder.classes_ = np.concatenate(
                    [self._label_encoder.classes_, sorted(new_classes)]
                )
            reset_model = False
        if reset_model:
            y = self._label_encoder.fit_transform(intents)

        else:
            _, y = encode_labels(intents, uniques=self._label_encoder.classes_, encode=True) if intents else (None, [])
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


class BagTokenClassifier(TokenClassifier):

    def __init__(self, sparse=True, **kwargs):
        super(BagTokenClassifier, self).__init__(**kwargs)
        self.sparse = sparse
        self._pipeline.extend([
            VectorizerFeaturesPostProcessor(sparse=self.sparse, sequential=False),
        ])


class GradientBoostingTokenClassifier(BagTokenClassifier):

    def __init__(self, lr=0.1, n_trees=100, subsample=0.5, **kwargs):
        """
        Any params from TokenClassifier, CommonFeaturesMixinTokenClassifier +
        :param lr: gradient boosting learning rate
        :param n_trees: number of trees in the final model
        :param subsample: fraction of samples per tree, lies in [0, 1]
        :return:
        """
        if kwargs.get('sparse'):
            raise NotImplementedError(
                'Gradient boosting model is unable to work with sparse inputs. '
                'Please specify sparse=False explicitly on initialization'
            )
        super(GradientBoostingTokenClassifier, self).__init__(**kwargs)

        self._pipeline.append(
            GradientBoostingClassifier(
                learning_rate=lr,
                n_estimators=n_trees,
                subsample=subsample,
                random_state=self.random_state
            )
        )


class MaxentTokenClassifier(BagTokenClassifier):

    def __init__(self, l2reg=1e-2, balanced_classes=True, multi_class='multinomial', **kwargs):
        """
        Any params from TokenClassifier, CommonFeaturesMixinTokenClassifier +
        :param l2reg: regularization coefficient (not inverse!)
        :param balanced_classes: whether automatic class balancing should be used
        :param multi_class: multi class mode
        :return:
        """
        super(MaxentTokenClassifier, self).__init__(**kwargs)

        self._pipeline.extend([
            LogisticRegression(
                multi_class=multi_class,
                solver='lbfgs',
                penalty='l2',
                C=1. / l2reg,
                class_weight='balanced' if balanced_classes else None,
                random_state=self.random_state,
                verbose=2,
                n_jobs=int(get_setting('NUM_PROCS', 1))
            )
        ])


@attr.s(frozen=True)
class LabelKNNMeta(object):
    label = attr.ib()  # type: str
    texts = attr.ib()  # type: List[str]
    vectors = attr.ib()  # type np.ndarray
    hnsw_index = attr.ib(default=None)  # type: Optional[str]

    @vectors.validator
    def _check_alignment(self, attribute, value):
        if value.shape[0] != len(self.texts):
            raise ValueError("texts and vectors must be aligned")

    def __len__(self):
        return len(self.texts)

    def to_dict(self, drop_label=True):
        d = attr.asdict(self, recurse=False)

        if drop_label:
            del d['label']

        return d

    @classmethod
    def from_dict(cls, d, label=None):
        args = {k: v for k, v in d.iteritems() if k in attr.fields_dict(cls)}
        if label is not None:
            args['label'] = label

        return cls(**args)


class _AbstractKNNModel(BaseEstimator, ClassifierMixin):
    is_updatable = True

    def __init__(self, num_neighbors=None, use_text_matching=None):
        self.num_neighbors = num_neighbors
        self.use_text_matching = use_text_matching
        if self.use_text_matching is None:
            self.use_text_matching = get_bool_setting('KNN_TEXT_MATCHING', default=True)

        self.update_only = None  # this entry is needed only for step_params

    def get_label_to_meta(self):
        raise NotImplementedError()

    def set_label_to_meta(self, label_to_meta):
        raise NotImplementedError()

    def _data_to_meta(self, X, y):
        vectors, texts = self._standartize_input(X)
        texts = np.array(texts)
        target_indices = defaultdict(list)
        for index, label in enumerate(y):
            target_indices[label].append(index)
        target_indices = {label: np.array(idx, dtype=np.int32) for label, idx in target_indices.iteritems()}
        for label, indices in target_indices.iteritems():
            meta = LabelKNNMeta(label, texts[indices].tolist(), vectors[indices])
            yield meta

    def fit(self, X, y, num_neighbors=1, update_only=False, **kwargs):
        logger.info('Start fitting %s', self.__class__.__name__)
        if self.num_neighbors is not None and self.num_neighbors != num_neighbors:
            logger.warning('num neighbors was set to {} in init and changed to {} in fit'.format(self.num_neighbors, num_neighbors))

        self.num_neighbors = num_neighbors

        if update_only:
            logger.debug('UPDATING ONLY knn_model on {} observations from {} classes'.format(len(y), len(set(y))))
            if len(set(y)) < 50:
                logger.debug('Classes are {}'.format(set(y)))

            self._fit_update(X, y)
        else:
            self._fit_reset(X, y)

    def _fit_update(self, X, y):
        existing_label_to_meta = self.get_label_to_meta()

        # todo: make sure that empty (removed) intents are handled properly
        for meta in self._data_to_meta(X, y):
            logger.debug('Updating class {} with {} rows'.format(meta.label, len(meta)))
            existing_label_to_meta[meta.label] = meta

        self.set_label_to_meta(existing_label_to_meta)

    def _fit_reset(self, X, y):
        label_to_meta = {meta.label: meta for meta in self._data_to_meta(X, y)}
        self.set_label_to_meta(label_to_meta)

    @time_info('predict_proba')
    @sensors.with_timer('knn_predict_time')
    def predict_proba(self, X):
        if self.num_neighbors is None:
            raise RuntimeError('num_neighbors was not set during fitting or configuring')

        input_vectors, input_texts = self._standartize_input(X)

        serialized_vectors = [[str(number) for number in vector] for vector in input_vectors]
        log = {"input_texts": input_texts, "input_vectors": serialized_vectors}
        json_log = dumps(log)
        features_logger.info(json_log)

        cache = get_knn_cache(default=None)
        invoke_prediction = partial(self._predict_w_text_match, input_vectors, input_texts)
        if cache is None:
            return invoke_prediction()
        try:
            cache_key = _get_knn_cache_key(self.__class__.__name__, input_vectors, input_texts)
            outputs = cache.get(cache_key)
            if outputs is None:
                outputs = invoke_prediction()
                cache.put(cache_key, pickle.dumps(outputs, pickle.HIGHEST_PROTOCOL))
                sensors.inc_counter('knn_redis_cache', labels={'type': 'miss'})
            else:
                outputs = pickle.loads(outputs)
                logger.info('using value from knn cache')
                sensors.inc_counter('knn_redis_cache', labels={'type': 'hit'})
            return outputs
        except Exception:
            logger.exception('unable to process cached prediction')
        return invoke_prediction()

    def _predict_w_text_match(self, input_vectors, input_texts):
        outputs = self._predict(input_vectors, input_texts)
        if not self.use_text_matching:
            return outputs

        # Fix exact matching samples, if it is not forbidden (for debugging purposes)
        for i, input_text in enumerate(input_texts):
            labels = self._match_text_to_labels(input_text)
            if labels:
                sensors.inc_counter('knn_exact_match_cache', labels={'type': 'hit'})
                outputs[i, labels] = 1
            else:
                sensors.inc_counter('knn_exact_match_cache', labels={'type': 'miss'})
                logger.debug('\"%s\" not found in knn cache', input_text)

        return outputs

    def _predict(self, input_vectors, input_texts):
        raise NotImplementedError()

    def _match_text_to_labels(self, text):
        raise NotImplementedError()

    def __setstate__(self, state):
        super(_AbstractKNNModel, self).__setstate__(state)
        self.update_only = None  # this entry is needed only for step_params

    def _standartize_input(self, X):
        vectors, batch_samples = X
        if isinstance(vectors, scipy.sparse.csr_matrix):
            vectors = vectors.toarray()
        return vectors, [sample.sample.text for sample in batch_samples]


def _get_found_neighbors(texts, outputs, number_of_maxima, num_inputs):
    sorted_from = outputs.shape[1] - number_of_maxima
    sorted_to = outputs.shape[1]
    result = []
    for i in xrange(num_inputs):
        result.append(
            texts[i, np.flipud(outputs[i].argpartition(range(sorted_from, sorted_to)))[:number_of_maxima]])
    return result


def _log_found_neighbors(input_texts, texts, outputs, num_inputs):
    number_of_maxima = 1 if not get_bool_setting('FEATURE_DEBUG') else 10
    found_neighbors = _get_found_neighbors(texts, outputs, number_of_maxima, num_inputs)
    log = []
    for sample_text, neighbors in zip(input_texts, found_neighbors):
        log.append('%s -->\n%s' % (sample_text, '\n'.join('\t' + n for n in neighbors)))

    log_msg = '\n'.join(log)
    logger.info('Found neighbors:\n%s' % log_msg)


class KNNModel(_AbstractKNNModel):
    def __init__(self, num_neighbors=None, use_text_matching=None):
        super(KNNModel, self).__init__(num_neighbors, use_text_matching)

        self._vectors, self._texts, self._labels, self._output_dim = None, None, None, None
        self._text_to_labels = BytesTrie()
        self._target_indices = defaultdict(list)
        self._knn_helper = None

    def get_label_to_meta(self):
        texts = np.array(self._texts)
        label2meta = {}
        for label, indices in self._target_indices.iteritems():
            label2meta[label] = LabelKNNMeta(label, texts[indices].tolist(), self._vectors[indices])

        return label2meta

    def set_label_to_meta(self, label_to_meta):
        self._target_indices = dict()
        self._vectors = []
        self._texts = []
        self._labels = []
        i = 0
        for label in sorted(label_to_meta.keys()):
            meta = label_to_meta[label]
            self._vectors.append(meta.vectors)
            self._texts.append(meta.texts)
            size = len(meta)
            self._labels.extend([label] * size)
            self._target_indices[label] = np.arange(i, i + size)
            i += size
        self._vectors = np.concatenate(self._vectors)
        self._texts = np.concatenate(self._texts).tolist()
        self._labels = np.array(self._labels)
        self._compile()

    def _compile(self):
        self._output_dim = max(self._labels) + 1
        self._target_indices = {label: np.array(idx, dtype=np.int32) for label, idx in self._target_indices.iteritems()}
        self._text_to_labels = BytesTrie(((text, bin(label)) for text, label in izip(self._texts, self._labels)))
        self._knn_helper = knn.Helper(self._target_indices)

    def _predict(self, input_vectors, input_texts):
        num_inputs = input_vectors.shape[0]
        outputs = np.zeros((num_inputs, self._output_dim), dtype=np.float32)
        scores = self._vectors.dot(input_vectors.T).astype(np.float32)
        argmax_y_all = np.zeros((num_inputs, self._output_dim), dtype=np.int32)
        self._knn_helper.get_average_scores(outputs, scores, argmax_y_all, num_inputs, self.num_neighbors)
        texts = np.empty((num_inputs, self._output_dim), dtype=object)

        for y in xrange(self._output_dim):
            argmax_y = argmax_y_all[:, y]
            texts[:, y] = [self._texts[input_index] for input_index in argmax_y]

        _log_found_neighbors(input_texts, texts, outputs, num_inputs)
        return outputs

    def _match_text_to_labels(self, text):
        bin_labels = self._text_to_labels.get(text, [])
        return [int(bin_label, base=2) for bin_label in bin_labels]

    def __setstate__(self, state):
        if 'store_found_neighbors' in state:
            del state['store_found_neighbors']
        super(KNNModel, self).__setstate__(state)
        # we assume here that the state doesn't contain vectors, texts, and text_to_labels
        # these properties will be saved and set externally

    def __getstate__(self):
        state = self.__dict__.copy()
        if '_knn_helper' in state:
            del state['_knn_helper']
        # vectors, texts, and text_to_labels trie should be renewed on model load - they will load from separate files
        state['_vectors'] = dict()
        state['_texts'] = dict()
        state['_labels'] = dict()
        state['_target_indices'] = dict()
        state['_text_to_labels'] = BytesTrie()
        return state


class HNSWKNNModel(_AbstractKNNModel):
    _SEARCH_NEIGHBORHOOD_SIZE = int(get_setting('SEARCH_NEIGHBORHOOD_SIZE', default=16))  # needed to be heuristically estimated with balance between quality / search time
    _INDEX_BUILD_SETTINGS = {
        'distance': EDistance.DotProduct,
        'max_neighbors': 32,
        'search_neighborhood_size': 1000,
        'num_exact_candidates': 300,
        'batch_size': 5000,
        'report_progress': True,
    }

    def __init__(self, num_neighbors=None, use_text_matching=None):
        super(HNSWKNNModel, self).__init__(num_neighbors, use_text_matching)

        # need to store pool reference for hnsw index to work correctly (python bindings bug)
        self._label_to_info = None  # label -> (meta, hnsw_index, hnsw_pool)

        self._text_to_labels = BytesTrie()
        self._output_dim = None

    def get_label_to_meta(self):
        return {label: meta for label, (meta, _, _) in self._label_to_info.iteritems()}

    def set_label_to_meta(self, label_to_meta):
        self._label_to_info = {}

        for label, meta in label_to_meta.iteritems():
            if not len(meta):
                continue

            if meta.hnsw_index is None:
                binary_index = self._build_binary_index(meta)
            else:
                binary_index = meta.hnsw_index

            meta_w_index = LabelKNNMeta(label, meta.texts, meta.vectors, binary_index)
            hnsw, pool = self._load_index(meta_w_index)
            self._label_to_info[label] = (meta_w_index, hnsw, pool)

        self._text_to_labels = BytesTrie(
            ((text, bin(label)) for label, (meta, _, _) in self._label_to_info.iteritems() for text in meta.texts))

        self._output_dim = max(self._label_to_info.iterkeys()) + 1

    def _load_index(self, meta):
        logger.info('Loading index for %s label, %d vectors', meta.label, len(meta))

        pool = self._create_pool(meta.vectors)
        hnsw = Hnsw()

        with tempfile.NamedTemporaryFile() as tmp:
            with open(tmp.name, 'wb') as f:
                f.write(meta.hnsw_index)

            hnsw.load(tmp.name, pool, EDistance.DotProduct)

        return hnsw, pool

    def _build_binary_index(self, meta):
        logger.info('Building index for %s label, %d vectors', meta.label, len(meta))

        pool = self._create_pool(meta.vectors)
        hnsw = Hnsw()
        hnsw.build(pool, **self._INDEX_BUILD_SETTINGS)

        with tempfile.NamedTemporaryFile() as tmp:
            hnsw.save(tmp.name)

            with open(tmp.name, 'rb') as f:
                return f.read()

    def _create_pool(self, vectors):
        vector_dim = vectors.shape[1]
        return Pool.from_bytes(vectors.astype(np.float32).tobytes(), EVectorComponentType.Float, vector_dim)

    def _predict(self, input_vectors, input_texts):
        input_vectors = input_vectors.astype(np.float32)
        num_inputs = input_vectors.shape[0]

        outputs = np.zeros((num_inputs, self._output_dim), dtype=np.float32)
        texts = np.empty((num_inputs, self._output_dim), dtype=object)

        for idx, input_vector in enumerate(input_vectors):
            vector_output, nearest_texts = self._predict_vector(input_vector)
            outputs[idx] = vector_output
            texts[idx] = nearest_texts

        _log_found_neighbors(input_texts, texts, outputs, num_inputs)

        return outputs

    def _predict_vector(self, input_vector):
        scores = np.zeros((self._output_dim, ), dtype=np.float32)
        texts = np.full((self._output_dim, ), fill_value="", dtype=object)

        for label, (meta, hnsw_index, _) in self._label_to_info.iteritems():
            neighbors = hnsw_index.get_nearest(
                input_vector,
                top_size=self.num_neighbors,
                search_neighborhood_size=self._SEARCH_NEIGHBORHOOD_SIZE,  # TODO(vl-trifonov) need to experiment with this param
            )  # sorted [(item_id, score_id)]

            if not neighbors:  # hnsw could find zero neighbors by design
                continue

            scores_sum = sum(score for _, score in neighbors)
            label_score = scores_sum / len(neighbors)
            nearest_id = neighbors[0][0]

            scores[label] = label_score
            texts[label] = meta.texts[nearest_id]

        return scores, texts

    def _match_text_to_labels(self, text):
        bin_labels = self._text_to_labels.get(text, [])
        return [int(bin_label, base=2) for bin_label in bin_labels]

    def __setstate__(self, state):
        super(HNSWKNNModel, self).__setstate__(state)
        # we assume here that the state doesn't contain vectors, texts, and text_to_labels
        # these properties will be saved and set externally

    def __getstate__(self):
        state = self.__dict__.copy()
        # inner state should be renewed on model load from saved files
        state['_label_to_info'] = dict()
        state['_text_to_labels'] = BytesTrie()
        return state


class KNNTokenClassifier(TokenClassifier):
    _META_FILENAME = 'model_meta_data'
    is_updatable = True

    def __init__(
        self, sparse=True, anchor=None, metric_function='euclidean',
        metric_learning=TrainMode.METRIC_LEARNING_FROM_SCRATCH, metric_learning_logdir=None,
        num_neighbors=None, std_normalize=True, intent_file_chunks=None, use_hnsw=False, **kwargs
    ):
        super(KNNTokenClassifier, self).__init__(**kwargs)
        logger.debug('Initializing KNN with metric_learning={}'.format(metric_learning))

        self.sparse = sparse
        self.anchor = anchor
        self.num_neighbors = num_neighbors
        self.metric_function = metric_function
        if intent_file_chunks:
            self.intent_file_chunks = load_data_from_file(intent_file_chunks)
        else:
            self.intent_file_chunks = {}
        self._metric_learning_from_scratch = False

        self._use_hnsw = use_hnsw or get_bool_setting('HNSW_KNN', default=False)

        if metric_function == 'metric_learning':
            if std_normalize:
                self._pipeline.append(StdNormalizerFeaturesPostProcessor())

            self._pipeline.append(MetricLearningFeaturesPostProcessor(
                train=metric_learning,
                logdir=metric_learning_logdir,
                **kwargs
            ))

            if metric_learning == TrainMode.METRIC_LEARNING_FROM_SCRATCH:
                self._metric_learning_from_scratch = True
        elif metric_function == 'euclidean':
            self._pipeline.append(VectorizerFeaturesPostProcessor(
                sparse=False, sequential=False, return_sample_features=True))
        else:
            raise ValueError('Unknown metric function "%s"' % metric_function)

        if not self._metric_learning_from_scratch:
            self._pipeline.append(self._create_knn_model())

    def _create_knn_model(self):
        model = HNSWKNNModel if self._use_hnsw else KNNModel
        return model(self.num_neighbors)

    def _get_fit_params(self, kwargs):
        fit_params = {}

        if not self._metric_learning_from_scratch:
            update_only = kwargs.get('update_only', False)
            fit_params.update(self.step_params({'num_neighbors': self.num_neighbors}))
            fit_params.update(self.step_params({'update_only': update_only}))

        if self.metric_function == 'metric_learning':
            fit_params.update(self.step_params({
                'class_labels': self.classes,
                'transition_model': kwargs.get('transition_model'),
                'intent_infos': kwargs.get('intent_infos')
            }))

        return fit_params

    @property
    def found_neighbors(self):
        return self.final_estimator.found_neighbors

    def predict(self, features, **kwargs):
        scores = super(KNNTokenClassifier, self).predict(features, **kwargs)
        if self.anchor:
            if isinstance(self.anchor, basestring):
                anchor_index = np.where(self.label_encoder.classes_ == self.anchor)[0][0]
                scores -= scores[:, anchor_index][:, np.newaxis]
            elif isinstance(self.anchor, numbers.Integral):
                anchor_index = np.argsort(scores, axis=1)[:, -self.anchor]
                scores -= scores[range(len(scores)), anchor_index]
        return scores

    def _get_model_chunks(self):
        chunks = defaultdict(list)
        patterns = [
            (chunk_config['name'], re.compile(chunk_config['pattern']))
            for chunk_config in self.intent_file_chunks.get('patterns', [])
        ]

        label_to_meta = self._model._final_estimator.get_label_to_meta()

        for class_index, class_name in enumerate(self._label_encoder.classes_):
            chunk_name = class_name
            if class_index not in label_to_meta:
                logger.warning('Class {} ({}) not found in class_map of len {}'.format(
                    class_index, class_name, len(label_to_meta))
                )
                continue

            meta = label_to_meta[class_index]
            class_data = meta.to_dict(drop_label=True)
            class_data['name'] = class_name

            is_matched = False
            for chunk_name, chunk_pattern in patterns:
                if re.match(chunk_pattern, class_name):
                    chunk_name = chunk_name
                    is_matched = True
                    break
            if not is_matched and self.intent_file_chunks.get('merge_children_intents'):
                chunk_name = class_name.split('__')[0]
            chunks[chunk_name].append(class_data)
        return chunks.iteritems()

    def save(self, archive, name):
        with archive.nested(name) as mother_archive:
            if not self._metric_learning_from_scratch:
                for cluster_filename, cluster_data in self._get_model_chunks():
                    tmp = archive.get_tmp_file()
                    with open(tmp, 'wb') as f:
                        pickle.dump(cluster_data, f, pickle.HIGHEST_PROTOCOL)
                    mother_archive.add(cluster_filename, tmp)
            tmp = archive.get_tmp_file()
            data = {
                'model': self._model
            }
            with open(tmp, 'wb') as f:
                pickle.dump(data, f, pickle.HIGHEST_PROTOCOL)
            mother_archive.add(self._META_FILENAME, tmp)

    def load(self, archive, name, **kwargs):
        with archive.nested(name) as mother_archive:
            self._name = name
            data = pickle.load(mother_archive.get_by_name(self._META_FILENAME))
            self._model = data['model']
            self._label_encoder = LabelEncoder()

            intents_data = []
            intents_names = []
            for filename in mother_archive.list():
                if filename == self._META_FILENAME:
                    continue
                cluster_data = pickle.load(mother_archive.get_by_name(filename))
                for class_data in cluster_data:
                    intents_data.append(class_data)
                    intents_names.append(class_data['name'])
            labels = self._label_encoder.fit_transform(intents_names)

            label_to_meta = {}
            for label, class_data in zip(labels, intents_data):
                label_to_meta[label] = LabelKNNMeta.from_dict(class_data, label=label)

            # if the last action has been metric learning training, then the final estimator is not KNN
            if isinstance(self._model._final_estimator, _AbstractKNNModel):
                # rebuild model to include right final KNN (type of KNN or its settings can change)
                new_steps = [step[1] for step in self._model.steps[:-1]]
                new_steps.append(self._create_knn_model())
                self._model = make_pipeline(*new_steps)

                # vectors are stored independently from model
                self._model._final_estimator.set_label_to_meta(label_to_meta)

            # if we wanted to learn metric from scratch, we would not load the model at all
            self._metric_learning_from_scratch = False
        # need to reconstruct the pipeline - for updates
        self._pipeline = [step[1] for step in self._model.steps]


class HNSWKNNTokenClassifierWrapper(object):
    """
    Use only for HNSW-based KNN AB-test
    TODO(vl-trifonov) Remove this after AB (https://st.yandex-team.ru/DIALOG-7775)
    """
    def __init__(self, *args, **kwargs):
        super(HNSWKNNTokenClassifierWrapper, self).__init__()

        kwargs_hnsw = dict(kwargs)
        kwargs_hnsw['use_hnsw'] = True

        kwargs_brute_force = dict(kwargs)
        kwargs_brute_force['use_hnsw'] = False

        self._hnsw_tc = KNNTokenClassifier(*args, **kwargs_hnsw)
        self._brute_force_tc = KNNTokenClassifier(*args, **kwargs_brute_force)

        if kwargs.get('use_hnsw', False):
            self._main_tc = self._hnsw_tc
        else:
            self._main_tc = self._brute_force_tc

    def load(self, *args, **kwargs):
        self._hnsw_tc.load(*args, **kwargs)
        self._brute_force_tc.load(*args, **kwargs)

    def __call__(self, *args, **kwargs):
        req_info = kwargs.get('req_info')
        use_hnsw = req_info is not None and req_info.experiments['hnsw_knn']
        use_brute = req_info is not None and req_info.experiments['brute_force_knn']

        if use_hnsw:
            clf = self._hnsw_tc
        elif use_brute:
            clf = self._brute_force_tc
        else:
            clf = self._main_tc

        return clf(*args, **kwargs)

    def __getattribute__(self, name):
        if name in ['_hnsw_tc', '_brute_force_tc', '_main_tc', 'load', '__call__']:
            return super(HNSWKNNTokenClassifierWrapper, self).__getattribute__(name)

        return self._main_tc.__getattribute__(name)

    def __setattr__(self, name, value):
        if name in ['_hnsw_tc', '_brute_force_tc', '_main_tc']:
            return super(HNSWKNNTokenClassifierWrapper, self).__setattr__(name, value)

        return self._main_tc.__setattr__(name, value)


def create_token_classifier(model, model_file=None, name=None, **kwargs):
    if model not in _token_classifier_factories:
        raise ValueError('Unknown intent classifier model: %s' % model)
    return _token_classifier_factories[model](model=model, name=name, **kwargs)


def register_token_classifier_type(cls_type, name):
    _token_classifier_factories[name] = cls_type


_token_classifier_factories = {}

register_token_classifier_type(GradientBoostingTokenClassifier, 'sgb')
register_token_classifier_type(MaxentTokenClassifier, 'maxent')
register_token_classifier_type(KNNTokenClassifier, 'knn')
