import base64

import pytest
from .conftest import TvMainScenarioTestingPreset
from alice.hollywood.library.python.testing.it2 import auth
from alice.hollywood.library.python.testing.it2.input import server_action
from alice.protos.data.tv.home.result_pb2 import TTvFeedResultData, TTvCarouselResultData
from google.protobuf import text_format
from hamcrest import assert_that, has_properties, greater_than, has_length, not_none


class TestTvGetGalleries(TvMainScenarioTestingPreset):
    def _get_galleries(self, alice, category_id, offset=None):
        payload = {
            'typed_semantic_frame': {
                'get_video_galleries_semantic_frame': {
                    'category_id': {
                        'string_value': category_id
                    }
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'get_video_card_detail'
            }
        }
        if offset is not None:
            payload['typed_semantic_frame']['get_video_galleries_semantic_frame']['offset'] = {'num_value': offset}

        return alice(server_action(name='@@mm_semantic_frame', payload=payload))

    def _get_gallery(self, alice, id):
        payload = {
            'typed_semantic_frame': {
                'get_video_gallery_semantic_frame': {
                    'id': {
                        'string_value': id
                    }
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'get_video_gallery'
            }
        }

        return alice(server_action(name='@@mm_semantic_frame', payload=payload))

    def test_get_video_galleries(self, alice):
        response = self._get_galleries(alice, 'kids')
        request_proto = TTvFeedResultData()
        request_proto.ParseFromString(base64.b64decode(response.run_response.ResponseBody.Layout.Directives[0].CallbackDirective.Payload['grpc_response']))
        return str(len(request_proto.Carousels))

    @pytest.mark.parametrize('offset', (0, 1))
    def test_get_video_galleries_main_category(self, alice, offset):
        response = self._get_galleries(alice, 'main', offset=offset)
        request_proto = TTvFeedResultData()
        request_proto.ParseFromString(base64.b64decode(response.run_response.ResponseBody.Layout.Directives[0].CallbackDirective.Payload['grpc_response']))
        return text_format.MessageToString(request_proto, as_utf8=True)

    def test_get_video_gallery_ok(self, alice):
        response = self._get_gallery(alice, 'ChFoaG9nd3dra3h5dWF1c3RoaBIIdmlkZW9odWIaDGVudGl0eV9taXhlZCABKAA=')
        request_proto = TTvCarouselResultData()
        request_proto.ParseFromString(base64.b64decode(response.run_response.ResponseBody.Layout.Directives[0].CallbackDirective.Payload['grpc_response']))
        return text_format.MessageToString(request_proto, as_utf8=True)

    def test_get_video_galleries_div_carousel(self, alice):
        response = self._get_galleries(alice, 'main')
        request_proto = TTvFeedResultData()
        request_proto.ParseFromString(base64.b64decode(response.run_response.ResponseBody.Layout.Directives[0].CallbackDirective.Payload['grpc_response']))
        divCarousel = request_proto.Carousels[0].DivCarousel
        assert_that(divCarousel, has_properties('Id', 'FRONTEND_CATEG_PROMO_MIXED',
                                                'ElementsCount', greater_than(0),
                                                'DivCardJson', not_none(),
                                                'Title', has_length(greater_than(0))))
        return text_format.MessageToString(divCarousel, as_utf8=True)

    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.experiments('use_ott_api_main')
    def test_get_ott_video_galleries(self, alice):
        response = self._get_galleries(alice, 'kids')
        request_proto = TTvFeedResultData()
        request_proto.ParseFromString(base64.b64decode(response.run_response.ResponseBody.Layout.Directives[0].CallbackDirective.Payload['grpc_response']))
        return text_format.MessageToString(request_proto, as_utf8=True)
