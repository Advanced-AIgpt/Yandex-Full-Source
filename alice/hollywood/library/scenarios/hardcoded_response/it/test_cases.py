from alice.hollywood.library.python.testing.integration.conftest import create_hollywood_fixture
from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import make_generator_params, \
    make_runner_params
from alice.library.python.testing.megamind_request.input_dialog import text
from alice.megamind.protos.scenarios.request_pb2 import TScenarioRunRequest, TUserClassification

from google.protobuf import text_format

TESTS_DATA_PATH = 'alice/hollywood/library/scenarios/hardcoded_response/it/data/'

SCENARIO_NAME = 'HardcodedResponse'
SCENARIO_HANDLE = 'hardcoded_response'

hollywood = create_hollywood_fixture([SCENARIO_HANDLE])

# For available presets see alice/acceptance/modules/request_generator/lib/app_presets.py
DEFAULT_APP_PRESETS = ['quasar', 'search_app_prod']

DEFAULT_EXPERIMENTS = ['mm_enable_protocol_scenario=HardcodedResponse']


def _enable_child_mode(run_request):
    run_request_proto = TScenarioRunRequest()
    text_format.Merge(run_request, run_request_proto)

    run_request_proto.BaseRequest.UserClassification.Age = TUserClassification.Child

    return text_format.MessageToString(run_request_proto, as_utf8=True)


TESTS_DATA = {
    'test_hardcoded_response': {
        'input_dialog': [
            text('скажи заготовленную реплику'),
        ],
    },
    "test_hardcoded_response_2": {
        'input_dialog': [
            text('скажи другую заготовленную реплику'),
        ],
    },
    'test_hardcoded_child_response': {
        'input_dialog': [
            text('скажи заготовленную реплику', request_patcher=_enable_child_mode),
        ],
    },
}

TEST_GEN_PARAMS = make_generator_params(TESTS_DATA, DEFAULT_APP_PRESETS)

TEST_RUN_PARAMS = make_runner_params(TESTS_DATA, DEFAULT_APP_PRESETS)
