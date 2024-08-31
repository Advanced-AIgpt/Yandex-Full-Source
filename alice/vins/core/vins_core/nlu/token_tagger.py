# coding: utf-8
from __future__ import unicode_literals

import cPickle as pickle
import logging
import operator

import os
import re
import copy
import fasteners.process_lock as locks
import numpy as np
import pandas as pd
import numbers
import shutil
import stat

from collections import Mapping
from itertools import izip
from tempfile import mkstemp, mkdtemp

from crfsuitex import Tagger
from pycrfsuite import ItemSequence, Trainer
from sklearn.base import BaseEstimator
from sklearn.metrics import make_scorer, f1_score
from sklearn.model_selection import GridSearchCV, cross_val_score, KFold
from sklearn.preprocessing import FunctionTransformer

from alice.nlu.py_libs.tagger import RnnTaggerTrainer, RnnTaggerApplier

from vins_core.nlu.sklearn_utils import make_pipeline
from vins_core.nlu.features.post_processor.splicer import SplicerFeaturesPostProcessor
from vins_core.utils.strings import smart_utf8
from vins_core.utils.misc import EPS
from vins_core.utils.iter import first_of
from vins_core.utils.data import vins_temp_dir, load_data_from_file
from vins_core.nlu.features.post_processor.selector import SelectorFeaturesPostProcessor
from vins_core.common.slots_map_utils import tags_to_slots

logger = logging.getLogger(__name__)


def tagging_accuracy(y, y_pred, average='micro'):
    if isinstance(y[0], dict):  # TODO: each tagger holds its own scorer
        tags_true = map(operator.itemgetter('tags'), y)
    else:
        tags_true = y
    tags_best_pred = map(operator.itemgetter(0), y_pred[0])
    y_true, y_best_pred = [], []
    for tag_true, tag_pred in izip(tags_true, tags_best_pred):
        y_true.extend(tag_true[:len(tag_pred)])
        y_best_pred.extend(tag_pred)

    score = f1_score(
        y_true=y_true,
        y_pred=y_best_pred,
        average=average
    )
    return score


