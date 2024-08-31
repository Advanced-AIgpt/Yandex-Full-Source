# coding: utf-8

import logging
import random
from collections import Mapping

import re
import validators

from vins_core.nlu.lookup_classifier import (
    LookupItem, LookupTokenClassifier
)
from vins_core.nlu.features.base import SampleFeatures
from vins_core.utils.data import load_data_from_file

from personal_assistant import intents


logger = logging.getLogger(__name__)


HARDCODED_RESPONSES_RESOURCE_PATH = 'personal_assistant/config/scenarios/hardcoded_responses.yaml'


def load_hardcoded(path):
    try:
        return load_data_from_file(path)
    except Exception as e:
        logger.exception("Error while parsing hardcoded response data: %s", e)
        raise


class HardcodedResponseLookupTokenClassifier(LookupTokenClassifier):
    def __init__(self, **kwargs):
        super(HardcodedResponseLookupTokenClassifier, self).__init__(
            source=load_hardcoded(HARDCODED_RESPONSES_RESOURCE_PATH),
            **kwargs
        )

    def _iter_lookup_data(self):
        for response_config in self._source.itervalues():
            app_id = response_config.get('app_id', '')
            for regexp in response_config.get('regexps') or []:
                yield intents.HARDCODED, LookupItem(regexp, app_id)


class AdHocHardcodedResponsesTokenClassifier(LookupTokenClassifier):
    def __init__(self, intent_texts, **kwargs):
        super(AdHocHardcodedResponsesTokenClassifier, self).__init__(source=intent_texts, **kwargs)

    def _iter_lookup_data(self):
        assert isinstance(self._source, Mapping)
        for intent, (texts, app_id) in self._source.iteritems():
            for text in texts:
                yield intent, LookupItem(text, app_id)


class HardcodedResponses(object):
    def __init__(self, samples_extractor):
        cfg, cls = self._load(HARDCODED_RESPONSES_RESOURCE_PATH, samples_extractor)

        self._config = cfg
        self._classifier = cls

    @classmethod
    def _validate_regexps(cls, response_name, config):
        regexps = config.get('regexps', [])

        regexps_with_errors = []
        correct_regexps = []

        for regexp in regexps:
            if not isinstance(regexp, basestring):
                regexps_with_errors.append(regexp)
                continue

            try:
                re.compile(regexp, flags=re.UNICODE)
            except re.error:
                regexps_with_errors.append(regexp)
                continue

            correct_regexps.append(regexp)

        for regexp in regexps_with_errors:
            logger.error(
                'Wrong format of regexp "%s" for hardcoded response "%s". Skipping.',
                regexp,
                response_name
            )

        if not correct_regexps:
            logger.error('Hardcoded response "%s" does not have any correct associated regexps', response_name)
            return None

        return correct_regexps

    @classmethod
    def _validate_responses(cls, response_name, config):
        responses = config.get('responses')

        responses_with_errors = []
        correct_responses = []

        for resp in responses:
            if not isinstance(resp, (basestring, dict)):
                responses_with_errors.append(resp)
                continue

            if isinstance(resp, dict):
                if not isinstance(resp.get('text'), basestring) or not isinstance(resp.get('voice'), basestring):
                    responses_with_errors.append(resp)
                    continue
            else:
                resp = {
                    'text': resp,
                    'voice': resp,
                }

            correct_responses.append(resp)

        for response in responses_with_errors:
            logger.error(
                'Wrong format of response "%s" for hardcoded response "%s". Skipping.',
                response,
                response_name
            )

        if not correct_responses:
            logger.error('Hardcoded response "%s" does not have any correct associated responses', response_name)
            return None

        return correct_responses

    @classmethod
    def _validate_links(cls, response_name, config):
        links = config.get('links', [])

        links_with_errors = []
        correct_links = []

        for link in links:
            if not isinstance(link, dict) or 'title' not in link or 'url' not in link:
                links_with_errors.append(link)
                continue

            if not isinstance(link['title'], basestring) or not isinstance(link['url'], basestring):
                links_with_errors.append(link)
                continue

            if not validators.url(link['url']):
                links_with_errors.append(link)
                continue

            correct_links.append(link)

        for link in links_with_errors:
            logger.error(
                'Wrong format of link "%s" for hardcoded response "%s". Skipping link.',
                link,
                response_name
            )

        return correct_links

    @classmethod
    def _validate_app_id(cls, response_name, config):
        app_id = config.get('app_id', '')
        if not isinstance(app_id, basestring):
            logger.error(
                'Wrong app_id type %r for hardcoded response "%s": string expected', type(app_id), response_name
            )
            return None
        return app_id

    @classmethod
    def validate_config(cls, config, raise_on_error=False):
        validated_config = {}

        for response_name, response_config in config.iteritems():
            regexps = cls._validate_regexps(response_name, response_config)
            responses = cls._validate_responses(response_name, response_config)
            links = cls._validate_links(response_name, response_config)
            app_id = cls._validate_app_id(response_name, response_config)

            if regexps is None or responses is None or app_id is None:
                if raise_on_error:
                    raise ValueError(
                        "Invalid config %s, %s."
                        " Is valid regexp: %s, responses: %s, app_id: %s " % (
                            response_name, response_config,
                            bool(regexps), bool(responses), bool(app_id)
                        )
                    )
                else:
                    continue

            validated_config[response_name] = {
                'regexps': regexps,
                'responses': responses,
                'links': links,
                'app_id': app_id
            }

        return validated_config

    def _load(self, path, samples_extractor):
        new_config = load_hardcoded(path)
        try:
            config = self.validate_config(new_config, raise_on_error=True)
        except Exception as e:
            logger.exception("Error while validating hardcoded response config: %s", e)
            raise
        else:
            classifier_config = {k: (v['regexps'] or [], v['app_id']) for k, v in config.iteritems()}
            intent_infos = {k: None for k in classifier_config.iterkeys()}
            classifier = AdHocHardcodedResponsesTokenClassifier(
                classifier_config,
                samples_extractor=samples_extractor,
                regexp=True,
                intent_infos=intent_infos
            )
            return config, classifier

    def get(self, sample):
        if self._classifier:
            features = SampleFeatures(sample)
            probs = self._classifier(features)
            for response_id, proba in probs.iteritems():
                if proba > 0:
                    responses = self._config[response_id]['responses']
                    random_response = random.choice(responses)
                    return random_response['text'], random_response['voice'], self._config[response_id]['links']

        return None
