from alice.uniproxy.library.events import Directive, EventException
from alice.uniproxy.library.experiments import experiments as global_experiments_object
from alice.uniproxy.library.personal_data import PersonalDataHelper
from alice.uniproxy.library.settings import config
from alice.uniproxy.library.uaas_mapper.uaas_mapper import GetUaasAppHeader
from alice.uniproxy.library.utils.experiments import conducting_experiment
from alice.uniproxy.library.backends_common.protohelpers import proto_to_json
from alice.uniproxy.library.protos.uniproxy_pb2 import TPushMessage
from alice.uniproxy.library.notificator_api import NotificatorApi
from alice.uniproxy.library.global_counter import GlobalTimings

from alice.cuttlefish.library.protos.session_pb2 import TSessionContext, TUserInfo


from . import EventProcessor, register_event_processor
from tornado import gen

import base64
import json
import time
import traceback


MSSNGR_ANONYMOUS_GUID = config['messenger'].get('anonymous_guid', '')
VINS_APP_TYPES = config['vins'].get('app_types', {})


class SystemProcessorBase(EventProcessor):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._experiments = global_experiments_object  # for mocking
        self._disable_local_experiments = False

    def process_event(self, event):
        self._disable_local_experiments = event.payload.pop("disable_local_experiments", False)
        super().process_event(event)

    def _process_external_experiments(self):
        if (not self._disable_local_experiments) and self._experiments.try_use_experiment(self.system):
            self.system.DLOG('set event patcher with flags from local experiments config')
            self.system.event_patcher.patch(self.event, self.system.session_data,
                                            staff_login=self.system.staff_login, rt_log=self.rt_log)
            self.system.update_session(self.event.payload)

        super()._process_external_experiments()


