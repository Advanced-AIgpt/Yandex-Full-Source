# coding: utf-8
"""
This module is a tool to debug and analyze classifiers and taggers in interactive mode.
It is especially handy inside a jupyter notebook.

An example of usage:
    from vins_tools.nlu.inspection.interactive_app_analyzer import InteractiveAppAnalyzer
    analyzer = InteractiveAppAnalyzer()
    print(analyzer.get_hypotheses('играй', 15))
"""

import pandas as pd
import numpy as np

from sklearn.pipeline import make_pipeline

from vins_core.dm.session import Session
from vins_core.utils.datetime import utcnow
from vins_core.dm.request import create_request, AppInfo
from vins_core.utils.misc import gen_uuid_for_tests
from vins_core.dm.intent import Intent

from vins_tools.nlu.inspection.nlu_processing_on_dataset import load_app_by_name
from vins_tools.nlu.inspection.nlu_result_info import NluResultInfo


class _KNNWrapper(object):
    def __init__(self, abstract_knn_model):
        label_to_meta = abstract_knn_model.get_label_to_meta()
        labels, texts, vectors = [], [], []

        for label, meta in label_to_meta.iteritems():
            labels.extend((label for _ in xrange(len(meta))))
            texts.extend(meta.texts)
            vectors.append(meta.vectors)

        self._labels = np.array(labels)
        self._texts = np.array(texts)
        self._vectors = np.concatenate(vectors)

    def compute_neighbors(self, vector, num_neighbors):
        proximity = self._vectors.dot(vector.T).ravel()
        idxmaxes = np.argsort(-proximity)
        return self._labels[idxmaxes], self._texts[idxmaxes], proximity[idxmaxes]


class InteractiveAppAnalyzer(object):
    def __init__(self, app='personal_assistant'):
        if isinstance(app, (str, unicode)):
            app, _ = load_app_by_name(app_name=app)
        self.app = app

        knn_classifier = self.app.nlu.get_classifier('scenarios')
        self._knn_feature_encoder = make_pipeline(*(step[1] for step in knn_classifier._model.steps[:-1]))
        self._knn_label_encoder = knn_classifier.label_encoder
        self._knn_wrapper = _KNNWrapper(knn_classifier._model._final_estimator)

    def get_session(self, utterance, device_state=None, prev_intent=None, app_id=None, experiments=('video_play',)):
        """ Create session and request_info with the given context """
        client_time = utcnow()
        req_info = create_request(
            uuid=gen_uuid_for_tests(),
            utterance=utterance,
            client_time=client_time,
            app_info=AppInfo(app_id=app_id),
            experiments=experiments,
            device_state=device_state,
        )
        session = Session(req_info.app_info.app_id, req_info.uuid)
        if prev_intent:
            session.change_intent(Intent(prev_intent))
        return session, req_info

    def handle(self, utterance, force_intent=None, postprocess=False, **kwargs):
        """ Turn utterance into a list of semantic frames """
        sess, req = self.get_session(utterance, **kwargs)
        sample = self.app.samples_extractor([utterance], sess, filter_errors=True)[0]
        if postprocess:
            nlu_result = self.app.dm.get_semantic_frames(
                sample, sess, req_info=req, force_intent=force_intent
            )
        else:
            nlu_result = self.app.nlu.handle(sample, sess, req_info=req, force_intent=force_intent)
        return nlu_result.semantic_frames

    def get_sample(self, utterance, **kwargs):
        """ Turn utterance into a processed sample """
        sess, req = self.get_session(utterance, **kwargs)
        sample = self.app.samples_extractor([utterance], sess, filter_errors=True)[0]
        return sample

    def get_slots(self, utterance, **kwargs):
        """ Turn utterance into a dict of slots """
        return self.handle(utterance, **kwargs)[0]['slots']

    def get_feature(self, utterance):
        """ Turn utterance into a SampleFeatures object """
        session, req_info = self.get_session(utterance)
        samples = self.app.samples_extractor([utterance], session, filter_errors=True)
        features = self.app.nlu._features_extractor(samples)
        return features[0]

    def get_hypotheses(self, utterance, top=10):
        """ Turn utterance into a sorted list of top matching intents """
        f = self.get_feature(utterance)
        return pd.Series(self.app.nlu.get_classifier('scenarios')(f)).sort_values(ascending=False).head(top)

    def utt2nlu(self, utterance, force_intent=None, print_intent=False, return_intent=False, index=0, **kwargs):
        """ Turn utterance into a NLU string """
        sess, req = self.get_session(utterance, **kwargs)
        sample = self.app.samples_extractor([utterance], sess, filter_errors=True)[0]
        nlu_result = self.app.dm.get_semantic_frames(
            sample, session=sess, req_info=req, force_intent=force_intent
        )
        frames = nlu_result.semantic_frames
        nlu = NluResultInfo.get_nlu_string(sample.tokens, frames[0]['slots'])
        if print_intent:
            print(frames[index]['intent_name'])
        if return_intent:
            return frames[index]['intent_name'], nlu
        return nlu

    def get_neighbors(self, utterance, top=20, intents=None):
        """ Find top neighbors of an utterance """
        f = self.get_feature(utterance)
        emb = self._knn_feature_encoder.transform([f])[0]

        labels, texts, distances = self._knn_wrapper.compute_neighbors(emb, top)

        nei = pd.DataFrame({
            "text": texts,
            "dist": distances,
            "label": self._knn_label_encoder.inverse_transform(labels).tolist(),
        })
        if intents is not None:
            nei = nei[nei.label.str.contains(intents)]
        if top is not None:
            nei = nei.head(top)
        return nei
