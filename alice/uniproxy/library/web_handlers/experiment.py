import tornado.escape
import json

from alice.uniproxy.library.events import Event
from alice.uniproxy.library.experiments import experiments
from alice.uniproxy.library.unisystem.unisystem import UniSystem
from alice.uniproxy.library.common_handlers import CommonRequestHandler


class ExperimentsHandler(CommonRequestHandler):
    unistat_handler_name = 'exp'

    def initialize(self, by_id=True):
        self._by_id = by_id

    def get(self):
        if self._by_id:
            return self.get_by_id()
        else:
            return self.get_by_app_id()

    def get_by_app_id(self):
        app_id = self.get_argument('vins_app_id')
        if not app_id:
            self.set_status(400)
            self.write('expected "vins_app_id" parameter\n')
            return

        is_staff = self.get_argument('is_staff', False)
        session_data = {'vins': {'application': {'app_id': app_id}}}

        unisystem = UniSystem(
            ipaddr=self.request.headers.get(
                'X-Real-Ip',
                self.request.headers.get('X-Forwarded-For', self.request.remote_ip)
            ),
            exps_check=True,
            session_data=session_data,
        )

        if not experiments.try_use_experiment(unisystem):
            self.set_status(400)
            self.write('No experiments')
            return

        try:
            exps = unisystem.event_patcher.patch(
                event=Event({
                    'header': {
                        'namespace': 'experiments',
                        'name': 'checking',
                        'messageId': '1',
                    },
                    'payload': {},
                }),
                session_data=session_data,
                staff_login='test@yandex-team.ru' if is_staff else None,
                rt_log=unisystem.rt_log,
            )

            ret = []
            for exp, flags in exps:
                ret.append({'id': exp.id, 'pool': exp.pool, 'flags': flags})

            self.set_status(200)
            self.write(json.dumps(ret))

        except Exception as exc:
            self.write('can not get experiments: {}\n'.format(tornado.escape.xhtml_escape(str(exc))))
            self.set_status(400)

    def get_by_id(self):
        exp_id = tornado.escape.xhtml_escape(self.get_argument('id', ''))
        if not exp_id:
            self.set_status(400)
            self.write('expected "id" parameter\n')
            return

        new_share = self.get_argument('share', None)
        if new_share is None:
            try:
                self.write('{} share={}\n'.format(exp_id, experiments.get_share(exp_id)))
                self.set_status(200)
            except Exception as exc:
                self.write('can not get experiment share: {}\n'.format(tornado.escape.xhtml_escape(str(exc))))
                self.set_status(400)
        else:
            try:
                experiments.set_share(exp_id, float(new_share))
                self.write('{} new share={}\n'.format(exp_id, new_share))
                self.set_status(200)
            except Exception as exc:
                self.write('can not set experiment share: {}\n'.format(tornado.escape.xhtml_escape(str(exc))))
                self.set_status(400)
