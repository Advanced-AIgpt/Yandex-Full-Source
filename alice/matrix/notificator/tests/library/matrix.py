import yatest.common
import time

from .constants import ItemTypes, ServiceHandlers
from .proto_builder_helpers import (
    get_client_state_change_connect,
    get_uniproxy_endpoint,
    get_update_connected_clients_request,
    get_user_device_info,

    get_device_locator,
)
from .ydb import init_matrix_ydb

from alice.megamind.protos.scenarios.notification_state_pb2 import (
    TNotificationState,
)
from alice.protos.api.matrix.client_connections_pb2 import (
    TUpdateConnectedClientsResponse,
)
from alice.protos.api.matrix.delivery_pb2 import (
    TDeliveryResponse,
)
from alice.protos.api.notificator.api_pb2 import (
    TDirectiveStatusResponse,
    TGetDevicesResponse,
)

from alice.matrix.library.testing.python.servant_base import ServantBase


class Matrix(ServantBase):
    def __init__(
        self,

        tvmtool_authtoken=None,
        tvmtool_port=None,

        iot_port=None,
        python_notificator_port=None,
        subway_port=None,

        disable_ydb_operations_in_locator_service=False,
        pushes_and_notifications_mock_mode=False,
        user_white_list=None,

        env={},
        *args,
        **kwargs,
    ):
        env.update({
            "TVMTOOL_LOCAL_AUTHTOKEN": tvmtool_authtoken,
        })
        super(Matrix, self).__init__(env=env, *args, **kwargs)

        self._tvmtool_port = tvmtool_port

        self._iot_port = iot_port
        self._python_notificator_port = python_notificator_port
        self._subway_port = subway_port

        self._disable_ydb_operations_in_locator_service = disable_ydb_operations_in_locator_service
        self._pushes_and_notifications_mock_mode = pushes_and_notifications_mock_mode
        self._user_white_list = user_white_list

    def _get_config(self):
        config = self._get_default_config()
        default_service_config = {
            "YDBClient": self._get_default_ydb_client_config(),
        }

        config["RtLog"]["Service"] = "matrix_notificator"
        config.update({
            "Neh": {
                "ProtocolOptions": [
                    {
                        "Key": "post/ConnectTimeout",
                        "Value": "25ms"
                    }
                ]
            },
            "IoTClient": {
                "Host": "localhost",
                "Port": self._iot_port or 0,
            },
            "PushesAndNotificationsClient": {
                "YDBClient": self._get_default_ydb_client_config(),
                "MockMode": self._pushes_and_notifications_mock_mode,
            },
            "SubwayClient": {
                "HardcodedForTestsHostOrIp" : "127.0.0.1",
                "HardcodedForTestsPort": self._subway_port or 0,
            },
            "TvmClient": {
                "TvmTool": {
                    "Port": self._tvmtool_port or 0,
                },
            },


            "DeliveryService": {},
            "DevicesService": default_service_config,
            "DirectiveService": default_service_config,
            "GDPRService": {},
            "LocatorService": {
                "YDBClient": self._get_default_ydb_client_config(),
                "DisableYDBOperations": self._disable_ydb_operations_in_locator_service,
            },
            "NotificationsService": default_service_config,
            "SubscriptionsService": {},
            "ProxyService": {
                "Address": {
                    "Host": "http://localhost",
                    "Port": self._python_notificator_port or 0,
                },
                "DestinationServiceName": "python-notificator",
                "Timeout": {
                    "DefaultTimeout": "10s",
                },
                "ProxyPingRequest": True,
            },
            "UpdateConnectedClientsService": default_service_config,
            "UpdateDeviceEnvironmentService": {},
        })

        if self._user_white_list is not None:
            config["UserWhiteList"] = {
                "Enabled": True,
                "Puids": self._user_white_list,
            }

        return config

    def _before_start(self):
        init_matrix_ydb()

    @property
    def name(self):
        return "matrix"

    @property
    def _binary_file_path(self):
        return yatest.common.build_path("alice/matrix/notificator/bin/matrix")

    async def register_connection(
        self,
        puid,
        device_id,
        shard_id=0,
        ip="127.0.0.1",
        device_model="yandexstation",
        port=123,
        use_old_connections_storage=False,
    ):
        update_connected_clients_response = await self.perform_grpc_request(
            items={
                ItemTypes.CONNECTED_CLIENTS_UPDATE_REQUEST: [
                    get_update_connected_clients_request(
                        get_uniproxy_endpoint(ip, port),
                        int(time.time() * 10**6),
                        shard_id,
                        client_state_changes=[
                            get_client_state_change_connect(get_user_device_info(puid, device_id, device_model=device_model)),
                        ],
                    ),
                ]
            },
            path=ServiceHandlers.UPDATE_CONNECTED_CLIENTS,
            timeout=5,
        )
        assert not update_connected_clients_response.has_exception(), f"Request failed with exception: {update_connected_clients_response.get_exception()}"
        assert len(
            list(
                update_connected_clients_response.get_item_datas(
                    item_type=ItemTypes.CONNECTED_CLIENTS_UPDATE_RESPONSE,
                    proto_type=TUpdateConnectedClientsResponse,
                )
            )
        ) == 1

        if use_old_connections_storage:
            self.perform_locator_add_request(get_device_locator(puid, device_id, ip=ip, device_model=device_model))

    def perform_locator_add_request(self, locator_request, raise_for_status=True):
        return self.perform_post_request(
            ServiceHandlers.HTTP_LOCATOR,
            data=locator_request.SerializeToString(),
            raise_for_status=raise_for_status,
        )

    def perform_sensors_request(self, path, response_format, raise_for_status=True):
        return self.perform_get_request(
            path,
            params={
                "format": response_format,
            },
            raise_for_status=raise_for_status,
        )

    def perform_locator_remove_request(self, locator_request, raise_for_status=True):
        return self.perform_delete_request(
            ServiceHandlers.HTTP_LOCATOR,
            data=locator_request.SerializeToString(),
            raise_for_status=raise_for_status,
        )

    def perform_get_devices_request(self, get_devices_request, raise_for_status=True, need_raw_response=False):
        raw_result = self.perform_post_request(
            ServiceHandlers.HTTP_DEVICES,
            data=get_devices_request.SerializeToString(),
            raise_for_status=raise_for_status,
        )
        get_devices_response = TGetDevicesResponse()
        try:
            get_devices_response.ParseFromString(raw_result.content)
        except:
            if not need_raw_response:
                raise
            else:
                get_devices_response = TNotificationState()

        return (get_devices_response, raw_result) if need_raw_response else get_devices_response

    def perform_delivery_request(self, delivery_request, raise_for_status=True):
        return self.perform_post_request(
            ServiceHandlers.HTTP_DELIVERY,
            data=delivery_request.SerializeToString(),
            raise_for_status=raise_for_status,
        )

    def perform_delivery_demo_request(self, puid, subscription_id, raise_for_status=True):
        params = dict()

        for key, value in [
            ("puid", puid),
            ("subscription_id", subscription_id)
        ]:
            if value is not None:
                params[key] = value

        return self.perform_get_request(
            ServiceHandlers.HTTP_DELIVERY_DEMO,
            params=params,
            raise_for_status=raise_for_status,
        )

    def perform_delivery_push_request(self, delivery_push_request, raise_for_status=True, need_raw_response=False):
        raw_result = self.perform_post_request(
            ServiceHandlers.HTTP_DELIVERY_PUSH,
            data=delivery_push_request.SerializeToString(),
            raise_for_status=raise_for_status,
        )
        delivery_push_response = TDeliveryResponse()
        delivery_push_response.ParseFromString(raw_result.content)
        return (delivery_push_response, raw_result) if need_raw_response else delivery_push_response

    def perform_delivery_on_connect_request(self, delivery_on_connect_request, raise_for_status=True):
        return self.perform_post_request(
            ServiceHandlers.HTTP_DELIVERY_ON_CONNECT,
            data=delivery_on_connect_request.SerializeToString(),
            raise_for_status=raise_for_status,
        )

    def perform_directive_status_request(self, directive_status_request, raise_for_status=True, need_raw_response=False):
        raw_result = self.perform_post_request(
            ServiceHandlers.HTTP_DIRECTIVE_STATUS,
            data=directive_status_request.SerializeToString(),
            raise_for_status=raise_for_status,
        )
        directive_status_response = TDirectiveStatusResponse()
        directive_status_response.ParseFromString(raw_result.content)
        return (directive_status_response, raw_result) if need_raw_response else directive_status_response

    def perform_directive_change_status_request(self, directive_change_status_request, raise_for_status=True):
        return self.perform_post_request(
            ServiceHandlers.HTTP_DIRECTIVE_CHANGE_STATUS,
            data=directive_change_status_request.SerializeToString(),
            raise_for_status=raise_for_status,
        )

    def perform_subscriptions_manage_request(self, subscriptions_manage_request, raise_for_status=True):
        return self.perform_post_request(
            ServiceHandlers.HTTP_SUBSCRIPTIONS_MANAGE,
            data=subscriptions_manage_request.SerializeToString(),
            raise_for_status=raise_for_status,
        )

    def perform_subscriptions_get_request(self, puid, user_ticket=None, raise_for_status=True):
        params = dict()
        headers = dict()

        for key, value in [
            ("puid", puid),
        ]:
            if value is not None:
                params[key] = value

        for key, value in [
            ("X-Ya-User-Ticket", user_ticket),
        ]:
            if value is not None:
                headers[key] = value

        return self.perform_get_request(
            ServiceHandlers.HTTP_SUBSCRIPTIONS,
            params=params,
            headers=headers,
            raise_for_status=raise_for_status,
        )

    def perform_subscriptions_user_list_request(self, subscription_id, timestamp=None, raise_for_status=True):
        params = dict()

        for key, value in [
            ("subscription_id", subscription_id),
            ("timestamp", timestamp),
        ]:
            if value is not None:
                params[key] = value

        return self.perform_get_request(
            ServiceHandlers.HTTP_SUBSCRIPTIONS_USER_LIST,
            params=params,
            raise_for_status=raise_for_status,
        )

    def perform_subscriptions_devices_get_request(self, puid, user_ticket, service_ticket="fake", raise_for_status=True):
        params = dict()
        headers = dict()

        for key, value in [
            ("puid", puid),
        ]:
            if value is not None:
                params[key] = value

        for key, value in [
            ("X-Ya-User-Ticket", user_ticket),
            ("X-Ya-Service-Ticket", service_ticket),
        ]:
            if value is not None:
                headers[key] = value

        return self.perform_get_request(
            ServiceHandlers.HTTP_SUBSCRIPTIONS_DEVICES,
            params=params,
            headers=headers,
            raise_for_status=raise_for_status,
        )

    def perform_subscriptions_devices_manage_request(self, puid, device_id, method, service_ticket="fake", raise_for_status=True):
        json = dict()
        headers = dict()

        for key, value in [
            ("puid", puid),
            ("device_id", device_id),
            ("method", method),
        ]:
            if value is not None:
                json[key] = value

        for key, value in [
            ("X-Ya-Service-Ticket", service_ticket),
        ]:
            if value is not None:
                headers[key] = value

        return self.perform_post_request(
            ServiceHandlers.HTTP_SUBSCRIPTIONS_DEVICES,
            json=json,
            headers=headers,
            raise_for_status=raise_for_status,
        )

    def perform_notifications_request(self, puid, device_id=None, device_model="yandexstation", raise_for_status=True, need_raw_response=False):
        params = dict()

        for key, value in [
            ("puid", puid),
            ("device_id", device_id),
            ("device_model", device_model),
        ]:
            if value is not None:
                params[key] = value

        raw_result = self.perform_get_request(
            ServiceHandlers.HTTP_NOTIFICATIONS,
            params=params,
            raise_for_status=raise_for_status,
        )
        notifications_response = TNotificationState()
        try:
            notifications_response.ParseFromString(raw_result.content)
        except:
            if not need_raw_response:
                raise
            else:
                notifications_response = TNotificationState()

        return (notifications_response, raw_result) if need_raw_response else notifications_response

    def perform_notifications_change_status_request(self, notifications_change_status_request, raise_for_status=True):
        return self.perform_post_request(
            ServiceHandlers.HTTP_NOTIFICATIONS_CHANGE_STATUS,
            data=notifications_change_status_request.SerializeToString(),
            raise_for_status=raise_for_status,
        )

    def perform_gdpr_request(self, puid, raise_for_status=True):
        params = dict()

        for key, value in [
            ("puid", puid),
        ]:
            if value is not None:
                params[key] = value

        return self.perform_delete_request(
            ServiceHandlers.HTTP_GDPR,
            params=params,
            raise_for_status=raise_for_status,
        )
