# coding: utf-8
from __future__ import unicode_literals

import argparse
import logging
import os
import numpy as np
import pandas as pd
import re
import codecs

from itertools import izip_longest

from keras import backend as keras_backend

from influence_nn import InfluenceCalculator
from influence_nn.feed_dict_generator import FeederFeedDictGenerator

from compile_app_model import create_app
from vins_core.nlu.token_classifier import create_token_classifier
from vins_core.utils.archives import TarArchive
from vins_core.nlu.base_nlu import FeatureExtractorFromItem


logger = logging.getLogger(__name__)


def set_logging(level='INFO'):
    logging.config.dictConfig({
        'version': 1,
        'disable_existing_loggers': False,
        'formatters': {
            'standard': {
                'format': '[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s',
            },
        },
        'handlers': {
            'console': {
                'class': 'logging.StreamHandler',
                'formatter': 'standard',
            },
        },
        'loggers': {
            'vins_tools.nlu.inspection.nlu_result_info': {
                'handlers': ['console'],
                'level': 'DEBUG',
                'propagate': False,
            },
            '': {
                'handlers': ['console'],
                'level': level,
                'propagate': True,
            },
        },
    })


def get_probe_model(app, classifier, feature_cache, probe_model_file, num_epochs):

    app = create_app(app)
    data = app.nlu.extract_features_on_train(
        classifiers=[classifier],
        taggers=False,
        feature_cache=feature_cache,
        validation=None
    )
    for intent in data:
        data[intent] = filter(None, data[intent])

    probe_model = create_token_classifier(model='rnn', unroll=True, nb_epoch=num_epochs)

    if not os.path.exists(probe_model_file):
        logging.info('%s not found. Start creating new probe model...', probe_model_file)
        probe_model.train(data)
        with TarArchive(probe_model_file, 'w:gz') as arch:
            probe_model.save(arch, 'probe_model')
    with TarArchive(probe_model_file) as arch:
        probe_model.load(arch, 'probe_model')
    # warm up model to avoid lazy initialization when VINS_LOAD_TF_ON_CALL=1
    probe_model(data.values()[0][0])
    return probe_model, data, app


def _get_influence_calculator(model):
    parameters = []
    for layer in model._model.layers:
        try:
            parameters.extend(layer.backward_layer._trainable_weights)
        except AttributeError:
            pass
        try:
            parameters.extend(layer.forward_layer._trainable_weights)
        except AttributeError:
            pass
        parameters.extend(layer._trainable_weights)

    return InfluenceCalculator(model._model.total_loss, parameters)


def _flatten(intent_to_features, use_intents=None):
    out_features, out_intents = [], []
    for intent, features in intent_to_features.iteritems():
        if use_intents and not re.match(use_intents, intent):
            continue
        out_features.extend(features)
        out_intents.extend([intent] * len(features))
    return out_features, out_intents


def _calc_features(app, test_file):
    with codecs.open(test_file, encoding='utf-8') as f:
        data = list(izip_longest(f.read().splitlines(), [None]))
    calc = FeatureExtractorFromItem(app.samples_extractor, app.nlu.features_extractor, feature_cache=None)
    return calc(data)


class ProbeModelInputFeed(object):

    def __init__(self, model):
        self._model = model
        self._input_tensors = model.final_estimator.get_tf_input_tensors()

    def __call__(self, batch):
        data, labels = batch
        return self._model.final_estimator.get_tf_input_feed(data, labels, self._input_tensors)


def _get_generator(model, sample_features, intents):
    data, labels = model.transform_to_final_estimator(sample_features, intents)
    return FeederFeedDictGenerator((np.array(data), np.array(labels)), ProbeModelInputFeed(model))


