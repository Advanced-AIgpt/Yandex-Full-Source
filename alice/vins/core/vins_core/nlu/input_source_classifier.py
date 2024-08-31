from vins_core.nlu.classifier import Classifier


class UtteranceInputSourceClassifier(Classifier):
    def __init__(self, input_source_to_intent, **kwargs):
        super(UtteranceInputSourceClassifier, self).__init__(**kwargs)

        self._input_source_to_intent = input_source_to_intent
        self._classes = set(input_source_to_intent.itervalues())

    def _process(self, feature, **kwargs):
        intent = self._input_source_to_intent.get(feature.sample.utterance.input_source)
        return {class_name: 1 if class_name == intent else 0 for class_name in self._classes}

    @property
    def default_score(self):
        return 0

    @property
    def classes(self):
        return self._classes

    def load(self, archive, name, **kwargs):
        pass

    def save(self, archive, name):
        pass
