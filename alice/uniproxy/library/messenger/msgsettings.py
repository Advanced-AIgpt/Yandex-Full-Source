UNIPROXY_MESSENGER_DEFAULT_ENABLED = False
UNIPROXY_MESSENGER_DEFAULT_BACKEND_HOST = None
UNIPROXY_MESSENGER_DEFAULT_BACKEND_PORT = 42024
UNIPROXY_MESSENGER_DEFAULT_SECURE = False
UNIPROXY_MESSENGER_DEFAULT_POOL_SIZE = 12
UNIPROXY_MESSENGER_DEFAULT_QUEUE_SIZE = 1000
UNIPROXY_MESSENGER_DEFAULT_CHECK_PERIOD = 45
UNIPROXY_MESSENGER_DEFAULT_PUSH_URL = '/push'
UNIPROXY_MESSENGER_DEFAULT_HISTORY_URL = '/history'
UNIPROXY_MESSENGER_DEFAULT_EDIT_HISTORY_URL = '/edit_history'
UNIPROXY_MESSENGER_DEFAULT_SUBSCRIPTION_URL = '/subscribe'
UNIPROXY_MESSENGER_DEFAULT_PING_URL = '/admin?action=ping'
UNIPROXY_MESSENGER_MINIMAL_VERSION = 2
UNIPROXY_MESSENGER_CURRENT_VERSION = 2


UNIPROXY_MESSENGER_JSON_CONTENT_FIELDS = {
    'TInMessage': ['PayloadData', 'PayloadResponse'],
    'TOutMessage': ['PayloadData', 'PayloadResponse'],
    'TPlain': ['CustomPayload'],
    'TCard': ['Card'],
    'TBotRequest': ['CustomPayload'],
    'TStateSync': ['Data'],
}