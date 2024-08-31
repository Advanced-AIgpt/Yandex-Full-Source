import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


def validate_response(response, scenario_name, context_must_be_filled):
    assert response.scenario == scenario_name

    assert response.scenario_analytics_info.nlg_render_history_records, 'Сценарий не отрендерил ни одного nlg'
    for record in response.scenario_analytics_info.nlg_render_history_records:
        if scenario_name != scenario.Vins:
            assert record.get('template_name'), 'Не заполнен template_name'

        assert record.get('phrase_name') or record.get('card_name'), 'Не заполнен phrase_name/card_name'

        if context_must_be_filled:
            is_all_context_fields_filled = record.get('context') and record.get('req_info') and record.get('form')
            assert is_all_context_fields_filled, 'Не заполнен context/req_info/form'
        else:
            is_any_context_field_filled = record.get('context') or record.get('req_info') or record.get('form')
            assert not is_any_context_field_filled, 'Заполнен context/req_info/form, хотя мы не указывали флаг dump_nlg_render_context'


@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.parametrize('command, scenario_name', [
    pytest.param('какая завтра погода в Москве', scenario.Weather, id='hollywood'),
    pytest.param('назови случайное число от 1 до 10', scenario.RandomNumber, id='hollywood_framework'),
    pytest.param('переведи 10 долларов в рубли', scenario.Vins, id='vins'),
])
class TestNlgRenderHistory(object):
    """
    Тестируем заполнение nlg_render_history_records для HOLLYWOOD-741.
    При рендеринге любого nlg в analytics_info должен откладываться контекст, по которому мы сможем отрендерить ту же фразу вне сценария.
    Контексты рендеринга nlg могут быть жирными, поэтому их сохранение включено только под флагом dump_nlg_render_context.
    Механизм добавления nlg_render_history_records в response отличается для голивуда, нового фреймворка голивуда, винса.
    Поэтому для тестирования выбраны разные сценарии, написанные на старом голивуде, новом голивуде и винсе.
    """

    owners = ('alexanderplat', 'abc:megamind')

    def test_nlg_render_history_without_exp(self, alice, command, scenario_name):
        response = alice(command)
        validate_response(response, scenario_name, False)

    @pytest.mark.experiments(dump_nlg_render_context='0')
    def test_nlg_render_history_empty_with_exp_disabled(self, alice, command, scenario_name):
        response = alice(command)
        validate_response(response, scenario_name, False)

    @pytest.mark.experiments(dump_nlg_render_context='1')
    def test_nlg_render_history_empty_with_exp_enabled(self, alice, command, scenario_name):
        response = alice(command)
        validate_response(response, scenario_name, True)
