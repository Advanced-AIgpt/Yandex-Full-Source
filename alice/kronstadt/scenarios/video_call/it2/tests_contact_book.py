import pytest
from alice.hollywood.library.python.testing.it2 import auth
from alice.hollywood.library.python.testing.it2.input import server_action
from alice.kronstadt.scenarios.video_call.src.main.proto.video_call_pb2 import TVideoCallScenarioData

import env_states
import predefined_contacts
from conftest import TestVideoCallBase


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.supported_features('phone_address_book')
class TestsContactBook(TestVideoCallBase):

    def _get_scenario_data(self, any):
        scenario_data = TVideoCallScenarioData()
        any.Unpack(scenario_data)
        return scenario_data

    @pytest.mark.environment_state(env_states.ENV_STATE_WITH_LOGIN)
    @pytest.mark.contacts(predefined_contacts.CONTACTS)
    def test_open_contacts(self, alice):
        response = alice(server_action(name='@@mm_semantic_frame', payload={
            'typed_semantic_frame': {
                'open_address_book_semantic_frame': {},
            },
            'analytics': {
                'origin': 'SmartSpeaker',
                'purpose': 'open_address_book'
            }
        }))
        assert response.scenario_stages() == {'run'}

        return str(response)

    @pytest.mark.environment_state(env_states.ENV_STATE_WITH_LOGIN)
    @pytest.mark.contacts(predefined_contacts.CONTACTS)
    def test_set_favorites(self, alice):
        response = alice(server_action(name='@@mm_semantic_frame', payload={
            'typed_semantic_frame': {
                'video_call_set_favorites_semantic_frame': {
                    'favorites': {
                        'contact_list': {
                            'contact_data': [
                                {
                                    'telegram_contact_data': {
                                        'user_id': '3333'
                                    }
                                },
                                {
                                    'telegram_contact_data': {
                                        'user_id': '5555'
                                    }
                                }
                            ]
                        }
                    }
                }
            },
            'analytics': {
                'origin': 'SmartSpeaker',
                'purpose': 'alice.video_call_set_favorites'
            }
        }))
        assert response.scenario_stages() == {'run'}

        server_directives = response.run_response.ResponseBody.ServerDirectives
        assert server_directives[0].HasField('MementoChangeUserObjectsDirective')
        memento = server_directives[0].MementoChangeUserObjectsDirective
        scenario_data = self._get_scenario_data(memento.UserObjects.ScenarioData)
        assert len(scenario_data.Favorites) == 2
        assert scenario_data.Favorites[0].LookupKey == 'org.telegram.messenger_1111_3333'
        assert scenario_data.Favorites[1].LookupKey == 'org.telegram.messenger_1111_5555'

        return str(response)
