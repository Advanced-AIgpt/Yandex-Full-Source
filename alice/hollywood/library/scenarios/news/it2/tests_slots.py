import logging

import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import server_action


logger = logging.getLogger(__name__)


# To run tests in RUNNER mode: ya make -Ar
# To run tests in GENERATOR mode: ya make -Ar -DIT2_GENERATOR -Z
@pytest.mark.scenario(name='News', handle='news')
@pytest.mark.parametrize('surface', [surface.station])
class TestSlots:

    def test_show_slots(self, alice):
        r = alice(server_action(name='@@mm_semantic_frame', payload={
            'typed_semantic_frame': {
                'news_semantic_frame': {
                    'max_count': {
                        'num_value': 1,
                    },
                    'skip_intro_and_ending': {
                        'bool_value': True,
                    },
                    'disable_voice_buttons': {
                        'bool_value': True,
                    },
                },
            },
            'analytics': {
                'origin': 'SmartSpeaker',
                'purpose': 'test',
            },
        }))
        return str(r)
