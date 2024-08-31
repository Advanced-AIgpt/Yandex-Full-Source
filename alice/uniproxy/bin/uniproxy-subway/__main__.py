import argparse
import logging
import tornado.ioloop

from alice.uniproxy.library.settings import config

from alice.uniproxy.library.subway.server import SubwayServer

from alice.uniproxy.library.global_state import GlobalState
from alice.uniproxy.library.global_counter import GlobalCounter
from alice.uniproxy.library.global_counter import GlobalTimings


def init_logging(verbose: bool):
    logging.basicConfig(
        level=logging.DEBUG if verbose else logging.WARNING,
        format='%(asctime)s %(levelname)s %(name)s: %(message)s'
    )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-p', '--port', default=config['subway']['port'], help='port to listen on')
    parser.add_argument('-v', '--verbose', action='store_true', help='enable DEBUG logging (default WARN)')
    args = parser.parse_args()

    try:
        verbose = args.verbose or config['subway']['debug']
        init_logging(args.verbose)

        GlobalCounter.init()
        GlobalTimings.init()
        GlobalState.init()

        server = SubwayServer(args.port)
        server.start()

        GlobalState.set_listening()
        GlobalState.set_ready()

        tornado.ioloop.IOLoop.current().start()
    except KeyboardInterrupt as ex:
        pass
    except Exception as ex:
        logging.getLogger('subway').exception(ex)


if __name__ == "__main__":
    main()
