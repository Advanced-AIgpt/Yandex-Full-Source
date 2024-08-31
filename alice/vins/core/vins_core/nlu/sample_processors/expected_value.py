# -*- coding: utf-8 -*-
from __future__ import unicode_literals
import logging

from vins_core.common.sample import Sample
from vins_core.nlu.sample_processors.base import BaseSampleProcessor
from vins_core.utils.lemmer import Lemmer
from sklearn.feature_extraction.text import TfidfVectorizer
from sklearn.linear_model import LogisticRegression
from vins_core.utils.strings import smart_unicode as u


logger = logging.getLogger(__name__)


def is_zero_vector(vector):
    non_zero_items = vector.nonzero()
    return not bool(non_zero_items[0].all() == 0 or non_zero_items[1].all() == 0)


class RealTimeTextClassifier(object):
    rumorph = Lemmer(['ru', 'en'])

    def __init__(self, train_set):
        self._cls = LogisticRegression(C=1000)
        self._count_vect = TfidfVectorizer(
            min_df=0.0, ngram_range=(0, 2),
            tokenizer=lambda x: x.split())
        data_x = [self.prepare_text(_[0]) for _ in train_set]
        data_y = [_[1] for _ in train_set]
        self._count_vect.fit(data_x)
        train_x = self._count_vect.transform(data_x)
        self._cls.fit(train_x, data_y)
        self._topics = sorted(list(set(data_y)))

    def prepare_text(self, text):
        return ' '.join([self.rumorph.parse(u(w))[0].normal_form for w in text.split()])

    def predict(self, utterance):
        prd = self.predict_proba(utterance)
        if prd and prd[0][1] > 0.544:  # TODO: we should create some set for selection the threshold
            return prd[0]
        else:
            return None, 0

    def predict_proba(self, utterance):
        vector = self._count_vect.transform([self.prepare_text(utterance)])
        if is_zero_vector(vector):
            return []
        prd = self._cls.predict_proba(vector)
        top_hyp = sorted(zip(self._topics, prd[0]), key=lambda x: -x[1])
        logger.debug('RealTimeTextClassifier.predict_proba: %s', top_hyp)
        return top_hyp


class ExpectedValueSampleProcessor(BaseSampleProcessor):
    """Processor for resolving active slot values which need to be gazetteer.

    For example, if utterance should be 'big' or 'small' and user types 'bigger', classifier will resolve it
    for 'big'.
    """

    @property
    def is_normalizing(self):
        return True

    def _process(self, sample, session, *args, **kwargs):
        if session and session.intent_name and session.form and session.form.has_expected_values():
            c = RealTimeTextClassifier([
                (' '.join(v.tokens), ' '.join(v.tokens))
                for v in session.form.get_expected_values()
            ])
            value, _ = c.predict(sample.text)
            if value:
                logger.debug('Find expected value: %s', value)
                return Sample(utterance=sample.utterance, tokens=value.split(),
                              partially_normalized_text=sample.partially_normalized_text)
        return sample
