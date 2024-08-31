import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import server_action


@pytest.mark.scenario(name='Weather', handle='weather')
@pytest.mark.parametrize('surface', [surface.station])
class _TestsSemanticFrameBase:
    pass


class TestsSemanticFrame(_TestsSemanticFrameBase):

    ANALYTICS = {
        'origin': 'SmartSpeaker',
        'purpose': 'test',
    }

    def test_empty_sf(self, alice):
        r = alice(server_action(name='@@mm_semantic_frame', payload={
            'typed_semantic_frame': {
                'weather_semantic_frame': {
                },
            },
            'analytics': self.ANALYTICS,
        }))
        return r.run_response.ResponseBody.Layout.Cards[0].Text

    def test_when_slot(self, alice):
        r = alice(server_action(name='@@mm_semantic_frame', payload={
            'typed_semantic_frame': {
                'weather_semantic_frame': {
                    'when': {
                        'datetime_value': '{"days":2,"days_relative":true}',
                    },
                },
            },
            'analytics': self.ANALYTICS,
        }))
        return r.run_response.ResponseBody.Layout.Cards[0].Text

    def test_where_string_slot(self, alice):
        r = alice(server_action(name='@@mm_semantic_frame', payload={
            'typed_semantic_frame': {
                'weather_semantic_frame': {
                    'where': {
                        'where_value': 'босния',
                    },
                },
            },
            'analytics': self.ANALYTICS,
        }))
        return r.run_response.ResponseBody.Layout.Cards[0].Text

    def test_where_lat_lon_slot(self, alice):
        r = alice(server_action(name='@@mm_semantic_frame', payload={
            'typed_semantic_frame': {
                'weather_semantic_frame': {
                    'where': {
                        'lat_lon_value': {
                            'latitude': 57.819231,
                            'longitude': 28.332202,
                        },
                    },
                },
            },
            'analytics': self.ANALYTICS,
        }))
        return r.run_response.ResponseBody.Layout.Cards[0].Text
