import pytest

from operator import itemgetter

from ..neural.nn_classifier import NNClassifierModel
from vins_core.nlu.token_tagger import CRFModel
from vins_core.nlu.features.post_processor.vectorizer import VectorizerFeaturesPostProcessor

from sklearn.decomposition.pca import PCA
from sklearn.linear_model import LogisticRegression

from vins_core.nlu.sklearn_utils import make_pipeline, get_estimator_name, name_estimators


@pytest.mark.parametrize("estimator, result", [
    (PCA, 'pca'),
    (PCA(), 'pca'),
    (LogisticRegression, 'logisticregression'),
    (LogisticRegression(), 'logisticregression'),
    (CRFModel, 'crfmodel'),
    (CRFModel(), 'crfmodel'),
    (NNClassifierModel, 'nnclassifiermodel'),
    (NNClassifierModel(), 'nnclassifiermodel'),
    (VectorizerFeaturesPostProcessor, 'vectorizerfeaturespostprocessor'),
    (VectorizerFeaturesPostProcessor(), 'vectorizerfeaturespostprocessor')
])
def test_get_estimator_name(estimator, result):
    assert get_estimator_name(estimator) == result


def test_name_estimators():
    assert map(itemgetter(0), name_estimators([
        PCA(), PCA(), VectorizerFeaturesPostProcessor(), LogisticRegression()
    ])) == ['pca-1', 'pca-2', 'vectorizerfeaturespostprocessor', 'logisticregression']


def test_make_pipeline():
    pipeline = make_pipeline(PCA(), LogisticRegression())
    assert isinstance(pipeline.named_steps['pca'], PCA)
    assert isinstance(pipeline.named_steps['logisticregression'], LogisticRegression)
