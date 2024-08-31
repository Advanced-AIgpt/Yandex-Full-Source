from .role_manager import parse_role, serialize_role

from alice.uniproxy.library.auth import tvm2

import tornado.web

from collections import defaultdict
import json


class BaseHandler(tornado.web.RequestHandler):
    def initialize(self, logger, idm_client_id, role_manager):
        self.logger = logger
        self.idm_client_id = idm_client_id
        self.role_manager = role_manager

    async def try_authenticate_tvm_app(self):
        tvm_ticket = self.request.headers.get('X-Ya-Service-Ticket')
        if tvm_ticket:
            client_id = await tvm2.authenticate_tvm_app(tvm_ticket, self.logger)
            if client_id == self.idm_client_id:
                return True
            else:
                self.logger.info('Unauthorized request /idm/. Service ticket belongs to id %s, expected %s',
                                 client_id, self.idm_client_id)
                return False
        else:
            self.logger.info('Unauthorized request /idm/. Header X-Ya-Service-Ticket not found.')
            return False

    async def prepare(self):
        idm_request_id = self.request.headers.get('X-IDM-Request-Id')
        self.logger.info('X-IDM-Request-Id: %s', idm_request_id)
        self.idm_request_authenticated = await self.try_authenticate_tvm_app()

    async def do_answer(self):
        if self.idm_request_authenticated:
            self.write(await self.answer())
        else:
            self.set_status(401, reason='Tvm check failed')


class GetHandler(BaseHandler):
    async def get(self):
        await self.do_answer()


class PostHandler(BaseHandler):
    async def post(self):
        await self.do_answer()


class GetInfoHandler(GetHandler):
    async def answer(self):
        return {
            'code': 0,
            'roles': await self.role_manager.get_role_info(),
        }


class GetRolesHandler(GetHandler):
    async def answer(self):
        offset = int(self.get_argument('offset', 0))
        limit = int(self.get_argument('limit', 500))

        result = {
            'code': 0,
            'roles': list(),
        }

        role_entries = await self.role_manager.get_roles(offset=offset, limit=limit)
        for login, user_type, role in role_entries:
            result['roles'].append({
                'login': login,
                'subject_type': user_type,
                'path': self.role_manager.make_slug_path(parse_role(role)),
            })

        if len(role_entries) > 0:
            result['next-url'] = f'/idm/v1/get-roles/?offset={offset+limit}&limit={limit}'

        return result


class GetAllRolesHandler(GetHandler):
    async def answer(self):
        result = {
            'code': 0,
            'users': list()
        }
        roles_by_login = defaultdict(list)
        role_entries = await self.role_manager.get_all_roles()

        for login, user_type, role in role_entries:
            roles_by_login[(login, user_type)].append(parse_role(role))

        for key, roles in roles_by_login.items():
            login, user_type = key
            result['users'].append({
                'login': login,
                'subject_type': user_type,
                'roles': roles,
            })

        return result


class AddRoleHandler(PostHandler):
    async def answer(self):
        login = self.get_argument('login')
        role = serialize_role(json.loads(self.get_argument('role')))
        user_type = self.get_argument('subject_type')
        if await self.role_manager.add_role(login, user_type, role):
            return {'code': 0}
        else:
            return {'code': 1, 'error': 'Unknown error'}


class RemoveRoleHandler(PostHandler):
    async def answer(self):
        login = self.get_argument('login')
        role = serialize_role(json.loads(self.get_argument('role')))
        user_type = self.get_argument('subject_type')
        if await self.role_manager.remove_role(login, user_type, role):
            return {'code': 0}
        else:
            return {'code': 1, 'error': 'Unknown error'}
