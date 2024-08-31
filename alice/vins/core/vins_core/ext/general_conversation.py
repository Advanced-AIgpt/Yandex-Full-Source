# coding: utf-8

from __future__ import unicode_literals

import logging
import json
import attr

from requests.exceptions import RequestException

from vins_core.utils.config import get_setting
from vins_core.ext.base import BaseHTTPAPI
from vins_core.utils.metrics import sensors


logger = logging.getLogger(__name__)


GC_API_URL_V1 = get_setting('GC_API_URL_V1', 'http://nikola2.search.yandex.net:3123/respond')
GC_API_URL_V2 = get_setting('GC_API_URL_V2', 'http://nikola2.search.yandex.net:3667/yandsearch')


def _get_url(url, api_version):
    if url:
        return url
    return GC_API_URL_V1 if api_version == 1 else GC_API_URL_V2


class GeneralConversationAPIError(Exception):
    pass


@attr.s
class GCResponse(object):
    text = attr.ib()
    docid = attr.ib()
    relevance = attr.ib()
    source = attr.ib()
    action = attr.ib()


class GeneralConversationAPI(object):
    _URL_CONFIG_PARAM = 'url'
    _API_VERSION_CONFIG_PARAM = 'api_version'
    _MODEL_CONFIG_PARAM = 'model'
    _RANKER_MODEL_CONFIG_PARAM = 'ranker_model'
    _TEMPERATURE_CONFIG_PARAM = 'temperature'
    _RANKING_MODE_CONFIG_PARAM = 'ranking_mode'
    _MAX_RESULTS_CONFIG_PARAM = 'max_results'
    _RETURN_REPLY_OR_CONTEXT_CONFIG_PARAM = 'return_reply_or_context'
    _MAX_RESPONSE_LENGTH_CONFIG_PARAM = 'max_response_len'
    _ADD_DEBUG_PREFIX_PARAM = 'add_debug_prefix'
    _SOURCE_KEY_CONFIG_PARAM = 'source_key'
    _ACTION_KEY_CONFIG_PARAM = 'action_key'

    _DEBUG_PREFIX = '[Говорилка] '
    GC_TIMEOUT = float(get_setting('GC_TIMEOUT', 1.5))
    GC_RETRIES = int(get_setting('GC_RETRIES', 3))

    def __init__(self, **kwargs):
        self._config = self._get_default_config()
        self._config.update(kwargs)
        self._http_session = BaseHTTPAPI(timeout=self.GC_TIMEOUT, max_retries=self.GC_RETRIES)
        self._validate_config(self._config)

    @sensors.with_timer('gc_response_time')
    def handle(self, prev_phrases, experiments):
        prev_phrases = [self._preprocess_phrase(p) for p in prev_phrases]

        url = _get_url(self._config[self._URL_CONFIG_PARAM], self._config[self._API_VERSION_CONFIG_PARAM])
        cgi_params = self._get_cgi_params(prev_phrases, experiments)

        try:
            r = self._http_session.get(url, params=cgi_params, request_label='gc')
            sensors.inc_counter('gc_response', labels={'status_code': r.status_code})
            r.raise_for_status()

        except RequestException as e:
            logger.error('GC communication error: %s.', e, exc_info=True)
            raise GeneralConversationAPIError

        r.encoding = 'utf-8'
        response = self._extract_response(r)

        if len(response) == 1:
            logger.debug(
                'General conversation response for "%s" is "%s" (docid=%s)' % (
                    ' | '.join(prev_phrases),
                    response[0].text,
                    response[0].docid
                )
            )

        results = [self._postprocess_one_response(resp) for resp in response]
        return results

    def _extract_response(self, response):
        if self._config[self._API_VERSION_CONFIG_PARAM] == 1:
            return [GCResponse(text=response.text, docid=None, relevance=None, source=None, action=None)]

        d = response.json()

        if 'Grouping' not in d:
            logger.error('Unexpected GC response: %s', json.dumps(d, ensure_ascii=False))
            raise GeneralConversationAPIError

        groups = d['Grouping'][0]['Group']
        results = []

        for g in groups:
            doc = g['Document'][0]
            relevance = doc['Relevance']
            docid = doc['DocId']
            doc_attrs = doc['ArchiveInfo']['GtaRelatedAttribute']
            text = None
            source = None
            action = None
            if ('FirstStageAttribute' in doc
                    and len(doc['FirstStageAttribute']) == 1
                    and doc['FirstStageAttribute'][0]['Key'] == '_Seq2SeqResult'
                    and doc['FirstStageAttribute'][0]['Value'] != ''):
                text = doc['FirstStageAttribute'][0]['Value']
                source = 'seq2seq'
            else:
                for doc_attr in doc_attrs:
                    if doc_attr['Key'] == self._config[self._RETURN_REPLY_OR_CONTEXT_CONFIG_PARAM]:
                        assert text is None
                        text = doc_attr['Value']
                    elif doc_attr['Key'] == self._config[self._SOURCE_KEY_CONFIG_PARAM]:
                        assert source is None
                        source = doc_attr['Value']
                    elif doc_attr['Key'] == self._config[self._ACTION_KEY_CONFIG_PARAM]:
                        assert action is None
                        action = doc_attr['Value']
            assert text is not None
            results.append(GCResponse(text, docid, relevance, source, action))

        results.sort(key=lambda r: -r.relevance)

        return results

    def _get_cgi_params(self, prev_phrases, experiments):
        prev_phrases_str = '\n'.join(prev_phrases)

        if self._config[self._API_VERSION_CONFIG_PARAM] == 1:
            return {
                'model': self._config[self._MODEL_CONFIG_PARAM],
                'num_samples': self._config[self._MAX_RESULTS_CONFIG_PARAM],
                'max_len': self._config[self._MAX_RESPONSE_LENGTH_CONFIG_PARAM],
                'temperature': self._config[self._TEMPERATURE_CONFIG_PARAM],
                'context': prev_phrases_str,
            }

        return {
            'g': '0..100',
            'ms': 'proto',
            'hr': 'json',
            'fsgta': '_Seq2SeqResult',
            'text': prev_phrases_str,
            'relev': 'MaxResults=%d;MinRatioWithBestResponse=%f;SearchBy=context;SearchFor=%s;DssmModelName=%s;RankerModelName=%s' % (  # noqa
                self._config[self._MAX_RESULTS_CONFIG_PARAM],
                self._config[self._TEMPERATURE_CONFIG_PARAM],
                self._config[self._RANKING_MODE_CONFIG_PARAM],
                self._config[self._MODEL_CONFIG_PARAM],
                self._config[self._RANKER_MODEL_CONFIG_PARAM],
            ),
            'pron': ["exp_%s" % (experiment,) for experiment, _ in experiments.items()
                     if experiment.startswith("gc_")]
        }

    def _preprocess_phrase(self, phrase):
        if self._config[self._ADD_DEBUG_PREFIX_PARAM] and phrase.startswith(self._DEBUG_PREFIX):
            phrase = phrase[len(self._DEBUG_PREFIX):]
        return phrase

    def _postprocess_one_response(self, response):
        tokens = response.text.split()

        # Remove _EOS_ in the end
        if len(tokens) > 0 and tokens[-1] == u'_EOS_':
            tokens = tokens[:-1]

        response.text = ' '.join(tokens)

        # Unescape quotes
        if self._config[self._API_VERSION_CONFIG_PARAM] == 2:
            response.text = response.text.replace('\\"', '"')
            response.text = response.text.replace('\\\'', '\'')

        # Add debug info to make it clear that the response came from the general conversation module
        if self._config[self._ADD_DEBUG_PREFIX_PARAM]:
            response.text = self._DEBUG_PREFIX + response.text

        return response

    @classmethod
    def _get_default_config(cls):
        return {
            cls._URL_CONFIG_PARAM: None,
            cls._API_VERSION_CONFIG_PARAM: 1,
            cls._MODEL_CONFIG_PARAM: 'dssm_tw_c2_filtered',
            cls._RANKER_MODEL_CONFIG_PARAM: '',
            cls._MAX_RESULTS_CONFIG_PARAM: 1,
            cls._TEMPERATURE_CONFIG_PARAM: 0.9,
            cls._RANKING_MODE_CONFIG_PARAM: 'reply',
            cls._RETURN_REPLY_OR_CONTEXT_CONFIG_PARAM: 'reply',
            cls._MAX_RESPONSE_LENGTH_CONFIG_PARAM: 30,
            cls._ADD_DEBUG_PREFIX_PARAM: False,
            cls._SOURCE_KEY_CONFIG_PARAM: 'source',
            cls._ACTION_KEY_CONFIG_PARAM: 'proactivity_action',
        }

    @classmethod
    def _validate_config(cls, cfg):
        assert cfg[cls._API_VERSION_CONFIG_PARAM] in [1, 2]
        assert cfg[cls._URL_CONFIG_PARAM] is None or isinstance(cfg[cls._URL_CONFIG_PARAM], basestring)
        assert cfg[cls._RANKING_MODE_CONFIG_PARAM] in ['reply', 'context', 'context_and_reply']
        assert cfg[cls._RETURN_REPLY_OR_CONTEXT_CONFIG_PARAM] in ['reply', 'context']
        assert isinstance(cfg[cls._MODEL_CONFIG_PARAM], basestring)
        assert isinstance(cfg[cls._RANKER_MODEL_CONFIG_PARAM], basestring)
        assert isinstance(cfg[cls._TEMPERATURE_CONFIG_PARAM], float)
        assert isinstance(cfg[cls._MAX_RESPONSE_LENGTH_CONFIG_PARAM], int)
        assert isinstance(cfg[cls._ADD_DEBUG_PREFIX_PARAM], bool)
        assert isinstance(cfg[cls._SOURCE_KEY_CONFIG_PARAM], basestring)
        assert isinstance(cfg[cls._ACTION_KEY_CONFIG_PARAM], basestring)

        if cfg[cls._API_VERSION_CONFIG_PARAM] == 1:
            assert cfg[cls._RANKING_MODE_CONFIG_PARAM] == 'reply', 'Context-dependent ranking is not supported in the old API'  # noqa
            assert cfg[cls._MAX_RESULTS_CONFIG_PARAM] == 1, 'Max-results must be 1 with old API.'
            assert cfg[cls._RANKER_MODEL_CONFIG_PARAM] == '', 'Ranker models are not supported in the old API.'


