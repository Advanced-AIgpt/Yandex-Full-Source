import os
import time
import logging
import pytest
import asyncio
import yatest.common
import yatest.common.network
from alice.cuttlefish.library.python.apphost_here import AppHostEnvironment, HorizonData
from alice.cuttlefish.library.python.apphost_here.utils import run_daemon, read_json
from alice.cuttlefish.library.python.apphost_grpc_client import AppHostGrpcClient

from alice.cuttlefish.library.python.uniproxy2_daemon import Uniproxy2Daemon
from alice.cuttlefish.library.python.uniproxy_mock import UniproxyMock
from alice.cuttlefish.library.python.mockingbird import Mockingbird

from alice.cuttlefish.tests.apphost_heavy.mocks import STANDARD_MOCKS
from .daemons import CuttlefishDaemon


CONFIGURATION = "ctype=testing;geo=sas"
PRESERVED_HORIZON_DATA = "alice/cuttlefish/tests/apphost/horizon-data"
APPHOST_CONFIG = "alice/uniproxy/configs/prod/configs/app_host.json"
CUTTLEFISH_MEGAMIND_URL = "CUTTLEFISH_MEGAMIND_URL"


def get_all_grpc_handlers(horizon_data):
    # here we look through all graphs to collect gRPC backends' handlers
    res = {}
    for graph in horizon_data.graphs.values():
        for node_name, node in graph.config.get("nodes", {}).items():
            nodeType = node.get("nodeType")
            if nodeType in ("TRANSPARENT", "TRANSPARENT_STREAMING", "EMBED"):
                continue

            backend = horizon_data.backends[node["backendName"]]
            if backend.config.get("protocol") == "inproc":
                continue
            if backend.transport != "GRPC":
                continue

            handler = node.get("params", {}).get("handler")
            if handler is None:
                continue

            logging.debug(f"gRPC handler '{handler}' of '{backend} is needed in {graph}/{node_name}")
            res.setdefault(backend.name, set()).add(handler)

    return res


# -------------------------------------------------------------------------------------------------
@pytest.fixture(scope="session")
def workdir():
    wd = yatest.common.output_path("home")
    os.makedirs(wd)
    return wd


# -------------------------------------------------------------------------------------------------
@pytest.fixture(scope="session")
def port_manager():
    with yatest.common.network.PortManager() as pm:
        yield pm


# -------------------------------------------------------------------------------------------------
@pytest.fixture(scope="session")
def horizon_data(workdir):
    # We use preserved horizon-data to avoid dependency on `arcadia/apphost` - it'd make our tests
    # run on every change in the directory.
    return HorizonData(yatest.common.source_path(PRESERVED_HORIZON_DATA))


# -------------------------------------------------------------------------------------------------
@pytest.fixture(scope="session")
def mocks(horizon_data, port_manager):
    def is_local(backend_name):
        if backend_name in CuttlefishDaemon.BACKEND_NAMES:
            return True
        return False

    mocks = Mockingbird()

    # we need to know handlers of gRPC backends to mock it
    grpc_handlers = get_all_grpc_handlers(horizon_data)

    logging.debug("Create mocks for all HTTP and gRPC backends...")
    for backend in horizon_data.backends.values():
        if is_local(backend.name):
            logging.debug(f"Do not mock {backend} for it'll run locally")
            continue

        if backend.transport == "GRPC":
            paths = [backend.config.get("path", "/")] + list(grpc_handlers.get(backend.name, []))
            callback = STANDARD_MOCKS.get(backend.name)
            port = port_manager.get_port_range(None, 2)
            mocks.add_apphost_backend(name=backend.name, port=port, paths=paths, callback=callback)
            logging.debug(f"Mock for AppHost {backend} created (paths='{paths}', port={port})")

        elif backend.transport == "NEH":
            port = port_manager.get_port()
            callback = STANDARD_MOCKS.get(backend.name)
            mocks.add_http_backend(name=backend.name, port=port, callback=callback)
            logging.debug(f"Mock for HTTP {backend} created (port={port})")

        else:
            logging.warning(f"Unknown transport: {backend.transport}")

    # extra mock for Megamind
    port = port_manager.get_port()
    mocks.add_http_backend(
        name=CUTTLEFISH_MEGAMIND_URL, port=port, callback=STANDARD_MOCKS.get(CUTTLEFISH_MEGAMIND_URL)
    )
    logging.debug(f"Mock for extra HTTP backend '{CUTTLEFISH_MEGAMIND_URL}' created (port={port})")

    return mocks


# -------------------------------------------------------------------------------------------------
@pytest.fixture(scope="session")
def event_loop():
    loop = asyncio.get_event_loop()
    yield loop
    loop.close()


# -------------------------------------------------------------------------------------------------
@pytest.fixture(scope="session")
async def mock_srv(mocks):
    async with mocks.run():
        yield mocks


# -------------------------------------------------------------------------------------------------
@pytest.fixture(scope="session")
async def cuttlefish(port_manager, workdir, mock_srv):
    wd = os.path.join(workdir, "cuttlefish")
    os.makedirs(wd)

    d = CuttlefishDaemon(port_manager)

    megamind_mock_endpoint = mock_srv.endpoint_for(CUTTLEFISH_MEGAMIND_URL)
    megamind_mock_url = f"http://{megamind_mock_endpoint[0]}:{megamind_mock_endpoint[1]}"

    async with d.running(workdir=wd, megamind_url=megamind_mock_url, ready_timeout=10):
        yield d


