from alice.uniproxy.library.messenger.client_locator import ClientLocator
from alice.uniproxy.library.messenger import init_mssngr
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.global_counter import GlobalCounter

import tornado
from functools import partial


def main():
    import argparse

    Logger.init(__file__, True)

    parser = argparse.ArgumentParser()
    parser.add_argument('--cmd', required=True)
    parser.add_argument('--guid', required=False)
    parser.add_argument('--guids', required=False)
    parser.add_argument('--client-id', required=False)
    parser.add_argument('--ts', required=False, type=int)
    parser.add_argument('--hostname', required=False)
    parser.add_argument('--use-ydb', required=False, default=False, type=int)
    parser.add_argument('--use-memcached', required=False, default=True, type=int)
    parser.add_argument('--wait-for-ydb', required=False, default=False, type=int)

    args = parser.parse_args()

    init_mssngr()
    GlobalCounter.init()

    locator = ClientLocator()

    if args.cmd == 'upsert':
        if not args.guid or not args.client_id:
            print('guid and client_id must be present!')
            return 1

        tornado.ioloop.IOLoop.current().run_sync(partial(locator.update_location, args.guid, args.client_id))

    elif args.cmd == 'remove':
        if not args.hostname or not args.ts or not args.guids:
            print('hostname, ts and guids must be present!')
            return 1

        tornado.ioloop.IOLoop.current().run_sync(partial(locator.remove_location_batch, args.guids.split(','), args.hostname, args.ts))

    elif args.cmd == 'select':
        if not args.guids:
            print('guids must be present')
            return 1

        @tornado.gen.coroutine
        def print_result():
            resp = yield locator.resolve_locations(args.guids.split(','))
            print(resp)
            for r in resp._data:
                print(r)
        tornado.ioloop.IOLoop.current().run_sync(print_result)


if __name__ == "__main__":
    ret_code = main()
    exit(0 if ret_code is None else ret_code)
