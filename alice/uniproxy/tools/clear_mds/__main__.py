import argparse
import os
os.environ['UNIPROXY_CUSTOM_ENVIRONMENT_TYPE'] = 'rtc_production'

from alice.uniproxy.library.backends_common.storage import MdsStorage
import tornado
import yt.wrapper as yt


@tornado.gen.coroutine
def process(table, mds_storage):
    num = 0
    num_ok = 0
    for row in yt.read_table(yt.TablePath(table)):
        mds_key = row['mds_key']
        if mds_key is not None:
            num += 1
            try:
                yield mds_storage.delete(mds_key)
                num_ok += 1
            except:
                pass
    print('Done: {}/{}'.format(num_ok, num))
    tornado.ioloop.IOLoop.current().stop()


def main():
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument(
        '-d',
        '--date',
        type=str,
        help='Date',
        default='2019-03-01'
    )
    args = parser.parse_args()
    date = args.date
    prefix = '//home/voice-speechbase/uniproxy/logs_unified_qloud/'
    yt.config["proxy"]["url"] = "hahn"
    mds_storage = MdsStorage()
    table = prefix + date
    print(table)
    tornado.ioloop.IOLoop.current().spawn_callback(process, table, mds_storage)
    tornado.ioloop.IOLoop.current().start()

if __name__ == '__main__':
    main()