class TokenTagger(object):

    def __init__(self, name=None, random_seed=42, features=(), dropout=None, intent_conditioned=False, **kwargs):
        self._name = name
        self.random_seed = random_seed
        self.select_features = features
        self._intent_conditioned = intent_conditioned
        self._pipeline = []
        if self.select_features:
            self._selector_post_processor = SelectorFeaturesPostProcessor(self.select_features, dropout)
            self._pipeline.append(self._selector_post_processor)
        else:
            self._selector_post_processor = None

        self._model = None

    @property
    def name(self):
        return self._name

    def get_input(self, features, reset_model, **kwargs):
        if isinstance(features, Mapping):  # intent-to-samples map is given
            features = sum(features.itervalues(), [])
        return features

    @classmethod
    def _extract_all_tags(cls, features):
        if isinstance(features, Mapping):  # intent-to-samples map is given
            samples = (feature.sample for features_ in features.itervalues() for feature in features_)
        else:
            samples = (feature.sample for feature in features)
        return [map(smart_utf8, sample.tags) for sample in samples]

    def get_output(self, features, reset_model, **kwargs):
        return self._extract_all_tags(features)

    def train(self, intent_to_features, reset_model=True, intents_to_train=None, **kwargs):
        if self._intent_conditioned:
            base_model = make_pipeline(*self._pipeline)
            self._model = self._model or {}
            for intent, features in intent_to_features.iteritems():
                if intents_to_train and not re.match(intents_to_train, intent):
                    continue
                logger.info('Start fitting tagger for intent %s', intent)
                x = self.get_input(features, reset_model, **kwargs)
                y = self.get_output(features, reset_model, **kwargs)

                if reset_model or intent not in self._model:
                    self._model[intent] = copy.deepcopy(base_model)
                self._model[intent].fit(x, y, **self._fit_params(do_dropout=True, **kwargs))
        else:
            x = self.get_input(intent_to_features, reset_model, **kwargs)
            y = self.get_output(intent_to_features, reset_model, **kwargs)
            if reset_model or not self._model:
                self._model = make_pipeline(*self._pipeline)
            self._model.fit(x, y, **self._fit_params(do_dropout=True, **kwargs))
        return self

    def _fit_params(self, do_dropout=False, **kwargs):
        if self._selector_post_processor:
            return self.step_params([(self._selector_post_processor, {'do_dropout': do_dropout})])
        else:
            return {}

    def _find_model(self, **kwargs):
        if self._intent_conditioned:
            if 'intent' not in kwargs:
                raise ValueError('For intent conditioned tagger "intent" argument should be passed to predict()')
            model = self._model and self._model.get(kwargs['intent'])
        else:
            model = self._model
        return model

    def predict(self, batch_features, **kwargs):
        model = self._find_model(**kwargs)
        if not model:
            return [], []
        x = self.get_input(batch_features, reset_model=False, **kwargs)
        y_pred, p_pred = model.predict(x)
        y_pred, p_pred = self._after_predict(y_pred, p_pred)
        y_pred, p_pred = self._remove_errors(y_pred, p_pred)

        return y_pred, p_pred

    def predict_slots(self, batch_samples, batch_features, batch_entities, verbose=True, **kwargs):
        batch_tagging_hypotheses, batch_score_list = self.predict(
            batch_features, batch_samples=batch_samples, **kwargs
        )

        batch_tokens = [self._get_sample_tokens(sample) for sample in batch_samples]
        batch_entities = [
            self._get_entities(sample, tokens, entities)
            for sample, tokens, entities in izip(batch_samples, batch_tokens, batch_entities)
        ]
        if verbose:
            self._logging(batch_tokens, batch_tagging_hypotheses, batch_score_list)

        batch_slots_list, batch_entities_list = [], []

        for tokens, tagging_hypotheses, entities in izip(batch_tokens, batch_tagging_hypotheses, batch_entities):
            slots_list, entities_list = [], []
            for tags in tagging_hypotheses:
                extracted_slots, free_entities = tags_to_slots(tokens, tags, entities)
                slots_list.append(extracted_slots)
                entities_list.append(free_entities)

            batch_slots_list.append(slots_list)
            batch_entities_list.append(entities_list)

        return batch_slots_list, batch_entities_list, batch_score_list

    def _get_sample_tokens(self, sample):
        return sample.tokens

    def _get_entities(self, sample, tokens, entities):
        return entities

    def _after_predict(self, y_pred, p_pred):
        return y_pred, p_pred

    def crossvalidation(self, features, cv=None, n_jobs=1, verbose=2, average='micro', **kwargs):
        reset_model = True
        fit_params = self._fit_params(**kwargs)
        scoring = make_scorer(tagging_accuracy, average=average)
        if cv is None:
            cv = KFold(n_splits=3, shuffle=True, random_state=self.random_seed)
        elif isinstance(cv, numbers.Integral):
            cv = KFold(n_splits=int(cv), shuffle=True, random_state=self.random_seed)
        if self._intent_conditioned and isinstance(features, Mapping):
            cross_val_means, cross_val_stds = [], []
            for intent, sample_features in features.iteritems():
                cross_val_scores = cross_val_score(
                    estimator=make_pipeline(*self._pipeline),
                    X=self.get_input(sample_features, reset_model, **kwargs),
                    y=self.get_output(sample_features, reset_model, **kwargs),
                    cv=cv, n_jobs=n_jobs, verbose=verbose,
                    fit_params=fit_params, scoring=scoring
                )
                cross_val_means.append(np.mean(cross_val_scores))
                cross_val_stds.append(np.std(cross_val_scores, ddof=1))
            return np.mean(cross_val_means), np.mean(cross_val_stds)
        cross_val_scores = cross_val_score(
            estimator=make_pipeline(*self._pipeline),
            X=self.get_input(features, reset_model, **kwargs),
            y=self.get_output(features, reset_model, **kwargs),
            cv=cv, n_jobs=n_jobs, verbose=verbose,
            fit_params=fit_params, scoring=scoring
        )
        return np.mean(cross_val_scores), np.std(cross_val_scores, ddof=1)

    def gridsearch(self, features, param_grid, n_jobs=1, cv=None, refit=True,
                   verbose=2, average='micro', **kwargs):
        """
        Makes grid search over parameters grid, and stores best-scoring model
        :param samples: input samples
        :param param_grid: dict where keys are paramater names, and values are list of parameter values
        :param n_jobs: number of parallel processes
        :param cv: number of cross-validation folds
        :param refit: if True, rebuilds inner model with best parameter set
        :param verbose: verbosity level
        :param average: f-score averaging method ('micro', 'macro')
        :param kwargs: input, output, fitting-stage optional parameters
        :return:
        """
        if self._intent_conditioned:
            raise NotImplementedError('Grid search is not implemented for intent conditioned token taggers')
        reset_model = True  # model always reset
        gs = GridSearchCV(
            estimator=make_pipeline(*self._pipeline),
            param_grid=self.step_params(param_grid),
            scoring=make_scorer(tagging_accuracy, average=average),
            n_jobs=n_jobs,
            cv=cv,
            fit_params=self._fit_params(**kwargs),
            verbose=verbose,
            refit=refit
        )
        x = self.get_input(features, reset_model, **kwargs)
        y = self.get_output(features, reset_model, **kwargs)

        gs.fit(x, y)
        if refit:
            self._model = gs.best_estimator_
        return pd.DataFrame(gs.cv_results_)  # TODO: squeeze back parameter names

    @classmethod
    def _logging(cls, batch_tokens, y_preds, p_preds):
        out = 'Tagger output of %s:\n' % cls.__name__
        for tokens, y_pred, p_pred in izip(batch_tokens, y_preds, p_preds):
            for tags, score in izip(y_pred, p_pred):
                out += ' '.join(
                    '{0}[{1}]'.format(tok, tag)
                    for tok, tag in izip(tokens, tags)
                ) + '\t(score={0})\n'.format(score)
        logger.info(out)

    def save(self, archive, name):
        tmp = archive.get_tmp_file()
        with open(tmp, 'wb') as f:
            pickle.dump(self, f, pickle.HIGHEST_PROTOCOL)

        archive.add(name, tmp)

    def load(self, archive, name):
        this = pickle.load(archive.get_by_name(name))
        self.__dict__.update(this.__dict__)

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
                if not step:
                    # try to find corresponding pipeline step
                    # that matches all specified parameters
                    target_step = first_of(_step for _step in self._pipeline if hasattr(_step, param))
                    if not target_step:
                        raise StopIteration("Invalid parameter specified: %s", param)
                else:
                    target_step = step
                step_name = type(target_step).__name__.lower()
                p['%s__%s' % (step_name, param)] = value
        return p

    @classmethod
    def _remove_errors(cls, paths, scores):

        out_paths, out_scores = [], []

        for sample_paths, sample_scores in izip(paths, scores):
            out_sample_paths, out_sample_scores = [], []

            for path, score in izip(sample_paths, sample_scores):

                # Do not remove paths where any tag starts with 'I-' prefix. It is done by MultislotFrameScoring.

                # Do not remove paths that contain duplicated slots. It is done by MultislotFrameScoring.

                # todo: for duplicated paths, scores should be added up
                if any(out_sample_path == path for out_sample_path in out_sample_paths):
                    continue

                out_sample_paths.append(path), out_sample_scores.append(score)

            if len(out_sample_paths) == 0:
                out_sample_paths.append(['O'] * len(sample_paths[0]))
                out_sample_scores.append(EPS)

            out_paths.append(out_sample_paths)
            out_scores.append(out_sample_scores)

        return out_paths, out_scores


