# coding: utf-8
import json
import logging
import re

import attr
from tornado import gen
from tornado.gen import Return
from tornado.httpclient import AsyncHTTPClient, HTTPError
from tornado.httputil import url_concat

from nlu_service import settings, named_entity
from nlu_service.services import unistat, normalizer
from nlu_service.services.wizard_parser import WizardParser
from nlu_service.timer import Timer

logger = logging.getLogger(__name__)

HTTP_CLIENT = AsyncHTTPClient()
WIZARD_PARSER = WizardParser()

RE_REMOVE_DIGITS = re.compile(r'\d')


@attr.s
class NerResult(object):
    entities = attr.ib()
    tokens = attr.ib()

    # optional attrs
    wizard_response_time = attr.ib(default=None)
    wizard_markup = attr.ib(default=None)
    normalized_utterance = attr.ib(default=None)


class WizardHttpError(Exception):
    pass


def write_wizard_response_metrics(status, time):
    if 200 <= status < 300:
        unistat.incr_counter(unistat.CounterName.wizard_requests_2xx)
    elif 300 <= status < 400:
        unistat.incr_counter(unistat.CounterName.wizard_requests_3xx)
    elif 400 <= status < 500:
        unistat.incr_counter(unistat.CounterName.wizard_requests_4xx)
    elif status == 599:
        unistat.incr_counter(unistat.CounterName.wizard_requests_timeout)
    else:
        unistat.incr_counter(unistat.CounterName.wizard_requests_5xx)
    unistat.write_hgram_value(unistat.HistogramName.wizard_response_time, time)


@gen.coroutine
def request_wizard(utterance, max_attempts=3):
    params = {
        'lr': 213,  # регион, сейчас прибита Москва
        'action': 'markup',
        'text': utterance.encode('utf-8'),
        'wizclient': 'paskills',
        'format': 'json',
        'markup': 'layers=Tokens,Morph,GeoAddr,Fio,Numbers,Date'
    }
    attempt = 1
    while attempt < max_attempts:
        timer = Timer()
        timer.start()
        try:
            response = yield HTTP_CLIENT.fetch(
                url_concat(settings.WIZARD_URL, params),
                request_timeout=settings.WIZARD_TIMEOUT,
            )
            write_wizard_response_metrics(response.code, timer.stop())
            raise Return(response)
        except HTTPError as exc:
            write_wizard_response_metrics(exc.code, timer.stop())
            if exc.code in {500, 502, 504, 599} and (attempt + 1) < max_attempts:
                unistat.incr_counter(unistat.CounterName.wizard_retry_count)
                logger.warn('Retrying request to wizard (attempt #%d)', attempt)
                attempt += 1
            else:
                raise


@gen.coroutine
def get_ner(utterance):
    if not utterance:
        raise Return(NerResult(
            entities=[],
            tokens=[],
            wizard_markup={},
        ))
    elif utterance == 'ping':
        # hotfix for skill healthchecks
        raise Return(NerResult(
            entities=[],
            tokens=['ping'],
            wizard_markup={},
        ))
    logger.info('Original utterance: %s', utterance)
    normalized_utterance = normalizer.reverse_normalize(utterance)
    logger.info('Normalized utterance: %s', normalized_utterance)
    with Timer() as timer:
        try:
            response = yield request_wizard(normalized_utterance)
        except Exception as e:
            unistat.incr_counter(unistat.CounterName.wizard_failed_unique)
            raise
    logger.info('Wizard total request time: %s', timer.time)
    wizard_markup = json.loads(response.body.decode('utf-8'))
    tokens = WIZARD_PARSER.extract_tokens(wizard_markup)

    entities = []
    for entity in WIZARD_PARSER.extract_entities(wizard_markup):
        if entity.is_valid():
            entities.append(entity)
            named_entity.incr_entity_unistat_counter(entity)
            unistat.incr_counter(unistat.CounterName.entities_validation_ok)
        else:
            logger.error('Found invalid entity %s', entity.to_dict())
            unistat.incr_counter(unistat.CounterName.entities_validation_error)

    entities.sort()
    raise Return(NerResult(
        entities=entities,
        tokens=tokens,
        wizard_markup=wizard_markup,
        wizard_response_time=timer.time,
        normalized_utterance=normalized_utterance,
    ))


