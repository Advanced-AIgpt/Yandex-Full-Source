import logging
import numpy as np
import json
import os.path
import time

from .tf_pipelines import fit_save_pipeline, load_predict_pipeline
from .metrics_counter import MetricsCounter
from .dataset import Dataset

import sklearn.metrics as metrics
import matplotlib.pyplot as plt

def handle_fit_mode(config):
    '''Handles all the logic of fit mode
    '''
    logging.info('Starting to prepare dataset {} with embeddings at {}'.format(config.positives_path, config.embeddings_path))

    dataset = Dataset(config)

    logging.info('Dataset of size {} was prepared. Starting to fit.'.format(dataset.pos.shape[0] + dataset.neg.shape[0]))

    fit_save_pipeline(dataset, config)

    logging.info('Model was fitted and then saved at {}. '.format(config.model_path))


def handle_predict_mode(config):
    '''Handles all the logic of predict mode
    '''

    logging.info('Starting to prepare dataset {} with embeddings at {}'.format(config.positives_path, config. embeddings_path))

    dataset = Dataset(config)

    logging.info('Dataset was prepared for testing.')

    predicted, threshold = load_predict_pipeline(dataset, config)

    real_answers = np.concatenate(
        (np.ones(dataset.pos.shape[0], dtype=int), np.zeros(dataset.neg.shape[0], dtype=int)), 
        axis=0)

    if config.result_path is not None:
        with open(config.result_path, 'w') as file:
            sorted_data = sorted(zip(
                predicted,
                real_answers,
                dataset.samples['pos'] + dataset.samples['neg']),
                key=lambda x: x[0],
                reverse=True)

            file.write('text\tpredicted\treal\n')
            for elem in sorted_data:
                file.write('{}\t{}\t{}\n'.format(elem[2], elem[0], elem[1]))

    logging.info('Got predicted results for {} positives and {} negatives. '.format(dataset.pos.shape[0], dataset.neg.shape[0]))

    fpr, tpr, _ = metrics.roc_curve(real_answers, predicted)
    roc_auc = metrics.auc(fpr, tpr)

    logging.info("ROC-AUC: {}".format(roc_auc))

    plt.title('Receiver Operating Characteristic')
    plt.plot(fpr, tpr, 'b', label = 'AUC = %0.2f' % roc_auc)
    plt.legend(loc = 'lower right')
    plt.plot([0, 1], [0, 1],'r--')
    plt.xlim([0, 1])
    plt.ylim([0, 1])
    plt.ylabel('True Positive Rate')
    plt.xlabel('False Positive Rate')

    plt.savefig('roc-auc.png')

    predicted = predicted > threshold

    weights = np.concatenate((dataset.pos_weights, dataset.neg_weights), axis=0)

    counter = MetricsCounter()
    counter.add_batch(real_answers, predicted, weights=weights)

    precision, recall, f1 = counter.precision, counter.recall, counter.f1

    logging.info('precision = {}, recall = {}, f1 = {}'.format(precision, recall, f1))
