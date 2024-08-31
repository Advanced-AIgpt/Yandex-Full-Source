import tornado
import traceback

from alice.megamind.protos.scenarios.directives_pb2 import TPushMessageDirective
from alice.megamind.protos.scenarios.push_pb2 import TDeletePushesDirective, TSendPushDirective
from alice.megamind.protos.scenarios.notification_state_pb2 import TNotificationState

from alice.protos.api.notificator.api_pb2 import TChangeStatus, EDirectiveStatus, TDeletePersonalCards, TDeliverySupCardRequest
from alice.protos.api.matrix.delivery_pb2 import TDelivery

from alice.uniproxy.library.async_http_client import QueuedHTTPClient, HTTPError, RTLogHTTPRequest
from alice.uniproxy.library.backends_common.protohelpers import proto_from_json
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.protos.uniproxy_pb2 import TPushMessage

from alice.uniproxy.library.protos.notificator_pb2 import TSupMessage
from alice.uniproxy.library.protos.notificator_pb2 import TNotificationChangeStatus
from alice.uniproxy.library.protos.notificator_pb2 import TManageSubscription
from alice.uniproxy.library.protos.notificator_pb2 import TDeliveryOnConnect
from alice.uniproxy.library.protos.notificator_pb2 import ENotificationRead

from alice.uniproxy.library.settings import config
from alice.uniproxy.library.utils import conducting_experiment
from alice.uniproxy.library.utils.hostname import current_hostname
from rtlog import null_logger

from alice.uniproxy.library.notificator_api.metrics import metrics_for


settings = config['notificator']['uniproxy']
DEFAULT_TIMEOUT = settings.get('timeouts', {}).get('default', 1.5)

DELIVERY = '/delivery'
DELIVERY_SUP = '/delivery/sup'
DELIVERY_SUP_CARD = '/delivery/sup_card'
DELETE_PUSHES_DIRECTIVE = '/personal_cards/delete'
PUSH_TYPED_SEMANTIC_FRAME = '/delivery/push'
DELIVERY_ON_CONNECT = '/delivery/on_connect'
MANAGE_SUBSCRIPTION = '/subscriptions/manage'
NOTIFICATIONS = '/notifications'
NOTIFICAITON_CHANGE_STATUS = '/notifications/change_status'
CHANGE_STATUS = '/directive/change_status'


def get_timeout(name):
    return settings.get('timeouts', {}).get(name, DEFAULT_TIMEOUT)


def get_client(url):
    return QueuedHTTPClient.get_client_by_url(
        url,
        pool_size=settings.get('pool_size', 1),
        queue_size=10,                          # 10 per process, 120 per instance
        wait_if_queue_is_full=False             # do not wait for free slot in queue
    )


