import json
import logging

logger = logging.getLogger(__name__)


def f1_score(precision, recall):
    if precision + recall == 0:
        logger.warning("F1 = 0 due to zero-valued precision+recall sum.")
        return 0
    return 2 * precision * recall / (precision + recall)


class IntentLabelEncoder(object):
    def __init__(self, table_path, target_column):
        self._intent_to_label = {}
        self._intents = []

        if not table_path:
            # TODO(DIALOG-8340) seems incorrect condition, but we need to get rid off this class
            self._intents = tuple()
            return

        for line in open(table_path, 'r'):
            encoding_element = json.loads(line)

            intent = encoding_element[target_column]

            if len(encoding_element) != 2:
                raise ValueError("encoding must contain class and label elements")

            # TODO(DIALOG-8340) hackish way to get label from dict without explicitly passing label_column in args
            label_column, = set(encoding_element).difference({target_column})
            label = encoding_element[label_column]

            self._intent_to_label[intent] = label

        self._intents = [None] * len(self._intent_to_label)
        for intent, label in self._intent_to_label.items():
            self._intents[label] = intent

        if None in self._intents:
            raise ValueError('incorrect label encoder')

        self._intents = tuple(self._intents)

    def encode(self, intent, default=None):
        return self._intent_to_label.get(intent, default)

    def decode(self, label):
        return self._intents[label]

    @property
    def intents(self):
        return self._intents
