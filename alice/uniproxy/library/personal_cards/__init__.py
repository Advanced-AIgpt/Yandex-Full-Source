import tornado.gen
import json
import time
from http import HTTPStatus
from enum import Enum, unique

from alice.uniproxy.library.async_http_client import QueuedHTTPClient, RTLogHTTPRequest, HTTPError
from alice.uniproxy.library.global_counter import GlobalCounter, GlobalTimings
from alice.uniproxy.library.global_counter import (
    PERSONAL_CARDS_REQUEST_ADD_HGRAM,
    PERSONAL_CARDS_REQUEST_DISMISS_HGRAM,
    PERSONAL_CARDS_REQUEST_GET_HGRAM
)
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.settings import config


@unique
class PersonalCardsHandler(Enum):
    AddPushCards = '/addPushCards'
    Dismiss = '/dismiss'
    Get = '/cards'


def _get_client(url):
    return QueuedHTTPClient.get_client_by_url(url, pool_size=2, queue_size=20)


class PersonalCardsHelper:
    def __init__(self, puid, rt_log, host=None, port=None):
        self.puid = puid
        self.rt_log = rt_log
        self._log = Logger.get('.personalcards')
        self.card_types = ['yandex.station_film']
        self.host = host or config['personal_cards']['url']
        self.port = port or config['personal_cards']['port']

        self.ok_counters = {
            PersonalCardsHandler.AddPushCards: GlobalCounter.PERSONAL_CARDS_REQUEST_ADD_OK_SUMM,
            PersonalCardsHandler.Dismiss: GlobalCounter.PERSONAL_CARDS_REQUEST_DISMISS_OK_SUMM,
            PersonalCardsHandler.Get: GlobalCounter.PERSONAL_CARDS_REQUEST_GET_OK_SUMM
        }
        self.fail_counters = {
            PersonalCardsHandler.AddPushCards: GlobalCounter.PERSONAL_CARDS_REQUEST_ADD_FAIL_SUMM,
            PersonalCardsHandler.Dismiss: GlobalCounter.PERSONAL_CARDS_REQUEST_DISMISS_FAIL_SUMM,
            PersonalCardsHandler.Get: GlobalCounter.PERSONAL_CARDS_REQUEST_GET_FAIL_SUMM,
        }
        self.hgrams = {
            PersonalCardsHandler.AddPushCards: PERSONAL_CARDS_REQUEST_ADD_HGRAM,
            PersonalCardsHandler.Dismiss: PERSONAL_CARDS_REQUEST_DISMISS_HGRAM,
            PersonalCardsHandler.Get: PERSONAL_CARDS_REQUEST_GET_HGRAM,
        }

    @tornado.gen.coroutine
    def _request_personal_cards(self, handler, data):
        url = f'{self.host}:{self.port}{handler.value}'
        jdata = json.dumps(data)
        request = RTLogHTTPRequest(
            url,
            request_timeout=config['personal_cards']['request_timeout'],
            method='POST',
            body=jdata.encode('utf-8'),
            rt_log=self.rt_log,
            rt_log_label='personal_cards'
        )

        self.DLOG(jdata)

        s_ts = time.time()
        response = None
        try:
            response = yield _get_client(url).fetch(request)
            self.ok_counters[handler].increment()

            result = response.code
        except Exception as err:
            self.fail_counters[handler].increment()

            if isinstance(err, HTTPError):
                msg = err.text() or ''
                self.ERR('request to personal cards failed: {} {}'.format(err, msg))
            else:
                self.ERR('request to personal cards failed: {}'.format(err))

            result = None

        GlobalTimings.store(self.hgrams[handler], time.time() - s_ts)
        return result, response

    @tornado.gen.coroutine
    def send_personal_card(self, payload):
        card = payload.get('card', {})
        add_card_data = {
            'card': {
                'card': {
                    'card_id': card.get('card_id'),
                    'data': {
                        'action_url': card.get('action_url'),
                        'action_text': card.get('action_text'),
                        'image_url': card.get('image_url'),
                        'button_url': card.get('button_url'),
                        'text': card.get('text')
                    },
                    'date_from': card.get('date_from'),
                    'date_to': card.get('date_to')
                }
            },
            'uid': self.puid
        }

        if 'tag' in card:
            add_card_data['card']['card']['tag'] = card['tag']

        card_type_present = any(card_type in card for card_type in self.card_types)
        if not card_type_present and 'tag' not in card:
            self.ERR('None of the card types present')
            return False
        for card_type in self.card_types:
            if card_type in card:
                add_card_data['card']['card']['type'] = card_type
                if isinstance(card[card_type], dict):
                    add_card_data['card']['card']['data'].update(card[card_type])

        if 'type' not in add_card_data['card']['card']:
            add_card_data['card']['card']['type'] = 'alice.general'

        if payload.get('remove_existing_cards'):
            ok = yield self.dismiss_cards()
            if not ok:
                return False

        code, _ = yield self._request_personal_cards(PersonalCardsHandler.AddPushCards, add_card_data)
        return code == HTTPStatus.OK

    @tornado.gen.coroutine
    def dismiss_cards(self, card_id='*'):
        dismiss_data = {
            'auth': {
                'uid': self.puid,
            },
            'card_id': card_id,
        }
        code, _ = yield self._request_personal_cards(PersonalCardsHandler.Dismiss, dismiss_data)
        return code == HTTPStatus.OK

    @tornado.gen.coroutine
    def get_cards(self):
        get_data = {
            'auth': {
                'uid': self.puid
            },
            'client_id': 'search_app',
        }
        code, response = yield self._request_personal_cards(PersonalCardsHandler.Get, get_data)
        return code == HTTPStatus.OK, response.json().get('blocks', [])

    @tornado.gen.coroutine
    def remove_cards_by_tag(self, tag):
        ok, cards = yield self.get_cards()
        if not ok:
            return False

        futures = []
        for c in cards:
            if c.get('tag') == tag:
                futures.append(self.dismiss_cards(c.get('card_id')))

        if futures:
            yield futures

        return True

    def DLOG(self, *args):
        try:
            self._log.debug('PersonalCardsHelper: {}'.format(args), rt_log=self.rt_log)
        except ReferenceError:
            pass

    def ERR(self, *args):
        try:
            self._log.error('PersonalCardsHelper: {}'.format(args), rt_log=self.rt_log)
        except ReferenceError:
            pass