def gc_mock(response, source=None, action=None, context=None, url=None, api_version=1, additional_params='', seq2seq=False, seq2seq_feature=False):
    import requests_mock

    mock = requests_mock.Mocker()

    url = _get_url(url, api_version)

    if context is not None:
        assert isinstance(context, list)
        context_str = '\n'.join(context)
        context_param = 'context' if api_version == 1 else 'text'
        url = ('%s?%s=%s%s' % (url, context_param, context_str, additional_params)).encode('utf-8')

    if api_version == 1:
        assert isinstance(response, basestring), "GC API V1 only supports a single answer."
        mock.get(url, text=response)
        return mock

    if source is not None:
        assert type(source) is type(response)

    if isinstance(response, basestring):
        response = [response]
        if source is not None:
            source = [source]
        if action is not None:
            action = [action]

    if source is not None:
        assert len(source) == len(response)
    if action is not None:
        assert len(action) == len(response)

    data = {
        'Grouping': [
            {
                'Group': []
            }
        ]
    }

    for i in xrange(len(response)):
        data['Grouping'][0]['Group'].append(
            {
                'Document': [
                    {
                        'ArchiveInfo': {
                            'GtaRelatedAttribute': [
                                {
                                    'Key': 'reply',
                                    'Value': response[i],
                                }
                            ] if not seq2seq else []
                        },
                        'Relevance': 1000,
                        'DocId': str(i),
                        'FirstStageAttribute': [
                            {
                                'Key': '_Seq2SeqResult',
                                'Value': response[i] if seq2seq else ''
                            }
                        ] if seq2seq_feature else []
                    }
                ]
            }
        )
        if not seq2seq and source is not None:
            data['Grouping'][0]['Group'][-1]['Document'][0]['ArchiveInfo']['GtaRelatedAttribute'].append(
                {
                    'Key': 'source',
                    'Value': source[i],
                }
            )
        if action is not None:
            data['Grouping'][0]['Group'][-1]['Document'][0]['ArchiveInfo']['GtaRelatedAttribute'].append(
                {
                    'Key': 'proactivity_action',
                    'Value': action[i]
                }
            )

    mock.get(url, json=data)

    return mock


def gc_fail_mock(url=None, api_version=1):
    import requests_mock

    url = _get_url(url, api_version)

    mock = requests_mock.Mocker()
    mock.get(_get_url(url, api_version), status_code=500)
    return mock
