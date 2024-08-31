# coding: utf-8
import os
import cPickle as pickle

from collections import Mapping

from vins_core.nlu.classifier import Classifier
from vins_core.utils.misc import dict_zip
from vins_core.utils.data import get_resource_full_path, DirectoryView
from vins_models_tf import TfNnClassifier


class NontrainableTokenClassifier(Classifier):
    _MODEL_DATA_FILE = 'model_data.pkl'

    def __init__(self, archive, name, allow_singletons=False, **kwargs):
        super(NontrainableTokenClassifier, self).__init__(name=name, **kwargs)
        self._allow_singletons = allow_singletons

        model_dir = archive.get_tmp_dir()
        with archive.nested(name) as model_archive:
            for file_name in model_archive.list():
                model_archive.save_by_name(file_name, model_dir)
            model_dir = os.path.join(model_dir, model_archive.base)
        self._model = TfNnClassifier(model_dir)

        with open(os.path.join(model_dir, self._MODEL_DATA_FILE), 'rb') as f:
            model_data = pickle.load(f)
        self._label_encoder = model_data['label_encoder']
        self._transformers = model_data['transformers']

    def save(self, archive, name):
        tmp = archive.get_tmp_dir()
        data = {
            'transformers': self._transformers,
            'label_encoder': self._label_encoder
        }

        with open(os.path.join(tmp, self._MODEL_DATA_FILE), 'wb') as f:
            pickle.dump(data, f, pickle.HIGHEST_PROTOCOL)

        self._model.save(tmp)

        for file_name in os.listdir(tmp):
            archive.add(os.path.join(name, file_name), os.path.join(tmp, file_name))

    def _process(self, feature, skip_classifiers=(), **kwargs):
        if feature:
            scores = self.predict([feature], **kwargs)[0]
        else:
            scores = [0] * len(self._label_encoder.classes_)
        return dict_zip(keys=self._label_encoder.classes_, values=scores)

    @property
    def label_encoder(self):
        return self._label_encoder

    @property
    def classes(self):
        if hasattr(self._label_encoder, 'classes_'):
            return self._label_encoder.classes_
        return []

    def predict(self, features, **kwargs):
        error_message = 'Classifier {} has not been trained or loaded and has no class labels.'
        assert hasattr(self._label_encoder, 'classes_'), error_message.format(self.name)
        if len(self._label_encoder.classes_) == 1 and not self._allow_singletons:
            return [[1.]] * len(features)
        x = self.get_input(features, **kwargs)
        for transformer in self._transformers:  # no fit implementation => no pipeline
            x = transformer.transform(x)
        return self._model.predict_proba(x)

    def get_input(self, features, **kwargs):
        if isinstance(features, Mapping):  # intent-to-samples map is given
            features = sum(features.itervalues(), [])
        return features

    @property
    def final_estimator(self):
        return self._model

    @property
    def default_score(self):
        return 0


def create_nontrainable_token_classifier(model, archive=None, model_file=None, name=None, **kwargs):
    if model not in _token_classifier_factories:
        return None
    assert archive or model_file, 'Archive with model must be provided for classifier to load.'

    def create_classifier(archive):
        return _token_classifier_factories[model](name=name or model, archive=archive, **kwargs)

    if model_file:
        assert not archive, 'Only one of arguments archive and model_file must be provided.'
        with DirectoryView(get_resource_full_path(model_file)) as archive:
            return create_classifier(archive)
    return create_classifier(archive)


def register_token_classifier_type(cls_type, name):
    _token_classifier_factories[name] = cls_type


_token_classifier_factories = {}

register_token_classifier_type(NontrainableTokenClassifier, 'charcnn')
register_token_classifier_type(NontrainableTokenClassifier, 'rnn')
