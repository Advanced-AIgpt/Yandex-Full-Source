import json
import logging
import os
import random
import re
import time
from collections import defaultdict, Mapping
from functools import partial
from itertools import izip

import attr

from vins_core.common.utterance import Utterance
from vins_core.ext.base import BaseHTTPAPI
from vins_core.ext.s3 import S3Updater
from vins_core.ext.yt import YtUpdater
from vins_core.nlu.classifier import Classifier
from vins_core.utils.config import get_setting
from vins_core.utils.data import load_data_from_file

logger = logging.getLogger(__name__)


@attr.s(frozen=True)
class LookupItem(object):
    DEFAULT_APP_ID = ''
    request = attr.ib()
    app_id = attr.ib(default=DEFAULT_APP_ID)


class Matcher(object):
    def __init__(self, intents, source):
        self._intents = intents
        self._intent_to_index = {intent: i for i, intent in enumerate(self._intents)}
        self._source = source

    def get_intents(self, text, app_id=LookupItem.DEFAULT_APP_ID):
        raise NotImplementedError()


def regexp_constructor(text):
    try:
        return re.compile(text, flags=re.U | re.IGNORECASE)
    except Exception:
        raise ValueError('%s is invalid regular expression' % text)


class RegexpMatcher(Matcher):
    def __init__(self, lookup_items, intents, correct_intents, source):
        super(RegexpMatcher, self).__init__(correct_intents, source)
        self._lookup = defaultdict(list)
        intent_app_id_to_patterns = defaultdict(list)
        for lookup_item, intent in izip(lookup_items, intents):
            if lookup_item is not None and self._validate(intent, lookup_item):
                intent_app_id_to_patterns[(self._intent_to_index[intent], lookup_item.app_id)].append(
                    lookup_item.request)

        for (intent_index, app_id), patterns in intent_app_id_to_patterns.iteritems():
            pattern = '^(?:%s)$' % '|'.join(map(lambda p: '(?:%s)' % p, patterns))
            app_id_pattern = '^(?:%s)$' % app_id if app_id != LookupItem.DEFAULT_APP_ID else LookupItem.DEFAULT_APP_ID
            self._lookup[(regexp_constructor(pattern), regexp_constructor(app_id_pattern))].append(
                intent_index)

    @staticmethod
    def _match(request_app_id, text, app_id):
        return re.match(request_app_id[0], text) and re.match(request_app_id[1], app_id)

    def _validate(self, intent, lookup_item):
        if intent not in self._intent_to_index:
            return False
        if not isinstance(lookup_item, LookupItem) or not isinstance(lookup_item.request, basestring):
            logger.error(
                ('Skipping lookup data {intent: "%s", item: "%r"} while loading lookup classifier'
                 ' from %s. Item is not an instance of %r.'),
                intent, lookup_item, self._source, LookupItem.__name__)
            return False
        try:
            re.compile(lookup_item.request)
            re.compile(lookup_item.app_id)
        except (re.error, TypeError):
            logger.error(
                ('Skipping lookup data {intent: "%s", item: "%r"} while loading lookup classifier'
                 ' from %s. Entries are not valid regexps.'),
                intent, lookup_item, self._source)
            return False
        return True

    def get_intents(self, text, app_id=LookupItem.DEFAULT_APP_ID):
        found_intents = set()
        for request_app_id, intents in self._lookup.iteritems():
            if self._match(request_app_id, text, app_id):
                found_intents.update(intents)
        return list(self._intents[index] for index in found_intents)


class TextMatcher(Matcher):
    def __init__(self, lookup_items, intents, correct_intents, source, samples_extractor):
        super(TextMatcher, self).__init__(correct_intents, source)
        self._samples_extractor = samples_extractor
        self._normalized_texts = {}
        self._lookup = defaultdict(list)
        if lookup_items is not None:
            for lookup_item, intent in izip(lookup_items, intents):
                if lookup_item is not None and self._validate(intent, lookup_item):
                    self._lookup[(self._normalize_text(lookup_item.request), lookup_item.app_id)].append(
                        self._intent_to_index[intent])

    def _validate(self, intent, lookup_item):
        if intent not in self._intent_to_index:
            return False
        if not isinstance(lookup_item, LookupItem) or not isinstance(lookup_item.request, basestring):
            logger.error(
                ('Skipping lookup data {intent: "%s", item: "%r"} while loading lookup classifier'
                 ' from %s. Item is not an instance of %r.'),
                intent, lookup_item, self._source, LookupItem.__name__)
            return False
        return True

    def _normalize_text(self, text):
        if self._samples_extractor is None:
            return text
        if text not in self._normalized_texts:
            utt = Utterance(text, Utterance.VOICE_INPUT_SOURCE)
            self._normalized_texts[text] = self._samples_extractor([utt], is_inference=False, normalize_only=True)[
                0].text
        return self._normalized_texts[text]

    def get_intents(self, text, app_id=LookupItem.DEFAULT_APP_ID):
        found_intents = self._lookup.get((text, app_id))
        if found_intents is None:
            found_intents = self._lookup.get((text, LookupItem.DEFAULT_APP_ID), [])
        return list(self._intents[index] for index in found_intents)


