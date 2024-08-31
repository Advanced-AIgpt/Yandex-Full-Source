import json

import pytest
from alice.hollywood.library.python.testing.it2.stubber import create_stubber_fixture, StubberEndpoint
from alice.megamind.protos.scenarios.request_pb2 import TScenarioRunRequest, TScenarioApplyRequest
from alice.protos.api.renderer.api_pb2 import TDivRenderData, TRenderResponse
from google.protobuf.json_format import MessageToDict


def _request_content_hasher(proto_cls, headers, content):
    proto = proto_cls()
    proto.ParseFromString(content)
    return json.dumps(MessageToDict(proto), sort_keys=True).encode('utf-8')


def run_hasher(headers, content):
    return _request_content_hasher(TScenarioRunRequest, headers, content)


def apply_hasher(headers, content):
    return _request_content_hasher(TScenarioApplyRequest, headers, content)


vins_stubber = create_stubber_fixture(
    ('sas', 'megamind-hamster'),
    84,
    [
        StubberEndpoint('/proto/app/pa/run', ['POST'], request_content_hasher=run_hasher),
        StubberEndpoint('/proto/app/pa/apply', ['POST'], request_content_hasher=apply_hasher),
        StubberEndpoint('/proto/app/pa/commit', ['POST'], request_content_hasher=apply_hasher),
    ],
    scheme='http',
    stubs_subdir='vins',
)


div_render_stubber = create_stubber_fixture(
    ('sas', 'div-renderer-prestable.div-renderer'),
    10000,
    [
        StubberEndpoint('/render', ['POST']),
    ],
    stubs_subdir='div_render',
    pseudo_grpc=True,
    type_to_proto={
        'render_data': TDivRenderData,
        'render_result': TRenderResponse,
    },
    header_filter_regexps=['content-length'],
)


@pytest.fixture(scope='function')
def srcrwr_params(vins_stubber, div_render_stubber):
    return {
        'ALICE__SCENARIO_VINS_PROXY': f'localhost:{vins_stubber.port}',
        'DIV_RENDERER': f'localhost:{div_render_stubber.port}',
    }
