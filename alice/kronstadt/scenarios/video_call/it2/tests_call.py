import re

import pytest
from alice.hollywood.library.python.testing.it2 import auth
from alice.hollywood.library.python.testing.it2.input import voice, server_action

import env_states
import predefined_contacts
from conftest import TestVideoCallBase


def _check_matched_contacts_analytics(response, expected_lookup_keys):
    analytics = response.run_response.ResponseBody.AnalyticsInfo
    assert analytics.ProductScenarioName == 'video_call'
    assert analytics.Intent == 'alice_scenarios.video_call_to'
    matched_contacts = analytics.Objects[0]
    assert matched_contacts.Id == 'call_contacts'
    assert matched_contacts.Name == 'matched contacts'
    assert matched_contacts.HumanReadable == 'Найденные контакты'

    actual_lookup_keys = [
        contact.LookupKey
        for contact in matched_contacts.MatchedContacts.Contacts
    ]
    assert actual_lookup_keys == expected_lookup_keys


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.supported_features('phone_address_book')
class TestsCall(TestVideoCallBase):

    @pytest.mark.experiments('test_video_call_const_start_video_call_id')
    @pytest.mark.contacts(predefined_contacts.CONTACTS)
    @pytest.mark.environment_state(env_states.ENV_STATE_WITH_LOGIN)
    def test_call_on_single_match(self, alice):
        response = alice(voice('позвони Маше'))
        assert response.scenario_stages() == {'run'}
        layout = response.run_response.ResponseBody.Layout
        output_speech = 'Маша Силантьева, уже набираю'
        assert re.match(output_speech, layout.OutputSpeech)
        directives = layout.Directives
        assert len(directives) == 2
        assert layout.Directives[0].HasField('StartVideoCallDirective')
        assert layout.Directives[1].HasField('ShowViewDirective')

        expected_lookup_keys = ['org.telegram.messenger_1111_3333']
        _check_matched_contacts_analytics(response, expected_lookup_keys)
        return str(response)

    @pytest.mark.contacts(predefined_contacts.CONTACTS)
    @pytest.mark.environment_state(env_states.ENV_STATE_WITH_LOGIN)
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

        expected_lookup_keys = ['org.telegram.messenger_1111_4444', 'org.telegram.messenger_1111_5555']
        _check_matched_contacts_analytics(response, expected_lookup_keys)
        return str(response)

    @pytest.mark.contacts(predefined_contacts.CONTACTS)
    @pytest.mark.environment_state(env_states.ENV_STATE_WITH_LOGIN)
    def test_call_on_no_match(self, alice):
        response = alice(voice('позвони Александре'))
        assert response.scenario_stages() == {'run'}
        layout = response.run_response.ResponseBody.Layout
        output_speech = 'Не нашла подходящего контакта'
        assert re.match(output_speech, layout.OutputSpeech)
        directives = layout.Directives
        assert len(directives) == 0

        return str(response)

    @pytest.mark.experiments('test_video_call_const_start_login_id')
    @pytest.mark.contacts(predefined_contacts.CONTACTS)
    @pytest.mark.environment_state(env_states.ENV_STATE_WITHOUT_LOGIN)
    def test_call_on_no_login(self, alice):
        response = alice(voice('позвони Александре'))
        assert response.scenario_stages() == {'run'}
        layout = response.run_response.ResponseBody.Layout
        assert not layout.OutputSpeech
        directives = layout.Directives
        assert len(directives) == 2
        assert layout.Directives[0].HasField('StartVideoCallLoginDirective')
        assert layout.Directives[1].HasField('ShowViewDirective')

        return str(response)

    @pytest.mark.environment_state(env_states.ENV_STATE_WITH_INCOMING_CALL)
    def test_call_accept_incoming_call(self, alice):
        response = alice(voice('возьми трубку'))
        assert response.scenario_stages() == {'run'}
        layout = response.run_response.ResponseBody.Layout
        assert not layout.OutputSpeech
        directives = layout.Directives
        assert len(directives) == 3
        assert layout.Directives[0].HasField('AcceptVideoCallDirective')
        assert layout.Directives[1].HasField('ShowViewDirective')
        assert layout.Directives[2].HasField('HideViewDirective')

        return str(response)

    @pytest.mark.environment_state(env_states.ENV_STATE_WITH_INCOMING_CALL)
    def test_call_discard_incoming_call(self, alice):
        response = alice(voice('пропусти звонок'))
        assert response.scenario_stages() == {'run'}
        layout = response.run_response.ResponseBody.Layout
        assert not layout.OutputSpeech
        directives = layout.Directives
        assert len(directives) == 1
        assert layout.Directives[0].HasField('DiscardVideoCallDirective')

        return str(response)

    @pytest.mark.environment_state(env_states.ENV_STATE_WITH_LOGIN)
    def test_outgoing_failed_callback(self, alice):
        response = alice(server_action(name='@@mm_semantic_frame', payload={
            'typed_semantic_frame': {
                'video_call_outgoing_failed_semantic_frame': {
                    'provider': {
                        'enum_value': 'Telegram'
                    }
                }
            },
            'analytics': {
                'origin': 'SmartSpeaker',
                'purpose': 'video_call_outgoing_failed'
            }
        }))
        assert response.scenario_stages() == {'run'}
        layout = response.run_response.ResponseBody.Layout
        output_speech = 'Что-то пошло не так'
        assert re.match(output_speech, layout.OutputSpeech)

        return str(response)

    @pytest.mark.environment_state(env_states.ENV_STATE_WITH_LOGIN)
    def test_incoming_accept_failed_callback(self, alice):
        response = alice(server_action(name='@@mm_semantic_frame', payload={
            'typed_semantic_frame': {
                'video_call_incoming_accept_failed_semantic_frame': {
                    'provider': {
                        'enum_value': 'Telegram'
                    },
                    'call_id': {
                        'string_value': 'call_id'
                    }
                }
            },
            'analytics': {
                'origin': 'SmartSpeaker',
                'purpose': 'video_call_incoming_accept_failed'
            }
        }))
        assert response.scenario_stages() == {'run'}
        layout = response.run_response.ResponseBody.Layout
        output_speech = 'Что-то пошло не так'
        assert re.match(output_speech, layout.OutputSpeech)
        return str(response)

    @pytest.mark.environment_state(env_states.ENV_STATE_WITH_ACTIVE_CURRENT_CALL)
    def test_outgoing_accepted(self, alice):
        response = alice(server_action(name='@@mm_semantic_frame', payload={
            'typed_semantic_frame': {
                'video_call_outgoing_accepted_semantic_frame': {
                    'provider': {
                        'enum_value': 'Telegram'
                    },
                    'call_id': {
                        'string_value': 'outgoing_video_call_id'
                    },
                    'user_id': {
                        'string_value': '1111'
                    },
                    'contact': {
                        'contact_data': {
                            'telegram_contact_data': {
                                'user_id': '3333'
                            }
                        }
                    }
                }
            },
            'analytics': {
                'origin': 'SmartSpeaker',
                'purpose': 'video_call_outgoing_accepted'
            }
        }))
        assert response.scenario_stages() == {'run'}
        layout = response.run_response.ResponseBody.Layout
        assert not layout.OutputSpeech
        directives = layout.Directives
        assert len(directives) == 1
        assert layout.Directives[0].HasField('ShowViewDirective')

        return str(response)

    @pytest.mark.experiments('test_video_call_const_start_video_call_id')
    @pytest.mark.contacts(predefined_contacts.CONTACTS)
    @pytest.mark.environment_state(env_states.ENV_STATE_WITH_LOGIN)
    def test_call_on_typed_semantic_frame(self, alice):
        response = alice(server_action(name='@@mm_semantic_frame', payload={
            'typed_semantic_frame': {
                'phone_call_semantic_frame': {
                    'item_name': {
                        'item_name_value': 'org.telegram.messenger_1111_3333'
                    }
                }
            },
            'analytics': {
                'origin': 'SmartSpeaker',
                'purpose': 'video_call_outgoing_accepted'
            }
        }))
        assert response.scenario_stages() == {'run'}
        layout = response.run_response.ResponseBody.Layout
        output_speech = 'Маша Силантьева, уже набираю'
        assert re.match(output_speech, layout.OutputSpeech)
        directives = layout.Directives
        assert len(directives) == 2
        assert layout.Directives[0].HasField('StartVideoCallDirective')
        assert layout.Directives[1].HasField('ShowViewDirective')

        expected_lookup_keys = ['org.telegram.messenger_1111_3333']
        _check_matched_contacts_analytics(response, expected_lookup_keys)
        return str(response)

    @pytest.mark.experiments('test_video_call_const_start_video_call_id')
    @pytest.mark.contacts(predefined_contacts.CONTACTS)
    @pytest.mark.environment_state(env_states.ENV_STATE_WITH_LOGIN)
    def test_call_on_video_call_to_frame(self, alice):
        response = alice(server_action(name='@@mm_semantic_frame', payload={
            'typed_semantic_frame': {
                'video_call_to_semantic_frame': {
                    'fixed_contact': {
                        'contact_data': {
                            'telegram_contact_data': {
                                'user_id': '3333',
                                'display_name': 'Маша Силантьева'
                            }
                        }
                    },
                    'video_enabled': {
                        'bool_value': True
                    }
                }
            },
            'analytics': {
                'origin': 'SmartSpeaker',
                'purpose': 'video_call_outgoing_accepted'
            }
        }))
        assert response.scenario_stages() == {'run'}
        layout = response.run_response.ResponseBody.Layout
        output_speech = 'Маша Силантьева, уже набираю'
        assert re.match(output_speech, layout.OutputSpeech)
        directives = layout.Directives
        assert len(directives) == 2
        assert layout.Directives[0].HasField('StartVideoCallDirective')
        assert layout.Directives[1].HasField('ShowViewDirective')

        expected_lookup_keys = ['org.telegram.messenger_1111_3333']
        _check_matched_contacts_analytics(response, expected_lookup_keys)
        return str(response)

    @pytest.mark.experiments('incoming_call_render_data_exp')
    @pytest.mark.contacts(predefined_contacts.CONTACTS)
    @pytest.mark.environment_state(env_states.ENV_STATE_WITH_LOGIN)
    def test_incoming_video_call(self, alice):
        response = alice(server_action(name='@@mm_semantic_frame', payload={
            'typed_semantic_frame': {
                'video_call_incoming_semantic_frame': {
                    'provider': {
                        'enum_value': 'Telegram'
                    },
                    'call_id': {
                        'string_value': 'incoming_video_call_id'
                    },
                    'user_id': {
                        'string_value': '1111'
                    },
                    'caller': {
                        'contact_data': {
                            'telegram_contact_data': {
                                'user_id': '3333'
                            }
                        }
                    }
                }
            },
            'analytics': {
                'origin': 'SmartSpeaker',
                'purpose': 'alice.video_call_incoming'
            }
        }))
        assert response.scenario_stages() == {'run'}
        layout = response.run_response.ResponseBody.Layout
        output_speech = 'Входящий звонок от Маша Силантьева'
        assert re.match(output_speech, layout.OutputSpeech)
        directives = layout.Directives
        assert len(directives) == 2
        assert layout.Directives[0].HasField('ShowViewDirective')
        assert layout.Directives[1].HasField('TtsPlayPlaceholderDirective')
        actions = response.run_response.ResponseBody.FrameActions
        assert len(actions) == 2
        assert 'accept_incoming_call' in actions
        assert 'discard_incoming_call' in actions

        accept_action = actions['accept_incoming_call']
        assert accept_action.NluHint.FrameName == 'alice.messenger_call.accept_incoming_call'
        assert len(accept_action.Directives.List) == 3
        assert accept_action.Directives.List[0].HasField('AcceptVideoCallDirective')
        assert accept_action.Directives.List[1].HasField('ShowViewDirective')
        assert accept_action.Directives.List[2].HasField('HideViewDirective')

        discard_action = actions['discard_incoming_call']
        assert discard_action.NluHint.FrameName == 'alice.messenger_call.stop_incoming_call'
        assert len(discard_action.Directives.List) == 1
        assert discard_action.Directives.List[0].HasField('DiscardVideoCallDirective')

        return str(response)

    @pytest.mark.contacts(predefined_contacts.CONTACTS)
    @pytest.mark.environment_state(env_states.ENV_STATE_WITH_LOGIN)
    def test_incoming_video_call_unknown_contact(self, alice):
        response = alice(server_action(name='@@mm_semantic_frame', payload={
            'typed_semantic_frame': {
                'video_call_incoming_semantic_frame': {
                    'provider': {
                        'enum_value': 'Telegram'
                    },
                    'call_id': {
                        'string_value': 'incoming_video_call_id'
                    },
                    'user_id': {
                        'string_value': '1111'
                    },
                    'caller': {
                        'contact_data': {
                            'telegram_contact_data': {
                                'user_id': '9999'
                            }
                        }
                    }
                }
            },
            'analytics': {
                'origin': 'SmartSpeaker',
                'purpose': 'alice.video_call_incoming'
            }
        }))
        assert response.scenario_stages() == {'run'}
        assert response.is_run_relevant()
        assert not response.run_response.ResponseBody.Layout.OutputSpeech
        return str(response)

    @pytest.mark.environment_state(env_states.ENV_STATE_WITH_ACTIVE_CURRENT_CALL)
    def test_call_hangup_current_call(self, alice):
        response = alice(voice('положи трубку'))
        assert response.scenario_stages() == {'run'}
        layout = response.run_response.ResponseBody.Layout
        assert not layout.OutputSpeech
        directives = layout.Directives
        assert len(directives) == 1
        assert layout.Directives[0].HasField('DiscardVideoCallDirective')
        return str(response)

    @pytest.mark.experiments('start_video_call_directives')
    @pytest.mark.contacts(predefined_contacts.CONTACTS)
    @pytest.mark.environment_state(env_states.ENV_STATE_WITHOUT_SUPPORTED_DIRECTIVES)
    def test_call_on_single_match_without_supporting(self, alice):
        response = alice(voice('позвони Маше'))
        assert response.scenario_stages() == {'run'}
        assert response.is_run_irrelevant()
        return str(response)

    @pytest.mark.environment_state(env_states.ENV_STATE_WITHOUT_SUPPORTED_DIRECTIVES)
    def test_accept_incoming_call_without_supporting(self, alice):
        response = alice(voice('возьми трубку'))
        assert response.scenario_stages() == {'run'}
        assert response.is_run_irrelevant()
        return str(response)

    @pytest.mark.contacts(predefined_contacts.CONTACTS)
    @pytest.mark.environment_state(env_states.ENV_STATE_WITHOUT_SUPPORTED_DIRECTIVES_AND_LOGIN)
    def test_call_on_no_login_without_supporting(self, alice):
        response = alice(voice('позвони Александре'))
        assert response.scenario_stages() == {'run'}
        assert response.is_run_irrelevant()
        return str(response)
