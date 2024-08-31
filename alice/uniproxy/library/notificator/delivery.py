import json
import os
import time
import traceback

import tornado.web

from alice.protos.api.notificator.api_pb2 import TDeliverySupCardRequest, TDeliverySupCardResponse
from alice.megamind.protos.scenarios.push_pb2 import TSendPushDirective

import alice.megamind.protos.common.app_type_pb2 as app_type
from alice.uniproxy.library.backends_common.protohelpers import proto_to_json

from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.global_counter import GlobalCounter
from alice.uniproxy.library.global_counter import GlobalTimings

from alice.uniproxy.library.events import Directive
from alice.uniproxy.library.protos.notificator_pb2 import TSupMessage

from alice.uniproxy.library.settings import config
from alice.uniproxy.library.personal_cards import PersonalCardsHelper

from alice.uniproxy.library.async_http_client import RTLogHTTPRequest, QueuedHTTPClient, HTTPError
from alice.uniproxy.library.notificator.common_handler import NotificatorCommonRequestHandler


sup_config = config.get('sup')
SUP_OAUTH = os.environ.get('SUP_OAUTH')
MOCK_DELIVERY = False
DEFAULT_TTL = 3600


class WhiteList:
    def __init__(self):
        self.enabled = config.get('notificator', {}).get('white_list', {}).get('enabled', False)
        self.puids = config.get('notificator', {}).get('white_list', {}).get('puids', [])

    def contains(self, puid):
        return str(puid) in self.puids if self.enabled else True


WHITE_LIST = WhiteList()


def report_white_list_error(handler, puid):
    err = 'Puid "{}" from request is not in white list.'.format(puid)
    handler.INFO(err)
    handler.set_status(403, err)
    handler.finish()


# =============================================================================================================
class DeliverySupHandler(NotificatorCommonRequestHandler):
    unistat_handler_name = 'delivery_sup'

    @staticmethod
    def convert_to_sup_card(req):
        sup_req = TDeliverySupCardRequest()

        sup_req.Puid = req.Puid
        sup_req.AppId = req.AppId
        sup_req.TestId = req.TestId

        sup_req.Directive.Settings.Title = req.PushMsg.Title
        sup_req.Directive.Settings.Text = req.PushMsg.Body
        sup_req.Directive.Settings.Link = req.PushMsg.Link

        sup_req.Directive.PushId = req.PushMsg.PushId
        sup_req.Directive.PushTag = req.PushMsg.PushTag

        sup_req.Directive.PushMessage.ThrottlePolicy = req.PushMsg.ThrottlePolicy
        sup_req.Directive.PushMessage.AppTypes.extend(req.PushMsg.AppTypes)

        for action in req.PushMsg.Actions:
            dir_action = TSendPushDirective.TAction()
            dir_action.Id = action.Id
            dir_action.Title = action.Title
            dir_action.Link = action.Link
            sup_req.Directive.Actions.append(dir_action)

        return sup_req

    def create_receiver(self, puid, device_types):
        receiver = 'tag: uid=={}'.format(puid)
        devices = []
        for t in device_types:
            mapped = None
            if t == app_type.AT_SEARCH_APP:
                mapped = 'pp'
            elif t == app_type.AT_MOBILE_BROWSER:
                mapped = 'mob_bro'
            elif t == app_type.AT_DESKTOP_BROWSER:
                mapped == 'desktop_bro'

            if mapped:
                devices.extend(sup_config['app_types'][mapped])

        app_ids = ','.join(["'" + d + "'" for d in devices])
        if app_ids:
            receiver += ' AND app_id IN (' + app_ids + ')'

        self.DEBUG(f'receiver = [{receiver}]')

        return receiver

    def get_notification(self, msg):
        notification = {
            "icon": sup_config['icon'],
            "icon_id": sup_config['icon_id'],
        }

        if msg.Settings.Title or msg.PushMessage.Settings.Title:
            notification["title"] = msg.PushMessage.Settings.Title or msg.Settings.Title

        if msg.Settings.Text or msg.PushMessage.Settings.Text:
            notification["body"] = msg.PushMessage.Settings.Text or msg.Settings.Text

        if msg.Settings.Link or msg.PushMessage.Settings.Link:
            notification["link"] = msg.PushMessage.Settings.Link or msg.Settings.Link

        return notification

    @tornado.gen.coroutine
    def send_sup(self, msg):
        client = QueuedHTTPClient.get_client_by_url(sup_config['url'])

        push_msg = msg.Directive.PushMessage

        headers = {
            'Content-Type': 'application/json;charset=UTF-8',
            'Authorization': 'OAuth ' + SUP_OAUTH,
        }

        actions_json = []
        for action in msg.Directive.Actions:
            action_json = {}
            if action.Id:
                action_json["id"] = action.Id
            if action.Title:
                action_json["title"] = action.Title
            if action.Link:
                action_json["link"] = action.Link
            actions_json.append(action_json)

        jreqbody = {
            "receiver": [
                self.create_receiver(msg.Puid, push_msg.AppTypes),
            ],
            'ttl': push_msg.Settings.TtlSeconds or msg.Directive.Settings.TtlSeconds or DEFAULT_TTL,
            'data': {
                "push_id": msg.Directive.PushId,
                "tag": msg.Directive.PushTag,
            },
            'notification': self.get_notification(msg.Directive),
            'project': 'notificator',
            'actions': actions_json,
        }

        if push_msg.ThrottlePolicy:
            jreqbody["throttle_policies"] = {
                "device_id": push_msg.ThrottlePolicy,
                "install_id": push_msg.ThrottlePolicy,
            }

        if msg.TestId:
            jreqbody["flags"] = {
                "testId": msg.TestId
            }

        request = RTLogHTTPRequest(
            sup_config['url'] + '?dry_run={}'.format(int(MOCK_DELIVERY)),
            method='POST',
            headers=headers,
            body=json.dumps(jreqbody).encode('utf-8'),
            request_timeout=sup_config['request_timeout'],
            retries=sup_config['retries'],
        )

        response = yield client.fetch(request)
        sup_resp = None
        sup_id = None
        try:
            jresp = json.loads(response.body)
            sup_id = jresp['id']
            sup_resp = {
                'id': sup_id,
                'receiver': jresp['receiver'],
            }
        except:
            sup_resp = response.body

        Logger.session(json.dumps(Directive(
            "Notificator",
            "Delivery",
            {
                'puid': msg.Puid,
                'type': 'sup',
                'push_id': msg.Directive.PushId,
                'sup_response': sup_resp,
                'app_id': msg.AppId,
            },
        ).create_message(None)), rt_log=self._rt_log)

        return sup_id

    @tornado.gen.coroutine
    def post(self):
        s_ts = time.monotonic()

        GlobalCounter.PUSH_SUP_MESSAGES_RECV_SUMM.increment()

        try:
            req = self.get_proto_request(TSupMessage)

            if not WHITE_LIST.contains(req.Puid):
                report_white_list_error(self, req.Puid)
                return

            msg = DeliverySupHandler.convert_to_sup_card(req)

            sup_id = yield self.send_sup(msg)

            GlobalCounter.PUSH_SUP_MESSAGES_SENT_SUMM.increment()

            self.set_status(200)
            self.finish({
                'code': 200,
                'sup_push_id': sup_id,
            })

        except Exception as e:
            GlobalCounter.PUSH_SUP_MESSAGES_FAIL_SUMM.increment()
            self.ERROR("sup error: {}".format(e))
            if isinstance(e, HTTPError):
                self.set_status(e.code)
            else:
                self.set_status(500)
            self.finish({
                'code': 500,
                'error': str(e),
            })

        finally:
            GlobalTimings.store('push_sup_msg_total', time.monotonic() - s_ts)