# -------------------------------------------------------------------------------------------------
@pytest.fixture(scope="session")
async def apphost_rt(horizon_data, mock_srv, cuttlefish, port_manager, workdir):
    apphost_config = read_json(yatest.common.source_path(APPHOST_CONFIG))
    apphost_config.pop("unified_agent_config", None)
    apphost_env = AppHostEnvironment(
        local_arcadia_path=yatest.common.source_path(""),  # not actually used
        apphost_bin_path=yatest.common.binary_path(AppHostEnvironment.DEFAULT_APPHOST_BIN_PATH),
        apphost_evlogdump_bin_path=yatest.common.binary_path(AppHostEnvironment.DEFAULT_APPHOST_EVLOGDUMP_BIN_PATH),
        apphost_config=apphost_config,
    )

    wd = os.path.join(workdir, "apphost")
    os.makedirs(wd)
    # https://docs.yandex-team.ru/apphost/pages/apphost_ports
    # Apphost uses ports [port - 1; port + 4], so we need to reserve six ports and use
    # the second one as the main one
    port = port_manager.get_port_range(None, 6) + 1
    logging.info(f"AppHost runtime: workdir='{wd}' port={port} horizon_data='{horizon_data.path}'")

    logging.debug("Override endpoints for all mocked backends...")
    for servant in mock_srv.servants:
        backend = horizon_data.backends.get(servant.name)
        if backend is None:
            continue
        backend.set_endpoint(servant.endpoint)
        logging.info(f"Set endpoint for {backend} = {servant.endpoint} (mock)")

    logging.debug("Override endpoints for cuttlefish's nodes...")
    for name in cuttlefish.BACKEND_NAMES:
        backend = horizon_data.backends.get(name)
        if backend is None:
            continue
        backend.set_endpoint(cuttlefish.endpoint)
        logging.info(f"Set endpoint for {backend} = {cuttlefish.endpoint} (local daemon)")

    task = asyncio.create_task(
        apphost_env.run(tvm_id=None, apphost_port=port, workdir=wd, horizon_data=horizon_data, env={})
    )

    try:
        logging.debug("Wait for AppHost runtime is ready...")
        await apphost_env.wait_ready()

        logging.debug("AppHost runtime is ready!")
        yield apphost_env
    except:
        logging.exception("Failed to launch AppHost runtime")
    finally:
        apphost_env.stop()
        await task


# -------------------------------------------------------------------------------------------------
@pytest.fixture(scope="session")
def apphost_evlogdump(apphost_rt):
    return apphost_rt.evlog_dump


# -------------------------------------------------------------------------------------------------
@pytest.fixture(scope="session")
def apphost_client(apphost_rt):
    client = AppHostGrpcClient(apphost_rt.apphostd.grpc_endpoint)
    return client


# -------------------------------------------------------------------------------------------------
@pytest.fixture(scope="session")
async def uniproxy_mock(port_manager):
    async with UniproxyMock(port=port_manager.get_port()) as u:
        yield u


# -------------------------------------------------------------------------------------------------
@pytest.fixture(scope="session")
async def uniproxy2(apphost_rt, port_manager, uniproxy_mock, workdir):
    wd = os.path.join(workdir, "uniproxy2")
    os.makedirs(wd)

    u2 = Uniproxy2Daemon(
        bin_path=yatest.common.binary_path(Uniproxy2Daemon.BIN_PATH),
        config_path=yatest.common.source_path(Uniproxy2Daemon.CONFIG_PATH),
        port=port_manager.get_port(),
    )

    apphost_endpoint = apphost_rt.apphostd.grpc_endpoint
    config_patch = {
        "UserSession": {
            "AppHost": {
                "Address": f"{apphost_endpoint[0]}:{apphost_endpoint[1]}",
            },
            "PythonUniproxy": {"Host": uniproxy_mock.host, "Port": uniproxy_mock.port},
        }
    }

    async with run_daemon(u2, workdir=workdir, config_patch=config_patch) as d:
        yield d


# -------------------------------------------------------------------------------------------------
@pytest.fixture(autouse=True)
async def case_scope(mocks, apphost_evlogdump, cuttlefish, uniproxy2):
    outdir = yatest.common.test_output_path()

    start_time_us = int(time.time_ns() / 1000)
    try:
        yield
    finally:
        end_time_us = int(time.time_ns() / 1000)
        mocks.clear_records()

        await apphost_evlogdump.dump_text(
            os.path.join(outdir, "apphost.log"), start_time=start_time_us, end_time=end_time_us
        )
        await cuttlefish.evlog_dump.dump_text(
            os.path.join(outdir, "cuttlefish.log"), start_time=start_time_us, end_time=end_time_us
        )
        await uniproxy2.evlog_dump.dump_text(
            os.path.join(outdir, "uniproxy2.log"), start_time=start_time_us, end_time=end_time_us
        )
