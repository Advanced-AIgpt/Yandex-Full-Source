# coding: utf-8
from __future__ import unicode_literals

import json
import re
import logging
import random
from collections import OrderedDict

from vins_core.dm.request import Experiments
from vins_core.ext.s3 import S3Updater
from vins_core.ext.general_conversation import GeneralConversationAPI, GeneralConversationAPIError
from vins_core.utils.config import get_setting, get_bool_setting
from vins_core.utils.misc import get_short_hash

from personal_assistant import clients


logger = logging.getLogger(__name__)


GC_CONFIG = {
    'url': get_setting(
        'GC_API_URL',
        default='http://general-conversation.yandex.net:80/yandsearch'
    ),
    'api_version': 2,
    'model': get_setting('GC_MODEL', default='insight_c3_rus_lister'),
    'temperature': float(get_setting('GC_TEMPERATURE', default=0.85)),
    'ranking_mode': get_setting('GC_RANKING_MODE', default='context_and_reply'),
    'max_results': get_setting('GC_MAX_RESULTS', default=10),
}

GC_CONFIG_WITH_RERANKER = dict(GC_CONFIG, ranker_model='catboost')
GC_CONFIG_SUGGESTS = dict(GC_CONFIG, temperature=0.85)

_MAX_CONTEXT_LEN = 3
MAX_PREVIOUS_REPLIES = get_setting('MAX_PREVIOUS_REPLIES', default=15)

GENERAL_CONVERSATION_BANLIST_S3_KEY = 'pa_gc_banlist.json'


