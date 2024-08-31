import pytest

from alice.matrix.notificator.tests.library.constants import ServiceHandlers
from alice.matrix.notificator.tests.library.test_base import MatrixTestBase

from alice.uniproxy.library.protos.notificator_pb2 import (
    TManageSubscription,
)
from alice.matrix.notificator.tests.library.proto_builder_helpers import (
    get_manage_subscription,
    get_push_message,
)

from alice.protos.data.device.info_pb2 import EUserDeviceType

from google.protobuf import json_format


def _get_user_subscriptions(matrix, puid, only_subscribed=True):
    subscriptions_response = matrix.perform_subscriptions_get_request(puid).json()
    assert subscriptions_response["code"] == 200
    result = list(sorted([(subscription_info["id"], subscription_info["subscribed"]) for subscription_info in subscriptions_response["payload"]["subscriptions"]]))

    if only_subscribed:
        result = [(id, subscribed) for id, subscribed in result if subscribed]

    return result


def _get_user_devices(matrix, puid, only_subscribed=True):
    subscriptions_devices_response = matrix.perform_subscriptions_devices_get_request(puid, "fake").json()
    assert subscriptions_devices_response["code"] == 200
    result = list(sorted([(device_info["quasar_info"]["device_id"], device_info["subscribed"]) for device_info in subscriptions_devices_response["payload"]["devices"]]))

    if only_subscribed:
        result = [(id, subscribed) for id, subscribed in result if subscribed]

    return result


def _perform_subscriptions_manage_request(
    matrix,
    subscriptions_manage_request,
    request_mode="protobuf",
    raise_for_status=True,
):
    if request_mode == "protobuf":
        return matrix.perform_subscriptions_manage_request(subscriptions_manage_request, raise_for_status=raise_for_status)
    elif request_mode == "json":
        return matrix.perform_post_request(
            ServiceHandlers.HTTP_SUBSCRIPTIONS_MANAGE,
            json={
                "puid": subscriptions_manage_request.Puid,
                "subscription_id": subscriptions_manage_request.SubscriptionId,
                "method": "subscribe" if subscriptions_manage_request.Method == TManageSubscription.EMethod.ESubscribe else "unsubscribe",
            },
            raise_for_status=raise_for_status,
        )
    elif request_mode == "cgi":
        return matrix.perform_get_request(
            ServiceHandlers.HTTP_SUBSCRIPTIONS_MANAGE,
            params={
                "puid": subscriptions_manage_request.Puid,
                "subscription_id": subscriptions_manage_request.SubscriptionId,
                "method": "subscribe" if subscriptions_manage_request.Method == TManageSubscription.EMethod.ESubscribe else "unsubscribe",
            },
            raise_for_status=raise_for_status,
        )
    else:
        assert False, f"Unknown request mode {request_mode}"


