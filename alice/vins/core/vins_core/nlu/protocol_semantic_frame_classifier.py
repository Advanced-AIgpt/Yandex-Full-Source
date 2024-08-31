# -*- coding: utf-8 -*-

from vins_core.nlu.classifier import Classifier


class ProtocolSemanticFrameClassifier(Classifier):
    def __init__(self, matching_score=1.0, **kwargs):
        super(ProtocolSemanticFrameClassifier, self).__init__(**kwargs)

        self._matching_score = matching_score

    def _process(self, sample_features, req_info=None, **kwargs):
        skip_relevants = req_info.additional_options.get('skip_relevant_intents', False)
        if req_info and req_info.semantic_frames and not skip_relevants:
            return {
                semantic_frame['name']: self._matching_score
                for semantic_frame in req_info.semantic_frames
            }
        else:
            return dict()

    @property
    def classes(self):
        return []

    @property
    def default_score(self):
        return 1.0

    def load(self, archive, name, **kwargs):
        pass

    def save(self, archive, name):
        pass
