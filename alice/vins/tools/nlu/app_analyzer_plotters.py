# coding: utf-8
from __future__ import unicode_literals

import inspect
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import tensorflow as tf
import logging
import click
import os

from tensorflow.contrib.tensorboard.plugins import projector
from sklearn.base import BaseEstimator

from vins_core.app_utils import load_app
from vins_core.nlu.features.post_processor.vectorizer import VectorizerFeaturesPostProcessor
from vins_core.utils.iter import first_of
from app_analyzer import APPS, KNNAnalyzer, app_analyzer_logging_config

logger = logging.getLogger(__name__)


def _check_estimator_methods(estimator):
    return all((
        hasattr(estimator, 'fit'),
        hasattr(estimator, 'transform'),
        hasattr(estimator, 'get_params'),
        hasattr(estimator, 'set_params')
    ))


def get_estimator_name(estimator):
    if inspect.isclass(estimator) and issubclass(estimator, BaseEstimator):
        return estimator.__name__.lower()
    elif isinstance(estimator, BaseEstimator) or _check_estimator_methods(estimator):
        return estimator.__class__.__name__.lower()
    else:
        raise TypeError('"estimator" argument should be class definition'
                        ' or sklearn.base.BaseEstimator instance')


def _feature_names(token_classifier):
    model = token_classifier._model
    vectorizer = get_estimator_name(VectorizerFeaturesPostProcessor)
    if vectorizer not in model.named_steps:
        raise ValueError(
            'Unrecognized model type: the unique "%s" step should be presented in the pipeline' % vectorizer
        )
    return model.named_steps[vectorizer].vocabulary


def plot_maxent_weights(app, app_name, classifier_name, target_intent_name, num_features, savefig):
    token_classifier = app.nlu.get_classifier(classifier_name)
    coef = token_classifier.final_estimator.coef_
    intents = token_classifier._label_encoder.classes_
    features = _feature_names(token_classifier)

    intent_index = first_of((
        i for i, intent_name in enumerate(intents)
        if intent_name.endswith(target_intent_name)
    ), default=-1)
    if intent_index < 0:
        raise ValueError('Target intent %s not found among intents registered in %s' % (
            target_intent_name, app_name
        ))

    intent_coef = coef[intent_index][:len(features)]
    intent_weights = pd.DataFrame(
        index=features,
        data=np.vstack((intent_coef, np.abs(intent_coef))).T,
        columns=['weight', 'signed_weight_mass']
    ).sort_values(by='signed_weight_mass', ascending=False)
    intent_weights.signed_weight_mass /= intent_weights.signed_weight_mass.sum()
    intent_weights.signed_weight_mass *= intent_weights.weight.apply(np.sign)
    intent_weights = intent_weights.iloc[:num_features]

    plt.barh(
        np.arange(len(intent_weights.index)),
        intent_weights.signed_weight_mass.values[::-1],
        align='center',
        zorder=3
    )
    labels = [
        intent_weights.index[-i] + ' (%.2f)' % intent_weights.ix[num_features - i - 1, 'weight']
        for i in range(len(intent_weights.index))
    ]

    plt.yticks(range(len(intent_weights.index)), labels, fontsize=6)
    plt.xticks([], [])

    plt.grid(True, zorder=0)
    plt.xlabel('Weights mass function * sign of weight', fontsize=8)
    plt.title('Feature importance for %s (intent = %s)' % (app_name, target_intent_name), fontsize=8)

    if savefig:
        plt.tight_layout()
        plt.savefig(savefig, dpi=200)
    else:
        plt.show()


def plot_knn_embeddings(app_name, classifier, logdir, points_per_intent=None):
    if not os.path.exists(logdir):
        os.makedirs(logdir)

    knn_vectors, knn_texts, knn_labels, intents = KNNAnalyzer.get_knn_model_view(app_name, classifier)

    intents, texts = [], []
    if points_per_intent:
        embeddings_per_intent = []
        for intent_label, intent_name in enumerate(intents):
            intent_indices = np.where(knn_labels == intent_label)[0]
            np.random.shuffle(intent_indices)
            intent_indices = intent_indices[:points_per_intent]
            embeddings_per_intent.append(knn_vectors[intent_indices])
            texts.extend(knn_texts[intent_indices])
            intents.extend([intent_name] * len(intent_indices))
        np_embeddings = np.vstack(embeddings_per_intent)
    else:
        np_embeddings = knn_vectors
        texts = knn_texts
        intents = [intents[i] for i in knn_labels]
    logging.info('%d points selected', np_embeddings.shape[0])

    tf_embeddings = tf.get_variable(
        'embeddings',
        shape=np_embeddings.shape,
        initializer=tf.constant_initializer(np_embeddings),
        dtype=tf.float32
    )

    metadata_path = os.path.join(logdir, 'metadata.tsv')
    pd.DataFrame(
        data=zip(texts, intents),
        columns=['text', 'intent']
    ).to_csv(metadata_path, sep='\t', index=False, encoding='utf-8')
    config = projector.ProjectorConfig()
    embedding = config.embeddings.add()
    embedding.tensor_name = tf_embeddings.name
    embedding.metadata_path = metadata_path
    summary_writer = tf.summary.FileWriter(logdir)
    projector.visualize_embeddings(summary_writer, config)

    saver = tf.train.Saver()

    with tf.Session() as sess:
        tf_embeddings.initializer.run()
        saver.save(sess, os.path.join(logdir, 'model.ckpt'))

    logger.info('Done. Now launch `tensorboard --logdir %s` and check "Embeddings" tab' % logdir)


@click.group()
def analyzer():
    pass


@analyzer.command()
@click.argument('app', type=click.Choice(APPS))
@click.argument('classifier_name')
@click.option('--intent', help='Intent name to be shown')
@click.option('--num-features', help='Number of the most important features to be shown', default=10)
@click.option('--savefig', help='Save figure to specified file', default=None)
def weights(app, classifier_name, intent, num_features, savefig):
    app_obj = load_app(app, load_model=True, load_data=False, force_rebuild=False)
    plot_maxent_weights(app_obj, app, classifier_name, intent, num_features, savefig)


@analyzer.command('knn_embeddings')
@click.argument('app', type=click.Choice(APPS))
@click.argument('classifier')
@click.option('--logdir', help='Logdir where tensorboard will be launched')
@click.option('--points-per-intent', type=int,
              help='Specify number of points randomly drawn from each intent')
def knn_embeddings(app, classifier, logdir, points_per_intent):
    plot_knn_embeddings(app, classifier, logdir, points_per_intent)


if __name__ == "__main__":
    app_analyzer_logging_config()

    analyzer(obj={})