class CRFModel(BaseEstimator):

    def __init__(self, max_iterations=50, feature_possible_transitions=True, feature_possible_states=True,
                 c2=1e-3, nbest=10, normalized_scores=True, **kwargs):
        """
        :param max_iteration: number of iterations for L-BFGS
        :param feature_possible_transitions: whether to add unseen transitions f(y,y)
        :param feature_possible_states: whether to add unseen states f(x,y)
        :param c2: l2 regularizer coefficient
        :param store_dir: temporary file storage with write access
        :param nbest: specifies maximum N-best paths to be retrieved
        :param normalized_scores: normalization wrt all possible paths (partition function)
        :return:
        """
        self.max_iterations = max_iterations
        self.feature_possible_transitions = feature_possible_transitions
        self.feature_possible_states = feature_possible_states
        self.c2 = c2
        self.nbest = nbest
        self.normalized_scores = normalized_scores

        self._model = None

    def fit(self, X, y):
        trainer = Trainer(verbose=False)
        for xseq, yseq in izip(X, y):
            trainer.append(xseq, yseq)
        trainer.set_params({
            'max_iterations': self.max_iterations,
            'feature.possible_transitions': self.feature_possible_transitions,
            'feature.possible_states': self.feature_possible_states,
            'c2': self.c2
        })
        _model_name = mkstemp(dir=vins_temp_dir())[1]
        with locks.InterProcessLock(_model_name + '.lock'):
            trainer.train(_model_name)
            with open(_model_name, mode='rb') as f:
                self._model = f.read()
            os.remove(_model_name)

        return self

    def predict(self, X):
        tagger = Tagger(
            npaths=self.nbest,
            normalized_scores=self.normalized_scores
        )
        if self._model:
            tagger.load(self._model)
        else:
            raise Exception("CRFModel not loaded")
        tags, scores = zip(*(tagger.tag(x) for x in X))
        return tags, scores


