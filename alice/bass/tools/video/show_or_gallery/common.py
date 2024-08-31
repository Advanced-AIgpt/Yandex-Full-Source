import numpy as np
import pandas as pd

from catboost import CatBoostClassifier


DEFAULT_DATA_PATH = 'data.csv'
DEFAULT_FPR=0.025
DEFAULT_MODEL_PATH = 'model.cbm'
DEFAULT_SEED = 42
DEFAULT_WEIGHT_SAMPLES=True


def load_data(path, load_weights=True):
    data = pd.read_csv(path)
    Xs = data.drop(['top_relevant_only', 'weight', 'query', 'id'], axis=1)
    ys = data['top_relevant_only']
    ws = data['weight'] if load_weights else np.ones(len(data))
    qs = data['query']
    return Xs, ys, ws, qs


def make_classifier(verbose=False, random_state=DEFAULT_SEED):
    return CatBoostClassifier(iterations=100, verbose=verbose, random_state=random_state)
