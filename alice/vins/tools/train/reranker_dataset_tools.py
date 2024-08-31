# -*- coding: utf-8 -*-

import numpy as np

# Imports used only for typing
from typing import AnyStr, NoReturn  # noqa: F401

from vins_core.nlu.token_classifier import KNNTokenClassifier
from vins_core.utils.misc import parallel

from dataset import VinsDataset
from vins_config_tools import open_model_archive, load_classifier_config
from vins_core.config.app_config import load_app_config


def split_dataset_for_reranker(dataset_path, train_dataset_path, boosting_train_dataset_path, test_dataset_path,
                               target_classifier_name, test_size, boosting_train_size):

    dataset = VinsDataset.restore(dataset_path)

    train_dataset, test_dataset = VinsDataset.split(
        dataset=dataset, holdout_classifiers=[target_classifier_name], holdout_size=test_size
    )
    train_dataset, boosting_train_dataset = VinsDataset.split(
        dataset=train_dataset, holdout_classifiers=[target_classifier_name], holdout_size=boosting_train_size
    )

    train_dataset.save(train_dataset_path)
    boosting_train_dataset.save(boosting_train_dataset_path)
    test_dataset.save(test_dataset_path)


def _load_knn_classifier():
    app_name = 'personal_assistant.app'
    classifier_name = 'scenarios'

    app_conf = load_app_config(app_name)
    classifier_conf = load_classifier_config(app_conf, classifier_name)
    classifier = KNNTokenClassifier(**classifier_conf['params'])
    with open_model_archive(load_app_config(app_name)) as archive:
        with archive.nested('classifiers') as arch:
            classifier.load(arch, classifier_name)

    return classifier


def _get_preds(sample_features, classifier, intent_mapping, **kwargs):
    preds = classifier(sample_features)
    qualities = np.zeros((1, len(intent_mapping)))

    for intent, pred in preds.iteritems():
        qualities[0, intent_mapping[intent]] = pred

    return qualities


def add_knn_feature(dataset_path, dataset_classifier):
    # type: (AnyStr, AnyStr) -> NoReturn

    classifier = _load_knn_classifier()
    intent_mapping = {intent: ind for ind, intent in enumerate(classifier.classes)}

    dataset = VinsDataset.restore(dataset_path)
    sample_features_list = dataset.to_sample_features_list(dataset_classifier)

    preds = parallel(
        _get_preds, sample_features_list,
        function_kwargs={'classifier': classifier, 'intent_mapping': intent_mapping}
    )
    preds = np.concatenate(preds, axis=0)
    dataset.add_feature(feature_name='knn', feature_type=VinsDataset.FeatureType.DENSE,
                        feature_matrix=preds, feature_mapping=intent_mapping)
    dataset.save(dataset_path)


def main():
    import os
    os.environ['VINS_LOAD_TF_ON_CALL'] = '1'

    add_knn_feature('../datasets/boosting_train_dataset.tar', 'scenarios')
    add_knn_feature('../datasets/toloka_test_dataset.tar', 'toloka')


if __name__ == '__main__':
    main()
