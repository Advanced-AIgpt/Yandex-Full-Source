import argparse
import tornado.ioloop
import tornado.httpclient
import logging
from alice.uniproxy.library.logging import Logger

from alice.uniproxy.library.settings import config
from alice.uniproxy.library.settings import QLOUD_HTTP_PORT
from alice.uniproxy.library.settings import UNIPROXY_MAX_FORKS

from alice.uniproxy.library.notificator.server import NotificatorServer

from alice.uniproxy.library.global_state import GlobalState
from alice.uniproxy.library.global_counter.notificator import NotificatorCounter, NotificatorTimings

# from alice.uniproxy.library.messenger.client_locator import YdbClientLocator

from alice.uniproxy.library.profiling.gcmon import gcmon_init
from .rtlog_grip import init as init_rtlog


def init_logging(verbose: bool):
    logging.basicConfig(
        level=logging.DEBUG if verbose else logging.INFO,
        format='%(asctime)s %(levelname)s %(name)s: %(message)s'
    )


def main():
    max_procs = config.get('notificator', {}).get('procs', UNIPROXY_MAX_FORKS)

    parser = argparse.ArgumentParser()
    parser.add_argument('-p', '--port', default=QLOUD_HTTP_PORT, help='port to listen on', type=int)
    parser.add_argument('-n', '--procs', default=max_procs, help='port to listen on', type=int)
    parser.add_argument('-s', '--subway-port', default=config['subway']['port'], help='subway port', type=int)
    parser.add_argument('-v', '--verbose', action='store_true', help='enable debug logging')
    parser.add_argument('--mock-delivery', default=False, action='store_true', help='disable all delivery')
    parser.add_argument('--no-remove-missing', default=False, action='store_true', help='do not remove missing devices from subway response')
    args = parser.parse_args()

    verbose = args.verbose or config["debug_logging"] or config['notificator'].get('debug')
    try:
        # init_logging(verbose)
        Logger.init('notificator', verbose)
        init_rtlog()

        NotificatorCounter.init()
        NotificatorTimings.init()
        GlobalState.init(args.procs)

        server = NotificatorServer(
            port=args.port,
            procs=args.procs,
            subway_port=args.subway_port,
            mock_delivery=args.mock_delivery,
            no_remove_missing=args.no_remove_missing,
        )
        server.start()

        gcmon_init(tornado.ioloop.IOLoop.current())

        GlobalState.set_listening()
        GlobalState.set_ready()

        tornado.ioloop.IOLoop.current().start()
    except KeyboardInterrupt:
        pass
    except Exception as ex:
        logging.getLogger('delivery').exception(ex)


if __name__ == "__main__":
    main()