class GeneralConversation(object):
    def __init__(self, max_suggests=None, gc_force_question_top_k=None):
        self._gc_api = GeneralConversationAPI(**GC_CONFIG)
        self._gc_api_with_reranker = GeneralConversationAPI(**GC_CONFIG_WITH_RERANKER)
        self._gc_api_suggests = GeneralConversationAPI(**GC_CONFIG_SUGGESTS)
        self._banlist_regex = None
        self._max_suggests = int(max_suggests or get_setting('GC_MAX_SUGGESTS', 0))
        self._gc_force_question_top_k = int(gc_force_question_top_k or get_setting('GC_FORCE_QUESTION_TOP_K', 0))

        update_interval = get_setting('GC_BANLIST_UPDATE_INTERVAL', default=150)
        self._banlist_updater = S3Updater(
            GENERAL_CONVERSATION_BANLIST_S3_KEY, interval=update_interval, on_update_func=self._update_banlist)

    def get_context(self, session, sample):
        prev_phrases = []
        last_sender = None
        for ph in session.dialog_history.last_phrases(session.dialog_history.MAX_TURNS):
            if prev_phrases and last_sender == ph.sender:
                prev_phrases[-1] = '{0}. {1}'.format(prev_phrases[-1], ph.text)
            else:
                prev_phrases.append(ph.text)
            last_sender = ph.sender
        if last_sender == 'user':
            prev_phrases[-1] = '{0}. {1}'.format(prev_phrases[-1], sample.text)
        else:
            prev_phrases.append(sample.text)
        prev_phrases = prev_phrases[-_MAX_CONTEXT_LEN:]
        return prev_phrases

    def _get_gc_api(self, req_info):
        if req_info.experiments['disable_gc_reranker'] is not None or get_bool_setting('DISABLE_GC_RERANKER'):
            return self._gc_api
        else:
            return self._gc_api_with_reranker

    def _get_patched_experiments(self, req_info, is_pure_gc=False, is_suggests=False):
        skip_proactivity = False
        if is_suggests:
            skip_proactivity = True
        if is_pure_gc:
            skip_proactivity = True
        if not clients.is_smart_speaker(req_info.app_info) and req_info.experiments['gc_proactivity_for_all'] is None:
            skip_proactivity = True

        if skip_proactivity and req_info.experiments['gc_proactivity'] is not None:
            return Experiments({k: v for k, v in req_info.experiments.items() if k != 'gc_proactivity'})

        return req_info.experiments

    def _update_banlist(self, file_path):
        try:
            with open(file_path) as f:
                patterns = json.load(f)
                regex_pattern = '^%s$' % '|'.join(map(lambda p: '(?:%s)' % p, patterns))
                self._banlist_regex = re.compile(regex_pattern, flags=re.U | re.IGNORECASE)
        except re.error:
            logger.error('Invalid GC banlist regular expression: "%s"', regex_pattern, exc_info=True)
        except Exception:
            logger.exception('GC banlist update failed')

    def _filter_responses(self, responses, sample_text):
        self._banlist_updater.update()

        if self._banlist_regex is None:
            return responses

        res = []
        for resp in responses:
            if re.match(self._banlist_regex, resp.text) is not None:
                logger.warning('All GC responses to "%s" have been banned.', sample_text)
            else:
                res.append(resp)

        return res

    def _get_responses(self, api, req_info, context, sample_text, is_pure_gc, is_suggests):
        try:
            filtered = self._filter_responses(
                api.handle(context, self._get_patched_experiments(req_info, is_pure_gc, is_suggests)),
                sample_text
            )
            if not filtered:
                logger.warning('All GC responses to "%s" have been banned.', sample_text)
                return None
            return filtered

        except GeneralConversationAPIError:
            return None

    def _random_gc_response(self, responses, context, req_info):
        if len(responses) == 0:
            return None

        if req_info.experiments['gc_first_reply'] is not None:
            return responses[0]

        response = random.choice(responses)

        return response

    def _select_gc_response(self, responses, context, req_info, used_replies):
        if responses is None:
            return None
        if clients.is_smart_speaker_without_screen(req_info):
            def is_video_proactivity(response):
                return response.source == 'proactivity' and response.action == 'personal_assistant.scenarios.video_general_scenario'

            responses = [response for response in responses if not is_video_proactivity(response)]
        if req_info.experiments['gc_force_question'] is not None:
            logger.debug('Forcing question from top-%d.', self._gc_force_question_top_k)
            for r in responses[:self._gc_force_question_top_k]:
                if '?' in r.text:
                    return r
        if req_info.experiments['gc_force_proactivity'] is not None:
            logger.debug('Forcing proactivity.')
            for r in responses:
                if r.source == 'proactivity':
                    return r

        filtered_responses = [response for response in responses if get_short_hash(response.text) not in used_replies]
        if filtered_responses:
            responses = filtered_responses

        if req_info.experiments['gc_force_proactivity_soft'] is not None:
            for r in responses:
                if r.source == 'proactivity':
                    responses = [r]
                    break

        response = self._random_gc_response(responses, context, req_info)

        if response:
            used_replies.append(get_short_hash(response.text))
            del used_replies[:-MAX_PREVIOUS_REPLIES]
        return response

    def get_response(self, req_info, session, sample):
        context = self.get_context(session, sample)
        responses = self._get_responses(
            self._get_gc_api(req_info),
            req_info,
            context,
            sample.text,
            is_pure_gc=True if session.get('pure_general_conversation') else False,
            is_suggests=False
        )

        if not session.has('used_gc_replies') or session.get('used_gc_replies') is None:
            session.set('used_gc_replies', [], persistent=False)
        used_replies = session.get('used_gc_replies', [])
        return self._select_gc_response(responses, context, req_info, used_replies)

    def get_response_with_suggests(self, req_info, session, sample):
        context = self.get_context(session, sample)
        gc_response = self.get_response(req_info, session, sample)

        if gc_response is None:
            return None, []

        context.append(gc_response.text)

        responses = self._get_responses(
            self._gc_api_suggests,
            req_info,
            context,
            gc_response.text,
            is_pure_gc=True if session.get('pure_general_conversation') else False,
            is_suggests=True) or []

        # Filter out phrases which are a bit too long
        phrases = [resp.text for resp in responses if len(resp.text) <= 100]

        def preprocess(phrase):
            phrase = ''.join(re.findall(r'[\w\s]', phrase, flags=re.UNICODE))
            phrase = re.sub(r'\s+', ' ', phrase, flags=re.UNICODE)
            phrase = phrase.strip().lower()
            return phrase

        suggests = OrderedDict(
            (preprocess(ph), ph) for ph in phrases
        ).values()[:self._max_suggests]

        return gc_response, suggests
