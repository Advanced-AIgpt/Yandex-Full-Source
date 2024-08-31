import functools
import json
from enum import Enum
from itertools import count

from jupyterhub.utils import url_path_join
from tornado import httpclient, web
from tornado.log import app_log

from jupytercloud.backend.lib.clients.tvm import TVMError
from jupytercloud.backend.lib.db import orm
from jupytercloud.backend.lib.idm import ROLE_TREE


ADMIN_ROLE_PATH = '/role/admin/'
ADMIN_ROLE_SPEC = {'role': 'admin'}
QUOTA_ROLE_PREFIX = '/role/quota/'


def decode(bytestr, strip=True):
    if bytestr is None:
        res = ''
    else:
        res = bytestr.decode('utf-8', errors='replace')
        if strip:
            res.rstrip()
    return res


def tvm_verified(method):
    # This supports multiple methods of verification
    @functools.wraps(method)
    def wrapper(self, *args, **kwargs):
        errors = []
        verification_steps = 0

        if self.tvm_client_verify:
            try:
                verification_steps += 1
                self.tvm_client.verify_request(self.request, self.tvm_whitelist)
            except TVMError as e:
                errors.append(e)

        if verification_steps == 0:
            app_log.warning('No client verification enabled!')
        elif len(errors) == verification_steps:
            raise web.HTTPError(
                403,
                'All client verification failed: {}'.format(
                    [str(e) for e in errors],
                ),
            ) from Exception(errors)
        elif errors:
            app_log.warning(
                'Some client verification failed: %s', [
                    str(e) for e in errors
                ],
            )

        return method(self, *args, **kwargs)
    return wrapper


class IDMResponseKind(Enum):
    success = 'success'
    warning = 'warning'
    error = 'error'
    fatal = 'fatal'


IDM_RESPONSES = {}
_IDM_RESPONSE_CODES_COUNTER = count(1)


def idm_response(cls):
    if cls.kind == IDMResponseKind.success:
        code = 0
    else:
        code = next(_IDM_RESPONSE_CODES_COUNTER)
        IDM_RESPONSES[code] = cls
    cls.code = code
    return cls


class IDMResponse(dict):
    code = 0
    kind = IDMResponseKind.success

    def __init__(self, message=None, **kwargs):
        kwargs['code'] = self.code
        if self.kind != IDMResponseKind.success:
            kwargs[self.kind.value] = message

        kwargs = {k: v for k, v in kwargs.items() if v is not None}
        super().__init__(**kwargs)


class IDMResponses:
    @idm_response
    class Success(IDMResponse):
        pass

    @idm_response
    class HubAPIError(IDMResponse):
        kind = IDMResponseKind.error

    @idm_response
    class BadRoleRequest(IDMResponse):
        kind = IDMResponseKind.fatal

    @idm_response
    class WrongWorkflow(IDMResponse):
        kind = IDMResponseKind.fatal

    @idm_response
    class RoleMismatch(IDMResponse):
        kind = IDMResponseKind.fatal

    @idm_response
    class QloudAPIError(IDMResponse):
        kind = IDMResponseKind.error

    @idm_response
    class QuotaExceeded(IDMResponse):
        kind = IDMResponseKind.fatal


class IDMRequestHandler(web.RequestHandler):
    http_client = None
    hub_api_headers = None

    @property
    def db(self):
        return self.settings['db']

    @property
    def tvm_client(self):
        return self.settings['tvm_client']

    @property
    def log(self):
        return self.settings.get('log', app_log)

    @property
    def config(self):
        return self.settings.get('config', None)

    @property
    def jupyterhub_api_url(self):
        return self.settings['jupyterhub_api_url']

    @property
    def tvm_client_verify(self):
        return self.settings['tvm_client_verify']

    @property
    def tvm_whitelist(self):
        return self.settings['tvm_whitelist']

    def initialize(self):
        self.hub_api_headers = {
            'Authorization': 'token ' + self.settings['jupyterhub_api_token'],
            'Content-Type': 'application/json',
        }
        self.http_client = httpclient.AsyncHTTPClient()

    async def jupyterhub_request(self, uri, data=None, **kwargs):
        url = f'{self.jupyterhub_api_url}/hub/api/{uri.lstrip("/")}'
        if data:
            kwargs['body'] = json.dumps(data)

        return await self.http_client.fetch(
            url,
            headers=self.hub_api_headers,
            raise_error=False,
            **kwargs,
        )

    async def get_hub_user(self, login):
        resp = await self.jupyterhub_request(f'/users/{login}')

        if resp.code == 200:
            return json.loads(decode(resp.body))

        if resp.code == 404:
            return None

        msg = (
            f'Cannot get user {login} information via JupyterHub API: '
            f'{resp.code} {decode(resp.body)}'
        )
        self.log.error(msg)
        self.write(IDMResponses.HubAPIError(msg))

        raise web.Finish

    def finish(self, *args, **kwargs):
        """Roll back any uncommitted transactions from the handler."""
        self.db.clean_dirty()
        super().finish(*args, **kwargs)


class InfoHandler(IDMRequestHandler):
    async def get(self):
        self.write(IDMResponses.Success(roles=ROLE_TREE))


class GetAllRolesHandler(IDMRequestHandler):
    async def get(self):
        users = []
        for user in self.db.query(orm.User).all():
            idm_roles = []
            for path, idm_role in user.idm_state.items():
                role_spec = idm_role.get('role')
                if not role_spec:
                    self.log.warning(
                        f'user {user.login} have bad idm role {idm_role} at path {path}',
                    )
                    continue

                role_fields = idm_role.get('fields')
                if role_fields is None:
                    idm_roles.append(role_spec)
                else:
                    idm_roles.append([role_spec, role_fields])

            users.append({'login': user.name, 'roles': idm_roles})

        self.write(IDMResponses.Success(users=users))


