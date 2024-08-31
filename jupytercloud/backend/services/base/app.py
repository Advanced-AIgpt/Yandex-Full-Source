import asyncio
import logging
import os

from jupyter_core.application import JupyterApp, NoStart
from jupyterhub.log import CoroutineLogFormatter
from jupyterhub.utils import url_path_join
from tornado import httpserver, web  # noqa
from tornado.ioloop import IOLoop
from tornado.log import access_log, app_log, gen_log
from tornado.platform.asyncio import AsyncIOMainLoop
from traitlets import Dict, Instance, Integer, List, Unicode, default
from traitlets.config.application import catch_config_error

from jupytercloud.backend.lib.util.metrics.configurable import MetricsConfigurableMixin
from jupytercloud.backend.services.base.handlers import base_handlers


class JupyterCloudApp(JupyterApp, MetricsConfigurableMixin):
    config_file = Unicode('jupyterhub_config.py', config=True)

    # Jupyterhub API config

    jupyterhub_api_url = Unicode(config=True)
    jupyterhub_api_token = Unicode(config=True)
    jupyterhub_service_prefix = Unicode(config=True)

    is_service = True

    @default('jupyterhub_api_url')
    def _jupyterhub_api_url(self):
        jupyterhub_api_url = os.environ.get('JUPYTERHUB_API_URL')
        assert jupyterhub_api_url
        return jupyterhub_api_url

    @default('jupyterhub_api_token')
    def _jupyterhub_api_token(self):
        jupyterhub_api_token = os.environ.get('JUPYTERHUB_API_TOKEN')
        assert jupyterhub_api_token
        return jupyterhub_api_token

    @default('jupyterhub_service_prefix')
    def _jupyterhub_service_prefix(self):
        jupyterhub_service_prefix = os.environ.get('JUPYTERHUB_SERVICE_PREFIX')
        assert jupyterhub_service_prefix
        return jupyterhub_service_prefix

    # Tornado config

    port = Integer(config=True)
    prefix = Unicode(config=True)
    tornado_settings = Dict(config=True)

    handlers = List()
    tornado_application = Instance(web.Application)
    # http_server cannot be trait Instance(httpserver.HTTPServer):
    # if exception raises at init-stage, we will call .stop()
    # which will raise at self.http_server because http_server is None
    # and it cannot be None because it is trait
    http_server = None

    # Logging config

    _log_formatter_cls = CoroutineLogFormatter

    @default('log_level')
    def _log_level_default(self):
        return logging.INFO

    @default('log_datefmt')
    def _log_datefmt_default(self):
        return '%Y-%m-%d %H:%M:%S'

    @default('log_format')
    def _log_format_default(self):
        return '%(color)s[%(levelname)1.1s %(asctime)s.%(msecs).03d %(name)s %(module)s:%(lineno)d]%(end_color)s %(message)s'

    extra_log_handlers = List(Instance(logging.Handler), config=True)

    io_loop = None

    def init_logging(self):
        self.log.propagate = False

        # disable curl debug, which is TOO MUCH
        logging.getLogger('tornado.curl_httpclient').setLevel(
            max(self.log_level, logging.INFO),
        )

        _formatter = self._log_formatter_cls(
            fmt=self.log_format, datefmt=self.log_datefmt,
        )

        for handler in self.extra_log_handlers:
            if handler.formatter is None:
                handler.setFormatter(_formatter)
            self.log.addHandler(handler)

        # hook up tornado 3's loggers to our app handlers
        for log in (app_log, access_log, gen_log):
            # ensure all log statements identify the application they come from
            log.name = self.log.name

        for log_name in ('tornado',):
            logger = logging.getLogger(log_name)
            logger.propagate = True
            logger.parent = self.log
            logger.setLevel(self.log.level)

    def init_app(self):
        pass

    def init_handlers(self):
        self.handlers.extend(
            (url_path_join(self.jupyterhub_service_prefix, path), handler)
            for (path, handler) in base_handlers
        )

    def init_tornado_settings(self):
        settings = {
            'config': self.config,
            'log': self.log,
            'jupyterhub_api_url': self.jupyterhub_api_url,
            'jupyterhub_api_token': self.jupyterhub_api_token,
            'app': self,
        }

        settings.update(self.tornado_settings)

        self.tornado_settings = settings

        self.set_jupyter_cloud_request_logger()

    def init_tornado_application(self):
        self.tornado_application = web.Application(
            self.handlers,
            **self.tornado_settings,
        )

    @catch_config_error
    def initialize(self, *args, **kwargs):
        super().initialize(*args, **kwargs)

        if self._dispatching:
            return

        self.init_logging()
        self.init_app()

        if self.is_service:
            self.init_tornado_settings()
            self.init_handlers()
            self.init_tornado_application()

    def start(self):
        super().start()

        if not self.is_service:
            return

        self.http_server = httpserver.HTTPServer(
            self.tornado_application,
        )

        assert self.port

        try:
            self.http_server.listen(self.port)
            self.log.info('Listening to port %d', self.port)
        except Exception:
            self.log.error('Failed to bind to %d', self.port)
            raise

    def stop(self):
        if self.http_server:
            self.http_server.stop()

        if not self.io_loop:
            return

        self.io_loop.add_callback(self.io_loop.stop)

    async def launch_instance_async(self, argv=None):
        try:
            self.initialize(argv)
            self.start()
        except Exception:
            self.stop()
            raise

    @classmethod
    def launch_instance(cls, argv=None):
        self = cls.instance()

        AsyncIOMainLoop().install()
        self.io_loop = loop = IOLoop.current()
        task = asyncio.ensure_future(self.launch_instance_async(argv))

        try:
            loop.start()
        except KeyboardInterrupt:
            print(f'\n{self.__class__.__name__} interrupted')
        finally:
            if task.done():
                try:
                    # re-raise exceptions in launch_instance_async
                    task.result()
                except NoStart:
                    pass
            self.stop()
            loop.close()
