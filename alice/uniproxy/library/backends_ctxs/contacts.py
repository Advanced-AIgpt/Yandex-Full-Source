import json
import logging

import tornado.gen

from alice.uniproxy.library.settings import config
from alice.uniproxy.library.async_http_client import QueuedHTTPClient as AsyncHTTPClient, HTTPError
from alice.uniproxy.library.async_http_client import RTLogHTTPRequest
from alice.uniproxy.library.global_counter import GlobalCounter, GlobalTimings
from alice.uniproxy.library.global_counter.uniproxy import CONTACTS_REQUEST_HGRAM
from rtlog import null_logger
from alice.uniproxy.library.auth.tvm2 import service_ticket_for_contacts


class Contacts:
    ACTIONS = [
        'вызвать',
        'вызов',
        'вызови',
        'дозвонись',
        'дозвонитесь',
        'дозвониться',
        'звони',
        'звонить',
        'звонок',
        'звякни',
        'звякнуть',
        'набери',
        'набрать',
        'позвони',
        'позвонить',
        'свяжись',
        'сделай звонок',
        'сделать звонок',
        'соедини',
        'телефонная книга',

        'алиса вызвать',
        'алиса вызов',
        'алиса вызови',
        'алиса дозвонись',
        'алиса дозвонитесь',
        'алиса дозвониться',
        'алиса звони',
        'алиса звонить',
        'алиса звонок',
        'алиса звякни',
        'алиса звякнуть',
        'алиса набери',
        'алиса набрать',
        'алиса позвони',
        'алиса позвонить',
        'алиса свяжись',
        'алиса сделай звонок',
        'алиса сделать звонок',
        'алиса соедини',
        'алиса телефонная книга',
    ]
    ID = 'contacts'

    def __init__(self):
        self._number = None
        self._values = None

    def __bool__(self):
        return bool(self._values)

    @property
    def number(self):
        return self._number if self._number is not None else -1

    @property
    def values(self):
        return self._values

    def is_empty(self):
        return self._values == []

    def is_undefined(self):
        return self._values is None

    def set_empty(self):
        self._number = 0
        self._values = []

    def set_contacts(self, contacts):
        self._number = len(contacts)
        self._values = sorted(set(contacts))

    def get_values(self, indices):
        return [self.values[i] for i in indices]

    def make_context(self):
        return {
            'id': self.ID,
            'trigger': self.ACTIONS,
            'content': self._values,
        }


# for mocking
def _get_client(url):
    return AsyncHTTPClient.get_client_by_url(url)


@tornado.gen.coroutine
def get_contacts(device_id, uid, rt_log=null_logger()):
    if not device_id or not uid:
        return None
    settings = config['contacts']
    ticket = yield service_ticket_for_contacts(rt_log)
    request = RTLogHTTPRequest(
        settings['url'],
        method='POST',
        body=json.dumps({
            'method': 'list_contacts_alice',
            'params': {
                'uid': int(uid),
            },
        }).encode(),
        headers={
            'Accept': 'application/json',
            'Content-Type': 'application/json',
            'X-Ya-Service-Ticket': ticket,
            'X-Device-ID': device_id,
        },
        request_timeout=settings['request_timeout'],
        retries=settings['retries'],
        rt_log=rt_log,
        rt_log_label='get_contacts',
    )
    try:
        response = yield _get_client(settings['url']).fetch(request)
        GlobalCounter.CONTACTS_REQUEST_OK_SUMM.increment()
        GlobalTimings.store(CONTACTS_REQUEST_HGRAM, response.response_time())
        resp_text = response.text()
        GlobalCounter.CONTACT_LIST_SIZE_BYTES_HGRAM.set(len(resp_text))
        return [c.get('display_name', '') for c in json.loads(resp_text)['data']['contacts']]
    except Exception as exc:
        if isinstance(exc, HTTPError):
            if exc.code == 401:
                GlobalCounter.CONTACTS_REQUEST_UNAUTHORIZED_SUMM.increment()
                return []
            elif exc.code == 403:
                GlobalCounter.CONTACTS_REQUEST_FORBIDDEN_SUMM.increment()
                return []
        else:
            logging.getLogger('uniproxy.backends.contacts').warning('contacts error: {}'.format(response.json()))
        GlobalCounter.CONTACTS_REQUEST_FAIL_SUMM.increment()
        raise