def idm_role_handler(method):
    @functools.wraps(method)
    def wrapper(self):
        resp = IDMResponses.BadRoleRequest

        login = self.get_body_argument('login', None)
        if not login:
            self.write(resp('Login not specified'))
            return

        path = self.get_body_argument('path', None)
        if not path:
            self.write(resp('Role path not specified'))
            return

        role_json = self.get_body_argument('role', None)
        if not role_json:
            self.write(resp('Role not specified'))
            return
        try:
            role = json.loads(role_json)
        except ValueError as exc:
            self.write(resp(f'Cannot decode role specification: {exc}'))
            return
        if not role:
            self.write(resp('Empty role specification'))
            return

        fields_json = self.get_body_argument('fields', None)
        if fields_json:
            try:
                fields = json.loads(fields_json)
            except ValueError as exc:
                self.write(resp(f'Cannot decode role fields: {exc}'))
                return
        else:
            fields = None

        self.log.info(
            '%s(login=%r, path=%r, role=%r, fields=%r)',
            self.__class__.__name__, login, path, role, fields,
        )

        return method(self, login, path, role, fields)
    return wrapper


class AddRoleHandler(IDMRequestHandler):
    def validate_role_fields_match(self, login, path, role, fields, idm_user_state):
        old_fields = idm_user_state[path].get('fields')

        if old_fields != fields:
            msg = (
                f'A role {path} is already exists for user {login} '
                'with different fields; '
                f'old_fields: {old_fields}; new_fields: {fields}'
            )
            self.log.error(msg)
            self.write(IDMResponses.WrongWorkflow(message=msg))
            raise web.Finish

    @tvm_verified
    @idm_role_handler
    async def post(self, login, path, role, fields):
        hub_user = await self.get_hub_user(login)

        with self.db.transaction():
            idm_user_state = self.db.get_user_idm_state(login)
            idm_role_state = {'role': role, 'fields': fields}

            if path in idm_user_state:
                self.validate_role_fields_match(login, path, role, fields, idm_user_state)

                warning = f'User {login} already have role {path}'
                self.log.warning(warning)
                idm_result = IDMResponses.Success(warning=warning, data=fields)
            else:
                idm_user_state[path] = idm_role_state
                self.db.add_or_update_user(login, idm_state=idm_user_state)
                idm_result = IDMResponses.Success(data=fields)

        hub_update = {
            'admin': ADMIN_ROLE_PATH in idm_user_state,
        }

        if hub_user:
            method = 'PATCH'
            self.log.info(f'updating jupyterhub user {login} with data {hub_update}')
        else:
            method = 'POST'
            self.log.info(f'creating jupyterhub user {login} with data {hub_update}')

        response = await self.jupyterhub_request(
            f'/users/{login}',
            data=hub_update,
            method=method,
        )

        if response.error:
            msg = (
                f'Cannot create or update via {method} '
                f'user {login} information: {response.error}'
            )
            self.log.error(msg)
            idm_result = IDMResponses.HubAPIError(msg)

        self.write(idm_result)


class RemoveRoleHandler(IDMRequestHandler):
    def validate_role_fields_match(self, login, path, role, fields, idm_user_state):
        role_fields = idm_user_state[path].get('fields')

        if role_fields != fields:
            msg = (
                f'Role {path} specification mismatch for user {login}: '
                f'trying to revoke role with fields {fields}, '
                f'but we store fields {role_fields}'
            )
            self.log.error(msg)
            self.write(IDMResponses.RoleMismatch(message=msg))
            raise web.Finish

    @tvm_verified
    @idm_role_handler
    async def post(self, login, path, role, fields):
        hub_user = await self.get_hub_user(login)

        with self.db.transaction():
            idm_user_state = self.db.get_user_idm_state(login)
            idm_role_state = idm_user_state.get(path)

            if idm_role_state:
                self.validate_role_fields_match(login, path, role, fields, idm_user_state)

                del idm_user_state[path]
                idm_result = IDMResponses.Success()
                self.db.add_or_update_user(login, idm_state=idm_user_state)
            else:
                msg = f'Role {path} does not exists for user {login}'
                self.log.warning(msg)
                idm_result = IDMResponses.Success(warning=msg)

        hub_update = None
        method = None
        if path == ADMIN_ROLE_PATH:
            hub_update = {
                'admin': False,
            }
            if hub_user:
                method = 'PATCH'
                self.log.info(f'updating jupyterhub user {login} with data {hub_update}')
            else:
                method = 'POST'
                self.log.info(f'creating jupyterhub user {login} with data {hub_update}')

        if method:
            response = await self.jupyterhub_request(
                f'/users/{login}',
                data=hub_update,
                method=method,
            )

            if response.error:
                msg = f'Cannot {method} user {login} information: {response.error}'
                self.log.error(msg)
                idm_result = IDMResponses.HubAPIError(msg)

        self.write(idm_result)


def get_handlers(prefix):
    paths = [
        (r'/info/?', InfoHandler),
        (r'/add-role/?', AddRoleHandler),
        (r'/remove-role/?', RemoveRoleHandler),
        (r'/get-all-roles/?', GetAllRolesHandler),
    ]

    return [
        (url_path_join(prefix, path), handler)
        for path, handler in paths
    ]
