# coding: utf-8
import json
import logging

import attr
from tornado import gen
from tornado.web import RequestHandler
from jinja2 import Environment, PackageLoader, select_autoescape

import nlu_service
from nlu_service import settings
from nlu_service.services import (
    inflector_wrapper,
    wizard_client,
    unistat,
)


logger = logging.getLogger(__name__)


JINJA_ENV = Environment(
    loader=PackageLoader('nlu_service', 'templates'),
    autoescape=select_autoescape(['html'])
)
JINJA_ENV.policies['json.dumps_kwargs'] = {
    'ensure_ascii': False,
    'indent': 4,
}


class BaseHandler(RequestHandler):
    pass


class PingHandler(BaseHandler):
    def get(self):
        self.add_header('Content-Type', 'text/plain')
        self.write('pong')


class RootHandler(BaseHandler):
    def get(self):
        self.add_header('Content-Type', 'text/plain')
        self.write('NLU service\n')
        self.write('version={}'.format(nlu_service.__version__))


class UiHandler(BaseHandler):
    def __init__(self, *args, **kwargs):
        super(UiHandler, self).__init__(*args, **kwargs)
        self.template = JINJA_ENV.get_template('index.html')

    def get(self):
        self.write(self.template.render())

    @gen.coroutine
    def post(self):
        utterance = self.get_body_argument('utterance')
        nlu_result = yield wizard_client.get_ner(utterance)
        ner_response = {
            'nlu': {
                'tokens': nlu_result.tokens,
                'entities': [e.to_dict() for e in nlu_result.entities],
            }
        }
        self.write(self.template.render(
            show_results=True,
            utterance=utterance,
            nlu_result=nlu_result,
            ner_response=ner_response,
        ))


class NerApiHandler(BaseHandler):

    def write_analytics_info(self, skill_id, utterance, nlu_result):
        if settings.LOG_NER_ENTITIES:
            logger.info('NER result: %s', json.dumps({
                'skill_id': skill_id,
                'utterance': utterance,
                'nlu_result': attr.asdict(nlu_result),
            }, ensure_ascii=False))

    @gen.coroutine
    def process_utterance(self, skill_id, utterance):
        nlu_result = yield wizard_client.get_ner(utterance)
        self.write_analytics_info(skill_id, utterance, nlu_result)

        self.set_header('Content-Type', 'application/json; charset=utf-8')
        response = {
            'nlu': {
                'tokens': nlu_result.tokens,
                'entities': [e.to_dict() for e in nlu_result.entities],
            }
        }
        self.write(json.dumps(response, ensure_ascii=False))

    @gen.coroutine
    def get(self):
        utterance = self.get_argument('utterance')
        skill_id = self.get_argument('skill_id')
        yield self.process_utterance(skill_id, utterance)

    def write_error(self, status_code, **kwargs):
        unistat.incr_counter(unistat.CounterName.api_ner_errors)
        self.set_status(200)
        self.set_header('Content-Type', 'application/json; charset=utf-8')
        self.write(json.dumps({
            'nlu': {
                'tokens': [],
                'entities': [],
            }
        }))

    @gen.coroutine
    def post(self):
        unistat.incr_counter(unistat.CounterName.api_ner_requests_total)
        try:
            body = json.loads(self.request.body)
            utterance = body['utterance']
            skill_id = body.get('skill_id')
        except (ValueError, KeyError):
            self.set_status(400)
            return
        yield self.process_utterance(skill_id, utterance)


class InflectHandler(BaseHandler):
    def post(self):
        try:
            body = json.loads(self.request.body)
            activation_phrases = body['activation_phrases']
            skill_id = body['skill_id']
        except (ValueError, KeyError):
            self.set_status(400)
            return
        activation_phrases = list(filter(lambda phrase: len(phrase) > 0, activation_phrases))
        inflected = inflector_wrapper.inflect(skill_id, activation_phrases)
        self.write(json.dumps({
            'skill_id': skill_id,
            'inflected_activation_phrases': inflected,
        }))


class FaviconHandler(BaseHandler):

    def get(self):
        pass


class UnistatHandler(BaseHandler):

    def get(self):
        self.write(json.dumps(unistat.get_serialized_values()))
