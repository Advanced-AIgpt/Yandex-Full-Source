import pytest
from alice.hollywood.library.python.testing.it2 import auth
from alice.hollywood.library.python.testing.it2.input import server_action

import env_states
import predefined_contacts
from conftest import TestVideoCallBase


COLLECT_MAIN_SCREEN_PAYLOAD = {
    'typed_semantic_frame': {
        'centaur_collect_main_screen': {},
    },
    'analytics': {
        'product_scenario': 'Centaur',
        'origin': 'SmartSpeaker',
        'purpose': 'centaur_collect_main_screen'
    }
}

MEMENTO_FAVORITES = {
    'ScenarioData': {
        'VideoCall': {
            '@type': 'type.googleapis.com/NAlice.NScenarios.NVideoCall.TVideoCallScenarioData',
            'Favorites': [
                {
                    'lookup_key': 'org.telegram.messenger_1111_3333'
                },
                {
                    'lookup_key': 'org.telegram.messenger_1111_5555'
                }
            ]
        }
    }
}


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.supported_features('phone_address_book')
class TestsMainScreen(TestVideoCallBase):

    @pytest.mark.experiments('video_call_widget_with_favorites')
    @pytest.mark.memento(MEMENTO_FAVORITES)
    @pytest.mark.environment_state(env_states.ENV_STATE_WITH_LOGIN)
    @pytest.mark.contacts(predefined_contacts.CONTACTS)
    def test_collect_main_screen_with_login(self, alice):
        response = alice(server_action(name='@@mm_semantic_frame', payload=COLLECT_MAIN_SCREEN_PAYLOAD))
        assert response.scenario_stages() == {'run'}
        layout = response.run_response.ResponseBody.Layout
        assert not layout.OutputSpeech
        assert response.run_response.ResponseBody.HasField('ScenarioData')
        scenario_data = response.run_response.ResponseBody.ScenarioData
        assert scenario_data.HasField('VideoCallMainScreenData')
        assert scenario_data.VideoCallMainScreenData.HasField('TelegramCardData')
        telegram_card_data = scenario_data.VideoCallMainScreenData.TelegramCardData
        assert telegram_card_data.LoggedIn is True
        assert telegram_card_data.ContactsUploaded is True
        assert telegram_card_data.UserId == '1111'
        assert len(telegram_card_data.FavoriteContactData) == 2
        assert telegram_card_data.FavoriteContactData[0].DisplayName == 'Маша Силантьева'
        assert telegram_card_data.FavoriteContactData[0].UserId == '3333'
        assert telegram_card_data.FavoriteContactData[0].LookupKey == 'org.telegram.messenger_1111_3333'

        return str(response)

    @pytest.mark.environment_state(env_states.ENV_STATE_WITHOUT_LOGIN)
    @pytest.mark.contacts(predefined_contacts.CONTACTS)
    def test_collect_main_screen_without_login(self, alice):
        response = alice(server_action(name='@@mm_semantic_frame', payload=COLLECT_MAIN_SCREEN_PAYLOAD))
        assert response.scenario_stages() == {'run'}
        layout = response.run_response.ResponseBody.Layout
        assert not layout.OutputSpeech
        assert response.run_response.ResponseBody.HasField('ScenarioData')
        scenario_data = response.run_response.ResponseBody.ScenarioData
        assert scenario_data.HasField('VideoCallMainScreenData')
        assert scenario_data.VideoCallMainScreenData.HasField('TelegramCardData')
        assert scenario_data.VideoCallMainScreenData.TelegramCardData.LoggedIn is False

        return str(response)

    @pytest.mark.experiments('video_call_widget_with_favorites')
    @pytest.mark.experiments('scenario_widget_mechanics')
    @pytest.mark.memento(MEMENTO_FAVORITES)
    @pytest.mark.environment_state(env_states.ENV_STATE_WITH_LOGIN)
    @pytest.mark.contacts(predefined_contacts.CONTACTS)
    def test_collect_main_screen_widget_mechanics_exp(self, alice):
        response = alice(server_action(name='@@mm_semantic_frame', payload=COLLECT_MAIN_SCREEN_PAYLOAD))
        assert response.scenario_stages() == {'run'}
        layout = response.run_response.ResponseBody.Layout
        assert not layout.OutputSpeech
        assert response.run_response.ResponseBody.HasField('ScenarioData')
        scenario_data = response.run_response.ResponseBody.ScenarioData
        assert scenario_data.HasField('CentaurScenarioWidgetData')
        assert scenario_data.CentaurScenarioWidgetData.WidgetType == 'video_call'
        assert len(scenario_data.CentaurScenarioWidgetData.WidgetCards) == 1
        card_data = scenario_data.CentaurScenarioWidgetData.WidgetCards[0]
        assert card_data.HasField('VideoCallCardData')
        telegram_card_data = card_data.VideoCallCardData.LoggedInCardData.TelegramCardData
        assert telegram_card_data.UserId == '1111'
        assert telegram_card_data.ContactsUploaded is True
        assert len(telegram_card_data.FavoriteContactData) == 2
        assert telegram_card_data.FavoriteContactData[0].DisplayName == 'Маша Силантьева'
        assert telegram_card_data.FavoriteContactData[0].UserId == '3333'
        assert telegram_card_data.FavoriteContactData[0].LookupKey == 'org.telegram.messenger_1111_3333'

        return str(response)
