import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


ANALYTICS_BLOCK = {
    'origin': 1,
    'origin_info': 'origin_infoorigin_infoorigin_info',
    'product_scenario': 'test_psn',
    'purpose': 'purposepurposepurpose'
}

CONDITIONAL_ACTION_CONTINUE_TO_PAUSE = {
    'screen_conditional_action': {
        'some_screen': {
            'conditional_actions': [
                {
                    'conditional_semantic_frame': {
                        'player_continue_semantic_frame': {
                        }
                    },
                    'effect_frame_request_data': {
                        'analytics': ANALYTICS_BLOCK,
                        'origin': {},
                        'typed_semantic_frame': {
                            'player_pause_semantic_frame': {}
                        }
                    }
                },
            ]
        },
    },
}

CONDITIONAL_ACTIONS_VIDEO_PLAY_TO_PLAYER_COMMAND = {
    'screen_conditional_action': {
        'some_screen': {
            'conditional_actions': [
                {
                    'conditional_semantic_frame': {
                        'video_play_semantic_frame': {
                            'season': {
                                'num_value': 1
                            }
                        }
                    },
                    'effect_frame_request_data': {
                        'analytics': ANALYTICS_BLOCK,
                        'origin': {},
                        'typed_semantic_frame': {
                            'sound_set_level_semantic_frame': {
                                'level': {
                                    'num_level_value': 2
                                }
                            }
                        }
                    }
                },
                {
                    'conditional_semantic_frame': {
                        'video_play_semantic_frame': {
                            'season': {
                                'num_value': 2
                            }
                        }
                    },
                    'effect_frame_request_data': {
                        'analytics': ANALYTICS_BLOCK,
                        'origin': {},
                        'typed_semantic_frame': {
                            'player_pause_semantic_frame': {}
                        }
                    }
                },
            ]
        },
    },
}

SPACE_ACTION_CONTINUE_TO_WEATHER = {
    'semantic_frames': {
        'personal_assistant.scenarios.player.continue': {
            'typed_semantic_frame': {
                'weather_semantic_frame': {}
            },
            'analytics': ANALYTICS_BLOCK
        }
    }
}

SPACE_ACTION_CONTINUE_TO_PAUSE = {
    'semantic_frames': {
        'personal_assistant.scenarios.player.continue': {
            'typed_semantic_frame': {
                'player_pause_semantic_frame': {}
            },
            'analytics': ANALYTICS_BLOCK
        }
    }
}

MULTI_SPACE_ACTION = {
    'semantic_frames': {
        'personal_assistant.scenarios.sound.louder': {
            'typed_semantic_frame': {
                'player_pause_semantic_frame': {}
            },
            'analytics': ANALYTICS_BLOCK
        },
        'personal_assistant.scenarios.sound.quiter': {
            'typed_semantic_frame': {
                'sound_set_level_semantic_frame': {
                    'level': {
                        'num_level_value': 2
                    }
                }
            },
            'analytics': ANALYTICS_BLOCK
        }
    }
}

MUSIC_VIDEO_SPACE_ACTION = {
    'semantic_frames': {
        'personal_assistant.scenarios.music_play': {
            'typed_semantic_frame': {
                'player_pause_semantic_frame': {}
            },
            'analytics': ANALYTICS_BLOCK
        },
        'personal_assistant.scenarios.video_play': {
            'typed_semantic_frame': {
                'sound_set_level_semantic_frame': {
                    'level': {
                        'num_level_value': 2
                    }
                }
            },
            'analytics': ANALYTICS_BLOCK
        }
    }
}


@pytest.mark.voice
@pytest.mark.parametrize('surface', [surface.smart_display])
class TestConditionalActions(object):
    """
    Тестируем логику мегамаинда для голосовых кнопок
    https://wiki.yandex-team.ru/users/nkodosov/primery-raboty-s-golosovymi-knopkami/
    Примеры фреймов и директив выбраны случайно
    """

    owners = ('nkodosov', )

    @pytest.mark.device_state(active_actions=CONDITIONAL_ACTION_CONTINUE_TO_PAUSE)
    def test_simple_voice_action(self, alice):
        response = alice('продолжи')
        assert response.scenario == scenario.Commands
        assert response.directive.name == directives.names.PlayerPauseDirective

    @pytest.mark.device_state(active_actions=CONDITIONAL_ACTIONS_VIDEO_PLAY_TO_PLAYER_COMMAND)
    @pytest.mark.parametrize('command, expected_directive', [
        ('включи сезон 1', directives.names.SoundSetLevelDirective),
        ('включи сезон 2', directives.names.PlayerPauseDirective),
    ])
    def test_item_selection_by_number(self, alice, command, expected_directive):
        response = alice(command)
        assert response.scenario == scenario.Commands
        assert response.directive.name == expected_directive


@pytest.mark.voice
@pytest.mark.parametrize('surface', [surface.smart_display])
class TestSpaceActions(object):

    owners = ('nkodosov', )

    @pytest.mark.device_state(active_actions=SPACE_ACTION_CONTINUE_TO_WEATHER)
    def test_simple_voice_action(self, alice):
        response = alice('продолжи')
        assert response.scenario == scenario.Weather
        assert response.directive.name == directives.names.ShowViewDirective

    @pytest.mark.device_state(active_actions=SPACE_ACTION_CONTINUE_TO_PAUSE)
    def test_simple_voice_action_commands(self, alice):
        response = alice('продолжи')
        assert response.scenario == scenario.Commands
        assert response.intent == intent.PlayerPause

    @pytest.mark.device_state(active_actions=MULTI_SPACE_ACTION)
    def test_multi_action(self, alice):
        response = alice('громче')
        assert response.scenario == scenario.Commands
        assert response.intent == intent.PlayerPause

        response = alice('тише')
        assert response.scenario == scenario.Commands
        assert response.intent == intent.SoundSetLevel

    @pytest.mark.device_state(active_actions=MUSIC_VIDEO_SPACE_ACTION)
    def test_two_activated_frames(self, alice):
        response1 = alice('громче')
        assert response1.scenario == scenario.Commands
        assert response1.intent == intent.PlayerPause

        response2 = alice('тише')
        assert response2.scenario == response1.scenario
        assert response2.intent == response1.intent
