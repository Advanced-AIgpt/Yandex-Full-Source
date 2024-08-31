import re

import pytest
from alice.hollywood.library.python.testing.it2.input import voice

import predefined_contacts
from conftest import TestVideoCallBase


@pytest.mark.experiments('video_call_ignore_capability_for_ue2e')
@pytest.mark.experiments('mocked_video_call_start')
@pytest.mark.supported_features('phone_address_book')
@pytest.mark.contacts(predefined_contacts.CONTACTS)
class TestsCall(TestVideoCallBase):

    @pytest.mark.experiments('test_video_call_const_start_video_call_id')
    def test_call_on_single_match(self, alice):
        response = alice(voice('позвони Маше'))
        assert response.scenario_stages() == {'run'}
        layout = response.run_response.ResponseBody.Layout
        output_speech = 'Маша Силантьева, уже набираю'
        assert re.match(output_speech, layout.OutputSpeech)
        directives = layout.Directives
        assert len(directives) == 1
        assert layout.Directives[0].HasField('ShowViewDirective')

        return str(response)

    def test_call_on_multiple_match(self, alice):
        response = alice(voice('позвони Артему'))
        assert response.scenario_stages() == {'run'}
        layout = response.run_response.ResponseBody.Layout
        output_speech = 'Выберите, кому звонить: Артем Белый, Артем Черный'
        assert re.match(output_speech, layout.OutputSpeech)
        directives = layout.Directives
        assert len(directives) == 2
        assert layout.Directives[0].HasField('ShowViewDirective')
        assert layout.Directives[1].HasField('TtsPlayPlaceholderDirective')

        return str(response)

    def test_call_on_no_match(self, alice):
        response = alice(voice('позвони Александре'))
        assert response.scenario_stages() == {'run'}
        layout = response.run_response.ResponseBody.Layout
        output_speech = 'Не нашла подходящего контакта'
        assert re.match(output_speech, layout.OutputSpeech)
        directives = layout.Directives
        assert len(directives) == 0

        return str(response)
