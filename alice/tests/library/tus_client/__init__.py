from dataclass import dataclass
from os import path

import requests
from requests.packages.urllib3 import Retry


TUS_HOST = 'tus.yandex-team.ru'


@dataclass
class Account(object):
    uid: str
    login: str
    password: str
    firstname: str
    lastname: str
    language: str
    country: str
    locked_until: str


class TusClient(object):
    def __init__(
        self,
        env,
        tus_consumer,
        oauth_token,
        server=None,
        api_prefix='1',
        max_retries=3,
        ssl_verify=True,
    ):
        self._env = env
        self._tus_consumer = tus_consumer

        self.server = server or TUS_HOST
        self.url_prefix = self.server if self.server.startswith('http') else f'https://{self.server}'
        self.url_prefix = path.join(self.url_prefix, api_prefix)
        self._session = requests.Session()
        retries = Retry(total=max_retries, backoff_factor=1, status_forcelist=[500, 502, 503, 504])
        self._session.mount('https://', requests.adapters.HTTPAdapter(max_retries=retries))
        self._session.headers.update({
            'Authorization': f'OAuth {oauth_token}',
            'Content-Type': 'application/x-www-form-urlencoded',
        })
        self._session.verify = ssl_verify

    def _get_url(self, relative_url):
        return path.join(self.url_prefix, relative_url)

    @staticmethod
    def _pack_data(**kwargs):
        return {k: v for k, v in kwargs if v is not None}

    def create_account(self, tags=None, account=None):
        response = self._session.post(
            self._get_url('create_account/portal'),
            data=self._pack_data(
                env=self._env,
                tus_consumer=self._tus_consumer,
                tags=tags,
                login=account.login if account else None,
                password=account.password if account else None,
                firstname=account.firstname if account else None,
                lastname=account.lastname if account else None,
                language=account.language if account else None,
                country=account.country if account else None,
            ),
        )
        response.raise_for_status()
        return Account(**response.json()['account'])

    def get_account(self, account=None, tags=None, ignore_locks=None, lock_duration=None, with_saved_tags=None):
        response = self._session.get(
            self._get_url('get_account'),
            params=self._pack_data(
                env=self._env,
                tus_consumer=self._tus_consumer,
                uid=account.uid if account else None,
                login=account.login if account else None,
                tags=tags,
                ignore_locks=ignore_locks,
                lock_duration=lock_duration,
                with_saved_tags=with_saved_tags,
            ),
        )
        response.raise_for_status()
        return Account(**response.json()['account'])

    def unlock_account(self, account):
        response = self._session.post(
            self._get_url('unlock_account'),
            data=self._pack_data(
                env=self._env,
                tus_consumer=self._tus_consumer,
                uid=account.uid,
            ),
        )
        response.raise_for_status()
        assert response.json()['status'] == 'ok'

    def save_account(self, account, tags=None):
        response = self._session.post(
            self._get_url('save_account'),
            data=self._pack_data(
                env=self._env,
                tus_consumer=self._tus_consumer,
                tags=tags,
                uid=account.uid,
                login=account.login,
                password=account.password,
            ),
        )
        response.raise_for_status()
        return Account(**response.json()['account'])

    def remove_account(self, account):
        response = self._session.get(
            self._get_url('remove_account_from_tus'),
            params=self._pack_data(
                env=self._env,
                tus_consumer=self._tus_consumer,
                uid=account.uid,
            ),
        )
        response.raise_for_status()
        assert response.json()['status'] == 'ok'

    def bind_phone(self, account, phone_number=None):
        response = self._session.post(
            self._get_url('bind_phone'),
            data=self._pack_data(
                env=self._env,
                uid=account.uid,
                phone_number=phone_number,
            ),
        )
        response.raise_for_status()
        return response.json()['phone_number']
