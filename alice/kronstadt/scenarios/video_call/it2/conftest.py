import pytest
from alice.hollywood.library.python.testing.it2 import surface


TEST_DEVICE_ID = 'feedface-e8a2-4439-b2e7-689d95f277b7'


@pytest.fixture(scope='module')
def enabled_scenarios():
    return 'video_call'


@pytest.fixture(scope='function')
def srcrwr_params(kronstadt_grpc_port, div_render_graph_stubber, render_result_response_merger_graph_stubber):
    return {
        'VIDEO_CALL': f'localhost:{kronstadt_grpc_port}',
        'RENDER_DIV2': f'localhost:{div_render_graph_stubber.port}',
        'RENDER_RESULT_RESPONSE_MERGER': f'localhost:{render_result_response_merger_graph_stubber.port}',
    }


@pytest.mark.scenario(name='VideoCall', handle='video_call')
@pytest.mark.parametrize('surface', [surface.smart_display])
@pytest.mark.experiments(
    'mm_enable_begemot_contacts',
    'mm_enable_protocol_scenario=VideoCall'
)
@pytest.mark.device_state({
    'device_id': TEST_DEVICE_ID
})
class TestVideoCallBase(object):
    pass
