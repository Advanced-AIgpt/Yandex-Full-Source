import requests
import click
import os
import threading

from alice.uniproxy.library.protos.uniproxy_pb2 import TPushMessage
import ydb

GET_NOTIFICATION = '''
    select device_id, notification, puid, subscription_id, time
        from [yt_notifications]
        where sent = false
        limit 10;
    '''

UPDATE_NOTIFICATION = '''
    update [yt_notifications]
        set sent = True,
            response = "{response}"
        where puid = '{puid}'
            and device_id = '{device_id}'
            and time = '{time}'
            and subscription_id = '{subscription_id}';
    '''


def send_notification(delivery_url, msg):
    r = requests.post(delivery_url, data=msg.SerializeToString())
    if r.status_code != 200:
        # print("not 200. can't send push")
        return 'not ok, status code = {}'.format(r.status_code)
    resp = r.json()
    if resp['code'] != 200:
        # print("can't send push")
        return 'not ok. resp = {}'.format(resp)

    return 'ok'


@click.command()
@click.option('--url', required=False, default='http://notificator.alice.yandex.net/delivery', help='notificator url')
@click.option('--ydb-token', required=False, help='YDB token', envvar='YDB_TOKEN')
@click.option('--ydb-endpoint', required=False, help='YDB endpoint', default='ydb-ru.yandex.net:2135')
@click.option('--ydb-database', required=False, help='YDB database', default='/ru/alice/prod/notificator')
def main(url, ydb_token, ydb_endpoint, ydb_database):
    # connect to ydb
    if not ydb_token:
        ydb_token = os.environ.get('YDB_TOKEN')
    connection_params = ydb.ConnectionParams(
        ydb_endpoint,
        ydb_database,
        credentials=ydb.AuthTokenCredentials(ydb_token)
    )
    driver = ydb.Driver(connection_params)
    driver.wait(timeout=10)
    pool = ydb.SessionPool(driver, 20)

    # select notifications while has unsent
    while True:
        rs = None
        print('get notifications')
        with pool.checkout(timeout=10) as session:
            with session.transaction() as tx:
                rs = tx.execute(GET_NOTIFICATION, commit_tx=True)
                if len(rs[0].rows) == 0:
                    return

        # send notifications
        thrs = []
        for r in rs[0].rows:
            def _send(row):
                msg = TPushMessage()

                notification = row.get('notification', None)
                if notification:
                    msg.Notification.ParseFromString(notification)
                    # print(msg.Notification)

                msg.SubscriptionId = int(row.get('subscription_id', None))
                msg.Ring = int(row.get('ring', 1))
                msg.Uid = row.get('puid', None)
                did = row.get('device_id', None)
                if did:
                    msg.DeviceId = did

                print('send to', msg.Uid)
                status = send_notification(url, msg)

                # update status
                with pool.checkout(timeout=10) as session:
                    with session.transaction() as tx:
                        query = UPDATE_NOTIFICATION.format(
                            puid=msg.Uid,
                            subscription_id=str(msg.SubscriptionId),
                            time=row.get('time', b'').decode(),
                            device_id=msg.DeviceId,
                            response=status
                        )
                        tx.execute(query, commit_tx=True)
                        print('status updated')

            t = threading.Thread(target=_send, args=(r,))
            t.start()
            thrs.append(t)

        for t in thrs:
            t.join()


if __name__ == '__main__':
    main()
