import argparse
import json
import logging
import sys
import tornado.gen
import tornado.httpserver
import tornado.ioloop
import tornado.web


from collections import defaultdict
from alice.uniproxy.library.async_http_client import QueuedHTTPClient
from alice.uniproxy.library.async_http_client import HTTPError


_g_ports = [8001, 8002, 7777, 7775]


class PingHandler(tornado.web.RequestHandler):
    def get(self):
        self.write("pong")


class UnistatHandler(tornado.web.RequestHandler):
    def aggregate_hgram(self, current_values, new_values):
        if type(current_values) != list:
            return new_values

        if len(current_values) != len(new_values):
            return new_values

        index = 0
        for k, v in new_values:
            current_values[index][1] += v
            index += 1

        return current_values

    @tornado.gen.coroutine
    def get(self):
        global _g_ports

        log = logging.getLogger("unistat")

        self.set_header("Content-Type", "application/json;charset=utf-8")

        result = defaultdict(int)

        try:
            log.info('creating requests for %s...', _g_ports)

            response_futures = [
                QueuedHTTPClient.get_client(
                    host="localhost",
                    port=port,
                    pool_size=1
                ).fetch("/unistat", request_timeout=0.8, retries=2) for port in _g_ports
            ]

            log.info('waiting for %d requests...', len(response_futures))
            for future in response_futures:
                log.info('processing future...')
                try:
                    response = yield future
                    data = response.json()
                    for k, v in data:
                        log.debug('merging %s...', k)
                        if k.endswith("_hgram"):
                            result[k] = self.aggregate_hgram(result.get(k), v)
                        else:
                            result[k] += v
                except HTTPError as ex:
                    log.error(ex)
                except ConnectionRefusedError as ex:
                    log.error(ex)
                except Exception as ex:
                    log.exception(ex)
        except Exception as ex:
            log.exception(ex)

        self.write(json.dumps([[k, v] for k, v in result.items()]))


handlers = [
    (r"/ping", PingHandler),
    (r"/unistat", UnistatHandler),
]


application = tornado.web.Application(handlers)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-p", "--port", dest="port", default=8003, help="port to listen on")
    parser.add_argument("-P", "--ports", dest="ports", default="8003,8004", help="ports to get unistat sum from")
    parser.add_argument("-v", "--verbose", action="store_true", help="enable debug logging")
    context = parser.parse_args(sys.argv[1:])

    logging.basicConfig(
        level=logging.DEBUG if context.verbose else logging.ERROR,
        format="%(asctime)s %(levelname)s %(name)s: %(message)s"
    )

    try:
        _g_ports = list(int(x) for x in context.ports.split(","))
        server = tornado.httpserver.HTTPServer(application, xheaders=True)
        server.bind(context.port)
        server.start(1)

        tornado.ioloop.IOLoop.current().start()
    except KeyboardInterrupt as ki:
        pass
    except Exception as ex:
        print(ex)
