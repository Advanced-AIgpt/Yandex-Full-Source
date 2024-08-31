import asyncio
import json

from traitlets import Bool, Dict, Instance, Unicode
from traitlets.config import SingletonConfigurable

from jupytercloud.backend.lib.clients.http import AsyncHTTPClientMixin, JCHTTPError
from jupytercloud.backend.lib.clients.oauth import JupyterCloudOAuth
from jupytercloud.backend.lib.util.exc import JupyterCloudException


VMPROXY_URL_MAP = {
    'sas': 'https://vmproxy.sas-swat.yandex-team.ru/',
    'man': 'https://vmproxy.man-swat.yandex-team.ru/',
    'vla': 'https://vmproxy.vla-swat.yandex-team.ru/',
    'iva': 'https://vmproxy.iva-swat.yandex-team.ru/',
    'myt': 'https://vmproxy.myt-swat.yandex-team.ru/',
}


class QYPException(JupyterCloudException):
    pass


class UnknownClusterError(QYPException):
    pass


class QYPClient(SingletonConfigurable, AsyncHTTPClientMixin):
    """
    NB: QYPClient is not always Singleton!

    For legacy reasons sometimes it's re-initialized with other parameters.
    Still, as it's helpful to cache instances in other cases, it's still subclassed from Singleton

    This cursed behaviour is used in `qyp.vm.QYPVirtualMachine`.
    """

    oauth_token = Unicode(config=True)
    clusters = Dict(VMPROXY_URL_MAP, config=True)
    oauth = Instance(JupyterCloudOAuth, allow_none=True)
    use_user_token = Bool()

    vm_name_prefix = Unicode(config=True)
    vm_short_name_prefix = Unicode(config=True)

    _user_oauth_token = None

    @property
    def log_context(self):
        return {
            'use_user_token': self.use_user_token,
            'have_oauth_token': self.user_oauth_token is not None,
        }

    @property
    def user_oauth_token(self):
        # TODO: move logic about user oauth token and its invalidation to AsyncHTTPClientMixin
        # or special oauth mixin
        if self._user_oauth_token is None:
            if self.oauth is None:
                return None

            # it will return None if there are no token
            self._user_oauth_token = self.oauth.get_jupyter_cloud_token()

        return self._user_oauth_token

    def invalidate_user_token(self, obj):
        assert self.use_user_token
        if obj.code == 401 and self.user_oauth_token is not None:
            self.oauth.invalidate_token()
            self._user_oauth_token = None
            return True

        return False

    def get_headers(self):
        headers = super().get_headers()
        if self.use_user_token and self.user_oauth_token:
            headers['Authorization'] = f'OAuth {self.user_oauth_token}'

        return headers

    @staticmethod
    def get_vm_host(id_, cluster):
        return f'{id_}.{cluster}.yp-c.yandex.net'

    async def request(self, cluster, uri, **kwargs):
        if cluster not in self.clusters:
            raise UnknownClusterError(cluster)

        url = self.clusters[cluster] + uri.lstrip('/')

        try:
            result = await self._raw_request(url=url, **kwargs)
        except JCHTTPError as e:
            if self.use_user_token and self.invalidate_user_token(e):
                return await self._raw_request(url=url, **kwargs)

            raise

        # in case of kwargs['raise_error'] = False
        if self.use_user_token and self.invalidate_user_token(result):
            return await self._raw_request(url=url, **kwargs)

        return result

    async def request_all(self, uri, **kwargs):
        coroutines = []
        for cluster in self.clusters:
            coroutines.append(self.request(cluster, uri, **kwargs))

        return dict(
            zip(
                self.clusters,
                await asyncio.gather(*coroutines),
            ),
        )

    async def request_accounts(self, logins, request_timeout=40):
        coroutines = []

        for login in logins:
            coro = self.request_all(
                '/api/ListUserAccounts/',
                data={'login': login},
                method='POST',
                do_retries=False,
                request_timeout=request_timeout,
            )

            coroutines.append(coro)

        result = {}
        raw_logins_data = await asyncio.gather(*coroutines)
        for login, raw_clusters_data in zip(logins, raw_logins_data):
            result[login] = {
                cluster: json.loads(resp.body)
                for cluster, resp in raw_clusters_data.items()
            }

        return result

    async def get_vms_raw_info(self):
        coroutines = []

        prefixes = (self.vm_name_prefix, self.vm_short_name_prefix)

        for prefix in prefixes:
            coro = self.request_all(
                '/api/ListYpVm',
                method='POST',
                data={
                    'query': {
                        'name': prefix,
                    },
                },
            )

            coroutines.append(coro)

        result = []
        raw_data = await asyncio.gather(*coroutines)

        for cluster_responses in raw_data:
            for cluster, response in cluster_responses.items():
                cluster_data = json.loads(response.body)
                vms = cluster_data.get('vms', [])

                for vm in vms:
                    id_ = vm.get('meta', {}).get('id')
                    if not id_:
                        self.log.error('failed to get vm id from %s', vm)
                        continue

                    for prefix in prefixes:
                        if id_.startswith(prefix):
                            login = id_.partition(prefix)[2]
                            break
                    else:
                        # yp filters by substr, but we need startswith
                        continue

                    labels = vm.get('spec', {}).get('labels', {})

                    vm['cluster'] = cluster
                    vm['id'] = id_
                    vm['login'] = login
                    vm['labels'] = labels

                    result.append(vm)

        return result

    async def get_user_hosts(self):
        vms_info = await self.get_vms_raw_info()

        result = {}
        for vm in vms_info:
            host = self.get_vm_host(id_=vm['id'], cluster=vm['cluster'])
            login = vm['login']

            if login in result:
                self.log.error(
                    'login %s have duplicate vms: %s, %s',
                    login, host, result[login]
                )

            result[login] = host

        return result


def check_evacuating(labels):
    return (
        labels.get('qyp_evacuation') == '1' or
        labels.get('move_in_progress') == '1'
    )
