import requests
import json
import uuid
import os
import time
from google.protobuf import json_format
import ydb
from alice.uniproxy.library.protos.uniproxy_pb2 import TPushMessage
from alice.megamind.protos.common.frame_pb2 import TSemanticFrame
from alice.megamind.protos.scenarios.notification_state_pb2 import TNotification
from alice.uniproxy.library.backends_common.protohelpers import proto_from_json

GET_NOTIFICATION = '''
        SELECT id, notification, subscription_id
          FROM [mass_notifications]
         WHERE sent = false
         LIMIT 1;
    '''

GET_NOTIF_BY_ID = '''
        SELECT id, notification
          FROM [mass_notifications]
         WHERE id = '{nid}';
    '''

MARK_SENT = '''
        UPDATE [mass_notifications]
           SET sent = true
         WHERE id = '{nid}';
    '''


def main():
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('--url', required=False, default='https://notificator.alice.yandex.net', help='notificator url')
    parser.add_argument('--database', required=False, help='ydb database')
    parser.add_argument('--endpoint', required=False, default='ydb-ru.yandex.net:2135', help='ydb endpoint')
    parser.add_argument('--ydb-token', required=False, default='', help='YDB token to db')
    parser.add_argument('--do-not-mark-read', required=False, default=False, type=bool, help='do not mark read')
    parser.add_argument('--puid', required=False)
    parser.add_argument('--id', required=False, default='', type=str)
    parser.add_argument('--file', required=False, default='', type=str)
    parser.add_argument('--subscription_id', required=False, type=int)
    args = parser.parse_args()

    notification = None
    if args.file:
        jf = None
        with open(args.file, 'r') as f:
            notification = TNotification()
            json_format.Parse(f.read(), notification)

        # print(notification)

        # with open(args.file, 'r') as f:
        #   jf = f.read()
        # notification = {
        #     'id': 'kinopoisk_4'.encode(),
        #     'text': '"Братья Винчестеры стараются не выпустить демонов".'
        #     'voice': '"Братья Винчестеры стараются не выпустить демонов".'
        #     'phrase': 'включи сверхъестественное'.encode(),
        #     'frame': jf.encode(),
        # }
    else:
        ydb_token = args.ydb_token
        if not ydb_token:
            ydb_token = os.environ.get('YDB_TOKEN')
        connection_params = ydb.ConnectionParams(
            args.endpoint,
            args.database,
            credentials=ydb.AuthTokenCredentials(ydb_token)
        )
        driver = ydb.Driver(connection_params)
        driver.wait(timeout=10)
        pool = ydb.SessionPool(driver, 1)

        with pool.checkout() as session:
            with session.transaction() as tx:
                rs = None
                if args.id:
                    print('sent id = {}'.format(args.id))
                    rs = tx.execute(GET_NOTIF_BY_ID.format(nid=args.id), commit_tx=True,)
                else:
                    rs = tx.execute(GET_NOTIFICATION, commit_tx=True)
                notification = rs[0].rows[0]

    if notification is None:
        print('nothing to send')
        return

    SUBSCRIPTION_ID = notification['subscription_id']

    # get uids
    uids = []
    print('getting user list')
    if not args.puid:
        timestamp = 0
        code = 200
        while code == 200:
            url = args.url + '/subscriptions/user_list?subscription_id={}&timestamp={}'.format(SUBSCRIPTION_ID, timestamp)
            print(url)
            r = requests.get(url)
            if r.status_code != 200:
                print(r.status_code)
                print(r.json())
                return

            code = r.json()['code']
            uids.extend(r.json()['payload']['users'])
            timestamp = r.json()['payload'].get('timestamp')
            if not timestamp:
                break
        print('length of user list:', len(uids))
    else:
        uids = [args.puid]
        print('user list:', uids)

    print(notification)
    print('wait 10 sec...')
    time.sleep(10)

    delivery_url = args.url + '/delivery'
    for uid in uids:
        print(f'try sent to {uid}')
        msg = TPushMessage()
        msg.Uid = uid
        msg.Ring = 1

        msg.Notification.ParseFromString(notification['notification'])
        msg.SubscriptionId = int(msg.Notification.SubscriptionId)

        r = requests.post(delivery_url, data=msg.SerializeToString())
        if r.status_code != 200:
            print(f"not 200. can't send push to {uid}")
        resp = r.json()
        if resp['code'] != 200:
            print(f"can't send push to {uid}")
        print(f"sent to {uid}")

    if args.do_not_mark_read:
        print('do not mark read')
        return

    print('set sent id={}'.format(notification['id'].decode()))
    with pool.checkout() as session:
        with session.transaction() as tx:
            query = MARK_SENT.format(nid=notification['id'].decode())
            print(query)
            tx.execute(query, commit_tx=True)

if __name__ == '__main__':
    main()
