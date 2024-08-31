import argparse
import tornado.ioloop
import tornado.httpclient
import logging

from alice.uniproxy.library.settings import config
from alice.uniproxy.library.settings import QLOUD_HTTP_PORT
from alice.uniproxy.library.settings import UNIPROXY_MAX_FORKS

from alice.uniproxy.library.delivery import DeliveryServer

from alice.uniproxy.library.global_state import GlobalState
from alice.uniproxy.library.global_counter.delivery import DeliveryCounter, DeliveryTimings

# from alice.uniproxy.library.messenger.client_locator import YdbClientLocator

from alice.uniproxy.library.profiling.gcmon import gcmon_init


def init_logging(verbose: bool):
    logging.basicConfig(
        level=logging.DEBUG if verbose else logging.INFO,
        format='%(asctime)s %(levelname)s %(name)s: %(message)s'
    )


def main():
    max_procs = config.get('delivery', {}).get('procs', UNIPROXY_MAX_FORKS)

    parser = argparse.ArgumentParser()
    parser.add_argument('-p', '--port', default=QLOUD_HTTP_PORT, help='port to listen on', type=int)
    parser.add_argument('-n', '--procs', default=max_procs, help='port to listen on', type=int)
    parser.add_argument('-s', '--subway-port', default=config['subway']['port'], help='subway port', type=int)
    parser.add_argument('-v', '--verbose', action='store_true', help='enable debug logging')
    args = parser.parse_args()

    verbose = args.verbose or config["debug_logging"] or config['delivery']['debug']
    try:
        init_logging(verbose)

        DeliveryCounter.init()
        DeliveryTimings.init()
        GlobalState.init(args.procs)

        server = DeliveryServer(
            port=args.port,
            procs=args.procs,
            subway_port=args.subway_port
        )
        server.start()

        gcmon_init(tornado.ioloop.IOLoop.current())

        # if config['messenger']['locator']['use_ydb']:
        #    tornado.ioloop.IOLoop.current().run_sync(YdbClientLocator.initialize)

        GlobalState.set_listening()
        GlobalState.set_ready()

        tornado.ioloop.IOLoop.current().start()
    except KeyboardInterrupt as ex:
        pass
    except Exception as ex:
        logging.getLogger('delivery').exception(ex)


if __name__ == "__main__":
    main()
