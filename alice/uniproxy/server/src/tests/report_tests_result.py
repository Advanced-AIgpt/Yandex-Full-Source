#!/usr/bin/env python3
# coding: utf-8

import sys
import argparse

import tornado.httpserver
import tornado.ioloop
import tornado.web


TESTS_OUTPUT = 'not ready'
TESTS_RESULT = 255  # tests return(exit) code


class PingHandler(tornado.web.RequestHandler):
    def get(self):
        self.write("pong")


class TestsOutputHandler(tornado.web.RequestHandler):

    def get(self):
        self.content_type = "plain/text; charset=utf-8"
        self.write(TESTS_OUTPUT)


class TestsResultHandler(tornado.web.RequestHandler):

    def get(self):
        self.content_type = "plain/text; charset=utf-8"
        self.write(str(TESTS_RESULT))


handlers = [
    (r"/ping", PingHandler),
    (r'/tests_output', TestsOutputHandler),
    (r'/tests_result', TestsResultHandler),
]


application = tornado.web.Application(handlers)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-p', '--port', dest='port', default=80, help='port to listen on')
    parser.add_argument('--tests-output', help='filename for reading tests output')
    parser.add_argument('--tests-result', help='tests result(exit) code (0 - success, else errorcode)', default=254)
    context = parser.parse_args(sys.argv[1:])

    try:
        TESTS_RESULT = context.tests_result
        with open(context.tests_output) as f:
            TESTS_OUTPUT = f.read()

        server = tornado.httpserver.HTTPServer(application, xheaders=True)
        server.bind(context.port)
        server.start(0)

        tornado.ioloop.IOLoop.current().start()
    except KeyboardInterrupt as ki:
        pass
    except Exception as ex:
        print(ex)
