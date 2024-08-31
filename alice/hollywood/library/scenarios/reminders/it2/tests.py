import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import server_action


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['reminders']


@pytest.mark.scenario(name='Reminders', handle='reminders')
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestReminders:
    @pytest.mark.device_state(device_reminders={'list': [{'id': 'guid', 'text': 'remind text', 'epoch': '12345678', 'timezone': 'Europe/Moscow'}]})
    def test_on_shoot(self, alice):
        text_to_remind = 'remind text'

        checks = [
            text_to_remind,
            '0 часов',
            '21 минуту',
            '24 мая',
            '1970 года'
        ]

        payload = {
            'typed_semantic_frame': {
                'reminders_on_shoot_semantic_frame': {
                    'id': {
                        'string_value': 'guid',
                    },
                    'text': {
                        'string_value': text_to_remind,
                    },
                    'epoch': {
                        'epoch_value': '12345678',
                    },
                    'timezone': {
                        'string_value': 'Europe/Moscow',
                    },
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'reminders'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run'}
        layout = r.run_response.ResponseBody.Layout
        for check in checks:
            assert layout.OutputSpeech.find(check) >= 0, f'check for text in reponse failed: response "{layout.OutputSpeech}", check: "{check}"'

    def test_on_shoot_not_in_db(self, alice):
        payload = {
            'typed_semantic_frame': {
                'reminders_on_shoot_semantic_frame': {
                    'id': {
                        'string_value': 'guid',
                    },
                    'text': {
                        'string_value': 'remind me',
                    },
                    'epoch': {
                        'epoch_value': '12345678',
                    },
                    'timezone': {
                        'string_value': 'Europe/Moscow',
                    },
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'reminders'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run'}

        checks = [
            'remind me',
            '0 часов',
            '21 минуту',
            '24 мая',
            '1970 года'
        ]

        layout = r.run_response.ResponseBody.Layout
        for check in checks:
            assert layout.OutputSpeech.find(check) >= 0, f'check for text in reponse failed: response "{layout.OutputSpeech}", check: "{check}"'

    def test_on_set_success(self, alice):
        payload = {
            'success': True,
            'type': 'Creation'
        }

        r = alice(server_action(name='reminders_on_success_callback', payload=payload))
        assert r.scenario_stages() == {'run'}
        layout = r.run_response.ResponseBody.Layout
        assert layout.OutputSpeech.find('успешно') >= 0

    def test_on_cancel_success(self, alice):
        payload = {
            'success': True,
            'type': 'Cancelation'
        }

        r = alice(server_action(name='reminders_on_success_callback', payload=payload))
        assert r.scenario_stages() == {'run'}
        layout = r.run_response.ResponseBody.Layout
        assert layout.OutputSpeech.find('успешно') >= 0
        assert layout.OutputSpeech.find('удалили') >= 0

    def test_on_set_fail(self, alice):
        payload = {
            '@scenario_name': 'Vins',
            'success': False,
            'type': 'Creation'
        }

        r = alice(server_action(name='reminders_on_success_callback', payload=payload))
        assert r.scenario_stages() == {'run'}
        layout = r.run_response.ResponseBody.Layout

        total = 0
        for s in ['ошибка', 'Не удалось']:
            total += (layout.OutputSpeech.find(s) >= 0)
        assert total > 0

    def test_on_cancel_fail(self, alice):
        payload = {
            '@scenario_name': 'Vins',
            'success': False,
            'type': 'Cancelation'
        }

        r = alice(server_action(name='reminders_on_success_callback', payload=payload))
        assert r.scenario_stages() == {'run'}
        layout = r.run_response.ResponseBody.Layout

        total = 0
        for s in ['ошибка', 'Не удалось']:
            total += (layout.OutputSpeech.find(s) >= 0)
        assert total > 0
