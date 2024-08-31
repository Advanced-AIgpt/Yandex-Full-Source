# coding: utf-8

import gunicorn.app.base


class GunicornApp(gunicorn.app.base.BaseApplication):

    def __init__(self, app, opts):
        self._app = app
        self._opts = opts
        super(GunicornApp, self).__init__()

    def load_config(self):
        for k, v in self._opts.iteritems():
            if k in self.cfg.settings and v is not None:
                self.cfg.set(k.lower(), v)

    def load(self):
        return self._app
