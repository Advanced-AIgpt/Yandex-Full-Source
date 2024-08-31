from alice.uniproxy.library.protos.uniproxy_pb2 import TPushMessage

import google.protobuf.text_format as text
import requests
import time
import uuid


def parse_message(proto_text):
    msg = TPushMessage()
    text.Merge(proto_text.decode(), msg)
    return msg


def send_message(msg):
    msg.Notification.Id = str(uuid.uuid4())
    msg.Notification.Timestamp = str(time.time())
    r = requests.post('https://notificator-test.alice.yandex.net/delivery', data=msg.SerializeToString())
    assert r.status_code == 200, f'Notificator service status: {r.status_code}, message: {r.text}'


def _manage_subscription(personal_uid, subscription_id, method):
    params = {
        "puid": personal_uid,
        "subscription_id": subscription_id,
        "method": method
    }
    r = requests.get('http://notificator-test.alice.yandex.net/subscriptions/manage', headers={"X-Ya-Service-Ticket": "2023285"}, params=params)
    assert r.status_code == 200, f'Subscriptions managing failed. Status: {r.status_code}, message: {r.text}'


# Subscribe or unsubscribe user with puid to notification subscription
def subscribe(personal_uid, subscription_id):
    _manage_subscription(personal_uid, subscription_id, 'subscribe')


def unsubscribe(personal_uid, subscription_id):
    _manage_subscription(personal_uid, subscription_id, 'unsubscribe')
