#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import alice.uniproxy.library.tornado_speedups as speedups
speedups.apply()

import os
import sys
import argparse
import logging
import psutil

from asyncio import AbstractEventLoop

import tornado.ioloop
import tornado.httpserver
from tornado.httpclient import AsyncHTTPClient
import tornado.web

from raven.contrib.tornado import AsyncSentryClient

from alice.uniproxy.library.extlog.async_file_logger import AsyncFileLogger
from alice.uniproxy.library.settings import config, environment
from alice.uniproxy.library.settings import UNIPROXY_MEMVIEW_HANDLER_ENABLED
from .rtlog_grip import init as init_rtlog

from alice.uniproxy.library.unisystem.uniwebsocket import UniWebSocket
from alice.uniproxy.library.unisystem.uniwebsocket import HUniWebSocket

from alice.uniproxy.library.subway.pull_client.singleton import subway_init as init_subway
from alice.uniproxy.library.backends_memcached import memcached_init as init_umc
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.messenger import init_mssngr
from alice.uniproxy.library.messenger.client_locator import YdbClientLocator
from alice.uniproxy.library.auth.blackbox import init_tvm
from alice.uniproxy.library.global_state import GlobalState
from alice.uniproxy.library.global_counter.uniproxy import UniproxyCounter, UniproxyTimings
from alice.uniproxy.library.global_counter import GlobalCountersUpdater
from alice.uniproxy.library.legacy_navi import LegacyNaviHandler
from alice.uniproxy.library.profiling.gcmon import gcmon_init
from alice.uniproxy.library.common_handlers import PingHandler, UnistatHandler, StopHookHandler, StartHookHandler
from alice.uniproxy.library.frontend import RESOURCES_ROOT, FileHandler
from alice.uniproxy.library.web_handlers import AsrHandler, RevisionHandler, ExperimentsHandler, AsrWebSocket, \
    TtsSpeakersHandler, TtsWebSocket, LoggingWebSocket, TtsDemoHandler, AsrDemoHandler, SettingsHandler, \
    MemViewHandler, MonitoringHandler


handlers = [
    (r"/exp", ExperimentsHandler),
    (r"/exps_check", ExperimentsHandler, {'by_id': False, }),
    (r"/legacy/toyota", LegacyNaviHandler),
    (r"/ping", PingHandler),
    (r"/stop_hook", StopHookHandler),
    (r"/start_hook", StartHookHandler),
    (r"/asr", AsrHandler),
    (r"/uni.ws", UniWebSocket),
    (r"/huni.ws", HUniWebSocket),
    (r'/unistat', UnistatHandler),
    (r"/settings.js", SettingsHandler),
    (r"/envcheck", MonitoringHandler, {"logfile": "/tmp/envcheck", "expected_oks": 3, "unistat": "envcheck"}),
    (r"/voiceinputcheck([^/]*)", MonitoringHandler, {"logfile": "/tmp/voiceinputcheck", "unistat": "voiceinputcheck"}),
    (r"/textinputcheck", MonitoringHandler, {"logfile": "/tmp/textinputcheck", "unistat": "textinputcheck"}),
    (r"/convertercheck", MonitoringHandler, {"logfile": "/tmp/convertercheck", "unistat": "convertercheck"}),
    (r"/memcheck", MonitoringHandler, {"logfile": "/tmp/memcheck", "unistat": "memcheck"}),
    (r"/revision", RevisionHandler),
    (r"/asrsocket.ws", AsrWebSocket),
    (r"/ttssocket.ws", TtsWebSocket),
    (r"/logsocket.ws", LoggingWebSocket),
    (r"/speakers", TtsSpeakersHandler),
    (r".*?/(?:(ru|en)/)?ttsdemo.html", TtsDemoHandler),
    (r".*?/(?:(ru|en)/)?demo.html", AsrDemoHandler),
]

if UNIPROXY_MEMVIEW_HANDLER_ENABLED:
    handlers.append((r"/memview", MemViewHandler))

if os.environ.get("UNIPROXY_ALLOW_PROFILING", "").lower() not in ("false", "no", "n", "off", "0"):
    from alice.uniproxy.library.profiling import StartProfilingHandler, StopProfilingHandler
    handlers += [
        (r"/start_profiling", StartProfilingHandler),
        (r"/stop_profiling", StopProfilingHandler)
    ]

handlers.append((r'/(.*)', FileHandler, {'path': RESOURCES_ROOT}))


application = tornado.web.Application(handlers)

application.sentry_client = AsyncSentryClient(config.get("sentry", {})["url"])

if __name__ == "__main__":
    defprocs = int(os.environ.get('UNIPROXY_MAX_FORKS', 0))
    parser = argparse.ArgumentParser()
    parser.add_argument('-p', '--port', dest='port', default=config["port"], help='port to listen on')
    parser.add_argument('-n', '--nproc', dest='processes', default=defprocs, help='processes count', type=int)
    parser.add_argument('-z', '--left-zomby', dest='left_zomby', action='store_true', default=False,
                        help='not kill all zomby-uniproxy left from previous run (with PPID==1 & same exe name)')
    context = parser.parse_args(sys.argv[1:])

    log_file = config.get('log_file', None)
    debug_logging = config["debug_logging"]
    try:
        with open('uniproxy_debug_log_enable') as f:
            print('found file uniproxy_debug_log_enable, - debug_log enabled')
            debug_logging = True
    except:
        pass
    Logger.init(
        "uniproxy",
        debug_logging,
        application.sentry_client,
        mssngr_log_level=logging.DEBUG if config['messenger']['debug'] else logging.INFO,
        subway_log_level=logging.DEBUG if config['subway']['debug'] else logging.INFO,
        delivery_log_level=logging.DEBUG if config['delivery']['debug'] else logging.INFO,
        async_file_logger=AsyncFileLogger(log_file) if log_file else None
    )
    init_rtlog()

    if not context.left_zomby:
        # zomby purger
        this_proc = psutil.Process()
        this_exe = this_proc.exe()
        for p in psutil.process_iter(['pid', 'ppid', 'exe']):
            exe = p.info.get('exe')
            if exe is None:
                continue
            if p.info.get('ppid', -1) == 1 and exe == this_exe:
                Logger.get('uniproxy').info("Kill detected uniproxy-zomby pid={}".format(p.info.get('pid')))
                p.terminate()
    Logger.get('uniproxy').info("Listening on port {0}".format(context.port))

    UniproxyCounter.init()
    GlobalState.init(context.processes)
    UniproxyTimings.init()

    try:
        server = tornado.httpserver.HTTPServer(application, xheaders=True)
        server.bind(context.port, reuse_port=True)
        server.start(context.processes)

        periodic_counters_updater = tornado.ioloop.PeriodicCallback(lambda: GlobalCountersUpdater.update(), 5000)
        periodic_counters_updater.start()

        gcmon_init(tornado.ioloop.IOLoop.current())

        AsyncHTTPClient.configure(None, max_clients=1000)

        AbstractEventLoop.slow_callback_duration = 1.0

        ignore_memcached = ['delivery', 'mssngr']
        init_umc(ignore=ignore_memcached)
        init_subway()
        init_tvm()
        init_mssngr()
        if environment != 'development':
            tornado.ioloop.IOLoop.current().run_sync(YdbClientLocator.initialize)
        GlobalState.set_listening()
        GlobalState.set_ready()
        tornado.ioloop.IOLoop.current().start()
    except KeyboardInterrupt as ki:
        pass
    except Exception as ex:
        import traceback
        traceback.print_exc()
        Logger.get('uniproxy').exception(ex)