def _to_item_sequence(features, *args, **kwargs):
    # XXX: args, kwargs
    output = []
    for sample_features in features:
        sample_onehots = []
        for i in xrange(len(sample_features)):
            token_i = {}
            for feature, value_lists in sample_features.sparse_seq.iteritems():
                token_i.update({'%s=%s' % (feature, value.value): value.weight for value in value_lists[i]})
            sample_onehots.append(token_i)
        output.append(ItemSequence(sample_onehots))
    return output


class CRFTokenTagger(TokenTagger):

    def __init__(self, window_size=5, **kwargs):
        """
        :param win: context size window
        :param kwargs: parameters for TokenTagger, CRFModel
        :return:
        """
        assert window_size % 2 == 1 and window_size >= 1
        super(CRFTokenTagger, self).__init__(**kwargs)
        self._pipeline.extend([
            SplicerFeaturesPostProcessor(window_size=window_size),
            FunctionTransformer(_to_item_sequence, validate=False),
            CRFModel(**kwargs)
        ])


class RNNTokenTagger(TokenTagger):
    def train(self, intent_to_features, reset_model=True, intents_to_train=None, **kwargs):
        if self._intent_conditioned:
            self._model = self._model or {}
            for intent, features in intent_to_features.iteritems():
                if intents_to_train and not re.match(intents_to_train, intent):
                    continue

                logger.info('Start fitting tagger for intent %s', intent)

                self._model[intent] = self._train_single_model(features)
        else:
            self._model = self._train_single_model(intent_to_features)

        return self

    def _train_single_model(self, features):
        model = RnnTaggerTrainer()
        data = self.get_input(features, reset_model=False)
        model.fit(data, None)

        return model.convert_to_applier()

    def save(self, archive, name):
        tmp = archive.get_tmp_dir()
        if self._intent_conditioned:
            for intent, model in self._model.iteritems():
                logger.info('Saving tagger for intent %s', intent)
                model.save(os.path.join(tmp, intent))
        else:
            self._model.save(tmp)
        archive.add(name + '.data', tmp)

        tagger_meta_info_path = os.path.join(tmp, 'meta.pkl')
        with open(tagger_meta_info_path, 'wb') as f:
            pickle.dump(self._intent_conditioned, f, pickle.HIGHEST_PROTOCOL)

        archive.add(name + '.info', tagger_meta_info_path)

    def load(self, archive, name):
        self._intent_conditioned = pickle.load(archive.get_by_name(name + '.info'))

        self._load(archive, name)
        logger.info('Model "%s" has been loaded %s '
                    'from archive, base=%s, name=%s',
                    self.__class__.__name__, 'on load', archive.base, name)

    def _load(self, archive, name):
        if self._intent_conditioned:
            self._model = self._model or {}

        tmp_dir = None
        try:
            tmp_dir = mkdtemp()
            with archive.nested(name + '.data') as arch:
                if self._intent_conditioned:
                    self._load_multiple_taggers(arch, tmp_dir)
                else:
                    self._load_single_tagger(arch, tmp_dir)
        finally:
            if tmp_dir:
                access_rights = stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO
                os.chmod(tmp_dir, access_rights)
                for dir_path, dir_names, file_names in os.walk(tmp_dir):
                    for dir_name in dir_names:
                        os.chmod(os.path.join(dir_path, dir_name), access_rights)
                    for file_name in file_names:
                        os.chmod(os.path.join(dir_path, file_name), access_rights)
                shutil.rmtree(tmp_dir)

    def _load_multiple_taggers(self, arch, tmp_dir):
        for intent in arch.list():
            tagger_data_dir = os.path.join(tmp_dir, arch.base, intent)
            with locks.InterProcessLock(tagger_data_dir + '.lock'):
                arch.extract_all(tmp_dir, os.path.join(arch.base, intent))
                logger.info('Load tagger for intent %s', intent)
                self._model[intent] = RnnTaggerApplier(tagger_data_dir)

    def _load_single_tagger(self, arch, tmp_dir):
        tagger_data_dir = os.path.join(tmp_dir, arch.base)
        with locks.InterProcessLock(tagger_data_dir + '.lock'):
            arch.extract_all(tmp_dir, arch.base)
            self._model = RnnTaggerApplier(tagger_data_dir)