class NotificatorApi:
    @staticmethod
    def parse_response(content):
        resp_msg = TNotificationState()
        if content:  # if user is not subscrubed to any subscriptions, response is null
            resp_msg.ParseFromString(content)
        return resp_msg

    def __init__(self, oauth_token, client_ip, rt_log=null_logger(), url=None, app_id=None, responses_storage=None, request_info={}, metrics_backend=None):
        self._user_ticket = None
        self._service_ticket = None
        self.oauth_token = oauth_token
        self.client_ip = client_ip
        self.rt_log = rt_log
        self.url = url or settings['url']
        self.metrics_backend = metrics_backend
        self.headers = {
            'Content-Type': 'application/protobuf',
        }

        self.app_id = app_id
        self.responses_storage = responses_storage
        self.store_responses = conducting_experiment('context_load_diff', request_info)
        if self.store_responses:
            self.req_id = request_info.get('header', {}).get('request_id')

    @tornado.gen.coroutine
    def prepare(self):
        '''here we need get user/service tickets, etc... '''
        return

    @tornado.gen.coroutine
    def send_push(self, yandexid, device_id=None, forced=False):
        try:
            msg = TPushMessage()
            msg.Uid = yandexid
            msg.Ring = 0
            if device_id:
                msg.DeviceId = device_id
            if self.app_id:
                msg.AppId = self.app_id

            url = self.url + DELIVERY
            if forced:
                url += '?forced=true'

            with metrics_for('notificator_send_push', self.metrics_backend) as m:
                request = RTLogHTTPRequest(
                    url,
                    method='POST',
                    headers=self.headers,
                    body=msg.SerializeToString(),
                    request_timeout=get_timeout(DELIVERY),
                    retries=settings['retries'],
                    rt_log=self.rt_log,
                    rt_log_label=m.label,
                    need_str=True,
                )

                yield get_client(url).fetch(request)

        except Exception as e:
            Logger.get('.send_push').error("send_push error: {}".format(e))

    @tornado.gen.coroutine
    def send_sup_push(self, puid, data: dict, test_id=None):
        try:
            if not puid:
                Logger.get('.send_sup_push').error('puid is not set')
                return

            msg = TSupMessage()
            msg.PushMsg.CopyFrom(proto_from_json(TPushMessageDirective, data))
            msg.Puid = puid
            if self.app_id:
                msg.AppId = self.app_id

            if test_id:
                msg.TestId = test_id

            with metrics_for('notificator_send_sup_push', self.metrics_backend) as m:
                request = RTLogHTTPRequest(
                    self.url + DELIVERY_SUP,
                    method='POST',
                    headers=self.headers,
                    body=msg.SerializeToString(),
                    request_timeout=get_timeout(DELIVERY_SUP),
                    retries=settings['retries'],
                    rt_log=self.rt_log,
                    rt_log_label=m.label,
                    need_str=True,
                )

            yield get_client(self.url).fetch(request)

        except Exception as e:
            Logger.get('.send_sup_push').error("send_sup_push error: {}".format(e))

    @tornado.gen.coroutine
    def send_sup_card_push(self, puid, data: dict, test_id=None):
        try:
            if not puid:
                Logger.get('.send_sup_card_push').error('puid is not set')
                return

            msg = TDeliverySupCardRequest()
            msg.Puid = puid
            msg.Directive.CopyFrom(proto_from_json(TSendPushDirective, data))

            if self.app_id:
                msg.AppId = self.app_id

            if test_id:
                msg.TestId = test_id

            with metrics_for('notificator_send_sup_card', self.metrics_backend) as m:
                request = RTLogHTTPRequest(
                    self.url + DELIVERY_SUP_CARD,
                    method='POST',
                    headers=self.headers,
                    body=msg.SerializeToString(),
                    request_timeout=get_timeout(DELIVERY_SUP_CARD),
                    retries=settings['retries'],
                    rt_log=self.rt_log,
                    rt_log_label=m.label,
                    need_str=True,
                )

                yield get_client(self.url).fetch(request)

        except Exception as e:
            Logger.get('.send_sup_card').error("send_sup_card error: {}".format(e))

    @tornado.gen.coroutine
    def delete_pushes_directive(self, puid, data: dict):
        try:
            if not puid:
                Logger.get('.delete_pushes_push').error('puid is not set')
                return

            msg = TDeletePersonalCards()
            msg.Puid = puid
            msg.Directive.CopyFrom(proto_from_json(TDeletePushesDirective, data))

            with metrics_for('notificator_delete_pushes', self.metrics_backend) as m:
                request = RTLogHTTPRequest(
                    self.url + DELETE_PUSHES_DIRECTIVE,
                    method='POST',
                    headers=self.headers,
                    body=msg.SerializeToString(),
                    request_timeout=get_timeout(DELETE_PUSHES_DIRECTIVE),
                    retries=settings['retries'],
                    rt_log=self.rt_log,
                    rt_log_label=m.label,
                    need_str=True,
                )

                yield get_client(self.url).fetch(request)

        except Exception as e:
            Logger.get('.delete_pushes').error("delete_pushes error: {}".format(e))

    @tornado.gen.coroutine
    def push_typed_semantic_frame(self, data: dict):
        try:
            with metrics_for('notificator_push_typed_semantic_frame', self.metrics_backend) as m:
                request = RTLogHTTPRequest(
                    self.url + PUSH_TYPED_SEMANTIC_FRAME,
                    method='POST',
                    headers=self.headers,
                    body=proto_from_json(TDelivery, data).SerializeToString(),
                    request_timeout=get_timeout(PUSH_TYPED_SEMANTIC_FRAME),
                    retries=settings['retries'],
                    rt_log=self.rt_log,
                    rt_log_label=m.label,
                    need_str=True,
                )

                yield get_client(self.url).fetch(request)

        except Exception as e:
            Logger.get('.push_typed_semantic_frame').error("push_typed_semantic_frame error: {}".format(e))

    @tornado.gen.coroutine
    def manage_subscriptions(self, puid, subscription_id, need_subscribe):
        try:
            msg = TManageSubscription()
            msg.SubscriptionId = subscription_id
            msg.Method = TManageSubscription.ESubscribe if need_subscribe else TManageSubscription.EUnsubscribe
            msg.Puid = puid
            if self.app_id:
                msg.AppId = self.app_id

            with metrics_for('notificator_manage_subscription', self.metrics_backend) as m:
                request = RTLogHTTPRequest(
                    self.url + MANAGE_SUBSCRIPTION,
                    method='POST',
                    headers=self.headers,
                    body=msg.SerializeToString(),
                    request_timeout=get_timeout(MANAGE_SUBSCRIPTION),
                    retries=settings['retries'],
                    rt_log=self.rt_log,
                    rt_log_label=m.label,
                    need_str=True,
                )

                resp = yield get_client(self.url).fetch(request)
                if resp.json()['code'] != 200:
                    Logger.get('.manage_subscriptions').error('internal error: code = {}'.format(resp.json()['code']))

        except Exception as e:
            Logger.get('.manage_subscriptions').error("manage_subscriptions error: {}".format(e))
            raise

    @tornado.gen.coroutine
    def get_notifications_state(self, puid, device_id, device_model):
        try:
            yield self.prepare()
            with metrics_for('notificator_get_notification_state', self.metrics_backend) as m:
                request = RTLogHTTPRequest(
                    self.url + NOTIFICATIONS + '?puid={}&device_id={}&device_model={}'.format(puid, device_id, device_model),
                    headers=self.headers,
                    request_timeout=get_timeout(NOTIFICATIONS),
                    retries=settings['retries'],
                    rt_log=self.rt_log,
                    rt_log_label=m.label,
                    need_str=True,
                )

                if self.store_responses and self.req_id:
                    response = yield get_client(self.url).fetch(request, raise_error=False)

                    self.responses_storage.store(self.req_id, 'notificator', response)

                    if response.code // 100 != 2:
                        raise HTTPError(response.code, response.body, response.request)
                else:
                    response = yield get_client(self.url).fetch(request)

                return self.parse_response(response.body) if response else TNotificationState()

        except Exception as e:
            Logger.get('.get_notifications_state').error('error: ', e, type(e), traceback.format_exc(5).replace("\n", ""))
            raise

    @tornado.gen.coroutine
    def mark_read(self, puid, ids):
        yield self.notification_change_status(puid, ids, ENotificationRead)

    @tornado.gen.coroutine
    def notification_change_status(self, puid, ids, status):
        try:
            yield self.prepare()

            msg = TNotificationChangeStatus()
            msg.Puid = puid
            msg.NotificationIds.extend(ids)
            msg.Status = status
            if self.app_id:
                msg.AppId = self.app_id

            with metrics_for('notificator_notification_change_status', self.metrics_backend) as m:
                request = RTLogHTTPRequest(
                    self.url + NOTIFICAITON_CHANGE_STATUS,
                    headers=self.headers,
                    method='POST',
                    body=msg.SerializeToString(),
                    request_timeout=get_timeout(NOTIFICAITON_CHANGE_STATUS),
                    retries=settings['retries'],
                    rt_log=self.rt_log,
                    rt_log_label=m.label,
                    need_str=True,
                )

                yield get_client(self.url).fetch(request)

        except Exception as e:
            Logger.get('.notification_change_status').error('error: ', e, type(e), traceback.format_exc(5).replace("\n", ""))
            raise

    @tornado.gen.coroutine
    def ack_directive(self, puid, device_id, ids, got_time):
        try:
            yield self.prepare()

            msg = TChangeStatus()
            msg.Puid = puid
            msg.DeviceId = device_id
            msg.Ids.extend(ids)
            msg.Status = EDirectiveStatus.ED_DELIVERED
            msg.StartTime = got_time

            with metrics_for('ack_directive', self.metrics_backend) as m:
                request = RTLogHTTPRequest(
                    self.url + CHANGE_STATUS,
                    headers=self.headers,
                    method='POST',
                    body=msg.SerializeToString(),
                    request_timeout=get_timeout(CHANGE_STATUS),
                    retries=settings['retries'],
                    rt_log=self.rt_log,
                    rt_log_label=m.label,
                    need_str=True,
                )

                yield get_client(self.url).fetch(request)

        except Exception as e:
            Logger.get('.directive_change_status').error('error: ', e, type(e), traceback.format_exc(5).replace("\n", ""))
            raise

    @tornado.gen.coroutine
    def on_connect(self, yandexid, device_id, device_model):
        try:
            msg = TDeliveryOnConnect()
            msg.Puid = yandexid
            msg.Hostname = current_hostname()
            msg.DeviceId = device_id
            model = device_model.lower() if device_model else ''
            if model == 'station':
                model = 'yandexstation'
            msg.DeviceModel = device_model
            if self.app_id:
                msg.AppId = self.app_id

            url = self.url + DELIVERY_ON_CONNECT

            with metrics_for('notificator_on_connect', self.metrics_backend) as m:
                request = RTLogHTTPRequest(
                    url,
                    method='POST',
                    headers=self.headers,
                    body=msg.SerializeToString(),
                    request_timeout=get_timeout(DELIVERY_ON_CONNECT),
                    retries=settings['retries'],
                    rt_log=self.rt_log,
                    rt_log_label=m.label,
                    need_str=True,
                )

                yield get_client(url).fetch(request)

        except Exception as e:
            Logger.get('.send_on_connect').error("send_on_connect error: {}".format(e))
