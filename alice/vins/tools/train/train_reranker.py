# -*- coding: utf-8 -*-

import os
import click
import cPickle as pickle
import numpy as np
import pandas as pd

from itertools import izip
from operator import itemgetter
from collections import Counter
from sklearn.metrics import precision_recall_fscore_support, accuracy_score

from dataset import VinsDataset
from train_word_nn import load_intent_normalizations
from reranker_models.intent_based_model import IntentBasedModel
from reranker_models.reranker_model import RerankerModel


def _train(dataset_path, classifier_name='scenarios', forced_intents=None, skip_intents=None, train_intents=None):
    dataset, ranking_dataset, intent_to_index, intent_based_model = _collect_train_dataset(
        dataset_path, classifier_name, forced_intents, skip_intents, train_intents
    )

    print 'Dataset size =', len(ranking_dataset[0])

    model = RerankerModel(intent_to_index, intent_based_model, dataset)

    feature_importance = model.fit(*ranking_dataset)
    _output_feature_importance(feature_importance, intent_based_model, model, dataset)

    return model, intent_to_index, intent_based_model


def _output_feature_importance(feature_importance, intent_based_model, model, dataset):
    features = model.get_features(intent_based_model, dataset)
    assert len(features) == len(feature_importance)

    for feat, feat_score in zip(features, feature_importance):
        print feat, feat_score


def _collect_train_dataset(dataset_path, classifier_name='scenarios', forced_intents=None,
                           skip_intents=None, train_intents=None):
    dataset = VinsDataset.restore(dataset_path)
    dataset = dataset.to_dataset_view(
        classifier_name, features=RerankerModel.required_feature_list() + ['texts'],
        use_intents=[intent for intent in set(dataset._intents) if intent not in skip_intents]
    )

    renames = load_intent_normalizations(is_true=False)
    dataset._intents = [renames(intent) for intent in dataset._intents]

    intent_to_index = dataset._classifier_feature_mappings[classifier_name]
    intent_based_model = IntentBasedModel(intent_to_index, train_intents)

    ranking_dataset = RerankerModel.convert_dataset(
        dataset=dataset, intent_to_index=intent_to_index, intent_based_model=intent_based_model,
        skip_ones=False, skip_all_negatives=True, forced_intents=forced_intents,
        skip_intents=skip_intents, use_labels=True, threshold=0.65
    )

    return dataset, ranking_dataset, intent_to_index, intent_based_model


def _dump_dataset(dataset, ranking_dataset, intent_to_index, intent_based_model, path):
    index_to_intent = [intent for intent, _ in sorted(intent_to_index.iteritems(), key=itemgetter(1))]
    ranking_variants = ranking_dataset[0][:, 0].astype(np.int)
    ranking_variants = [index_to_intent[intent_index] for intent_index in ranking_variants]

    ranking_data = ranking_dataset[0][:, 1:]
    RerankerModel.dump_dataset_to_tsv(
        dataset=dataset,
        ranking_data=ranking_data,
        ranking_groups=ranking_dataset[1],
        ranking_labels=ranking_dataset[2],
        ranking_variants=ranking_variants,
        sample_labels=np.array(dataset._intents),
        sample_texts=np.array(dataset._preprocessed_texts),
        intent_based_model=intent_based_model,
        path=path + '.tsv',
        cd_path=path + '.cd'
    )


