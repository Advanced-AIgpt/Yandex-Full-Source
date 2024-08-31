import json

import yatest.common


TEST_TVM_CLIENTS_PATH = "library/recipes/tvmapi/clients/clients.json"
TVM_API_PORT_FILE = "tvmapi.port"


def get_test_tvm_api_clients_data():
    with open(yatest.common.source_path(TEST_TVM_CLIENTS_PATH), "r") as f:
        test_tvm_clients = json.loads(f.read())

    return test_tvm_clients


def get_test_tvm_api_clients_info(destinations=None):
    if destinations is None:
        destinations = []

    clients_data = get_test_tvm_api_clients_data()
    assert 1 + len(destinations) <= len(clients_data), "Too many tvm clients requested"

    client_ids = list(sorted(clients_data.keys()))

    self_id = client_ids[0]
    destination_ids = client_ids[1:len(destinations) + 1]

    return {
        "self": {
            "id": int(self_id),
            "secret": clients_data[self_id]["secret"],
        },
        "destinations": {
            destination: int(destination_id) for destination, destination_id in zip(destinations, destination_ids)
        },
    }


def get_tvm_api_port():
    with open("tvmapi.port", "r") as f:
        tvmapi_port = int(f.read())

    return tvmapi_port


def get_tvmtool_authtoken():
    with open("tvmtool.authtoken", "r") as f:
        tvmtool_authtoken = f.read()

    return tvmtool_authtoken


def get_tvmtool_port():
    with open("tvmtool.port", "r") as f:
        tvmtool_port = int(f.read())

    return tvmtool_port
