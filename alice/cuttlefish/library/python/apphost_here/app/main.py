import os
import asyncio
import logging
import signal
from alice.cuttlefish.library.python.apphost_here import AppHostEnvironment
from alice.cuttlefish.library.python.apphost_here.utils import Daemon


async def run(env, *args, **kwargs):
    loop = asyncio.get_event_loop()
    for sig in (signal.SIGINT, signal.SIGTSTP, signal.SIGTERM):
        loop.add_signal_handler(sig, env.stop)
    return await env.run(*args, **kwargs)


class LocalServantDaemon(Daemon):
    def __init__(self, port, command):
        executable, args = command.split(" ", maxsplit=1)
        command = os.path.abspath(executable) + " " + args
        self.NAME = executable.rsplit("/", maxsplit=1)[-1]
        super().__init__()
        self._cmd = command
        self._port = port

    async def run(self, workdir="./"):
        await super().run(args=self._cmd, workdir=workdir, shell=True)

    async def _wait_ready(self):
        await asyncio.sleep(1)  # check if stays alive after 1 second

    @property
    def endpoint(self):
        return ("localhost", self._port)


def main():
    import argparse
    import sys

    DEFAULT_VERTICAL = "VOICE"
    DEFAULT_APPHOST_PORT = 44000
    DEFAULT_WORK_DIR = os.path.abspath("./ah")
    DEFAUL_ARCADIA_PATH = os.path.abspath(  # works fine if the executable locates in its' birthplace
        os.path.join(os.path.dirname(sys.executable), "../" * (os.path.dirname(__file__).count("/") + 1))
    )

    parser = argparse.ArgumentParser()

    parser.add_argument("-V", "--verbose", action="store_true", help="Enable debug logs")
    parser.add_argument(
        "-A",
        "--arcadia-path",
        default=DEFAUL_ARCADIA_PATH,
        help=f"Local Arcadia's checkout path (default: {DEFAUL_ARCADIA_PATH})",
    )
    parser.add_argument(
        "-I", "--ignore-patch-errors", action="store_true", help="Ignore errors ocured during backends' patching"
    )
    parser.add_argument(
        "-a",
        "--apphost-port",
        type=int,
        default=DEFAULT_APPHOST_PORT,
        help=f"AppHost's base port (default: {DEFAULT_APPHOST_PORT})",
    )
    parser.add_argument(
        "-v", "--vertical", default=DEFAULT_VERTICAL, help=f"AppHost's vertical (default: {DEFAULT_VERTICAL})"
    )
    parser.add_argument(
        "-w",
        "--workdir",
        default=DEFAULT_WORK_DIR,
        help=f"Work directory to keep all stuff in (default: {DEFAULT_WORK_DIR})",
    )
    parser.add_argument("-t", "--tvm", help="AppHost's TVM ID (pass secret via TVM_SECRET env variable)")
    parser.add_argument(
        "-s",
        "--local-servant",
        action="append",
        nargs=3,
        help="Servant to run locally (BACKEND PORT COMMAND)",
        metavar="",
    )

    args = parser.parse_args()

    logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO)

    local_servants = {}
    if args.local_servant:
        for name, port, cmd in args.local_servant:
            local_servants[name] = LocalServantDaemon(port=int(port), command=cmd)

    env = AppHostEnvironment(
        vertical=args.vertical, local_arcadia_path=args.arcadia_path, local_servants=local_servants
    )

    asyncio.run(
        run(
            env,
            apphost_port=args.apphost_port,
            workdir=args.workdir,
            tvm_id=args.tvm,
            ignore_patch_errors=args.ignore_patch_errors,
        )
    )
