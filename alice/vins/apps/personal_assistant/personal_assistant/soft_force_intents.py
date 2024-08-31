from vins_core.nlu.classifier import Classifier

EXP_SOFT_FORCE_INTENTS = 'soft_force_intents'


class SoftForceIntentsClassifier(Classifier):
    """ Predict score 1 for all intents that came with the experiment flag
    This should work as a fixlist that does not override transition model """
    def __init__(self, **kwargs):
        super(SoftForceIntentsClassifier, self).__init__(**kwargs)

    def _process(self, feature, req_info, **kwargs):
        experiment_value = req_info.experiments[EXP_SOFT_FORCE_INTENTS] or ''
        forced_intents = [intent.strip() for intent in experiment_value.split(',')]
        forced_intents = [intent for intent in forced_intents if intent]
        return {class_name: 1 for class_name in forced_intents}

    @property
    def default_score(self):
        return 0

    @property
    def classes(self):
        return set()

    def load(self, archive, name, **kwargs):
        pass

    def save(self, archive, name):
        pass