def _validate(model, intent_to_index, dataset_path, intent_based_model, classifier_name='scenarios',
              forced_intents=None, skip_intents=None, train_intents=None):

    index_to_intent = [intent for intent, _ in sorted(intent_to_index.iteritems(), key=itemgetter(1))]

    dataset, (ranking_data, ranking_groups, ranking_labels), intent_to_index, intent_based_model = _collect_val_dataset(
        dataset_path, classifier_name, forced_intents, skip_intents, train_intents
    )

    print 'Baseline:'
    renames = load_intent_normalizations()
    true_intents = np.array([renames(intent) for intent in dataset._intents])

    y_true = [intent_to_index.get(intent, -1) for intent in true_intents]
    y_pred, left_items = _collect_answers(ranking_data[:, 1], ranking_groups, ranking_data[:, 0], y_true)
    baseline_report, baseline_accuracy = _eval_model(
        true_intents[np.unique(ranking_groups)][left_items], y_pred, index_to_intent, skip_intents, dataset
    )

    print 'Reranker:'
    y_pred, scores = model.predict(ranking_data, ranking_groups)
    y_pred, left_items = _collect_answers(scores, ranking_groups, ranking_data[:, 0], y_true)
    result_report, result_accuracy = _eval_model(
        true_intents[np.unique(ranking_groups)][left_items], y_pred, index_to_intent, skip_intents, dataset
    )

    report = pd.concat([baseline_report[['precision', 'recall', 'f1-score']], result_report], axis=1)
    report['delta'] = result_report['f1-score'].values - baseline_report['f1-score'].values

    print 'Report:\n{}\n'.format(report)
    print 'Accuracy: {:.2%} -> {:.2%}'.format(baseline_accuracy, result_accuracy)
    print

    intent_based_model.save('../classifier-models/')

    if 'ner_feature' in dataset._classifier_feature_mappings:
        with open('../classifier-models/ner_mapping.pkl', 'wb') as f:
            pickle.dump(dataset._classifier_feature_mappings['ner_feature'], f, pickle.HIGHEST_PROTOCOL)

        with open('../classifier-models/wizard_mapping.pkl', 'wb') as f:
            pickle.dump(dataset._classifier_feature_mappings['wizard_feature'], f, pickle.HIGHEST_PROTOCOL)

    with open('../classifier-models/catboost/reranker.desc', 'wb') as f:
        pickle.dump((intent_based_model.known_intents, model._cat_feature_indices), f, pickle.HIGHEST_PROTOCOL)


def _collect_val_dataset(dataset_path, classifier_name, forced_intents, skip_intents, train_intents):
    dataset = VinsDataset.restore(dataset_path)
    dataset = dataset.to_dataset_view(
        classifier_name, features=RerankerModel.required_feature_list() + ['texts'],
        use_intents=[intent for intent in set(dataset._intents) if intent not in skip_intents]
    )

    renames = load_intent_normalizations(is_true=False)
    dataset._intents = [renames(intent) for intent in dataset._intents]

    intent_to_index = dataset._classifier_feature_mappings[classifier_name]
    intent_based_model = IntentBasedModel(intent_to_index, train_intents)

    ranking_dataset = RerankerModel.convert_dataset(
        dataset=dataset, intent_based_model=intent_based_model, intent_to_index=intent_to_index,
        forced_intents=forced_intents, skip_intents=skip_intents, use_labels=False,
        skip_ones=False, skip_all_negatives=False, threshold=0.65
    )
    return dataset, ranking_dataset, intent_to_index, intent_based_model


def _collect_answers(scores, groups, variants, y_true):
    def _iterate_group_indices():
        cur_group_id, cur_group_begin = 0, 0
        for ind, group_id in enumerate(groups):
            if group_id != cur_group_id:
                yield cur_group_begin, ind
                cur_group_id, cur_group_begin = group_id, ind
        yield cur_group_begin, len(groups)

    y_pred, left_items = [], []
    variants = variants.astype(np.int32)
    for sample_ind, ((group_begin, group_end), true_index) in enumerate(izip(_iterate_group_indices(), y_true)):
        group_variants = variants[group_begin: group_end]
        group_scores = scores[group_begin: group_end]

        if len(group_variants) == 0:
            continue

        best_variants = group_variants[group_scores == group_scores.max()]
        predicted_intent_index = best_variants[0]
        for variant_index in best_variants:
            if variant_index == true_index:
                predicted_intent_index = variant_index
                break
        y_pred.append(predicted_intent_index)
        left_items.append(sample_ind)
    return y_pred, left_items


def _eval_model(y_true, y_pred, index_to_intent, skip_intents=None, dataset=None, log_file=None):
    renames = load_intent_normalizations(is_true=False)
    y_pred = [renames(index_to_intent[ind]) for ind in y_pred]

    errors_stats = Counter()
    for true_intent, predicted_intent in izip(y_true, y_pred):
        if true_intent != predicted_intent:
            errors_stats[(true_intent, predicted_intent)] += 1

    print 'Most common errors:'
    for ((true_intent, predicted_intent), count) in errors_stats.most_common(10):
        print '{} -> {} - {} times'.format(true_intent, predicted_intent, count)
    print

    labels = sorted(set(y_true) - {'personal_assistant.scenarios.other'})

    precisions, recalls, f1_scores, supports = precision_recall_fscore_support(
        y_true, y_pred, labels=labels, average=None
    )
    total_precision, total_recall, total_f1_score, _ = precision_recall_fscore_support(
        y_true, y_pred, labels=labels, average='weighted'
    )

    report = pd.DataFrame(
        data={
            'precision': list(precisions) + [total_precision],
            'recall': list(recalls) + [total_recall],
            'f1-score': list(f1_scores) + [total_f1_score],
            'support': list(supports) + [supports.sum()]
        },
        index=list(labels) + ['total'],
        columns=['precision', 'recall', 'f1-score', 'support']
    )
    return report, accuracy_score(y_true, y_pred)


