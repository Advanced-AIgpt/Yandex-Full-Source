import json
import base64

from alice.uniproxy.library.utils.json_to_proto import DictToMessage2
from alice.uniproxy.library.utils.proto_to_json import MessageToDict


from mssngr.router.lib.protos.message_pb2 import TInMessage


UNIPROXY_MESSENGER_JSON_CONTENT_FIELDS = {
    'TInMessage': ['PayloadData', 'PayloadResponse'],
    'TOutMessage': ['PayloadData', 'PayloadResponse'],
    'TPlain': ['CustomPayload'],
    'TCard': ['Card'],
    'TBotRequest': ['CustomPayload'],
    'TStateSync': ['Data'],
}


def test_simple_in_message():
    data = {
        'Guid':         'GUID',
        'ChatId':       'CHAT_ID',
        'ToGuid':       'TO_GUID',
        'PayloadData':  {
            'type': 'message_v2',
            'text': 'this is payload data',
        },
        'PayloadId':    'PAYLOAD_ID',
    }

    msg = DictToMessage2(TInMessage, data, json_fields=UNIPROXY_MESSENGER_JSON_CONTENT_FIELDS)

    assert msg.Guid == 'GUID'
    assert msg.ChatId == 'CHAT_ID'
    assert msg.ToGuid == 'TO_GUID'
    assert msg.PayloadId == 'PAYLOAD_ID'
    assert json.loads(msg.PayloadData.decode('utf-8'))['text'] == 'this is payload data'


def test_in_message_plain_text():
    data = {
        'ClientMessage': {
            'Plain': {
                'ChatId':       'CHAT_ID',
                'Timestamp':    42,
                'Text': {
                    'MessageText':  'This is message text',
                },
                'PayloadId':    'PAYLOAD_ID',
            }
        }
    }

    msg = DictToMessage2(TInMessage, data, json_fields=UNIPROXY_MESSENGER_JSON_CONTENT_FIELDS)

    assert msg.ClientMessage.Plain.ChatId == 'CHAT_ID'
    assert msg.ClientMessage.Plain.Timestamp == 42
    assert msg.ClientMessage.Plain.PayloadId == 'PAYLOAD_ID'
    assert msg.ClientMessage.Plain.Text.MessageText == 'This is message text'


def test_in_message_plain_sticker():
    data = {
        'ClientMessage': {
            'Plain': {
                'ChatId':       'CHAT_ID',
                'Timestamp':    42,
                'Sticker': {
                    'Id':       '420024',
                    'SetId':    '240042',
                },
                'PayloadId':    'PAYLOAD_ID',
            }
        }
    }

    msg = DictToMessage2(TInMessage, data, json_fields=UNIPROXY_MESSENGER_JSON_CONTENT_FIELDS)

    assert msg.ClientMessage.Plain.ChatId == 'CHAT_ID'
    assert msg.ClientMessage.Plain.Timestamp == 42
    assert msg.ClientMessage.Plain.PayloadId == 'PAYLOAD_ID'
    assert msg.ClientMessage.Plain.Sticker.Id == '420024'
    assert msg.ClientMessage.Plain.Sticker.SetId == '240042'


def test_in_message_typing():
    data = {
        'ClientMessage': {
            'Typing': {
                'ChatId':       'CHAT_ID',
            }
        }
    }

    msg = DictToMessage2(TInMessage, data, json_fields=UNIPROXY_MESSENGER_JSON_CONTENT_FIELDS)

    assert msg.ClientMessage.Typing.ChatId == 'CHAT_ID'


def test_in_message_statesync():
    data = {
        'ClientMessage': {
            'StateSync': {
                'Data': {
                    'foo': 'bar',
                    'baz': 42
                }
            }
        }
    }

    msg = DictToMessage2(TInMessage, data, json_fields=UNIPROXY_MESSENGER_JSON_CONTENT_FIELDS)

    assert json.loads(msg.ClientMessage.StateSync.Data.decode('utf-8'))['foo'] == 'bar'
    assert json.loads(msg.ClientMessage.StateSync.Data.decode('utf-8'))['baz'] == 42


def test_in_message_heartbeat():
    data = {
        'ClientMessage': {
            'Heartbeat': {
                'ChatId': '77806c0c-a70a-494d-9124-cee0a155044a',
            }
        },
    }

    msg = DictToMessage2(TInMessage, data, json_fields=UNIPROXY_MESSENGER_JSON_CONTENT_FIELDS)

    assert MessageToDict(msg).get('ClientMessage', {}).get('Heartbeat') is not None


def test_in_message_never_be_supported():
    DATA_BINARY = b'data:__hello__'
    DATA_B64ENC = base64.b64encode(DATA_BINARY).decode('utf-8')

    data = {
        'ClientMessage': {
            'Plain': {
                'NeverToBeSupported': {
                    'MysteriousCrap': DATA_B64ENC,
                },
                'CustomPayload': {
                    'Foo': 42,
                    'Bar': 24,
                },
                'PayloadId': 'THIS-IS-PAYLOAD',
                'ForwardedMessageRefs': [
                    {'ChatId': 'ChatId-42', 'Timestamp': 42},
                    {'ChatId': 'ChatId-53', 'Timestamp': 53},
                ]
            },
        },
        'PayloadId': 'THIS-IS-PAYLOAD',
        'PayloadData': {
            'JSON': 'This-Is-Json-Payload',
            'JSON2': 'This-Is-Json2-Payload',
        }
    }

    msg = DictToMessage2(TInMessage, data, UNIPROXY_MESSENGER_JSON_CONTENT_FIELDS)

    assert msg.ClientMessage.Plain.NeverToBeSupported.MysteriousCrap == DATA_BINARY
    assert b'"Foo"' in msg.ClientMessage.Plain.CustomPayload
    assert b'"Bar"' in msg.ClientMessage.Plain.CustomPayload
    assert b'"This-Is-Json-Payload"' in msg.PayloadData
    assert b'"This-Is-Json2-Payload"' in msg.PayloadData
    assert msg.ClientMessage.Plain.ForwardedMessageRefs[0].ChatId == 'ChatId-42'
    assert msg.ClientMessage.Plain.ForwardedMessageRefs[1].ChatId == 'ChatId-53'
    assert msg.ClientMessage.Plain.ForwardedMessageRefs[0].Timestamp == 42
    assert msg.ClientMessage.Plain.ForwardedMessageRefs[1].Timestamp == 53


def test_in_message_client_log_data():
    data = {
        'ClientMessage': {
            'LogData': {
                'ICookie': 'the-best-icookie-ever'
            }
        }
    }

    msg = DictToMessage2(TInMessage, data, json_fields=UNIPROXY_MESSENGER_JSON_CONTENT_FIELDS)
    assert msg.ClientMessage.LogData.ICookie == 'the-best-icookie-ever'
