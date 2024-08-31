import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import server_action


@pytest.fixture(scope='module')
def enabled_scenarios():
    return 'photoframe'


@pytest.fixture(scope='function')
def srcrwr_params(kronstadt_grpc_port, div_render_graph_stubber, render_result_response_merger_graph_stubber):
    return {
        'PHOTO_FRAME': f'localhost:{kronstadt_grpc_port}',
        'RENDER_DIV2': f'localhost:{div_render_graph_stubber.port}',
        'RENDER_RESULT_RESPONSE_MERGER': f'localhost:{render_result_response_merger_graph_stubber.port}',
    }


@pytest.mark.scenario(name='PhotoFrame', handle='photoframe')
@pytest.mark.parametrize('surface', [surface.smart_display])
@pytest.mark.experiments(
    'use_app_host_pure_PhotoFrame_scenario',
    'mm_enable_protocol_scenario=PhotoFrame',
    'teaser_settings'
)
class TestPhotoFrame():

    COLLECT_CARDS_PAYLOAD = {
        'typed_semantic_frame': {
            'centaur_collect_cards': {
            }
        },
        'utterance': '',
        'analytics': {
            'product_scenario': 'Centaur',
            'origin': 'SmartSpeaker',
            'purpose': 'collect_cards'
        }
    }

    COLLECT_TEASERS_PREVIEW_PAYLOAD = {
        'typed_semantic_frame': {
            'centaur_collect_teasers_preview': {
            }
        },
        'utterance': '',
        'analytics': {
            'product_scenario': 'Centaur',
            'origin': 'SmartSpeaker',
            'purpose': 'collect_teasers_preview'
        }
    }

    def test_teasers(self, alice):
        response = alice(server_action('@@mm_semantic_frame', self.COLLECT_CARDS_PAYLOAD))
        assert response.scenario_stages() == {'run'}

        return str(response)

    def test_teasers_preview(self, alice):
        response = alice(server_action('@@mm_semantic_frame', self.COLLECT_TEASERS_PREVIEW_PAYLOAD))
        assert response.scenario_stages() == {'run'}

        return str(response)
