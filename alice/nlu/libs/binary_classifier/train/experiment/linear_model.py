# coding: utf-8

import attr
import argparse
import cPickle as pickle
import joblib
import json
import logging
import numpy as np
import os

from sklearn.linear_model import LogisticRegression

from dataset_utils import DatasetConfig, load_dataset, prepare_data, download_dataset
from metric_utils import compute_metrics

logger = logging.getLogger(__name__)


@attr.s(frozen=True)
class ExperimentConfig(object):
    classifier_params = attr.ib()
    vectorizer_params = attr.ib()
    output_path = attr.ib()


def _train(train_vectors, train_labels, classifier_params):
    classifier = LogisticRegression(n_jobs=-1, solver='saga', **classifier_params)
    classifier.fit(train_vectors, train_labels)

    return classifier


def _evaluate(classifier, valid_vectors, valid_labels, output_dir, output_prefix):
    valid_predictions = classifier.predict_proba(valid_vectors)[:, 1]
    compute_metrics(valid_predictions, valid_labels, output_dir, output_prefix)


def _save(experiment_config, classifier, vectorizer=None):
    output_dir = experiment_config.output_path
    joblib.dump(classifier, os.path.join(output_dir, 'model.pkl'), protocol=pickle.HIGHEST_PROTOCOL)
    if vectorizer:
        joblib.dump(vectorizer, os.path.join(output_dir, 'vectorizer.pkl'), protocol=pickle.HIGHEST_PROTOCOL)

    with open(os.path.join(output_dir, 'config.json'), 'w') as f:
        json.dumps(attr.asdict(experiment_config))


def _run_experiment(train_dataset, valid_dataset, experiment_config):
    logger.info('Training model with config %s...', experiment_config)

    train_vectors, train_labels, valid_vectors, valid_labels, vectorizer = prepare_data(
        train_dataset, valid_dataset, experiment_config.vectorizer_params
    )

    classifier = _train(train_vectors, train_labels, experiment_config.classifier_params)

    if not os.path.isdir(experiment_config.output_path):
        os.makedirs(experiment_config.output_path)

    _evaluate(classifier, train_vectors, train_labels, experiment_config.output_path, output_prefix='train')
    _evaluate(classifier, valid_vectors, valid_labels, experiment_config.output_path, output_prefix='valid')

    _save(experiment_config, classifier, vectorizer)


def _run_experiments(dataset_config_path):
    with open(dataset_config_path) as f:
        dataset_config = DatasetConfig(**json.load(f))

    train_dataset, valid_dataset = load_dataset(dataset_config)

    _run_experiment(
        train_dataset,
        valid_dataset,
        experiment_config=ExperimentConfig(
            classifier_params={},
            vectorizer_params={
                'source': 'sklearn',
                'type': 'tfidf',
                'ngram_range': (1, 2)
            },
            output_path='models/tfidf_linear'
        )
    )

    embedding_types = [key for key in train_dataset[0] if key.endswith('embedding')]
    for embedding_type in embedding_types:
        _run_experiment(
            train_dataset,
            valid_dataset,
            experiment_config=ExperimentConfig(
                classifier_params={},
                vectorizer_params={
                    'source': 'dssm',
                    'type': embedding_type,
                },
                output_path='models/{}_linear'.format(embedding_type)
            )
        )


def _vectorize_dataset(dataset, vectorizer_params, model_dir):
    if vectorizer_params['source'] == 'dssm':
        return np.array([sample[vectorizer_params['type']] for sample in dataset])

    if vectorizer_params['source'] == 'sklearn':
        vectorizer = joblib.load(os.path.join(model_dir, 'vectorizer.pkl'))

        texts = [sample['utterance_text'] for sample in dataset]
        return vectorizer.transform(texts)

    assert False, 'Unknown vectorizer type {}'.format(vectorizer_params['type'])


def _run_evaluation(dataset_config_path, model_dir, eval_table_path):
    with open(dataset_config_path) as f:
        dataset_config = DatasetConfig(**json.load(f))

    dataset = download_dataset(eval_table_path)

    with open(os.path.join(model_dir, 'config.json')) as f:
        model_config = json.load(f)

    classifier = joblib.load(os.path.join(model_dir, 'model.pkl'))

    test_vectors = _vectorize_dataset(dataset, model_config['vectorizer_params'], model_dir)
    test_labels = np.array([int(sample['intent'] == dataset_config.intent) for sample in dataset])

    output_prefix = eval_table_path.split('/')[-1]
    _evaluate(classifier, test_vectors, test_labels, model_dir, output_prefix)


def main():
    logging.basicConfig(level=logging.INFO, format='[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s')

    parser = argparse.ArgumentParser()
    parser.add_argument('--mode', choices=['experiments', 'evaluation'], required=True)
    parser.add_argument('--dataset-config-path', required=True)
    parser.add_argument('--model-dir')
    parser.add_argument('--eval-table-path')
    args = parser.parse_args()

    if args.mode == 'experiments':
        _run_experiments(args.dataset_config_path)
    elif args.mode == 'evaluation':
        _run_evaluation(args.dataset_config_path, args.model_dir, args.eval_table_path)
    else:
        assert False


if __name__ == "__main__":
    main()
