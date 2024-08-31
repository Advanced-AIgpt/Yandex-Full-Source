from jupyterhub.utils import token_authenticated
from library.python.vault_client.errors import ClientError
from tornado import web

from jupytercloud.backend.lib.clients.vault import VaultClient

from .base import JCAPIHandler


UUID_RE = r'(?P<uuid>[^/]+)'
TOKEN_RE = r'(?P<token>[^/]+)'


class VaultDelegationTokenHandler(JCAPIHandler):
    async def post(self):
        uuid = self.get_json_body().get('uuid')
        auth_data = await self.authenticator.check_auth(self)

        auth_state = auth_data.get('auth_state')
        user_ticket = auth_state.get('user_ticket')
        assert user_ticket is not None

        username = auth_data['name']

        vault = VaultClient(user_ticket=user_ticket, decode_files=True)

        try:
            response = await vault._call_method(
                vault.client.create_token, uuid, signature=username, tvm_client_id=vault.tvm_client.self_id,
            )
            result = {'token': response[0], 'tid': response[1]}
        except ClientError as e:
            raise web.HTTPError(400, e.kwargs.get('message'))

        self.write(result)


class VaultSecretHandler(JCAPIHandler):
    """This handler should be POST as it receives sensitive data (tokens) as input"""

    def check_xsrf_cookie(self) -> None:
        return None  # Accept requests without cookies

    @token_authenticated
    async def post(self):
        token = self.get_json_body().get('token')
        username = self.get_current_user_token().name

        vault = VaultClient(config=self.settings['config'])
        vault_response = (
            await vault._call_method(
                vault.client.send_tokenized_requests,
                [
                    {
                        'token': token,
                        'signature': username,
                        'service_ticket': vault.tvm_client.get_service_ticket(dst_alias='vault'),
                    },
                ],
                consumer='jupytercloud',
            )
        )[0]

        if vault_response['status'] != 'ok':
            raise web.HTTPError(400, vault_response.get('message'))

        result = {x['key']: x for x in vault_response['value']}
        self.write(result)


default_handlers = [
    ('/api/vault/token', VaultDelegationTokenHandler),
    ('/api/vault/secret', VaultSecretHandler),
]