# ====================================================================================================================
@register_event_processor
class SetState(SystemProcessorBase):
    def process_event(self, event):
        super().process_event(event)
        self.system.set_uniproxy2({})  # it's always uniproxy2 - we MUST send EventProcessorFinished anyway

        try:
            if not self.is_local_session():
                raise EventException("remote session", event.message_id)

            payload, context = self.get_session_context(event)
            if not context:
                raise EventException("session_context is empty", event.message_id)

            if not self.update_system_from_context(payload, context):
                raise EventException("failed to update payload", event.message_id)

            self.system.set_uniproxy2(payload.pop("uniproxy2", None))
        except EventException as ex:
            self.logev("SetStateError", {
                "reason": ex.message,
            })
            self.system.write_directive(ex)
        except Exception as ex:
            self.system.EXC("System.SetState")
            self.logev("SetStateError", {
                "reason": "exception",
                "exception": str(ex),
            })
            self.system.write_directive(EventException("failed", event.message_id))
        self.close()

    def update_system_from_context(self, payload, context):
        if not self.system:
            self.logev("SetStateError", {
                "reason": "unisystem is none",
            })
            return False

        self.system.synchronize_state_message_id = context.InitialMessageId

        if context.HasField("AppToken"):
            payload["auth_token"] = context.AppToken
            payload["key"] = context.AppToken
            payload["apiKey"] = context.AppToken

        #
        #   TSessionContext.UserInfo
        #
        if context.HasField("UserInfo"):
            user = context.UserInfo
            if user.HasField("Uuid"):
                self.system.set_uuid(user.Uuid)

            if user.HasField("Yuid"):
                self.system.yuid = user.Yuid

            if user.HasField("Puid"):
                self.system.puid = user.Puid
                self.system.logger.log_directive(
                    {
                        'ForEvent': self.event.message_id,
                        'GotPuid': user.Puid,
                        'Type': 'from user context'
                    },
                    rt_log=self.rt_log,
                )

            if self.system.puid is None:
                self.system.uid = self.system.yuid
                self.system.send_uniproxy2_update_user_session(uid=self.system.uid)
            else:
                self.system.uid = self.system.puid

            if user.HasField("Guid"):
                self.system.guid = user.Guid

            if user.HasField("AuthToken") and (user.AuthTokenType == TUserInfo.ETokenType.OAUTH):
                self.system.oauth_token = user.AuthToken
                payload.setdefault("request", {}).setdefault(
                    "additional_options",
                    {}
                ).setdefault("oauth_token", user.AuthToken)

            if user.HasField("Cookie"):
                pass

            if user.HasField("ICookie"):
                if not self.system.icookie_for_uaas:
                    self.system.icookie_for_uaas = user.ICookie

            if user.HasField("StaffLogin"):
                self.system.staff_login = user.StaffLogin

            # TODO (paxakor): remove
            if user.HasField("LaasRegion"):
                payload.setdefault("request", {})["laas_region"] = json.loads(user.LaasRegion)

        #
        #   TSessionContext.ConnectionInfo
        #
        if context.HasField("ConnectionInfo"):
            info = context.ConnectionInfo
            if info.HasField("IpAddress"):
                pass
            if info.HasField("UserAgent"):
                pass

        #
        #   TSessionContext.UserOptions
        #
        user_options = context.UserOptions
        self.system.save_to_mds = user_options.SaveToMds
        self.system.do_not_use_user_logs = user_options.DoNotUseLogs
        self._disable_local_experiments = user_options.DisableLocalExperiments
        if user_options.DisableUtteranceLogging:
            self.system.logger.disable_utterance()

        #
        #   TSessionContext.DeviceInfo
        #
        if context.HasField("DeviceInfo"):
            device_info = context.DeviceInfo
            self.system.device_model = device_info.DeviceModel
            self.system.device_manufacturer = device_info.DeviceManufacturer
            self.system.device_id = device_info.DeviceId
            self.system.platform = device_info.Platform

        self.system.app_id = context.AppId
        self.system.app_type = context.AppType

        if context.Experiments.FlagsJsonData.HasField("AppInfo"):
            self.system.set_x_yandex_appinfo(context.Experiments.FlagsJsonData.AppInfo)

        #
        #   Update session data before applying uaas experiments
        #
        self.system.amend_event_payload(payload)
        self.system.update_session(payload)
        self.system.set_client()
        self.system.sync_state_complete()

        # hack for flags providers settings
        if isinstance(payload, dict):
            settings_from_manager = payload.get("settings_from_manager", {})
            self.payload_with_session_data["settings_from_manager"] = settings_from_manager
            self.system.try_update_activity_check_period_seconds(
                settings_from_manager.get("activity_check_period_seconds"),
                settings_from_manager.get("activity_check_period_variation_seconds")
            )

        self._try_process_external_experiments()

        return True

    # ----------------------------------------------------------------------------------------------------------------
    def on_flags_json_response(self, resp):
        try:
            self.system.uaas_test_ids = resp.get_all_test_id()
            self.system.uaas_exp_boxes = resp._exp_boxes
            self.system.uaas_asr_flags = resp.get_ab_config('ASR')
            self.system.uaas_bio_flags = resp.get_ab_config('BIO')

            flags = resp.get_all_flags()
            self.system.uaas_flags.update(flags)

            for test_id in self.system.uaas_test_ids:
                self.system.log_experiment({
                    'id': test_id,
                    'type': 'flags_json',
                    'control': False,
                })

            # Try to set vins url via experiment
            for flag in flags:
                if flag.startswith('UaasVinsUrl_'):
                    self._try_use_vins_url(self.system.session_data, flag)
                if flag.startswith('set_topic'):
                    kv = flag.split('=')
                    if len(kv) == 2:
                        _, topic = kv
                        if self.system:
                            self.system.ab_asr_topic = topic.strip()

        except Exception as exc:
            self.WARN("error on handling flags_json response: {}".format(exc))
            self.EXC(exc)

    def get_session_context(self, event):
        payload = event.payload.pop("original_payload")
        if not payload:
            raise EventException("original payload is empty", event.message_id)

        context_encoded = event.payload.pop("session_context")
        if not context_encoded:
            raise EventException("session context is empty", event.message_id)
        context = TSessionContext()
        context.ParseFromString(base64.b64decode(context_encoded))

        return payload, context

    def is_local_session(self):
        return True

    def logev(self, name, payload):
        self.system.logger.log_directive(
            {
                "type": name,
                "ForEvent": self.event.message_id,
                "data": payload if payload else {},
            },
            rt_log=self.rt_log,
        )