class RegexTokenTagger(TokenTagger):
    _group_matcher = r'(\(\?\P<((?!\_\_)[a-z0-9\_])+)(>)'
    _group_replacer = r'\1__{}\3'
    _replaced_group_matcher = r'\(\?\P\<[a-z0-9\_]+__\d+>'
    _matching_score = 1

    def __init__(self, source, **kwargs):
        super(RegexTokenTagger, self).__init__(**kwargs)
        self.source = source
        self._expressions = {}
        self._train()

    def _add_suffixes(self, pattern, counter):
        replaces = 1
        while replaces > 0:
            pattern, replaces = re.subn(self._group_matcher, self._group_replacer.format(counter), pattern, count=1)
            counter += 1
        return pattern, counter

    def _train(self):
        if isinstance(self.source, basestring):
            raw_data = load_data_from_file(self.source)
        elif isinstance(self.source, Mapping):
            raw_data = self.source
        else:
            raise ValueError('self.source should be a string (filename) or mapping (intent -> [expressions])')
        self._expressions = {}
        for intent, expressions_list in raw_data.iteritems():
            expressions_to_compile = []
            counter = 0
            for expression in expressions_list:
                assert isinstance(expression, basestring), 'Regex tagger config item should be a regex string.'
                # hacking group name in the expression
                # check that group names do not contain __\d+$ yet
                if re.search(self._replaced_group_matcher, expression):
                    raise ValueError('Group names for regex tagger should not contain double underscore.')
                expression, counter = self._add_suffixes(expression, counter)
                expressions_to_compile.append(expression)
            # todo: combine expressions in such a way that multiple matches are possible
            self._expressions[intent] = re.compile(
                '({})'.format('|'.join(['({})'.format(e) for e in expressions_to_compile]))
            )

    def predict(self, features, intent=None, verbose=True, **kwargs):

        if intent not in self._expressions:
            return [[] for _ in features], [[] for _ in features]

        expression = self._expressions[intent]

        result = []
        scores = []
        for f in features:
            text = f.sample.text
            sample_result = []
            sample_scores = []
            match = re.match(expression, text)
            if match is not None:
                # span extracted by RE match should be mapped into token positions
                starts = {}
                ends = {}
                cursor = 0
                for i, token in enumerate(f.sample.tokens):
                    starts[cursor] = i
                    cursor += len(token)
                    ends[cursor] = i
                    cursor += 1

                current_result = ['O' for _ in f.sample.tokens]
                for tag_name, tag_text in match.groupdict().items():
                    if tag_text is None:
                        continue
                    start_char, end_char = match.span(tag_name)
                    clean_tag_name = re.sub(r'__\d+$', '', tag_name)

                    start_index = starts.get(start_char)
                    end_index = ends.get(end_char)

                    if start_index is None or end_index is None:
                        continue

                    first = True
                    for token_id in range(start_index, end_index + 1):
                        current_result[token_id] = ('B-' if first else 'I-') + clean_tag_name
                        first = False

                sample_result.append(current_result)
                sample_scores.append(self._matching_score)
            result.append(sample_result)
            scores.append(sample_scores)

        return result, scores

    def train(self, intent_to_features, reset_model=True, intents_to_train=None, **kwargs):
        return self

    def save(self, archive, name):
        pass

    def load(self, archive, name):
        pass


