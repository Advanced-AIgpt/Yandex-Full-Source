from . import EventProcessor, register_event_processor
from alice.uniproxy.library.global_counter import GlobalCounter
from alice.uniproxy.library.utils.tree import value_by_path
from alice.cuttlefish.library.protos.context_load_pb2 import TContextLoadResponse
from alice.megamind.protos.common.iot_pb2 import TIoTUserInfo
import json
import base64


def try_json(raw):
    try:
        return json.loads(raw)
    except:
        return None


def remove_flappy_laas_fields(val):
    val.pop("location_unixtime", None)
    val.pop("suspected_location_unixtime", None)


@register_event_processor
class ApplySessionContext(EventProcessor):

    @staticmethod
    def _comparable_oauth_token(system):
        return None if system.oauth_token is None else f"OAuth {system.oauth_token}"

    @staticmethod
    def _comparable_messenger_user_type(system):
        if system.mssngr_user_is_fake:
            return "Fake"
        if system.mssngr_user_is_anon:
            return "Anonymous"
        return "Real"

    def process_event(self, event):
        super().process_event(event)
        GlobalCounter.U2_APPLY_SESSION_CONTEXT_SUMM.increment()

        system = self.system
        session_data = system.session_data

        diffs = []  # list of different fields

        def check(expected, *path):
            if expected != value_by_path(event.payload, path):
                diffs.append(f"value at {'/'.join(path)} != '{expected}'")
                return False
            return True

        self.DLOG("Validate ApplySessionContext content...")

        try:
            check(system.synchronize_state_message_id, "InitialMessageId")
            if not check(session_data["apiKey"], "AppToken"):
                GlobalCounter.U2_ASC_DIFF_APP_TOKEN_SUMM.increment()

            # Application
            check(system.app_id, "Application", "Id")
            check(system.app_type, "Application", "Type")

            # Device
            check(system.device_id, "Device", "Id")
            check(system.platform, "Device", "Platform")
            check(system.device_model, "Device", "Model")
            check(system.device_manufacturer, "Device", "Manufacturer")

            # User
            check(system.icookie_for_uaas, "User", "ICookie")
            if not check(self._comparable_oauth_token(system), "User", "AuthToken"):
                GlobalCounter.U2_ASC_DIFF_OAUTH_TOKEN_SUMM.increment()
            if not check(session_data["uuid"], "User", "Uuid"):
                GlobalCounter.U2_ASC_DIFF_UUID_SUMM.increment()
            if not check(system.puid, "User", "Puid"):
                GlobalCounter.U2_ASC_DIFF_PUID_SUMM.increment()
            if not check(system.guid, "User", "Guid"):
                GlobalCounter.U2_ASC_DIFF_GUID_SUMM.increment()

            laas_region = value_by_path(event.payload, ("User", "LaasRegion"))
            if laas_region is not None:
                laas_region = try_json(laas_region)
                if laas_region is not None:
                    remove_flappy_laas_fields(laas_region)
            expected_laas_region = value_by_path(system.session_data, ("request", "laas_region"))
            if expected_laas_region is not None:
                expected_laas_region = expected_laas_region.copy()
                remove_flappy_laas_fields(expected_laas_region)
            if expected_laas_region != laas_region:
                diffs.append(f"value at User/LaasRegion != '{expected_laas_region}'")
                GlobalCounter.U2_ASC_DIFF_LAAS_SUMM.increment()

            if system.bb_uid4oauth_error is False:
                # if BB fails then system.uid will be left unspecified
                if (system.puid is None):  # otherwise system.uid == system.puid
                    if not check(system.uid, "User", "Yuid"):
                        GlobalCounter.U2_ASC_DIFF_YUID_SUMM.increment()

            # UserOptions
            if not check(system.do_not_use_user_logs, "UserOptions", "DoNotUseLogs"):
                GlobalCounter.U2_ASC_DIFF_NOLOGS_SUMM.increment()

            # Experiments
            check(system.x_yandex_appinfo, "Experiments", "FlagsJson", "AppInfo")

            # Messenger
            check(system.mssngr_version, "Messenger", "Version")
            check(system.fanout_auth, "Messenger", "FanoutAuth")
            check(self._comparable_messenger_user_type(system), "Messenger", "UserType")

            if not diffs:
                GlobalCounter.U2_ASC_NODIFF_SUMM.increment()
            else:
                system.logger.log_directive({"type": "ApplySessionContext", "ForEvent": event.message_id, "diff": diffs})
        except Exception:
            self.EXC("Validation of ApplySessionContext failed with an exeption")

        self.close()


