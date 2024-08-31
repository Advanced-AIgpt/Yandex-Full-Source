import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import server_action


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['repeat_after_me']


@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.scenario(name='RepeatAfterMe', handle='repeat_after_me')
class _TestsBase:
    pass


class Tests(_TestsBase):
    def test_server_action(self, alice):
        payload = {
            'typed_semantic_frame': {
                'repeat_after_me_semantic_frame': {
                    'text': {
                        'string_value': 'Пыщь пыщь',
                    },
                    'voice': {
                        'string_value': '<ultra_loud>Пыщь пыщь</ultra_loud>',
                    },
                },
            },
            'analytics': {
                'origin': 'Web',
                'purpose': 'test',
            },
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.run_response.ResponseBody.Layout.Cards[0].Text == 'Пыщь пыщь'
        assert r.run_response.ResponseBody.Layout.OutputSpeech == '<ultra_loud>Пыщь пыщь</ultra_loud>'
