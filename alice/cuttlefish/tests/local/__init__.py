import os
import sys
import shutil
import logging
import asyncio
import random
import signal
from alice.cuttlefish.library.python.apphost_here import AppHostEnvironment, HorizonData, Graph
from alice.cuttlefish.library.python.apphost_here.utils import run_daemon, read_json, ensure_dir, remove_dir
from alice.cuttlefish.library.python.uniproxy2_daemon import Uniproxy2Daemon
from .daemons import CuttlefishDaemon, UniproxyDaemon, SubwayDaemon


APPHOST_CONFIG_PATH = "alice/uniproxy/configs/prod/configs/app_host.json"
APPHOST_CONFDIR_PATH = "apphost/conf"
VERTICAL = "VOICE"
CUTTLEFISH_BACKEND_NAMES = ["VOICE__CUTTLEFISH", "VOICE__CUTTLEFISH_BIDIRECTIONAL", "VOICE__CUTTLEFISH_MM"]


class Defaults:
    ARCADIA_PATH = os.path.abspath(  # works fine if the executable locates in its' birthplace
        os.path.join(os.path.dirname(sys.executable), "../" * (os.path.dirname(__file__).count("/") + 1))
    )

    CONFIGURATION = "ctype=testing;geo=sas"
    HORIZON_DATA_PATH = os.path.join(ARCADIA_PATH, "alice/cuttlefish/tests/apphost/horizon-data")
    CUTTLEFISH_PORT = 50000
    UNIPROXY_PORT = 50010
    UNIPROXY2_PORT = 50020
    APPHOST_PORT = 50030
    SETTINGS_PATHCER_CONFIG_PATH = "alice/cuttlefish/tests/local/patcher_dynamic_cfg.yaml"


# -------------------------------------------------------------------------------------------------
def real_endpoints(apphost_confdir_path, vertical, configuration):
    vertical_dir_path = os.path.join(apphost_confdir_path, "verticals", VERTICAL)
    backends_dir_path = os.path.join(apphost_confdir_path, "backends")

    endpoints = {}
    for graph in Graph.list_graphs(vertical_dir_path):
        for backend in graph.backends(backends_dir_path):
            if backend in endpoints:
                continue
            if backend.get_transport(configuration) not in ("NEH", "GRPC"):
                continue

            instances = backend.get_instances(configuration)
            if not instances:
                endpoint = ("localhost", 1)  # aka devnull
            else:
                instance = instances[random.randint(0, len(instances) - 1)]  # get random instance
                endpoint = (instance["host"], instance["port"])
            endpoints[backend.name] = endpoint

    return endpoints


