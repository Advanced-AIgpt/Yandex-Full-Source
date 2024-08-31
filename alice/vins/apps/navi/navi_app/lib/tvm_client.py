# coding: utf-8
from __future__ import unicode_literals

import logging
import time

import ticket_parser2
import ticket_parser2.low_level as tp2

from requests import RequestException
from threading import Event, Thread

from vins_core.ext.base import BaseHTTPAPI

logger = logging.getLogger(__name__)


class TvmClientError(Exception):
    pass


class _TvmClientUpdater(Thread):
    def __init__(self, event, callback, period):
        Thread.__init__(self)
        self.daemon = True
        self.stopped = event
        self.period = period
        self.callback = callback
        self.start()

    def run(self):
        while not self.stopped.wait(self.period):
            self.callback()


class TvmClient(object):
    def get_tvm_ticket(self, alias):
        return None

    def get_headers(self, alias):
        return {}

    def refresh_keys(self):
        pass

    def refresh_tickets(self):
        pass


class DisabledTvmClient(TvmClient):
    def get_headers(self, alias):
        return {
            'Accept': 'application/x-protobuf',
        }


class TvmClientWrapper(TvmClient, BaseHTTPAPI):
    def __init__(self, client_id, secret, destinations, **kwargs):
        self._tickets = {}
        self._client_id = client_id
        self._destinations = destinations

        if 'max_retries' not in kwargs:
            kwargs['max_retries'] = 3
        super(TvmClientWrapper, self).__init__(**kwargs)

        self._service_context = tp2.ServiceContext(client_id, secret, self._get_tvm_keys())
        self.refresh_tickets()

    def get_tvm_ticket(self, alias):
        if alias in self._tickets:
            return self._tickets[alias]
        else:
            raise TvmClientError('Not found ticket for %s' % alias)

    def get_headers(self, alias):
        return {
            'Accept': 'application/x-protobuf',
            'X-Ya-Service-Ticket': self.get_tvm_ticket(alias)
        }

    def refresh_keys(self):
        self._service_context.reset_keys(self._get_tvm_keys())

    def refresh_tickets(self):
        ts = str(int(time.time()))
        dst = ','.join(str(x) for x in self._destinations.values())

        try:
            response = self.post(
                url='https://tvm-api.yandex.net/2/ticket/',
                data={
                    'grant_type': 'client_credentials',
                    'src': self._client_id,
                    'dst': dst,
                    'ts': ts,
                    'sign': self._service_context.sign(ts, dst, '')
                },
                request_label='tvm_refresh_tickets',
            )
        except RequestException as e:
            raise TvmClientError(e)
        if response.status_code != 200:
            raise TvmClientError('%s %s' % (response.status_code, response.reason))

        tickets = response.json()

        for k, v in self._destinations.iteritems():
            ticket = tickets[str(v)]
            if 'ticket' in ticket:
                self._tickets[k] = tickets[str(v)]['ticket']
            else:
                self._tickets[k] = None
                logger.error("Unexpected error in update refresh tickets")

    def _get_tvm_keys(self):
        try:
            response = self.get(
                url='https://tvm-api.yandex.net/2/keys',
                params={
                    'lib_version': ticket_parser2.__version__,
                },
                request_label='tvm_refresh_keys',
            )
        except RequestException as e:
            raise TvmClientError(e)
        if response.status_code != 200:
            raise TvmClientError('%s %s' % (response.status_code, response.reason))

        return response.content.decode()


class TvmClientFactory(object):
    DISABLED_TVM_CLIENT = DisabledTvmClient()

    def __init__(self):
        self._clients = {}
        self._stop = Event()
        self._keys_refresher = _TvmClientUpdater(self._stop, self.refresh_keys, 24.0 * 3600.0)  # every 24 hour
        self._tickets_refresher = _TvmClientUpdater(self._stop, self.refresh_tickets, 3600.0)  # every 1 hour

    def stop(self):
        self._stop.set()

    def add_tvm_client(self, name, client_id, secret, destinations):
        if not secret:
            logger.warning('Secret for TVM is None')
            return

        self._clients[name] = TvmClientWrapper(client_id, secret, destinations)

    def get_tvm_ticket(self, client, alias):
        return self._clients.get(client, TvmClientFactory.DISABLED_TVM_CLIENT).get_tvm_ticket(alias)

    def get_headers(self, client, alias):
        return self._clients.get(client, TvmClientFactory.DISABLED_TVM_CLIENT).get_headers(alias)

    def refresh_keys(self):
        for client in self._clients.values():
            client.refresh_keys()

    def refresh_tickets(self):
        for client in self._clients.values():
            client.refresh_tickets()


tvm_client_factory = TvmClientFactory()
