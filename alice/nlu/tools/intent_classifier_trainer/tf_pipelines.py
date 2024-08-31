import logging
import numpy as np
import os
import tensorflow as tf
from tensorflow.keras import layers
from tensorflow import keras
import h5py
import json

from .binary_classifier import BinaryClassifier


def fit_save_pipeline(dataset, config):
    dataset.normalize_weights()

    assert((dataset.pos.size != 0) and (dataset.neg.size != 0)), 'In fit mode need both positives and negatives'

    train_data = [
        np.concatenate((dataset.pos, dataset.neg), axis=0),
        np.concatenate((np.ones(dataset.pos.shape[0]), np.zeros(dataset.neg.shape[0])), axis=0)]

    model = BinaryClassifier(config, True)

    model.fit(train_data)

    model.save()


def load_predict_pipeline(dataset, config):
    model = BinaryClassifier.restore(config)

    return np.concatenate(
        (model.predict_probas(dataset.pos),
         model.predict_probas(dataset.neg)),
        axis=0), model.threshold
