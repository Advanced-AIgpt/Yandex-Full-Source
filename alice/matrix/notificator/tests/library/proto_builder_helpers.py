import json
import time
import uuid

from alice.megamind.protos.common.atm_pb2 import TAnalyticsTrackingModule
from alice.megamind.protos.scenarios.notification_state_pb2 import TNotification
from alice.megamind.protos.speechkit.directives_pb2 import TDirective as TSpeechKitDirective
from alice.matrix.notificator.library.storages.connections.protos.device_info_pb2 import TDeviceInfo as TInternalDeviceInfo
from alice.protos.api.matrix.client_connections_pb2 import (
    TUpdateConnectedClientsRequest,

    TClientStateChange,
    TClientsDiff,
    TClientsFullInfo,
    TUniproxyEndpoint,
)
from alice.protos.api.matrix.device_environment_pb2 import (
    EDeviceEnvironmentType,

    TUpdateDeviceEnvironmentRequest,
)
from alice.protos.api.matrix.delivery_pb2 import TDelivery
from alice.protos.api.notificator.api_pb2 import (
    TChangeStatus,
    TDirectiveStatus,
    TGetDevicesRequest,
)
from alice.protos.api.matrix.user_device_pb2 import (
    TUserDeviceIdentifier,
    TUserDeviceInfo,
)
from alice.uniproxy.library.protos.notificator_pb2 import (
    TDeliveryOnConnect,
    TDeviceConfig as TDeviceLocatorDeviceConfig,
    TDeviceLocator,
    TManageSubscription,
    TNotificationChangeStatus,
)
from alice.uniproxy.library.protos.uniproxy_pb2 import TPushMessage

from google.protobuf import json_format
from google.protobuf.struct_pb2 import Struct as ProtoStruct


def get_device_locator(
    puid,
    device_id,
    ip="127.0.0.1",
    device_model="yandexstation",
    supported_features=None,
):
    return TDeviceLocator(
        Puid=puid,
        DeviceId=device_id,
        # We use ip now
        # However t's hard to rename this field
        Host=ip,
        Timestamp=int(time.time() * 10**6),
        DeviceModel=device_model,
        Config=TDeviceLocatorDeviceConfig(
            SupportedFeatures=supported_features or [],
        ),
    )


def get_get_devices(
    puid,
    supported_features=None,
):
    return TGetDevicesRequest(
        Puid=puid,
        SupportedFeatures=supported_features or [],
    )


def get_speech_kit_directive():
    return TSpeechKitDirective(
        Type="custom",
        Name="test",
        Payload=json_format.Parse(
            json.dumps({
                "speech": "kit",
                "directive": 5,
            }),
            ProtoStruct()
        ),
    )


def get_delivery(puid, device_id, ttl=86400, push_id=None, speech_kit_directive=None):
    result = TDelivery(
        Puid=puid,
        DeviceId=device_id,
        Ttl=ttl,
    )

    if push_id is not None:
        result.PushId = push_id

    if speech_kit_directive is None:
        result.SemanticFrameRequestData.TypedSemanticFrame.SoundSetLevelSemanticFrame.Level.NumLevelValue = 0
        result.SemanticFrameRequestData.Analytics.Origin = TAnalyticsTrackingModule.EOrigin.Scenario
        result.SemanticFrameRequestData.Analytics.Purpose = "sound_set_level"
    else:
        result.SpeechKitDirective = speech_kit_directive.SerializeToString()

    return result


def get_delivery_on_connect(puid, device_id, ip="127.0.0.1", device_model="yandexstation"):
    return TDeliveryOnConnect(
        Puid=puid,
        DeviceId=device_id,
        DeviceModel=device_model,
        # We use ip now
        # However it's hard to rename this field
        Hostname=ip,
    )


def get_push_message(puid, device_id):
    return TPushMessage(
        Uid=puid,
        SubscriptionId=1,
        Ring=1,
        DeviceId=device_id,
        Notification=TNotification(
            Id="mock_notification_id",
            Text=f"some text_{uuid.uuid4()}",
            SubscriptionId="1",
        ),
    )


def get_directive_status(puid, device_id, push_id):
    return TDirectiveStatus(
        Id=push_id,
        Puid=puid,
        DeviceId=device_id,
    )


def get_directive_change_status(puid, device_id, push_ids, status):
    return TChangeStatus(
        Ids=push_ids,
        Puid=puid,
        DeviceId=device_id,
        Status=status,
    )


def get_uniproxy_endpoint(ip, port):
    return TUniproxyEndpoint(
        Ip=ip,
        Port=port
    )


def get_random_uniproxy_endpoint():
    return get_uniproxy_endpoint(f"ip_{uuid.uuid4()}", 845)


def get_user_device_info(puid, device_id, device_model="station", supported_features=None):
    if supported_features is None:
        supported_features = [
            TUserDeviceInfo.ESupportedFeature.AUDIO_CLIENT,
        ]

    return TUserDeviceInfo(
        UserDeviceIdentifier=TUserDeviceIdentifier(
            Puid=puid,
            DeviceId=device_id,
        ),
        DeviceModel=device_model,
        SupportedFeatures=supported_features,
    )


def get_client_state_change(action, user_device_info):
    return TClientStateChange(
        Action=action,
        UserDeviceInfo=user_device_info,
    )


def get_client_state_change_connect(user_device_info):
    return get_client_state_change(TClientStateChange.EAction.CONNECT, user_device_info)


def get_client_state_change_disconnect(user_device_info):
    return get_client_state_change(TClientStateChange.EAction.DISCONNECT, user_device_info)


def get_clients_diff(client_state_changes):
    return TClientsDiff(
        ClientStateChanges=client_state_changes,
    )


def get_clients_full_info(clients):
    return TClientsFullInfo(
        Clients=clients
    )


def get_internal_device_info(device_model="station", supported_features=None):
    if supported_features is None:
        supported_features = [
            TUserDeviceInfo.ESupportedFeature.AUDIO_CLIENT,
        ]

    return TInternalDeviceInfo(
        DeviceModel=device_model,
        SupportedFeatures=supported_features,
    )


def get_update_connected_clients_request(
    uniproxy_endpoint,
    monotonic_timestamp,
    shard_id,
    client_state_changes=None,
    clients_full_state=None,
    all_connections_dropped_on_shutdown=False,
):
    result = TUpdateConnectedClientsRequest(
        UniproxyEndpoint=uniproxy_endpoint,
        MonotonicTimestamp=monotonic_timestamp,
        ShardId=shard_id,
        AllConnectionsDroppedOnShutdown=all_connections_dropped_on_shutdown,
    )

    if client_state_changes is not None:
        result.ClientsDiff.CopyFrom(get_clients_diff(client_state_changes))

    if clients_full_state is not None:
        result.ClientsFullInfo.CopyFrom(get_clients_full_info(clients_full_state))

    return result


def get_update_device_environment_request():
    return TUpdateDeviceEnvironmentRequest(
        Type=EDeviceEnvironmentType.DEVICE_STATE,
    )


def get_manage_subscription(puid, subscription_id, method):
    return TManageSubscription(
        SubscriptionId=subscription_id,
        Method=method,
        Puid=puid,
    )


def get_notification_change_status(puid, notification_ids):
    return TNotificationChangeStatus(
        Puid=puid,
        NotificationIds=notification_ids,
    )