@register_event_processor
class ContextLoadDiffCheck(EventProcessor):

    @staticmethod
    def fix_quasar_iot_content(content):
        try:
            resp_proto = TIoTUserInfo()
            resp_proto.ParseFromString(content)
            value = json.loads(resp_proto.RawUserInfo)
        except:
            return content

        # this field is always different uuid
        if "request_id" in value:
            value["request_id"] = "***"

        # sort inner lists
        def sort_recursively(value):
            if isinstance(value, list):
                for item in value:
                    sort_recursively(item)
                value.sort(key=lambda item: json.dumps(item, sort_keys=True))
            elif isinstance(value, dict):
                for key, item in value.items():
                    sort_recursively(item)

        sort_recursively(value)
        return json.dumps(value, sort_keys=True)

    @staticmethod
    def fix_datasync_content(content):
        try:
            value = json.loads(content)

            for item in value.get("items", {}):
                if isinstance(item, dict) and item.get("headers", {}).get("Yandex-Cloud-Request-ID"):
                    del item["headers"]["Yandex-Cloud-Request-ID"]

            return json.dumps(value, sort_keys=True)
        except:
            return content

    def process_event(self, event):
        super().process_event(event)
        system = self.system
        if not hasattr(system, "responses_storage"):
            self.close()
            return

        payload = event.payload
        reqid = payload.get("requestId")
        response_base64 = payload.get("response")

        if reqid is None or response_base64 is None:
            self.close()
            return

        try:
            GlobalCounter.U2_CONTEXT_LOAD_DIFF_CHECK_SUMM.increment()

            response = TContextLoadResponse()
            response.MergeFromString(base64.b64decode(response_base64))

            diffs = []  # list of different fields
            storage = system.responses_storage.load(reqid) or {}

            def check(storage_hash, proto_field):
                expected = storage.get(storage_hash)  # uniproxy1 answer
                actual = getattr(response, proto_field)  # context_load answer

                if expected is None:
                    # the item is missing at uniproxy1 (but may present at context_load) - it's okay
                    return True

                expected_content = expected.body
                actual_content = actual.Content

                if isinstance(expected_content, bytes):
                    try:
                        decoded = expected_content.decode()
                        expected_content = decoded
                    except:
                        pass

                if isinstance(actual_content, bytes):
                    try:
                        decoded = actual_content.decode()
                        actual_content = decoded
                    except:
                        pass

                if storage_hash == "quasar_iot" and proto_field == "QuasarIotResponse":
                    expected_content = self.fix_quasar_iot_content(expected_content)
                    actual_content = self.fix_quasar_iot_content(actual_content)

                if storage_hash.startswith("datasync") and proto_field.startswith("Datasync"):
                    expected_content = self.fix_datasync_content(expected_content)
                    actual_content = self.fix_datasync_content(actual_content)

                if expected.code != actual.StatusCode or expected_content != actual_content:
                    diff = {
                        "hash": storage_hash,
                        "expected": {
                            "code": expected.code,
                            "body": expected_content,
                        },
                        "actual": {
                            "code": actual.StatusCode,
                            "body": actual_content,
                        },
                    }

                    try:
                        json.dumps(diff)
                    except:
                        diff["expected"]["body"] = "<hidden body>"
                        diff["actual"]["body"] = "<hidden body>"

                    diffs.append(diff)
                    return False

                return True

            def check_megamind_session(storage_hash, proto_field):
                expected = storage.get(storage_hash)  # uniproxy1 answer
                actual = getattr(response, proto_field)  # context_load answer
                if expected is None:
                    return True

                # "expected" is from uniproxy1, is encoded (base64) string
                # "actual" is from apphost, is NCachalotProtocol.TResponse object
                actual_encoded = base64.b64encode(actual.MegamindSessionLoadResp.Data).decode("ascii")
                if expected != actual_encoded:
                    diff = {
                        "hash": storage_hash,
                        "expected": {
                            "length": len(expected),
                        },
                        "actual": {
                            "length": len(actual_encoded),
                        },
                    }
                    diffs.append(diff)
                    return False

                return True

            if not check("memento", "MementoResponse"):
                GlobalCounter.U2_CLD_DIFF_MEMENTO_SUMM.increment()
            if not check("datasync", "DatasyncResponse"):
                GlobalCounter.U2_CLD_DIFF_DATASYNC_SUMM.increment()
            if not check("datasync_device_id", "DatasyncDeviceIdResponse"):
                GlobalCounter.U2_CLD_DIFF_DATASYNC_DEVICE_ID_SUMM.increment()
            if not check("datasync_uuid", "DatasyncUuidResponse"):
                GlobalCounter.U2_CLD_DIFF_DATASYNC_UUID_SUMM.increment()
            if not check("quasar_iot", "QuasarIotResponse"):
                GlobalCounter.U2_CLD_DIFF_QUASAR_IOT_SUMM.increment()
            if not check("notificator", "NotificatorResponse"):
                GlobalCounter.U2_CLD_DIFF_NOTIFICATOR_SUMM.increment()
            if not check_megamind_session("megamind_session", "MegamindSessionResponse"):
                GlobalCounter.U2_CLD_DIFF_MEGAMIND_SESSION_SUMM.increment()

            if not diffs:
                GlobalCounter.U2_CLD_NODIFF_SUMM.increment()
            else:
                system.logger.log_directive({"type": "ContextLoadDiff", "ForEvent": event.message_id, "diff": diffs})
        except Exception as exc:
            system.logger.log_directive({"type": "ContextLoadDiff", "ForEvent": event.message_id, "diff": str(exc)})
            self.EXC("Validation of ContextLoad failed with an exception")

        self.close()
