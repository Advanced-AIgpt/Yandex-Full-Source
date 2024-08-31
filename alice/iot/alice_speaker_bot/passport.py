# -*- coding: utf-8 -*-
from urllib.parse import urljoin

import requests


class YandexOauthClient:
    def __init__(self, url, client_id, client_secret):
        self.url = url
        self.client_id = client_id
        self.client_secret = client_secret

    def get_token(self, code):
        headers = {'Content-Type': 'application/x-www-form-urlencoded'}
        data = {'grant_type': 'authorization_code',
                'client_id': self.client_id,
                'client_secret': self.client_secret,
                'code': code}
        response = requests.post(url=urljoin(self.url, 'token'),
                                 headers=headers,
                                 data=data)

        if response.status_code != 200:
            print('bad response')
            print(response.status_code)
            print(response.text)
            return None

        response_json = response.json()
        token = response_json.get('access_token', None)
        if token is None:
            print('cannot get access_token')
            print(response.status_code)
            print(response.text)
            return None

        return token


class YandexPassportClient:
    def __init__(self, url):
        self.url = url

    def login(self, token):
        params = {'format': 'json'}
        headers = {'Authorization': 'OAuth {}'.format(token)}
        response = requests.post(url=urljoin(self.url, 'info'),
                                 headers=headers,
                                 params=params)

        if response.status_code != 200:
            print('bad response')
            print(response.status_code)
            print(response.text)
            return None

        return LoginInfo(response.json())


class LoginInfo:
    def __init__(self, login_info):
        self.login_info = login_info

    def get_user_id(self):
        return self.login_info.get('id')

    def get_name(self):
        name = self.login_info.get('real_name')
        if name is None or len(name) == 0:
            name = self.login_info.get('login')
        return name
