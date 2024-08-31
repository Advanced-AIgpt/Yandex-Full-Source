import logging
import uuid
from alice.cuttlefish.library.python.apphost_grpc_client import AppHostGrpcClient, AppHostGrpcStream
import apphost.lib.grpc.protos.service_pb2_grpc as apphost_grpc
from alice.cuttlefish.library.python.testing.items import add_grpc_request_items
from alice.cuttlefish.library.python.testing.constants import ServiceHandles
from .tests import TESTS, SECRET


DEFAULT_SECRET_ID = "sec-01ee8ekx6t3rbx1dcgec4r6jya"


# -------------------------------------------------------------------------------------------------
class Stream(AppHostGrpcStream):
    def __init__(self, *args, app_host_params=None, **kwargs):
        super().__init__(*args, **kwargs)
        self.__app_host_params = app_host_params
        self.__first = True

    def write(self, request, last=False):
        if self.__first:
            request.add_params(self.__app_host_params)
            self.__first = False

        logging.debug(f"request: {request}")
        for it in request.get_items():
            logging.debug(f"request item: {it}")

        super().write(request, last=last)


class Client(AppHostGrpcClient):
    def __init__(self, *args, app_host_params=None, **kwargs):
        super().__init__(*args, **kwargs)
        self.__app_host_params = app_host_params

    def create_stream(self, path, **kwargs):
        return Stream(stub=self.stub, path=path, app_host_params=self.__app_host_params, **kwargs)


# -------------------------------------------------------------------------------------------------
def srcrwr_cuttlefish_sources(endpoint):
    srcrwr = {}
    for name, handle in ((a, getattr(ServiceHandles, a)) for a in dir(ServiceHandles) if not a.startswith('__')):
        srcrwr[name] = f"{endpoint}/{handle}"
    return srcrwr


def match_wildcarded(pattern, name):
    if "*" not in pattern:
        return pattern == name
    if (pattern[0] == "*") and (pattern[-1] == "*"):
        return pattern[1:-1] in name
    if pattern[0] == "*":
        return name.endswith(pattern[1:])
    if pattern[-1] == "*":
        return name.startswith(pattern[:-1])
    raise RuntimeError(f"Invalid check's pattern: {pattern}")


def get_secret(uuid):
    from library.python.vault_client.instances import Production as VaultClient

    yav = VaultClient(decode_files=True)
    secret = yav.get_version(uuid)
    ver = secret["version"]
    logging.info(f"Secret's version: {ver}")
    return secret["value"]


# -------------------------------------------------------------------------------------------------
def main():
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument("--address", "-a", help="AppHost address in 'HOST:PORT/GRAPH' format", type=str, metavar="ADDR")
    parser.add_argument("--verbose", "-v", help="Enable debug output", action="store_true")
    parser.add_argument("--secret", "-s", help=f"Vault secret/version UUID (default: {DEFAULT_SECRET_ID})", default=DEFAULT_SECRET_ID)

    parser.add_argument("--list", help="List available checks", action="store_true")

    parser.add_argument("--debug", "-d",  help="Add 'dbg' AppHost parameter", action="store_true")
    parser.add_argument("--srcrwr", help="Add SRCRWR into app_host_params", nargs=2, action="append")
    parser.add_argument("--repeat", "-R", help="Repeat each check N times", type=int, metavar="N", default=1)
    parser.add_argument("checks", help="List of checks to run", nargs="*")

    args = parser.parse_args()
    logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO)

    if args.list:
        logging.debug("List available checks..")
        for k, v in TESTS.items():
            print(f" - {k}")
        exit(0)

    SECRET.update(get_secret(args.secret))

    app_host_params = {
        "srcrwr": {k: v for k,v in args.srcrwr} if args.srcrwr else {}
    }

    if args.debug:
        app_host_params["dbg"] = True

    client = Client(args.address, app_host_params=app_host_params)

    if not args.checks:
        is_needed = lambda _: True
        logging.debug(f"Run all {len(TESTS)} checks...")
    else:
        def is_needed(name):
            for i in args.checks:
                if match_wildcarded(i, name):
                    return True
            return False

    checks = args.checks
    for name, check in TESTS.items():
        if is_needed(name):
            for _ in range(args.repeat):
                check(client)
