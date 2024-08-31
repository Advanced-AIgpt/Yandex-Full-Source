# -*- coding: utf-8 -*-

import click
import os
import numpy as np

from functools import partial

from vins_core.utils.intent_renamer import IntentRenamer
from dataset import VinsDataset
from data_loaders import load_extractors
from vins_config_tools import load_app_config
from reranker_models.word_nn_model import RnnWordNNModel, ConvWordNNModel


def load_intent_normalizations(is_true=True, rename_paths=None):
    rename_paths = rename_paths or [
        'apps/personal_assistant/personal_assistant/tests/validation_sets/toloka_intent_renames_quasar.json',
        'apps/personal_assistant/personal_assistant/tests/validation_sets/toloka_intent_renames.json'
    ]

    intent_renamer = IntentRenamer(rename_paths)
    renames_type = IntentRenamer.By.TRUE_INTENT if is_true else IntentRenamer.By.PRED_INTENT
    return partial(intent_renamer, by=renames_type)


def _load_alice_request_embs_matrix():
    app_config = load_app_config('personal_assistant.app')
    _, features_extractor, _ = load_extractors(app_config, features_extractor_list=['emb_ids'])
    return features_extractor.classifiers_info['metric_learning'].embeddings_matrix


def _get_model_cls_by_type(model_type):
    if model_type == 'rnn':
        return RnnWordNNModel
    if model_type == 'conv':
        return ConvWordNNModel
    assert False, 'Unknown model type {}'.format(model_type)


def _train(model_path, classifier_name, model_type, dataset_path, epochs_count, renames_path):
    Model = _get_model_cls_by_type(model_type)

    click.echo('Initializing model')
    model = Model(embeddings_matrix=_load_alice_request_embs_matrix(), is_train_model=True)

    click.echo('Loading dataset')
    train_dataset = VinsDataset.restore(dataset_path)

    train_dataset, val_dataset = VinsDataset.split(
        train_dataset, holdout_classifiers=(classifier_name,), holdout_size=0.1
    )

    train_dataset = train_dataset.to_dataset_view(classifier_name, features=Model.train_feature_list())
    val_dataset = val_dataset.to_dataset_view(classifier_name, features=Model.train_feature_list())

    click.echo('Fitting model')
    try:
        model.fit(train_dataset, val_dataset, epochs_count=epochs_count)
    except KeyboardInterrupt:
        pass

    click.echo('Saving model')
    model.save(model_path)
    model = Model.restore(model_path)

    if renames_path:
        model.save_frozen_graph(model_path + '_memm', load_intent_normalizations(renames_path))
    else:
        model.save_frozen_graph(model_path + '_memm', lambda intent: intent)


def _predict(model_path, classifier_name, feature_name, model_type, map_to, dataset_paths):
    Model = _get_model_cls_by_type(model_type)

    click.echo('Loading model')
    model = Model.restore(model_path)
    model.save_frozen_graph(model_path + '_memm', lambda intent: intent)
    embeddings_matrix = _load_alice_request_embs_matrix()

    for dataset_path in dataset_paths:
        click.echo('Loading dataset from {}'.format(dataset_path))

        test_dataset = VinsDataset.restore(dataset_path)
        test_dataset_view = test_dataset.to_dataset_view(
            classifier=classifier_name,
            features=Model.inference_feature_list(),
            embeddings_matrices={'emb_ids': ('alice_requests_emb', embeddings_matrix)}
        )

        click.echo('Predicting features')
        probs = model.predict_proba(test_dataset_view)
        assert probs.shape[0] == len(test_dataset)

        test_dataset.add_feature(
            feature_name=feature_name,
            feature_type=VinsDataset.FeatureType.DENSE,
            feature_matrix=probs,
            feature_mapping={intent: ind for ind, intent in enumerate(model.classes)}
        )

        if map_to:
            test_dataset.convert_classifier_feature_by_renaming(
                source_feature_name=feature_name,
                target_feature_name=map_to,
                renames=load_intent_normalizations()
            )

        click.echo('Saving dataset')
        test_dataset.save(dataset_path)


@click.group()
@click.option('--model-name', default='lstm-model')
@click.option('--model-type', type=click.Choice(['rnn', 'conv']), default='rnn')
@click.option('--renames-path', type=click.Path(), default=None)
@click.option('--classifier-name', type=click.Choice(['scenarios', 'toloka']), default='scenarios',
              help='The name of classifier which features would be extracted for training/inference from the dataset')
@click.pass_context
def main(ctx, model_name, model_type, renames_path, classifier_name):
    np.random.seed(42)

    ctx.ensure_object(dict)
    ctx.obj['model_name'] = model_name
    ctx.obj['classifier_name'] = classifier_name
    ctx.obj['model_type'] = model_type
    ctx.obj['renames_path'] = renames_path


@main.command()
@click.option('--model-dir', type=click.Path(exists=True))
@click.option('--dataset-path', type=click.Path(exists=True))
@click.option('--epochs-count', type=int, default=20)
@click.pass_context
def train(ctx, model_dir, dataset_path, epochs_count):
    model_path = os.path.join(model_dir, ctx.obj['classifier_name'], ctx.obj['model_name'])
    _train(
        model_path=model_path,
        classifier_name=ctx.obj['classifier_name'],
        model_type=ctx.obj['model_type'],
        dataset_path=dataset_path,
        epochs_count=epochs_count,
        renames_path=ctx.obj['renames_path']
    )


@main.command()
@click.argument('dataset-paths', type=click.Path(exists=True), nargs=-1)
@click.option('--model-path', type=click.Path(exists=True))
@click.option('--feature-name')
@click.option('--map-to', default=None)
@click.pass_context
def predict(ctx, dataset_paths, model_path, feature_name, map_to):
    _predict(
        model_path=model_path,
        classifier_name=ctx.obj['classifier_name'],
        feature_name=feature_name,
        model_type=ctx.obj['model_type'],
        map_to=map_to,
        dataset_paths=dataset_paths
    )


if __name__ == '__main__':
    main(obj={})
