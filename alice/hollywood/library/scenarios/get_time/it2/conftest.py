import json

import pytest
from alice.hollywood.library.python.testing.it2.stubber import create_stubber_fixture, StubberEndpoint
from alice.megamind.protos.scenarios.request_pb2 import TScenarioRunRequest
from google.protobuf.json_format import MessageToDict


def _request_content_hasher(proto_cls, headers, content):
    proto = proto_cls()
    proto.ParseFromString(content)
    return json.dumps(MessageToDict(proto), sort_keys=True).encode('utf-8')


def run_hasher(headers, content):
    return _request_content_hasher(TScenarioRunRequest, headers, content)


vins_stubber = create_stubber_fixture(
    ('sas', 'megamind-hamster'),
    84,
    [
        StubberEndpoint('/proto/app/pa/run', ['POST'], request_content_hasher=run_hasher),
    ],
    scheme='http',
    stubs_subdir='vins',
)


@pytest.fixture(scope='function')
def srcrwr_params(vins_stubber):
    return {
        'ALICE__SCENARIO_VINS_PROXY': f'localhost:{vins_stubber.port}',
    }
