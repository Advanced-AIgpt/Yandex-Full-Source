import pytest
from alice.hollywood.library.python.testing.it2.input import server_action
from alice.hollywood.library.python.testing.it2.stubber import create_stubber_fixture, StubberEndpoint
from google.protobuf import json_format, text_format
from alice.protos.data.video.card_detail_pb2 import TTvCardDetailResponse, TTvThinCardDetailResponse

from conftest import VideoScenarioTestingPreset


droideka_backend_stubber = create_stubber_fixture(
    'droideka.tst.smarttv.yandex.net',
    443,
    [
        StubberEndpoint('/api/v7/card_detail', ['GET']),
        StubberEndpoint('/api/v7/card_detail/thin', ['GET']),
    ],
    scheme='https',
    stubs_subdir='backend_stubs',
)


@pytest.fixture(scope="function")
def srcrwr_params(droideka_backend_stubber):
    return {
        'HOLLYWOOD_DROIDEKA_PROXY': f'http://localhost:{droideka_backend_stubber.port}:10000',
    }


def get_callback_directive(alice_response):
    assert alice_response.continue_response
    assert alice_response.continue_response.ResponseBody.Layout.Directives
    return alice_response.continue_response.ResponseBody.Layout.Directives[0].CallbackDirective


@pytest.mark.device_state({'device_id': 'f986f9a10ad22a08d000'})
class TestTvCardDetail(VideoScenarioTestingPreset):
    def _get_alice_response(self, alice, content_id, onto_id, content_type):
        payload = {
            'typed_semantic_frame': {
                'get_video_card_detail_semantic_frame': {
                    'content_id': {'string_value': content_id},
                    'onto_id': {'string_value': onto_id},
                    'content_type': {'string_value': content_type},
                }
            },
            'analytics': {'origin': 'Scenario', 'purpose': 'get_video_card_detail'},
        }
        return alice(server_action(name='@@mm_semantic_frame', payload=payload))

    def _get_response(self, alice, content_id, onto_id, content_type):
        r = self._get_alice_response(alice, content_id, onto_id, content_type)
        self._make_general_assertions(r)

        callback_directive = get_callback_directive(r)
        request_proto = TTvCardDetailResponse()
        json_format.Parse(callback_directive.Payload['grpc_response'], request_proto)
        return text_format.MessageToString(request_proto, as_utf8=True)

    def _make_general_assertions(self, alice_response):
        assert alice_response.scenario_stages() == {'run', 'continue'}

        layout = alice_response.continue_response.ResponseBody.Layout
        assert not layout.OutputSpeech
        directives = layout.Directives
        assert len(directives) == 1

        callback_directive = directives[0].CallbackDirective
        assert "grpc_response.card_detail" == callback_directive.Name

    def test_get_avod_card_detail(self, alice):
        return self._get_response(alice, '48863683b5ec3ad08474a73adab9a975', 'ruw2868427', 'vod-episode')

    def test_get_tvod_card_detail(self, alice):
        return self._get_response(alice, '4ddbfea409325a75bb692d1323d10883', 'ruw1207663', 'vod-episode')

    def test_get_svod_card_detail(self, alice):
        return self._get_response(alice, '4da281224370e16ab278891369521d6f', 'ruw2305573', 'vod-library')


@pytest.mark.device_state({'device_id': 'f986f9a10ad22a08d000'})
class TestVideoThinCardDetail(VideoScenarioTestingPreset):
    def _get_alice_response(self, alice, content_id):
        payload = {
            'typed_semantic_frame': {
                'get_video_thin_card_detail_semantic_frame': {'content_id': {'string_value': content_id}}
            },
            'analytics': {'origin': 'Scenario', 'purpose': 'video_get_thin_card_details'},
        }

        return alice(server_action(name='@@mm_semantic_frame', payload=payload))

    def _get_response(self, alice, content_id):
        r = self._get_alice_response(alice, content_id)

        callback_directive = get_callback_directive(r)
        request_proto = TTvThinCardDetailResponse()
        json_format.Parse(callback_directive.Payload['grpc_response'], request_proto)

        return text_format.MessageToString(request_proto, as_utf8=True)

    def test_get_avod_film_thin_card_detail(self, alice):
        return self._get_response(alice, '48863683b5ec3ad08474a73adab9a975')

    def test_get_avod_series_thin_card_detail(self, alice):
        return self._get_response(alice, '4a35049834f3975ebc804df28b6e11d3')

    def test_get_svod_series_thin_card_detail(self, alice):
        return self._get_response(alice, '4da4c709915c8287a181cb86f3d1b19e')