# -------------------------------------------------------------------------------------------------
async def run(args, stop_fut):
    arcadia_path = args.arcadia_path

    def in_arcadia(path):
        return os.path.join(arcadia_path, path)

    workdir = os.path.abspath("./wd")
    try:
        remove_dir(workdir)
    except:
        pass
    ensure_dir(workdir)

    cuttlefish_bin_path = in_arcadia(CuttlefishDaemon.BIN_PATH)
    cuttlefish = CuttlefishDaemon(bin_path=in_arcadia(CuttlefishDaemon.BIN_PATH), port=args.cuttlefish_port)

    uniproxy = UniproxyDaemon(in_arcadia(UniproxyDaemon.BIN_PATH), port=args.uniproxy_port)

    subway = SubwayDaemon(in_arcadia(SubwayDaemon.BIN_PATH))  # port is hardcoded as 7777

    uniproxy2 = Uniproxy2Daemon(
        bin_path=in_arcadia(Uniproxy2Daemon.BIN_PATH),
        config_path=in_arcadia(Uniproxy2Daemon.CONFIG_PATH),
        port=args.uniproxy2_port,
        patcher_config_path=in_arcadia(Defaults.SETTINGS_PATHCER_CONFIG_PATH),
    )

    apphost_config = read_json(in_arcadia(APPHOST_CONFIG_PATH))
    apphost_bin_path = in_arcadia(AppHostEnvironment.DEFAULT_APPHOST_BIN_PATH)
    apphost_evlog_bin_path = in_arcadia(AppHostEnvironment.DEFAULT_APPHOST_EVLOGDUMP_BIN_PATH)
    apphost_env = AppHostEnvironment(
        local_arcadia_path=arcadia_path,
        apphost_bin_path=apphost_bin_path,
        apphost_evlogdump_bin_path=apphost_evlog_bin_path,
        apphost_config=apphost_config,
    )

    horizon_data = HorizonData(args.horizon_data)
    for name, endpoint in real_endpoints(in_arcadia(APPHOST_CONFDIR_PATH), VERTICAL, args.configuration).items():
        backend = horizon_data.backends[name]
        backend.set_endpoint(endpoint)
        logging.info(f"Set endpoint for {backend} = {endpoint}")

    # launch the world
    async with run_daemon(cuttlefish, workdir=workdir):
        for name in CUTTLEFISH_BACKEND_NAMES:
            if name not in horizon_data.backends:
                logging.warning(f"Skip '{name}' backend name of cuttlefish for it doesn't exist in horizon-data")
                continue
            horizon_data.backends[name].set_endpoint(cuttlefish.endpoint)

        async with apphost_env.running(apphost_port=args.apphost_port, horizon_data=horizon_data, workdir=workdir):
            apphost_endpoint = apphost_env.apphostd.grpc_endpoint

            async with run_daemon(subway, workdir=workdir):
                async with run_daemon(uniproxy, workdir=workdir):

                    uniproxy2_config_patch = {
                        "UserSession": {
                            "AppHost": {"Address": f"{apphost_endpoint[0]}:{apphost_endpoint[1]}"},
                            "PythonUniproxy": {"Host": uniproxy.endpoint[0], "Port": uniproxy.endpoint[1]},
                        }
                    }

                    async with run_daemon(uniproxy2, workdir=workdir, config_patch=uniproxy2_config_patch):
                        logging.info("all is ready!")
                        await stop_fut
                        logging.info("stop all...")


# -------------------------------------------------------------------------------------------------
def main():
    import argparse

    async def async_main(args):
        stop_fut = asyncio.Future()

        def set_stop_fut(*args):
            if not stop_fut.done():
                stop_fut.set_result(None)

        loop = asyncio.get_event_loop()
        for sig in (signal.SIGINT, signal.SIGTSTP, signal.SIGTERM):
            loop.add_signal_handler(sig, set_stop_fut)

        try:
            await run(args, stop_fut)
        except Exception as err:
            logging.exception(f"FAILED: {err}")

    parser = argparse.ArgumentParser()

    def add_argument(*args, **kwargs):
        if "default" in kwargs:
            default = kwargs["default"]
            kwargs["help"] = kwargs.pop("help", "") + f" (default: {default})"
        parser.add_argument(*args, **kwargs)

    add_argument(
        "-A", "--arcadia-path", metavar="PATH", default=Defaults.ARCADIA_PATH, help=f"Path to local Arcadia root"
    )
    add_argument(
        "-H",
        "--horizon-data",
        metavar="PATH",
        default=Defaults.HORIZON_DATA_PATH,
        help=f"Path to horizon-data directory",
    )
    add_argument(
        "-C",
        "--configuration",
        metavar="CONFIG",
        default=Defaults.CONFIGURATION,
        help=f"AppHost's configuration for backend resolving",
    )
    add_argument("-w", "--work-dir", metavar="PATH", default="./wd", help=f"Work directory, will be recreate if exists")

    add_argument(
        "-c",
        "--cuttlefish-port",
        type=int,
        metavar="PORT",
        default=Defaults.CUTTLEFISH_PORT,
        help=f"Base port (HTTP) for cuttlefish, gRPC port will be +1",
    )
    add_argument(
        "-a",
        "--apphost-port",
        type=int,
        metavar="PORT",
        default=Defaults.APPHOST_PORT,
        help=f"Base port (HTTP) for app_host, gRPC port will be +1",
    )
    add_argument(
        "-u", "--uniproxy-port", type=int, metavar="PORT", default=Defaults.UNIPROXY_PORT, help=f"Port for uniproxy"
    )
    add_argument(
        "-2", "--uniproxy2-port", metavar="PORT", type=int, default=Defaults.UNIPROXY2_PORT, help=f"Port for uniproxy2"
    )

    args = parser.parse_args()

    logging.basicConfig(level=logging.DEBUG)
    asyncio.run(async_main(args))