# =============================================================================================================
class DeliverySupCardHandler(DeliverySupHandler):
    unistat_handler_name = 'delivery_sup_card'

    @tornado.gen.coroutine
    def create_card(self, req):
        directive = req.Directive

        if MOCK_DELIVERY:
            return directive.PushId

        helper = PersonalCardsHelper(req.Puid, self._rt_log)
        now = int(time.time())
        date_from = directive.PersonalCard.DateFrom or now
        date_to = directive.PersonalCard.DateTo
        if not date_to:
            date_to = now + (directive.PersonalCard.Settings.TtlSeconds or directive.Settings.TtlSeconds or DEFAULT_TTL)

        payload = {
            'card': {
                'card_id': directive.PushId,
                'tag': directive.PushTag,

                'action_url': directive.PersonalCard.Settings.Link or directive.Settings.Link,
                'action_text': directive.PersonalCard.Settings.Title or directive.Settings.Title,
                'text': directive.PersonalCard.Settings.Text or directive.Settings.Text,
                'image_url': directive.PersonalCard.ImageUrl,

                'date_from': date_from,
                'date_to': date_to,
            }
        }

        jcard = proto_to_json(directive.PersonalCard)
        if 'settings' in jcard:
            del jcard['settings']

        payload.update(jcard)

        if directive.RemoveExistingCards:
            payload['card']['remove_existing_cards'] = True

        if not directive.DoNotDeleteCardsWithSameTag:
            yield helper.remove_cards_by_tag(directive.PushTag)

        try:
            yield helper.send_personal_card(payload)
        except Exception as exc:
            self.ERROR("error:", exc, type(exc), traceback.format_exc(5).replace("\n", ""))
            return None

        return directive.PushId

    @tornado.gen.coroutine
    def post(self):
        s_ts = time.monotonic()

        GlobalCounter.PUSH_SUP_CARD_MESSAGES_RECV_SUMM.increment()
        resp = TDeliverySupCardResponse()

        try:
            req = self.get_proto_request(TDeliverySupCardRequest)

            if not WHITE_LIST.contains(req.Puid):
                report_white_list_error(self, req.Puid)
                return

            resp.SupId = yield self.send_sup(req)
            resp.CardId = yield self.create_card(req)

            GlobalCounter.PUSH_SUP_CARD_MESSAGES_SENT_SUMM.increment()

            self.send_response(resp)

        except Exception as exc:
            GlobalCounter.PUSH_SUP_MESSAGES_FAIL_SUMM.increment()
            self.ERROR("error:", exc, type(exc), traceback.format_exc(5).replace("\n", ""))
            if isinstance(exc, HTTPError):
                self.set_status(exc.code)
            else:
                self.set_status(500)

        finally:
            GlobalTimings.store('push_sup_msg_total', time.monotonic() - s_ts)
