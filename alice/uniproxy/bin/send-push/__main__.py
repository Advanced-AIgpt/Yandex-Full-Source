from alice.uniproxy.library.protos.uniproxy_pb2 import TPushMessage
import requests
import uuid


def main():
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('--url', required=True, help='delivery url')
    parser.add_argument('--uid', required=True, help='yandex uid')
    parser.add_argument('--text', required=True, help='text')
    parser.add_argument('--ring-type', required=False, help='ring type according ti ERingType', default=0, type=int)
    args = parser.parse_args()

    msg = TPushMessage()
    msg.Uid = args.uid
    n = msg.Directive.Notifications.add()
    n.Text = args.text
    n.Id = str(uuid.uuid4())
    msg.Directive.Ring = args.ring_type

    print(msg.SerializeToString())

    r = requests.post(args.url, data=msg.SerializeToString())
    if r.status_code != 200:
        print('delivery return http error {}'.format(r.status_code))
    else:
        print(r.json())


if __name__ == '__main__':
    main()