def run_influence(model, test_intent, train_features, test_features, output_file, hessian_batch_size,
                  hessian_num_iterations, hessian_num_repetitions, ihvp_scale, ihvp_dump, train_intents=None):

    train_sample_features, train_intents = _flatten(train_features, use_intents=train_intents)
    test_sample_features, test_intents = _flatten({test_intent: test_features})
    influence_calc = _get_influence_calculator(model.final_estimator)
    influences = influence_calc.influence(
        session=keras_backend.get_session(),
        train_feed_dict_generator=_get_generator(model, train_sample_features, train_intents),
        test_feed_dict_batch_generator=_get_generator(model, test_sample_features, test_intents),
        hessian_batch_size=hessian_batch_size,
        hessian_num_iterations=hessian_num_iterations,
        hessian_num_repetitions=hessian_num_repetitions,
        ihvp_scale=ihvp_scale,
        ihvp_damp=ihvp_dump,
        verbose_step=100
    )
    influences_sorted_index = np.argsort(influences)[::-1]
    output = []
    for i in influences_sorted_index:
        text = train_sample_features[i].sample.text
        influence_score = influences[i]
        intent = train_intents[i]
        output.append((text, intent, influence_score))

    pd.DataFrame(output, columns=['text', 'intent', 'influence_score']).to_csv(
        output_file, sep=b'\t', encoding='utf-8', index=False
    )


def main():
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument(
        '--app', metavar='APP', dest='app',
        help='The name of the app to compile models for, multiple names can be specified.',
    )
    parser.add_argument(
        '--classifier', dest='classifier',
        help='Name of classifier that used to collect target data'
    )
    parser.add_argument(
        '--feature-cache', dest='feature_cache',
        help='File path to store / retrieve precomputed train features'
    )
    parser.add_argument(
        '--intent', dest='intent',
        help='Target intent for which to estimate train data influence'
    )
    parser.add_argument(
        '--train-intents', dest='train_intents',
        help='Regexp to identify among which train intents influence is calculated'
    )
    parser.add_argument(
        '--test-file', dest='test_file',
        help='Overrides --intent option: use data from this source to estimate train data influence'
    )
    parser.add_argument(
        '--output-dir', dest='output_dir',
        help='Output directory to find/save probe model archive and output results'
    )
    parser.add_argument(
        '--hessian-batch-size', dest='hessian_batch_size',
        help='Size of training data batches used for estimating the Hessian at each iteration '
             'of inverse hessian-vector product calculation.', type=int, default=10
    )
    parser.add_argument(
        '--hessian-num-iterations', dest='hessian_num_iterations',
        help='Number of Neumann series iterations for estimating inverse hessian-vector product.',
        type=int, default=500
    )
    parser.add_argument(
        '--hessian-num-repetitions', dest='hessian_num_repetitions',
        help='Number of independent launches of inverse hessian-vector product calculation to average over.',
        type=int, default=3
    )
    parser.add_argument(
        '--ihvp-dump', dest='ihvp_dump',
        help='Damping parameter used for making the Hessian positive-definite and, thus, invertible. Several values '
             'should be tried until the method starts converging.',
        type=float, default=0.5
    )
    parser.add_argument(
        '--ihvp-scale', dest='ihvp_scale',
        help='Scaling parameter for the Hessian that bounds the Hessian norm from above. Several values should be tried'
             ' until the method starts converging; convergence (e.g. of the estimate '
             'norm) can be monitored via ``verbose_step``. If ``estimate``, this upper bound on the '
             'Hessian norm will also be estimated via stochastic (batched) power iteration;'
             ' see https://en.wikipedia.org/wiki/Power_iteration .',
        type=float, default=500.
    )
    parser.add_argument(
        '--num-epochs', dest='num_epochs',
        help='Number of epochs needed to create probe model',
        type=int, default=5
    )
    args = parser.parse_args()

    if not os.path.exists(args.output_dir):
        os.makedirs(args.output_dir)
    output_name = '%s.%s' % (args.app, args.classifier)
    model_file = os.path.join(args.output_dir, output_name + '.tar.gz')
    probe_model, train_features, app = get_probe_model(
        args.app, args.classifier, args.feature_cache, model_file, args.num_epochs)
    if args.test_file:
        test_features = _calc_features(app, args.test_file)
    else:
        test_features = train_features[args.intent]
    output_file = os.path.join(args.output_dir, '%s.%s.tsv' % (output_name, args.intent))
    run_influence(
        probe_model, args.intent, train_features, test_features, output_file, args.hessian_batch_size,
        args.hessian_num_iterations, args.hessian_num_repetitions, args.ihvp_scale, args.ihvp_dump, args.train_intents
    )


if __name__ == "__main__":
    set_logging()
    main()
