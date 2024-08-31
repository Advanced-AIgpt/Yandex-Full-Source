import json

import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice, callback
from google.protobuf import json_format


@pytest.mark.scenario(name='Weather', handle='weather')
class TestsFeedback:

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_searchapp_like(self, alice):
        r = alice(voice('погода'))
        buttons = r.run_response.ResponseBody.Layout.SuggestButtons

        # like button is the first button
        like_button = buttons[0]
        assert like_button.ActionButton.Title == '👍'
        assert like_button.ActionButton.ActionId == 'suggest_feedback_positive_weather'

        # what if we press 'like' button?
        # type_silent should be the first directive
        # callback should be the second directive
        frame_action = r.run_response.ResponseBody.FrameActions[like_button.ActionButton.ActionId]
        dirs = frame_action.Directives.List

        type_text = dirs[0].TypeTextSilentDirective
        assert type_text.Text == '👍'

        cb_proto = dirs[1].CallbackDirective
        cb_json = json.loads(json_format.MessageToJson(cb_proto))
        cb_json['payload']['@scenario_name'] = 'Weather'  # Megamind would add this

        # click 'like' button
        r = alice(callback(name=cb_json['name'], payload=cb_json['payload']))
        assert r.run_response.ResponseBody.Layout.OutputSpeech in [
            'Спасибо за поддержку!',
            'Спасибо, хвалите меня почаще!',
            'Доброе слово и боту приятно.',
            'Спасибо, я стараюсь.',
            'Спасибо, вы мне тоже сразу понравились!',
            'Спасибо, вы тоже очень классный человек.',
            'Спасибо, я вам тоже поставила внутренний лайк!',
        ]
