# coding: utf-8
""" Sample config:

ybot.modules.backdoor:
    port: 5002

"""

from gevent.backdoor import BackdoorServer

from ..core.events import emitter

MODULE_NAME = 'ybot.modules.backdoor'

__server = None


@emitter()
def backdoor(ctx):
    if MODULE_NAME not in ctx.config:
        return

    port = ctx.config[MODULE_NAME].get('port', 5001)

    global __server
    __server = BackdoorServer(('127.0.0.1', port),
                              banner="Hello from gevent backdoor!",
                              locals={})

    __server.serve_forever()
    yield None
