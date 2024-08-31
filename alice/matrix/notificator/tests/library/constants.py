class ItemTypes:
    CONNECTED_CLIENTS_UPDATE_REQUEST = "connected_clients_update_request"
    CONNECTED_CLIENTS_UPDATE_RESPONSE = "connected_clients_update_response"

    DEVICE_ENVIRONMENT_UPDATE_REQUEST = "device_environment_update_request"
    DEVICE_ENVIRONMENT_UPDATE_RESPONSE = "device_environment_update_response"


class ServiceHandlers:
    HTTP_SUBWAY_PUSH = "push"

    HTTP_SENSORS = "sensors"
    HTTP_YDB_SENSORS = "ydb_sensors"

    HTTP_IOT_USER_INFO = "v1.0/user/info"

    HTTP_DELIVERY = "delivery"
    HTTP_DELIVERY_DEMO = "delivery/demo"
    HTTP_DELIVERY_ON_CONNECT = "delivery/on_connect"
    HTTP_DELIVERY_PUSH = "delivery/push"
    HTTP_DEVICES = "devices"
    HTTP_DIRECTIVE_CHANGE_STATUS = "directive/change_status"
    HTTP_DIRECTIVE_STATUS = "directive/status"
    HTTP_GDPR = "gdpr"
    HTTP_LOCATOR = "locator"
    HTTP_NOTIFICATIONS = "notifications"
    HTTP_NOTIFICATIONS_CHANGE_STATUS = "notifications/change_status"
    HTTP_SUBSCRIPTIONS = "subscriptions"
    HTTP_SUBSCRIPTIONS_DEVICES = "subscriptions/devices"
    HTTP_SUBSCRIPTIONS_MANAGE = "subscriptions/manage"
    HTTP_SUBSCRIPTIONS_USER_LIST = "subscriptions/user_list"

    UPDATE_CONNECTED_CLIENTS = "update_connected_clients"
    UPDATE_DEVICE_ENVIRONMENT = "update_device_environment"
