import alice.tests.library.intent as intent
import alice.tests.library.locale as locale
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [surface.loudspeaker])
@pytest.mark.parametrize('locale', [locale.ar_sa(use_tanker=False)])
class _TestBase(object):
    owners = ('moath-alali', 'g:alice_quality')


class TestPalmTime(_TestBase):

    @pytest.mark.parametrize('command', [
        'ما هو الوقت الآن؟',  # what is the time now?
        'كم الساعة؟',  # what time is it?
        'كم الساعة الآن؟',  # what time is it now?
        'اخبرني التوقيت المحلي',  # tell me the local time
        'قولي لي الوقت بالضبط',  # say me the exact time
        'ما الوقت الحالي؟',  # what is the current time?
        'كم الوقت؟'  # what time is it?
    ])
    def test_current_time_0(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.GetTime
        assert response.intent == intent.GetTime
