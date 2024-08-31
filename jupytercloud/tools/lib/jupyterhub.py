# -*- coding: utf-8 -*-

from __future__ import absolute_import, print_function, division, unicode_literals

import json
import requests


class JupyterHubClient(object):
    def __init__(self, host, token):
        self._host = host
        self._token = token

    @property
    def base_url(self):
        return 'https://{}/hub/api'.format(self._host)

    def request(self, method, uri, json=None, **kwargs):
        url = '{}/{}'.format(self.base_url, uri.lstrip('/'))

        headers = kwargs.pop('headers', {})

        headers['Authorization'] = "token {}".format(self._token)

        response = requests.request(
            method=method,
            url=url,
            json=json,
            headers=headers,
            **kwargs
        )

        return response

    def _get_request(self, uri):
        response = self.request(
            'GET', uri
        )

        if response.status_code == 404:
            return None

        response.raise_for_status()

        return response.json()

    def get_user(self, user):
        return self._get_request(
            'users/{}'.format(user)
        )

    def get_server_info(self, user):
        user_info = self.get_user(user)

        if user_info is not None:
            return user_info.get('servers', {}).get('')

        return None

    def spawn_user(self, user, user_options):
        response = self.request(
            'POST', 'users/{}/server'.format(user),
            json=user_options,
        )

        response.raise_for_status()

        response = self.request(
            'GET',
            'users/{}/server/progress'.format(user),
            stream=True
        )

        response.raise_for_status()

        HEADER_LEN = len('data: ')

        for line in response.iter_lines():
            if line:
                data = line[HEADER_LEN:]
                yield json.loads(data)

    def get_servers(self):
        raw = self._get_request('users')

        ret = {}

        for user_info in raw:
            if user_info['kind'] != 'user':
                continue

            user = user_info['name']
            server = user_info.get('servers', {}).get('')
            if server is not None:
                ret[user] = server

        return ret