# ====================================================================================================================
@register_event_processor
class SynchronizeState(SystemProcessorBase):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def close(self):
        super().close()

    # ----------------------------------------------------------------------------------------------------------------
    def process_event(self, event):
        super().process_event(event)

        self.system.suspend_message_processing(True, "SynchronizeState")
        self.system.on_synchronize_state()

        self.system.save_to_mds = event.payload.get('save_to_mds', True)

        oauth_token = event.payload.pop("oauth_token", None)
        yandexuid = event.payload.get('yandexuid')

        self.system.set_auth_token(event.payload.pop("auth_token", None), event.payload.pop("service_name", None))
        self.system.set_csrf_token(event.payload.pop("csrf_token", None))
        self.system.set_uuid(event.payload.pop("uuid", None))
        self.system.set_wifi_nets(event.payload.pop("wifi_networks", None))
        if 'Messenger' in event.payload:
            self.system.set_messenger_data(event.payload.get('Messenger', {}))
        self.system.set_synchronize_state_id(event.message_id)
        self.system.set_uniproxy2(event.payload.pop("uniproxy2", None))

        if self.event.payload.get("disable_utterance_logging", False):
            self.system.logger.disable_utterance()

        self.system.update_session(event.payload)
        # set self.system app_id, device_id, platform
        self.uaas_app_info(event)

        # start coroutine
        self.process_event_coro(event, oauth_token, yandexuid)

    @gen.coroutine
    def process_event_coro(self, event, oauth_token, yandexuid):
        yield self.system.set_oauth_token(oauth_token, yandexuid, event.message_id)
        self.system.wait_initialization(message_id=event.message_id)

        if 'Messenger' not in event.payload:
            yield self.try_get_personal_settings()
            self.system.suspend_message_processing(True, "Experiments on SynchronizeState")
            self.system.logger.log_directive(
                {
                    'ForEvent': self.event.message_id,
                    'UaasPuid': self.system.puid
                },
                rt_log=self.rt_log,
            )
            self._try_use_experiment(puid=self.system.puid)  # it'll unsusepend in after all

        self.vins_app_data(event)

        if event.payload.get('Messenger', {}).get('anonymous', False):
            self.system.set_messenger_guid(MSSNGR_ANONYMOUS_GUID, ticket=None, anonymous=True)
        elif self._is_messenger_fake_user():
            self.system.set_messenger_guid(event.payload["guid"], event.payload["serviceticket"])
        elif self._is_messenger(oauth_token):
            self.system.update_messenger_guid(oauth_token)
        else:
            self.system.set_client()

        self.system.sync_state_complete()

        self.system.suspend_message_processing(False, "SynchronizeState")
        if self.system.suspend_future:
            yield self.system.suspend_future

        self.close()

    # ----------------------------------------------------------------------------------------------------------------
    def uaas_app_info(self, event):
        app = event.payload.get('vins', {}).get('application', {})
        if not app:
            return

        app_id = app.get('app_id', None)
        if app_id:
            self.system.app_id = app_id
            self.system.app_type = VINS_APP_TYPES.get(app_id, 'other_apps')

        device_id = app.get('device_id', None)
        if device_id:
            self.system.device_id = device_id

        platform = app.get('platform', None)
        if platform:
            self.system.platform = platform

        #
        # VOICESERV-2886 & MEGAMIND-741
        #
        if app_id and platform:
            self.system.set_x_yandex_appinfo(GetUaasAppHeader(app_id, platform))

    # ----------------------------------------------------------------------------------------------------------------
    def on_flags_json_response(self, resp):
        try:
            self.system.uaas_test_ids = resp.get_all_test_id()
            self.system.uaas_exp_boxes = resp._exp_boxes
            self.system.uaas_asr_flags = resp.get_ab_config('ASR')
            self.system.uaas_bio_flags = resp.get_ab_config('BIO')

            flags = resp.get_all_flags()
            self.system.uaas_flags.update(flags)

            for test_id in self.system.uaas_test_ids:
                self.system.log_experiment({
                    'id': test_id,
                    'type': 'flags_json',
                    'control': False,
                })

            # Try to set vins url via experiment
            for flag in flags:
                if flag.startswith('UaasVinsUrl_'):
                    self._try_use_vins_url(self.system.session_data, flag)
                    break

        except Exception as exc:
            self.WARN("error on handling flags_json response: {}".format(exc))
            self.EXC(exc)

    # ----------------------------------------------------------------------------------------------------------------
    def vins_app_data(self, event):
        app = event.payload.get('vins', {}).get('application', {})
        if not app:
            return

        self.system.device_model = app.get('device_model', None)
        if self.system.device_model:
            device_model = self.system.device_model.lower().replace(' ', '_').replace('-', '_')
            if device_model == "station" and \
               conducting_experiment("use_yandexstation_instead_of_station", self.payload_with_session_data):
                device_model = 'yandexstation'
            self.system.device_model = device_model

        dev_manuf = app.get('device_manufacturer', None)
        if dev_manuf:
            self.system.device_manufacturer = dev_manuf.lower().replace(' ', '_').replace('-', '_')

    # ----------------------------------------------------------------------------------------------------------------
    def _is_messenger(self, oauth_token):
        return (
            'Messenger' in self.event.payload
            or self.system._preprocess_oauth_token(oauth_token)[1] == 'yambauth'
            or self.system.x_yamb_cookie and self.system.csrf_token
        )

    # ----------------------------------------------------------------------------------------------------------------
    def _is_messenger_fake_user(self):
        return 'guid' in self.event.payload and 'serviceticket' in self.event.payload

    # ----------------------------------------------------------------------------------------------------------------
    def _process_external_experiments(self):
        super()._process_external_experiments()
        try:
            self.system.suspend_message_processing(False, "Experiments on SynchronizeState")
            if conducting_experiment('disable_balancing_hint', self.payload_with_session_data):
                self.system.use_balancing_hint = False
        except ReferenceError:
            pass

    @gen.coroutine
    def try_get_personal_settings(self):
        self.system.suspend_message_processing(True, 'wait_datasync')
        if self.system.puid is not None:  # successfull BlackBox response
            personal_data_helper = PersonalDataHelper(self.system, {}, self.rt_log)
            res, has_error = yield personal_data_helper.get_personal_data(only_settings=True)
            wrong_number_of_items = len(res) != 1 if res else True
            if has_error:
                do_not_use_user_logs = True
                self.system.logger.log_directive(
                    {
                        "type": "DatasyncDoNotUseUserLogsResult",
                        "ForEvent": self.event.message_id,
                        "result": "Error",
                    },
                    rt_log=self.rt_log,
                )
            elif res is None:
                do_not_use_user_logs = False
            elif wrong_number_of_items:
                # log error
                self.WARN("Got wrong number of items from datasync: {}".format(res))
                do_not_use_user_logs = False
            else:
                do_not_use_user_logs = False
                try:
                    values = list(res.values())
                    if len(values):
                        settings = values[0]
                        do_not_use_user_logs = settings.get('do_not_use_user_logs', False)
                except Exception as ex:
                    self.EXC(ex)
        else:
            # two variants: BB error or no data to auth
            if self.system.bb_uid4oauth_error:
                do_not_use_user_logs = True
            else:
                do_not_use_user_logs = False
        self.system.do_not_use_user_logs = do_not_use_user_logs
        self.system.suspend_message_processing(False, 'wait_datasync')


