import time
import tornado.ioloop
import tornado.gen
import tornado.queues
import gc
import os
import socket

from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.settings import config

g_gc_last_started = 0.0
g_gc_last_stopped = 0.0
g_gc_last_run = 0.0
g_gc_timings = None
g_gc_ioloop = None
g_gc_log = None


# --------------------------------------------------------------------------------------------------------------------
@tornado.gen.coroutine
def __gc_put_timings(data):
    global g_gc_timings, g_gc_log

    try:
        yield g_gc_timings.put(data)
    except Exception as e:
        g_gc_log.error('failed to put gc timings data: {}'.format(str(e)))


# --------------------------------------------------------------------------------------------------------------------
def __gc_mon_callback(phase, info):
    ts = time.time()

    global g_gc_last_started, g_gc_last_run, g_gc_last_stopped, g_gc_log

    if phase == 'start':
        g_gc_last_started = ts
    elif phase == 'stop':
        g_gc_last_run = ts - g_gc_last_started
        g_gc_ioloop.spawn_callback(__gc_put_timings, (ts - g_gc_last_stopped, g_gc_last_run,))
        g_gc_last_stopped = ts


# --------------------------------------------------------------------------------------------------------------------
def __gc_print_stats(gcdata):
    global g_gc_log
    runtime, duration = gcdata
    g_gc_log.info('GC_TIMINGS : %.9f : %.9f' % (runtime, duration))

    stats = gc.get_stats()
    for i in range(0, len(stats)):
        g_gc_log.info(
            'GC_GEN_{} : {} : {} : {}'.format(
                i,
                stats[i].get('collections'),
                stats[i].get('collected'),
                stats[i].get('uncollectable')
            )
        )


# --------------------------------------------------------------------------------------------------------------------
@tornado.gen.coroutine
def __gc_queue_handler():
    global g_gc_timings
    while True:
        gcdata = yield g_gc_timings.get()
        try:
            __gc_print_stats(gcdata)
        finally:
            g_gc_timings.task_done()


# --------------------------------------------------------------------------------------------------------------------
def gcmon_init(ioloop=None):
    location = os.environ.get('QLOUD_DISCOVERY_INSTANCE', socket.getfqdn())

    # ................................................................................................................
    enabled_hosts = os.environ.get('UNIPROXY_GCMON_EXPERIMENT_HOSTS')
    g_gcmon_enabled = config.get('gcmon', {}).get('enabled', False)

    if not g_gcmon_enabled and enabled_hosts:
        hosts = list([h.strip() for h in enabled_hosts.split(';')])
        if len(hosts) > 0:
            if location in hosts:
                Logger.get().debug('gc monitor is enabled for this host ({})'.format(location))
                g_gcmon_enabled = True

    # ................................................................................................................
    if not g_gcmon_enabled:
        Logger.get().debug('gcmon is not enabled on this host')
        return

    global g_gc_ioloop, g_gc_timings, g_gc_log, g_gc_last_started

    g_gc_last_started = time.time()

    g_gc_ioloop = ioloop if ioloop else tornado.ioloop.IOLoop.current()
    g_gc_timings = tornado.queues.Queue()
    g_gc_log = Logger.get('gcmon')
    g_gc_log.info('spawning __gc_queue_handler')
    g_gc_ioloop.spawn_callback(__gc_queue_handler)

    g_gc_log.info('setting gc callback')
    gc.callbacks.append(__gc_mon_callback)
