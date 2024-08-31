import pytest
from alice.hollywood.library.python.testing.it2 import auth, surface
from alice.hollywood.library.python.testing.it2.input import voice, server_action


SCENARIO_NAME = 'Weather'
SCENARIO_HANDLE = 'weather'


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.scenario(name=SCENARIO_NAME, handle=SCENARIO_HANDLE)
@pytest.mark.parametrize('surface', [surface.smart_display])
class SmartDisplayTestBase:
    pass


class TestsGetWeather(SmartDisplayTestBase):

    @pytest.mark.parametrize('command', [
        pytest.param('погода в москве', id='current'),
        pytest.param('какая погода сегодня', id='today'),
        pytest.param('какой прогноз погоды на завтра', id='tommorow'),
        pytest.param('скажи погоду на завтрашнее утро', id='day_part'),
        pytest.param('какая погода в казани в четверг', id='some_day'),
        pytest.param('скажи погоду на следующую неделю', id='day_range')
    ])
    def test_get_weather(self, alice, command):
        r = alice(voice(command))

        assert r.scenario_stages() == {'run'}
        response_body = r.run_response.ResponseBody
        directives = response_body.Layout.Directives
        assert len(directives) == 2

        assert directives[0].HasField('ShowViewDirective')
        assert directives[0].ShowViewDirective.HasField('Div2Card')

        assert directives[1].HasField('TtsPlayPlaceholderDirective')

        return str(r)

    def test_get_teaser(self, alice):
        r = alice(server_action(name='@@mm_semantic_frame', payload={
            'typed_semantic_frame': {
                'centaur_collect_cards': {}
            },
            'analytics': {
                'origin': 'SmartSpeaker',
                'purpose': 'teasers'
            }
        }))

        assert r.scenario_stages() == {'run'}
        response_body = r.run_response.ResponseBody
        directives = response_body.Layout.Directives
        assert len(directives) == 1

        assert directives[0].HasField('AddCardDirective')
        assert directives[0].AddCardDirective.HasField('Div2Card')

        return str(r)