@register_event_processor
class UserInactivityReport(EventProcessor):
    def process_event(self, event):
        super().process_event(event)
        self.close()


@register_event_processor
class ResetUserInactivity(EventProcessor):
    def process_event(self, event):
        super().process_event(event)
        self.close()


@register_event_processor
class ExceptionEncountered(EventProcessor):
    def process_event(self, event):
        super().process_event(event)
        self.close()


@register_event_processor
class EchoRequest(EventProcessor):
    def process_event(self, event):
        super(EchoRequest, self).process_event(event)
        self.payload_with_session_data["uaas_flags"] = self.system.uaas_flags
        self.payload_with_session_data["uaas_test_ids"] = self.system.uaas_test_ids
        self.system.write_directive(Directive(
            "System",
            "EchoResponse",
            self.payload_with_session_data,
            self.event.message_id
        ))
        self.close()


@register_event_processor
class Push(EventProcessor):
    def process_event(self, event):
        super().process_event(event)

        self.INFO('push proc')
        if not self.system.is_notification_supported():
            self.system.ERR('feature "notification" is not supported. ignore nofication')
            self.close()
            return

        try:
            # client_message = data.get('ServerMessage', {}).get('ClientMessage', {})

            quasar_msg = TPushMessage()
            quasar_msg.ParseFromString(event.payload['data'])

            # TODO: use special speechkit serializer
            directives = []
            for d in [proto_to_json(d) for d in quasar_msg.Directives]:
                directive = {
                    'name': 'notify',
                    'payload': d['notify_directive'],
                }

                del directive['payload']['name']

                directives.append(directive)

            directives.extend([proto_to_json(d) for d in quasar_msg.SkDirectives])

            data = {'directives': directives}

            ack_data = {
                'time': quasar_msg.StartTime,
                'push_ids': [i for i in quasar_msg.PushIds],
            }
            self.send_message(data, ack_data)
        except Exception as exc:
            self.ERR('push fail {}: ({})'.format(exc, traceback.format_exc(5).replace("\n", "")))
        finally:
            self.close()

    # ----------------------------------------------------------------------------------------------------------------
    def send_message(self, data, ack_data):
        directive = Directive('System', 'Push', data, need_ack=True, ack_data=ack_data)
        message = directive.create_message(self.system)
        self.system.DLOG(message)
        self.system.write_message(message)


