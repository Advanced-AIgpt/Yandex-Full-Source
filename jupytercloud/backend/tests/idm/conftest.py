import os
import sys
from binascii import hexlify

import pytest
from jupyterhub.proxy import ConfigurableHTTPProxy
from jupyterhub.tests.mocking import MockHub
from jupyterhub.tests.utils import async_requests
from jupyterhub.utils import random_port, url_path_join
from traitlets import Integer, default

from jupytercloud.backend.lib.db.configurable import JupyterCloudDB
from jupytercloud.backend.lib.idm.app import IDMIntegrationApp


def get_secret():
    return hexlify(os.urandom(5))


class MockIDM(IDMIntegrationApp):
    users_added = Integer(0)

    def load_config_file(self, *args, **kwargs):
        pass

    async def request(self, *api_path, method='get', **kwargs):
        f = getattr(async_requests, method)

        url = url_path_join(self.jupyterhub_api_url, 'services', 'idm', *api_path)

        self.log.debug('requesting %s', url)

        resp = await f(url, **kwargs)
        resp.raise_for_status()

        return resp.content

    def generate_username(self):
        self.users_added += 1
        return f'user{self.users_added}'


class MockProxy(ConfigurableHTTPProxy):
    @default('api_url')
    def _api_url_default(self):
        return f'http://127.0.0.1:{random_port()}'


@pytest.fixture
def jupyterhub_app(request, io_loop):
    mocked_app = MockHub(
        cookie_secret=get_secret(),
        proxy_class=MockProxy,
        hub_port=random_port(),
    )

    async def make_app():
        await mocked_app.initialize([])
        await mocked_app.start()

    def fin():
        # disconnect logging during cleanup because pytest closes captured FDs prematurely
        mocked_app.log.handlers = []
        try:
            mocked_app.stop()
        except Exception as e:
            print(f'Error stopping Hub: {e}', file=sys.stderr)

    request.addfinalizer(fin)
    io_loop.run_sync(make_app)
    return mocked_app


@pytest.fixture(autouse=True)
def disable_alembic_logging_configuration(monkeypatch):
    import logging

    def void(*args, **kwargs):
        pass

    monkeypatch.setattr(logging.config, 'fileConfig', void)


@pytest.fixture
def jupytercloud_db(jupyterhub_app):
    return JupyterCloudDB(
        db_url='sqlite:///:memory:',
        parent=jupyterhub_app,
        # debug_db=True,
    )


@pytest.fixture
def idm_app(request, io_loop, jupyterhub_app, jupytercloud_db):
    app = MockIDM(
        jupyter_cloud_db=jupytercloud_db,
        jupyterhub_api_url=jupyterhub_app.proxy.public_url,
        jupyterhub_api_token=get_secret(),
        prefix=url_path_join(jupyterhub_app.base_url, 'services', 'idm'),
        log_level=10,
        port=random_port(),
        tvm_client_verify=False,
        tvm_whielist=[],
    )

    jupyterhub_app.services = [{
        'name': 'idm',
        'admin': True,
        'url': f'http://127.0.0.1:{app.port}',
        'api_token': app.jupyterhub_api_token,
    }]

    async def make_app():
        jupyterhub_app.init_services()
        await jupyterhub_app.init_api_tokens()
        await jupyterhub_app.proxy.add_all_services(jupyterhub_app._service_map)

        app.initialize([])
        app.start()

    def fin():
        # disconnect logging during cleanup because pytest closes captured FDs prematurely
        app.log.handlers = []
        try:
            app.stop()
        except Exception as e:
            print(f'Error stopping IDMIntegrationApp: {e}', file=sys.stderr)

    request.addfinalizer(fin)
    io_loop.run_sync(make_app)
    return app