class TestSubscriptions(MatrixTestBase):

    @pytest.mark.asyncio
    @pytest.mark.parametrize("request_mode", [
        "protobuf",
        "json",
        "cgi",
    ])
    async def test_subscriptions_manage(
        self,
        matrix,
        puid,
        request_mode,
    ):
        assert _get_user_subscriptions(matrix, puid) == []
        all_subscriptions = [id for id, subscribed in _get_user_subscriptions(matrix, puid, only_subscribed=False)]
        assert len(all_subscriptions) > 0

        # Check that subscriptions of different users are independent
        assert _perform_subscriptions_manage_request(matrix, get_manage_subscription(f"{puid}_other", all_subscriptions[0], TManageSubscription.EMethod.ESubscribe), request_mode).json()["code"] == 200
        assert _get_user_subscriptions(matrix, puid) == []

        for i in range(len(all_subscriptions)):
            assert _perform_subscriptions_manage_request(matrix, get_manage_subscription(puid, all_subscriptions[i], TManageSubscription.EMethod.ESubscribe), request_mode).json()["code"] == 200
            assert _get_user_subscriptions(matrix, puid) == [(all_subscriptions[j], True) for j in range(i + 1)]

        for i in range(len(all_subscriptions)):
            assert _perform_subscriptions_manage_request(matrix, get_manage_subscription(puid, all_subscriptions[i], TManageSubscription.EMethod.EUnsubscribe), request_mode).json()["code"] == 200
            assert _get_user_subscriptions(matrix, puid) == [(all_subscriptions[j], True) for j in range(i + 1, len(all_subscriptions))]

    @pytest.mark.asyncio
    @pytest.mark.parametrize("request_mode", [
        "protobuf",
        "json",
        "cgi",
    ])
    @pytest.mark.parametrize("error_type, error_message", [
        ("no_puid", b"Puid is empty"),
        ("unkonwn_subscription_id", b"Unknown subscription id"),
    ])
    async def test_invalid_subscriptions_manage(
        self,
        matrix,
        puid,
        request_mode,
        error_type,
        error_message,
    ):
        manage_subscription_request = get_manage_subscription(puid, 1, TManageSubscription.EMethod.ESubscribe)

        if error_type == "no_puid":
            manage_subscription_request.ClearField("Puid")
        elif error_type == "unkonwn_subscription_id":
            manage_subscription_request.SubscriptionId = 0
        else:
            assert False, f"Unknown error_type {error_type}"

        http_response = _perform_subscriptions_manage_request(matrix, manage_subscription_request, request_mode, raise_for_status=False)
        assert http_response.status_code == 400
        assert error_message in http_response.content

    @pytest.mark.asyncio
    async def test_subscriptions_manage_protobuf_parse_error(
        self,
        matrix,
        puid,
    ):
        http_response = matrix.perform_post_request(
            ServiceHandlers.HTTP_SUBSCRIPTIONS_MANAGE,
            data="qwe",
            headers={
                "Content-Type": "application/protobuf"
            },
            raise_for_status=False,
        )

        assert http_response.status_code == 400
        assert b"Unable to parse proto" in http_response.content

    @pytest.mark.asyncio
    @pytest.mark.parametrize("error_type, error_message", [
        ("no_puid", b"'puid' param not found"),
        ("no_subscription_id", b"'subscription_id' param not found"),
        ("bad_subscription_id", b"'subscription_id' param not found"),
        ("no_method", b"'method' param not found"),
        ("empty_json", b"'puid' param not found; 'subscription_id' param not found; 'method' param not found"),
        ("bad_json", b"NJson::TJsonException"),
    ])
    async def test_subscriptions_manage_json_parse_error(
        self,
        matrix,
        puid,
        error_type,
        error_message,
    ):
        json = {
            "puid": puid,
            "subscription_id": 1,
            "method": "subscribe",
        }

        if error_type == "no_puid":
            del json["puid"]
        elif error_type == "no_subscription_id":
            del json["subscription_id"]
        elif error_type == "bad_subscription_id":
            json["subscription_id"] = "not_int"
        elif error_type == "no_method":
            del json["method"]
        elif error_type == "empty_json":
            json = dict()
        elif error_type == "bad_json":
            pass
        else:
            assert False, f"Unknown error_type {error_type}"

        if error_type != "bad_json":
            http_response = matrix.perform_post_request(
                ServiceHandlers.HTTP_SUBSCRIPTIONS_MANAGE,
                json=json,
                raise_for_status=False,
            )
        else:
            http_response = matrix.perform_post_request(
                ServiceHandlers.HTTP_SUBSCRIPTIONS_MANAGE,
                data="qwe",
                headers={
                    "Content-Type": "application/json"
                },
                raise_for_status=False,
            )

        assert http_response.status_code == 400
        assert error_message in http_response.content

    @pytest.mark.asyncio
    @pytest.mark.parametrize("error_type, error_message", [
        ("no_puid", b"'puid' param not found"),
        ("no_subscription_id", b"'subscription_id' param not found"),
        ("bad_subscription_id", b"Unable to cast subscription id \'not_int\' to ui64"),
        ("no_method", b"'method' param not found"),
        ("empty_params", b"'puid' param not found; 'subscription_id' param not found; 'method' param not found"),
    ])
    async def test_subscriptions_manage_cgi_parse_error(
        self,
        matrix,
        puid,
        error_type,
        error_message,
    ):
        params = {
            "puid": puid,
            "subscription_id": 1,
            "method": "subscribe",
        }

        if error_type == "no_puid":
            del params["puid"]
        elif error_type == "no_subscription_id":
            del params["subscription_id"]
        elif error_type == "bad_subscription_id":
            params["subscription_id"] = "not_int"
        elif error_type == "no_method":
            del params["method"]
        elif error_type == "empty_params":
            params = dict()
        else:
            assert False, f"Unknown error_type {error_type}"

        http_response = matrix.perform_get_request(
            ServiceHandlers.HTTP_SUBSCRIPTIONS_MANAGE,
            params=params,
            raise_for_status=False,
        )

        assert http_response.status_code == 400
        assert error_message in http_response.content

    @pytest.mark.asyncio
    @pytest.mark.parametrize("subscribed_to_first_subscription", [False, True])
    async def test_subscriptions_get_without_iot_request(
        self,
        matrix,
        puid,
        subscribed_to_first_subscription,
    ):
        assert matrix.perform_subscriptions_manage_request(
            get_manage_subscription(
                puid,
                1,
                TManageSubscription.EMethod.ESubscribe if subscribed_to_first_subscription else TManageSubscription.EMethod.EUnsubscribe,
            )
        ).json()["code"] == 200

        subscriptions_response = matrix.perform_subscriptions_get_request(puid).json()
        assert subscriptions_response["code"] == 200
        assert {
            "id": 1,
            "name": "Общие",
        } in subscriptions_response["payload"]["categories"]

        assert {
            "id": 1,
            "name": "Дайджест Алисы",
            "description": "Буду иногда рассказывать о том, что интересного происходит во мне и вокруг меня",
            "logo": "https://static-alice.s3.yandex.net/subscriptions/digest_icon.png",
            "category": 1,
            "type": 1,
            "has_demo": True,

            "subscribed": subscribed_to_first_subscription,

            "settings": {
                "device_models": [
                    "yandexstation",
                    "yandexmini",
                    "station_2",
                    "jbl_link_portable",
                    "jbl_link_music",
                    "yandexmicro",
                    "yandexmini_2",
                    "yandexmini_2_no_clock",
                    "prestigio_smart_mate",
                    "yandexmidi",
                ],
                "platforms": [
                    "yandexstation",
                    "yandexmini",
                    "yandexstation_2",
                    "jbl_link_portable",
                    "jbl_link_music",
                    "yandexmicro",
                    "yandexmini_2",
                    "prestigio_smart_mate",
                    "yandexmidi",
                ],
            },
        } in subscriptions_response["payload"]["subscriptions"]
        assert {
            "id": 2,
            "name": "Новые серии с доставкой",
            "description": "Напомню, когда выйдет новый эпизод сериала, который вы смотрите",
            "logo": "https://static-alice.s3.yandex.net/subscriptions/film.png",
            "category": 1,
            "type": 1,
            "has_demo": True,

            # Not subscribed by default
            "subscribed": False,

            "settings": {
                "device_models": [
                    "yandexstation",
                    "station_2",
                ],
                "platforms": [
                    "yandexstation",
                    "yandexstation_2",
                ],
            },
        } in subscriptions_response["payload"]["subscriptions"]

    @pytest.mark.asyncio
    @pytest.mark.parametrize("subscribed_to_first_subscription", [False, True])
    async def test_subscriptions_get_with_iot_request(
        self,
        matrix,
        iot_mock,
        puid,
        device_id,
        subscribed_to_first_subscription,
    ):
        def _check_iot_request(iot_mock, expected_req_count, expected_user_ticket):
            last_service_ticket, last_user_ticket, req_count = iot_mock.get_requests_info()

            assert req_count == expected_req_count
            assert last_service_ticket.startswith("3:serv:")
            assert last_user_ticket == expected_user_ticket

        def _check_subscriptions_response_basic_parts(subscriptions_response):
            assert subscriptions_response["code"] == 200
            assert {
                "id": 1,
                "name": "Общие",
            } in subscriptions_response["payload"]["categories"]

        assert matrix.perform_subscriptions_manage_request(
            get_manage_subscription(
                puid,
                1,
                TManageSubscription.EMethod.ESubscribe if subscribed_to_first_subscription else TManageSubscription.EMethod.EUnsubscribe,
            )
        ).json()["code"] == 200

        iot_mock.set_devices([])
        subscriptions_response = matrix.perform_subscriptions_get_request(puid, user_ticket="fake1").json()
        _check_iot_request(iot_mock, 1, "fake1")
        _check_subscriptions_response_basic_parts(subscriptions_response)
        assert subscriptions_response["payload"]["subscriptions"] == []

        iot_mock.set_devices([
            {
                "id": "system_id1",
                "name": "First device",
                "type": EUserDeviceType.YandexStationDeviceType,
                "room_id": "system_room_id",
                "analytics_type": "devices.types.smart_speaker.yandex.station.station",

                "quasar_id": f"{device_id}_1",
                "quasar_platform": "yandexstation",
            },
            {
                "id": "system_id2",
                "name": "Second device",
                "type": EUserDeviceType.YandexStationMiniDeviceType,
                "room_id": "",
                "analytics_type": "devices.types.smart_speaker.yandex.station.mini",

                "quasar_id": f"{device_id}_2",
                "quasar_platform": "yandexmini",
            },
        ])
        subscriptions_response = matrix.perform_subscriptions_get_request(puid, user_ticket="fake2").json()
        _check_iot_request(iot_mock, 2, "fake2")
        _check_subscriptions_response_basic_parts(subscriptions_response)
        assert {
            "id": 1,
            "name": "Дайджест Алисы",
            "description": "Буду иногда рассказывать о том, что интересного происходит во мне и вокруг меня",
            "logo": "https://static-alice.s3.yandex.net/subscriptions/digest_icon.png",
            "category": 1,
            "type": 1,
            "has_demo": True,

            "subscribed": subscribed_to_first_subscription,

            "settings": {
                "device_models": [
                    "yandexstation",
                    "yandexmini",
                    "station_2",
                    "jbl_link_portable",
                    "jbl_link_music",
                    "yandexmicro",
                    "yandexmini_2",
                    "yandexmini_2_no_clock",
                    "prestigio_smart_mate",
                    "yandexmidi",
                ],
                "platforms": [
                    "yandexstation",
                    "yandexmini",
                ],
            },
        } in subscriptions_response["payload"]["subscriptions"]

        iot_mock.set_devices([
            {
                "id": "system_id1",
                "name": "First device",
                "type": EUserDeviceType.YandexStationMiniDeviceType,
                "room_id": "",
                "analytics_type": "devices.types.smart_speaker.yandex.station.mini",

                "quasar_id": f"{device_id}_1",
                "quasar_platform": "yandexmini",
            },
        ])
        subscriptions_response = matrix.perform_subscriptions_get_request(puid, user_ticket="fake3").json()
        _check_iot_request(iot_mock, 3, "fake3")
        _check_subscriptions_response_basic_parts(subscriptions_response)
        assert {
            "id": 1,
            "name": "Дайджест Алисы",
            "description": "Буду иногда рассказывать о том, что интересного происходит во мне и вокруг меня",
            "logo": "https://static-alice.s3.yandex.net/subscriptions/digest_icon.png",
            "category": 1,
            "type": 1,
            "has_demo": True,

            "subscribed": subscribed_to_first_subscription,

            "settings": {
                "device_models": [
                    "yandexstation",
                    "yandexmini",
                    "station_2",
                    "jbl_link_portable",
                    "jbl_link_music",
                    "yandexmicro",
                    "yandexmini_2",
                    "yandexmini_2_no_clock",
                    "prestigio_smart_mate",
                    "yandexmidi",
                ],
                "platforms": [
                    "yandexmini",
                ],
            },
        } in subscriptions_response["payload"]["subscriptions"]
        # Check that subscription only for devices with HDMI is not in the response
        assert 2 not in map(lambda x: x["id"], subscriptions_response["payload"]["subscriptions"])

    @pytest.mark.asyncio
    @pytest.mark.parametrize("error_type, error_message", [
        ("no_puid", b"'puid' param not found"),
        ("empty_puid", b"Puid is empty"),
    ])
    async def test_invalid_subscriptions_get(
        self,
        matrix,
        puid,
        error_type,
        error_message,
    ):
        if error_type == "no_puid":
            puid = None
        elif error_type == "empty_puid":
            puid = ""
        else:
            assert False, f"Unknown error_type {error_type}"

        http_response = matrix.perform_subscriptions_get_request(puid, raise_for_status=False)

        assert http_response.status_code == 400
        assert error_message in http_response.content

    @pytest.mark.asyncio
    async def test_subscriptions_user_list(
        self,
        matrix,
        puid,
    ):
        assert matrix.perform_subscriptions_manage_request(get_manage_subscription(puid, 1, TManageSubscription.EMethod.ESubscribe)).json()["code"] == 200

        timestamp = None
        for i in range(2):
            user_list = matrix.perform_subscriptions_user_list_request(1, timestamp=timestamp).json()
            assert user_list["code"] == 200

            # Check payload
            payload = user_list["payload"]
            assert puid in payload["users"]
            assert f"{puid}_other" not in payload["users"]
            timestamp = payload["timestamp"] - 1

        assert matrix.perform_subscriptions_user_list_request(1, timestamp=timestamp + 1).json() == {
            "code": 404,
            "payload": {
                "users": [],
                "timestamp": 0,
            }
        }

    @pytest.mark.asyncio
    @pytest.mark.parametrize("error_type, error_message", [
        ("no_subscription_id", b"'subscription_id' param not found"),
        ("bad_subscription_id", b"Unable to cast subscription id \'not_int\' to ui64"),
        ("bad_timestamp", b"Unable to cast timestamp \'not_int\' to ui64"),
    ])
    async def test_invalid_subscriptions_user_list(
        self,
        matrix,
        error_type,
        error_message,
    ):
        if error_type == "no_subscription_id":
            http_response = matrix.perform_subscriptions_user_list_request(None, raise_for_status=False)
        elif error_type == "bad_subscription_id":
            http_response = matrix.perform_subscriptions_user_list_request("not_int", raise_for_status=False)
        elif error_type == "bad_timestamp":
            http_response = matrix.perform_subscriptions_user_list_request(1, timestamp="not_int", raise_for_status=False)
        else:
            assert False, f"Unknown error_type {error_type}"

        assert http_response.status_code == 400
        assert error_message in http_response.content

    @pytest.mark.asyncio
    @pytest.mark.parametrize("first_device_subscribed", [False, True])
    async def test_subscriptions_devices_get(
        self,
        matrix,
        iot_mock,
        puid,
        device_id,
        first_device_subscribed,
    ):
        iot_mock.reset(device_id_base=device_id)
        assert matrix.perform_subscriptions_devices_manage_request(
            puid,
            f"{device_id}_1",
            "subscribe" if first_device_subscribed else "unsubscribe",
        ).json()["code"] == 200

        subscriptions_devices_response = matrix.perform_subscriptions_devices_get_request(puid, "fake_user_ticket").json()

        last_service_ticket, last_user_ticket, req_count = iot_mock.get_requests_info()
        assert req_count == 1
        assert last_service_ticket.startswith("3:serv:")
        assert last_user_ticket == "fake_user_ticket"

        assert subscriptions_devices_response == {
            "code": 200,
            "payload": {
                "devices": [
                    {
                        "id": "system_id1",
                        "name": "First device",
                        "type": "devices.types.smart_speaker.yandex.station.station",
                        "room_id": "system_room_id",

                        "subscribed": first_device_subscribed,

                        "quasar_info": {
                            "device_id": f"{device_id}_1",
                            "platform": "yandexstation"
                        },
                    },
                    {
                        "id": "system_id3",
                        "name": "Third device",
                        "type": "devices.types.smart_speaker.yandex.station.mini",
                        "room_id": "",

                        # Subscribed by default
                        "subscribed": True,

                        "quasar_info": {
                            "device_id": f"{device_id}_3",
                            "platform": "yandexmidi"
                        },
                    }
                ],
                "rooms": [
                    {
                        "id": "system_room_id1",
                        "name": "First room",
                        "household_id": "household_id1",
                    },
                    {
                        "id": "system_room_id2",
                        "name": "Second room",
                        "household_id": "household_id2",
                    },
                ],
            }
        }

    @pytest.mark.asyncio
    @pytest.mark.parametrize("error_type, error_message", [
        ("no_puid", b"'puid' param not found"),
        ("empty_puid", b"Puid is empty"),
        ("no_user_ticket", b"User ticket not provide"),
        ("no_service_ticket", b"Service ticket not provide"),
    ])
    async def test_invalid_subscriptions_devices_get(
        self,
        matrix,
        puid,
        error_type,
        error_message,
    ):
        if error_type == "no_puid":
            http_response = matrix.perform_subscriptions_devices_get_request(None, "user_ticket", raise_for_status=False)
        elif error_type == "empty_puid":
            http_response = matrix.perform_subscriptions_devices_get_request("", "user_ticket", raise_for_status=False)
        elif error_type == "no_user_ticket":
            http_response = matrix.perform_subscriptions_devices_get_request(puid, None, raise_for_status=False)
        elif error_type == "no_service_ticket":
            http_response = matrix.perform_subscriptions_devices_get_request(puid, "user_ticket", service_ticket=None, raise_for_status=False)
        else:
            assert False, f"Unknown error_type {error_type}"

        assert http_response.status_code == (403 if error_type == "no_service_ticket" else 400)
        assert error_message in http_response.content

    @pytest.mark.asyncio
    async def test_subscriptions_devices_manage(
        self,
        matrix,
        iot_mock,
        puid,
        device_id,
    ):
        device_ids = [f"{device_id}_{i}" for i in range(5)]
        iot_mock.set_devices(
            [
                {
                    "id": f"system_id_{device_id}",
                    "name": f"First device {device_id}",
                    "type": EUserDeviceType.YandexStationDeviceType,
                    "room_id": "",
                    "analytics_type": "devices.types.smart_speaker.yandex.station.station",

                    "quasar_id": device_id,
                    "quasar_platform": "yandexstation",
                }
                for device_id in device_ids
            ]
        )

        # Check that subscriptions of different users are independent
        assert matrix.perform_subscriptions_devices_manage_request(f"{puid}_other", device_ids[0], "unsubscribe").json() == {
            "code": 200,
            "payload": {
                "status": "unsubscribed",
            }
        }
        assert _get_user_devices(matrix, puid, only_subscribed=False) == [(device_id, True) for device_id in device_ids]

        for i in range(len(device_ids)):
            assert matrix.perform_subscriptions_devices_manage_request(puid, device_ids[i], "unsubscribe").json() == {
                "code": 200,
                "payload": {
                    "status": "unsubscribed",
                }
            }
            assert _get_user_devices(matrix, puid) == [(device_ids[j], True) for j in range(i + 1, len(device_ids))]

        for i in range(len(device_ids)):
            assert matrix.perform_subscriptions_devices_manage_request(puid, device_ids[i], "subscribe").json() == {
                "code": 200,
                "payload": {
                    "status": "subscribed",
                }
            }
            assert _get_user_devices(matrix, puid) == [(device_ids[j], True) for j in range(i + 1)]

    @pytest.mark.asyncio
    @pytest.mark.parametrize("method", [
        "subscribe",
        "unsubscribe",
    ])
    @pytest.mark.parametrize("error_type, error_message", [
        ("no_puid", b"'puid' param not found"),
        ("empty_puid", b"Puid is empty"),
        ("no_device_id", b"'device_id' param not found"),
        ("empty_device_id", b"Device id is empty"),
        ("no_method", b"'method' param not found"),
        ("no_service_ticket", b"Service ticket not provide"),
    ])
    async def test_invalid_subscriptions_devices_manage(
        self,
        matrix,
        puid,
        device_id,
        method,
        error_type,
        error_message,
    ):
        if error_type == "no_puid":
            http_response = matrix.perform_subscriptions_devices_manage_request(None, device_id, method, raise_for_status=False)
        elif error_type == "empty_puid":
            http_response = matrix.perform_subscriptions_devices_manage_request("", device_id, method, raise_for_status=False)
        elif error_type == "no_device_id":
            http_response = matrix.perform_subscriptions_devices_manage_request(puid, None, method, raise_for_status=False)
        elif error_type == "empty_device_id":
            http_response = matrix.perform_subscriptions_devices_manage_request(puid, "", method, raise_for_status=False)
        elif error_type == "no_method":
            http_response = matrix.perform_subscriptions_devices_manage_request(puid, device_id, None, raise_for_status=False)
        elif error_type == "no_service_ticket":
            http_response = matrix.perform_subscriptions_devices_manage_request(puid, device_id, method, service_ticket=None, raise_for_status=False)
        else:
            assert False, f"Unknown error_type {error_type}"

        assert http_response.status_code == (403 if error_type == "no_service_ticket" else 400)
        assert error_message in http_response.content

    @pytest.mark.asyncio
    async def test_subscriptions_devices_manage_send_state_to_device(
        self,
        matrix,
        subway_mock,
        puid,
        device_id,
    ):
        await matrix.register_connection(puid, device_id)

        push_message = get_push_message(puid, device_id)

        # Subscribe
        assert matrix.perform_subscriptions_manage_request(get_manage_subscription(puid, push_message.SubscriptionId, TManageSubscription.EMethod.ESubscribe)).json()["code"] == 200

        delivery_response = matrix.perform_delivery_request(push_message).json()
        assert delivery_response["code"] == 200
        notification_id = delivery_response["id"]

        subway_request, req_count = subway_mock.get_requests_info()
        assert req_count == 1
        assert len(subway_request.QuasarMsg.SkDirectives) == 1
        assert json_format.MessageToDict(subway_request.QuasarMsg.SkDirectives[0]) == {
            "name": "notify",
            "payload": {
                "notifications": [
                    {
                        "id": notification_id,
                        "text": push_message.Notification.Text,
                        "subscription_id": "1",
                    }
                ],
                "version_id": "0",
                "ring": "Delicate",
            }
        }

        assert matrix.perform_subscriptions_devices_manage_request(puid, device_id, "unsubscribe").json()["code"] == 200

        subway_request, req_count = subway_mock.get_requests_info()
        assert req_count == 2
        assert len(subway_request.QuasarMsg.SkDirectives) == 1
        assert json_format.MessageToDict(subway_request.QuasarMsg.SkDirectives[0]) == {
            "name": "notify",
            "payload": {
                "version_id": "1",
            }
        }

        assert matrix.perform_subscriptions_devices_manage_request(puid, device_id, "subscribe").json()["code"] == 200

        subway_request, req_count = subway_mock.get_requests_info()
        assert req_count == 3
        assert len(subway_request.QuasarMsg.SkDirectives) == 1
        assert json_format.MessageToDict(subway_request.QuasarMsg.SkDirectives[0]) == {
            "name": "notify",
            "payload": {
                "notifications": [
                    {
                        "id": notification_id,
                        "text": push_message.Notification.Text,
                        "subscription_id": "1",
                    }
                ],
                "version_id": "2",
            }
        }


class TestSubscriptionsMockMode(MatrixTestBase):
    matrix_pushes_and_notifications_mock_mode = True

    @pytest.mark.asyncio
    async def test_subscriptions_devices_manage_send_state_to_device(
        self,
        matrix,
        subway_mock,
        puid,
        device_id,
    ):
        await matrix.register_connection(puid, device_id)

        push_message = get_push_message(puid, device_id)

        # Subscribe
        assert matrix.perform_subscriptions_manage_request(get_manage_subscription(puid, push_message.SubscriptionId, TManageSubscription.EMethod.ESubscribe)).json()["code"] == 200

        matrix.perform_delivery_request(push_message).json()["code"] == 200
        assert subway_mock.get_requests_count() == 0

        assert matrix.perform_subscriptions_devices_manage_request(puid, device_id, "unsubscribe").json()["code"] == 200
        assert subway_mock.get_requests_count() == 0

        assert matrix.perform_subscriptions_devices_manage_request(puid, device_id, "subscribe").json()["code"] == 200
        assert subway_mock.get_requests_count() == 0
