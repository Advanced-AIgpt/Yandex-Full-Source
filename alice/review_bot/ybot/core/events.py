# coding: utf-8
import signal
import logging
import time
from collections import defaultdict

import attr
import gevent
import gevent.signal
from gevent.queue import Queue

logger = logging.getLogger(__name__)

__events = Queue()
__listeners = set()
__listeners_actors = defaultdict(list)
__emitters = set()
__jobs = []


def emit(event_name, value):
    """ Emit event with event_name and value """
    logger.info('Event %s emited', event_name)
    __events.put((event_name, value))


def subscribe(event_name, listener_func):
    """ Make Listener from listener_func, and subscribe to event_name """
    __listeners.add((event_name, listener_func))
    logger.info('Listener %s subscribed to event %s', listener_func, event_name)


class Actor(gevent.Greenlet):
    def __init__(self, wrapped, ctx):
        super(Actor, self).__init__()
        self._obj = wrapped
        self._ctx = ctx
        self._running = False
        self._events = Queue()

    def _run(self):
        self._running = True

        while self._running:
            name, value = self._events.get()
            self._obj.on_event(self._ctx, name, value)
            gevent.sleep(0)

    def send(self, name, value):
        self._events.put((name, value))

    def stop(self):
        self._running = False

    def __repr__(self):
        return 'Actor<{}>'.format(self._obj)


def repr_f(value):
    return '<{}: {}>'.format(value.__module__, value.__name__)


class Wrapper(object):
    pass


@attr.s(frozen=True)
class Listener(Wrapper):
    func = attr.ib(repr=repr_f)

    def on_event(self, ctx, name, value):
        try:
            self.func(ctx, name, value)
        except Exception:
            logger.exception('Exception in %s on event %s. Value %s', self, name, value)


@attr.s(frozen=True)
class Splitter(Wrapper):
    func = attr.ib(repr=repr_f)
    emit_events = attr.ib(converter=frozenset, repr=False)
    check = attr.ib(default=True, repr=False)

    def on_event(self, ctx, name, value):
        try:
            for new_event_name, new_value in self.func(ctx, name, value):
                if not self.check or new_event_name in self.emit_events:
                    emit(new_event_name, new_value)
                else:
                    logger.warning(
                        "%s emited unknown event %s, on args %s",
                        self, new_event_name, (name, value)
                    )
        except Exception:
            logger.exception('Exception in %s, event %s', self, name)


@attr.s(frozen=True)
class Emitter(Wrapper):
    func = attr.ib(repr=repr_f)
    emit_events = attr.ib(converter=frozenset, repr=False)
    sleep = attr.ib(default=0, repr=False)
    multi = attr.ib(default=False, repr=False)
    check = attr.ib(default=True, repr=False)

    def on_event(self, ctx, name, value):
        start_time = time.time()

        while True:
            try:
                for item in self.func(ctx):
                    if self.multi:
                        event_name, value = item
                    else:
                        event_name = list(self.emit_events)[0]
                        value = item

                    if not self.check or event_name in self.emit_events:
                        emit(event_name, value)
                    else:
                        logger.warning("%s emited unknown event %s", self, event_name)
            except Exception:
                if time.time() - start_time < 10:
                    logger.critical("Can't run emitter %s", self)
                    raise
                else:
                    logger.exception("Exception in emitter %s", self)

            gevent.sleep(self.sleep)


def listener(*event_names):
    """ Decorator for listener functions

    event_names - names of events, that listener can handle
    """
    def wrapper(f):
        obj = Listener(f)
        for event in set(event_names):
            subscribe(event, obj)

        return obj
    return wrapper


def emitter(*event_names, **kwargs):
    """ Decorator for event generators

    event_names - names of events, that generator emits, if multi=False (default)
                  it must be only one event_name, and generator must return only
                  event values

    multi       - optional, default False, if True generator must yields tuples:
                  (event_name, value), all posible event names must be listed
                  in event_names decorator parameter

    sleep       - optional, default 0, timeout between emitter restarts
    check       - optional, default True, check that emited events in event_names
    """
    sleep = kwargs.get('sleep', 0)
    multi = kwargs.get('multi', False)
    check = kwargs.get('check', True)

    def wrapper(f):
        obj = Emitter(f, event_names, sleep, multi, check)
        __emitters.add(obj)
        return obj

    return wrapper


def splitter(listen_events, emit_events=(), check=True):
    """ Decorator for event splitter
    decorated function must take two parameters like any listener:
    <event_name> and <value>

    and return generator of tuples like multi emitter:
    (new_event_name, value)

    """
    def wrapper(f):
        obj = Splitter(f, emit_events, check)

        for event in listen_events:
            subscribe(event, obj)

        return obj

    return wrapper


def run_emitters(ctx):
    logger.info('Running emitters: %s', __emitters)
    for emitter in __emitters:
        act = Actor.spawn(emitter, ctx)
        act.send(None, None)    # fake event to start emmitter
        __jobs.append(act)


def run_listeners(ctx):
    fmt = ', '.join('{}={}'.format(*i) for i in __listeners)
    logger.info('Running listeners: %s', fmt)

    for event, listener in __listeners:
        act = Actor.spawn(listener, ctx)
        __jobs.append(act)
        __listeners_actors[event].append(act)


def run_eventloop():
    while True:
        event_name, value = __events.get()

        for listener in __listeners_actors[event_name]:
            logger.debug('Send event %s to %s', event_name, listener)
            listener.send(event_name, value)

        gevent.sleep(0)


def run_all(ctx):
    gevent.signal.signal(signal.SIGQUIT, gevent.kill)
    run_emitters(ctx)
    run_listeners(ctx)
    eventloop = gevent.spawn(run_eventloop)
    __jobs.append(eventloop)
    gevent.joinall(__jobs, raise_error=True)


def kill_all():
    for job in __jobs:
        if isinstance(job, Actor):
            job.stop()

    gevent.killall(__jobs, block=True)