@click.group()
def main():
    pass


def _read_train_intents(mappings_dir):
    train_intents_path = os.path.join(mappings_dir, 'intent_info_known_intents.txt')

    with open(train_intents_path) as f:
        return {line.rstrip(): index for index, line in enumerate(f)}


@main.command()
@click.option('--train-data-path', type=click.Path(exists=True))
@click.option('--val-data-path', type=click.Path(exists=True))
@click.option('--classifier-name', type=click.Choice(['scenarios', 'toloka']), default='scenarios',
              help='The name of classifier which features would be extracted for training/inference from the dataset')
@click.option('--output-model-dir', type=click.Path(), default=None)
@click.option('--forced-intents', default='')
@click.option('--skip-intents', default='')
@click.option('--mappings-dir', default='apps/personal_assistant/personal_assistant/data/post_classifier')
def train(train_data_path, val_data_path, classifier_name, output_model_dir,
          forced_intents, skip_intents, mappings_dir):

    pd.set_option('display.width', 1000)
    pd.set_option('display.max_rows', 1000)
    pd.set_option('display.max_colwidth', 100)
    pd.set_option('precision', 4)

    forced_intents = forced_intents.split(',') if forced_intents else []
    skip_intents = skip_intents.split(',') if skip_intents else []

    train_intents = _read_train_intents(mappings_dir)

    model, intent_to_index, intent_based_model = _train(
        dataset_path=train_data_path, classifier_name=classifier_name,
        forced_intents=forced_intents, skip_intents=skip_intents, train_intents=train_intents
    )

    _validate(
        model=model, intent_to_index=intent_to_index, classifier_name=classifier_name,
        intent_based_model=intent_based_model, dataset_path=train_data_path,
        forced_intents=forced_intents, skip_intents=skip_intents, train_intents=train_intents
    )

    _validate(
        model=model, intent_to_index=intent_to_index, classifier_name=classifier_name,
        intent_based_model=intent_based_model, dataset_path=val_data_path,
        forced_intents=forced_intents, skip_intents=skip_intents, train_intents=train_intents
    )

    if output_model_dir:
        model.save(output_model_dir)


@main.command()
@click.option('--data-path', type=click.Path(exists=True))
@click.option('--classifier-name', type=click.Choice(['scenarios', 'toloka']), default='scenarios',
              help='The name of classifier which features would be extracted for training/inference from the dataset')
@click.option('--is-train', is_flag=True)
@click.option('--forced-intents', default='')
@click.option('--skip-intents', default='')
@click.option('--dump-dataset-dir', type=click.Path(), default=None)
@click.option('--mappings-dir', default='apps/personal_assistant/personal_assistant/data/post_classifier')
@click.option('--name', default=None)
def dump(data_path, classifier_name, is_train, forced_intents, skip_intents, dump_dataset_dir, mappings_dir, name):
    forced_intents = forced_intents.split(',') if forced_intents else []
    skip_intents = skip_intents.split(',') if skip_intents else []

    click.echo('Loading dataset')

    train_intents = _read_train_intents(mappings_dir)

    if is_train:
        dataset, ranking_dataset, intent_to_index, intent_based_model = _collect_train_dataset(
            data_path, classifier_name, forced_intents, skip_intents, train_intents
        )
        name = name or 'train'
    else:
        dataset, ranking_dataset, intent_to_index, intent_based_model = _collect_val_dataset(
            data_path, classifier_name, forced_intents, skip_intents, train_intents
        )
        name = name or 'val'

    click.echo('Dumping dataset')
    _dump_dataset(dataset, ranking_dataset, intent_to_index, intent_based_model, os.path.join(dump_dataset_dir, name))


if __name__ == '__main__':
    main()