class LookupTokenClassifier(Classifier):
    def __init__(self, source, samples_extractor=None, regexp=False, intent_infos=None, matching_score=1,
                 nonmatching_score=None, default_score=0, **kwargs):
        """
        :param source: description of the data source (format depends on the implementation)
        :param samples_extractor: extractor used for normalization
        :param regexp: whether the lookup model uses regular expressions or exact string matching
        :param kwargs: additional parameters to initialize the superclass
        :param intent_infos: dict of all registered intents (needed to have the full set of class labels)
        """
        super(LookupTokenClassifier, self).__init__(**kwargs)

        self._source = source
        self._samples_extractor = samples_extractor
        self._regexp = regexp
        self._intent_infos = intent_infos

        self._classes = self._intent_infos.keys()

        self._default_score = default_score
        self._matching_score = matching_score
        self._nonmatching_score = nonmatching_score

        if self._regexp:
            self._matcher_constructor = partial(RegexpMatcher, correct_intents=self._classes, source=self._source)
        else:
            self._matcher_constructor = partial(TextMatcher, samples_extractor=self._samples_extractor,
                                                correct_intents=self._classes, source=self._source)
        self._matcher = None

        self._update()

    def _update(self):
        intents = []
        lookup_items = []
        for intent, lookup_item in self._iter_lookup_data():
            intents.append(intent)
            lookup_items.append(lookup_item)
        self._matcher = self._matcher_constructor(lookup_items, intents)

    def _iter_lookup_data(self):
        raise NotImplementedError()

    def _process(self, feature, **kwargs):
        intents = []
        if feature:
            intents = self._matcher.get_intents(feature.sample.text, feature.sample.app_id)
        if self._nonmatching_score is None or not intents:
            nonmatching_score = self.default_score
        else:
            nonmatching_score = self._nonmatching_score
        result = {intent: nonmatching_score for intent in self.classes}
        for intent in intents:
            result[intent] = self._matching_score
        return result

    def load(self, archive, name, **kwargs):
        pass

    def save(self, archive, name):
        pass

    @property
    def default_score(self):
        return self._default_score

    @property
    def classes(self):
        return self._classes


class DataLookupTokenClassifier(LookupTokenClassifier):
    def __init__(self, intent_texts, **kwargs):
        super(DataLookupTokenClassifier, self).__init__(source=intent_texts, **kwargs)

    def _iter_lookup_data(self):
        assert isinstance(self._source, Mapping)
        for intent, texts in self._source.iteritems():
            for text in texts:
                yield intent, LookupItem(text)


class FileLookupTokenClassifier(LookupTokenClassifier):

    def _iter_lookup_data(self):
        assert isinstance(self._source, basestring)
        raw_data = load_data_from_file(self._source)
        for intent, texts in raw_data.iteritems():
            for text in texts:
                yield intent, LookupItem(text)


class YTHttp(BaseHTTPAPI):
    TIMEOUT = 4.0
    HEADERS = {
        'X-YT-Output-Format': json.dumps({'$attributes': {'encode_utf8': False}, '$value': 'json'}),
        'Authorization': 'OAuth %s' % get_setting('YT_TOKEN', prefix='', default=''),
    }


class BaseExtLookupTokenClassifier(LookupTokenClassifier):
    EXTERNAL_SOURCE_NAME = 'unknown source'

    def __init__(self, source, update_period=600, **kwargs):
        self._source = source
        self._update_period = update_period
        self._next_update_time = time.time() + random.random() * self._update_period  # Randomize update times
        self._updater = self._get_updater()
        self._file_path = self._updater.file_path
        super(BaseExtLookupTokenClassifier, self).__init__(source, **kwargs)

    def _get_updater(self):
        raise NotImplementedError

    def __call__(self, *args, **kwargs):
        self._updater.update()
        return super(BaseExtLookupTokenClassifier, self).__call__(*args, **kwargs)

    def update_from_file(self, file_path):
        self._file_path = file_path
        self._update()


class YTLookupTokenClassifier(BaseExtLookupTokenClassifier):
    EXTERNAL_SOURCE_NAME = 'YT'

    def _get_updater(self):
        return YtUpdater(self._source, interval=self._update_period, on_update_func=self.update_from_file)

    def _iter_lookup_data(self):
        if self._file_path and os.path.exists(self._file_path):
            with open(self._file_path, 'rb') as f:
                for line in f:
                    item = json.loads(line)
                    yield item['intent'], LookupItem(item['text'])


class S3LookupTokenClassifier(BaseExtLookupTokenClassifier):
    EXTERNAL_SOURCE_NAME = 'S3'

    def _get_updater(self):
        return S3Updater(self._source, interval=self._update_period, on_update_func=self.update_from_file)

    def _iter_lookup_data(self):
        if self._file_path and os.path.exists(self._file_path):
            with open(self._file_path, 'rb') as f:
                for item in json.load(f):
                    yield item['intent'], LookupItem(item['text'])