@register_event_processor
class Ack(EventProcessor):
    def process_event(self, event):
        super().process_event(event)

        message_id = self.payload_with_session_data.get('header', {}).get('messageId')
        if message_id is None:
            self.INFO('header->messageId is not present in payload')
            self.close()
            return

        ack = self.system.acks.pop(message_id, None)
        if ack is None:
            ack = self.payload_with_session_data.get('from_uniproxy2')

        if ack is None:
            self.INFO('ack-data not found for {}'.format(message_id))
            self.close()
            return

        self.INFO(f'ack_data: {ack}')

        header = self.payload_with_session_data['header']
        first_ts = self.payload_with_session_data['first_recv_ts']
        last_ts = self.payload_with_session_data['last_recv_ts']

        try:
            # fork coroutine
            if ack['name'] == 'system.push':
                self.ack_push(header, first_ts, last_ts, ack['data'])
            else:
                self.close()
        except Exception:
            pass

    @gen.coroutine
    def ack_push(self, header, first_ts, last_ts, data):
        try:
            dt = int(time.time() * 1000) - header['ack']
            self.INFO('got ack for push in {} ms'.format(dt))

            GlobalTimings.store('push_ack_waiting_hgram', dt/1000)

            if not data:
                self.ERR('data empty')
                return

            if 'push_ids' not in data:
                self.INFO('nothing to mark "delivered"')
                return

            yield NotificatorApi(
                self.system.get_oauth_token,
                self.system.client_ip,
                rt_log=self.rt_log,
                url=self.system.srcrwr['NOTIFICATOR'],
                app_id=self.system.app_id,
            ).ack_directive(
                self.system.puid,
                self.system.device_id,
                data['push_ids'],
                data['time'],
            )

        finally:
            self.close()
