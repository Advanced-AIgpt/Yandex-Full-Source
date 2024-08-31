import inspect
import io
import json
import tempfile

import yappi
from jupyterhub import orm
from jupyterhub.apihandlers.base import APIHandler
from jupyterhub.utils import url_path_join
from sqlalchemy.orm import selectinload
from tornado import web

from .base import JCPageHandler, JCAPIHandler


class PendingSpawnHandler(APIHandler):
    async def get(self):
        result = {}
        for user in self.users.values():
            for spawner in user.spawners.values():
                pending = spawner.pending
                if pending:
                    url = url_path_join(self.hub.base_url, 'spawn-pending', user.name)

                    result[user.name] = url

        self.write(json.dumps(result))


class FeatureBoardHandler(JCPageHandler):
    async def get(self):
        html = await self.render_template('feature_board.html')
        self.write(html)


class HandlersPageHandler(JCPageHandler):
    async def get(self):
        from . import default_handlers

        raw_handlers = sorted(default_handlers)

        handlers = [
            {
                'url': path,
                'cls': cls.__name__,
                'filename': inspect.getfile(cls),
            } for (path, cls, *_) in raw_handlers
        ]

        html = await self.render_template(
            'handlers.html',
            handlers=handlers,
        )

        self.write(html)


class BadVMStatusesHandler(JCAPIHandler):
    async def get(self):
        bad_vms = []
        not_polled = 0
        total = 0

        for orm_user in self.db.query(orm.User).options(selectinload(orm.User._orm_spawners)).all():
            user = self._user_from_orm(orm_user)

            spawner = user.spawners.get('')  # "" - is default non-named server

            if not spawner or not spawner.vm:
                continue

            total += 1

            status = spawner._last_poll_status
            if not status:
                not_polled += 1
                continue

            if 0 < status.value < 5:
                continue

            bad_vms.append({
                'user': user.name,
                'status': status.name,
                'link': spawner.qyp_link,
                'host': spawner.vm.host,
            })

        self.write({
            'vms': bad_vms,
            'number': len(bad_vms),
            'total': total,
            'not_polled': not_polled,
        })


class ProfileHandler(JCAPIHandler):
    async def get(self, format=None):
        if not yappi.is_running():
            raise web.HTTPError(418, 'Profiler is not running')

        fstats = yappi.get_func_stats()

        if format is not None:
            with tempfile.NamedTemporaryFile() as fp:
                fstats.save(fp.name, type=format)

                fp.seek(0)
                self.write(fp.read())
                self.set_header('Content-Type', 'application/octet-stream')
        else:
            fp = io.StringIO()
            fstats.print_all(
                fp, columns={0: ('name', 120), 1: ('ncall', 12), 2: ('tsub', 8), 3: ('ttot', 8), 4: ('tavg', 8)},
            )
            self.write(fp.getvalue())
            self.set_header('Content-Type', 'text/plain')

    async def post(self, command=None):
        if command not in ('start', 'stop'):
            raise web.HTTPError(400, 'Send a command')
        elif command == 'start':
            yappi.start()
        elif command == 'stop':
            yappi.stop()


class TelegramUsernameHandler(JCPageHandler):
    async def get(self, telegram_username):
        login = await self.staff_client.get_single_username_by_telegram(telegram_username)
        self.redirect(url_path_join(self.hub.base_url, 'user', login))


default_handlers = [
    ('/api/pending_spawn', PendingSpawnHandler),
    ('/api/bad_vm_statuses', BadVMStatusesHandler),
    ('/feature_board', FeatureBoardHandler),
    ('/handlers', HandlersPageHandler),
    (r'/api/profile(?:/(\w+))?', ProfileHandler),
    (r'/@(?P<telegram_username>[^/]+)', TelegramUsernameHandler)
]