class MixedModelTokenTagger(TokenTagger):
    def __init__(self, intent_infos, default_config, **kwargs):
        super(MixedModelTokenTagger, self).__init__(**kwargs)

        self._intent_infos = intent_infos
        self._default_config = default_config
        self._model = {}

    def _get_intent_config(self, intent):
        return self._intent_infos[intent].utterance_tagger or self._default_config

    def _create_tagger(self, config):
        return create_token_tagger(
            model=config['model'],
            select_features=config.get('features', ()),
            **config.get('params', {})
        )

    def train(self, intent_to_features, reset_model=True, intents_to_train=None, **kwargs):
        for intent, features in intent_to_features.iteritems():
            if intents_to_train and not re.match(intents_to_train, intent):
                continue
            logger.info('Start fitting tagger for intent %s', intent)

            self._model[intent] = self._create_tagger(self._get_intent_config(intent))
            self._model[intent].train(features, reset_model, **kwargs)
        return self

    def predict(self, features, verbose=True, **kwargs):
        if 'intent' not in kwargs:
            raise ValueError('For intent conditioned tagger "intent" argument should be passed to predict()')
        return self._model[kwargs['intent']].predict(features, verbose=verbose, **kwargs)

    def save(self, archive, name):
        temp_file = archive.get_tmp_file()

        model_configs = {intent: self._get_intent_config(intent) for intent in self._model}
        with open(temp_file, 'wb') as f:
            pickle.dump(model_configs, f, pickle.HIGHEST_PROTOCOL)
        archive.add(name + '.info', temp_file)

        with archive.nested(name + '.data') as arch:
            for intent, model in self._model.iteritems():
                model.save(arch, intent)

    def load(self, archive, name):
        model_configs = pickle.load(archive.get_by_name(name + '.info'))

        with archive.nested(name + '.data') as arch:
            for intent, config in model_configs.iteritems():
                self._model[intent] = self._create_tagger(config)
                self._model[intent].load(arch, intent)


# Tagger factory
_tagger_factories = {}


def register_token_tagger_type(tagger_type, name):
    _tagger_factories[name] = tagger_type


def create_token_tagger(model, **kwargs):
    assert model is not None, 'A model must be specified in the token tagger config'
    if model not in _tagger_factories:
        raise ValueError('Unknown speech tagger model: %s' % model)

    return _tagger_factories[model](**kwargs)


register_token_tagger_type(CRFTokenTagger, 'crf')
register_token_tagger_type(RNNTokenTagger, 'rnn_new')
register_token_tagger_type(RegexTokenTagger, 'regex')
register_token_tagger_type(MixedModelTokenTagger, 'mixed')
