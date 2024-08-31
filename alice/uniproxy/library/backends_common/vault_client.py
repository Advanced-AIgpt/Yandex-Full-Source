import getpass
import json
import os
import time

import tornado.httpclient
from tornado.ioloop import IOLoop

from alice.uniproxy.library.auth.tvm2 import tvm_client
from alice.uniproxy.library.settings import config
from alice.uniproxy.library.logging import Logger


VAULT_URL = config.get('vault', {}).get('url')


class VaultClient(object):
    def __init__(self, base_url=VAULT_URL):
        self._log = Logger.get('.backends.vaultclient')
        if base_url is None:
            raise Exception('vault server base url not configured')
        self.base_url = base_url

    async def get_secret(self, secret_uuid):
        """get secret metainfo
        """
        self.url = self.base_url + '/1/secrets/%s/' % secret_uuid
        r = await self._call_native_client('get', self.url, data=None)
        self._check_status(r)
        return r

    async def get_version(self, version):
        self.url = self.base_url + '/1/versions/%s/' % version
        r = await self._call_native_client('get', self.url, data=None)
        self._check_status(r)
        return r

    async def get_secret_last_version(self, secret_uuid):
        r = await self.get_secret(secret_uuid)
        last_v_time = 0
        last_v_value = None
        secret = r['secret']
        for v in secret['secret_versions']:
            if v['created_at'] > last_v_time:
                last_v_time = v['created_at']
                last_v_value = v['version']
        if last_v_value is None:
            raise Exception('not found last secret version (requested url={})'.format(self.url))

        return last_v_value

    async def get_secret_value(self, secret_uuid, name):
        last_version = await self.get_secret_last_version(secret_uuid)
        version = await self.get_version(last_version)
        version = version['version']
        for v in version['value']:
            if v['key'] == name:
                return v['value']

        raise Exception('not found given value inside last secret version')

    def _check_status(self, r):
        if r['status'] != 'ok':
            raise Exception('bad response status={} for request to url={}'.format(r['status'], self.url))

    async def _call_native_client(self, method, url, data):
        response = None
        path = url.rstrip('?') + '?'
        headers = {}

        if data is None:
            data = ''
        elif isinstance(data, dict):
            data = json.dumps(data)
            headers['content_type'] = 'application/json'

        for ctx in VaultClient._proceed_rsa_auth(
            method=method,
            path=path,
            data=data,
        ):
            try:
                response = await self._http_request(path, data=data, headers=ctx['headers'])
            except Exception as exc:
                self._log.exception('request to secrets vault failed')
                raise Exception('request to VaultClient failed: {}'.format(exc))
        return response

    @staticmethod
    def _proceed_rsa_auth(method, path, data):
        rsa_login = os.environ.get('SUDO_USER') or getpass.getuser()
        timestamp = str(int(time.time()))
        ctx = dict(headers={})
        for sign in tvm_client().make_signs(VaultClient._serialize_request(method, path, data, timestamp, rsa_login)):
            ctx['headers']['X-Ya-Rsa-Login'] = rsa_login
            ctx['headers']['X-Ya-Rsa-Timestamp'] = timestamp
            ctx['headers']['X-Ya-Rsa-Signature'] = sign
            yield ctx

    @staticmethod
    def _serialize_request(method, path, data, timestamp, login):
        r = '%s\n%s\n%s\n%s\n%s\n' % (method.upper(), path, data, timestamp, login)
        return r.encode('utf-8')

    async def _http_request(self, url, data, headers):
        default_timeout = 3.0
        request = tornado.httpclient.HTTPRequest(
            url,
            headers=headers,
            method='GET',
            connect_timeout=default_timeout,
            request_timeout=default_timeout,
        )
        self._log.debug('request vault_client url={}'.format(url))
        response = await tornado.httpclient.AsyncHTTPClient().fetch(request)
        if response.code != 200:
            raise Exception('bad response status={} (requested url={} body={})'.format(
                response.code,
                url,
                response.body.decoder('utf-8'),
            ))
        ticket_response = json.loads(response.body.decode('utf-8'))
        return ticket_response


if __name__ == "__main__":  # for debug purpose
    async def run():
        vault_client = VaultClient('https://vault-api.passport.yandex.net')
        # res = yield vault_client.get_secret('sec-01cte1dbs9xda5mayct7bgw5ny')
        # print('get secret meta-info', res)
        # last_version = yield vault_client.get_secret_last_version('sec-01cte1dbs9xda5mayct7bgw5ny')
        # print('get last secret version', last_version)
        # res = yield vault_client.get_version(last_version)
        # print('get secret version content', res)
        res = await vault_client.get_secret_value('sec-01cte1dbs9xda5mayct7bgw5ny', 'voicetech_vins_ydb_token')
        print('secret value:', res)

    IOLoop.current().run_sync(run)
