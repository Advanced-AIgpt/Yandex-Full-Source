import pytest
from alice.hollywood.library.python.testing.it2 import surface, auth
from alice.hollywood.library.python.testing.it2.input import voice
from alice.hollywood.library.scenarios.zero_testing.proto.zero_testing_state_pb2 import TZeroTestingState # noqa


@pytest.fixture(scope="module")
def enabled_scenarios():
    return ['zero_testing']


def get_scenario_state(response):
    state = TZeroTestingState()
    response.ResponseBody.State.Unpack(state)
    return state


@pytest.mark.oauth(auth.YandexStaff)
@pytest.mark.scenario(name='ZeroTesting', handle='zero_testing')
@pytest.mark.parametrize('surface', [surface.searchapp])
class Tests:

    def test_activate_exp(self, alice):
        r = alice(voice('Активируй эксперимент 123'))
        assert r.is_run_relevant_with_second_scenario_stage()
        layout = r.run_response.ResponseBody.Layout
        outputText = 'Активировано залипание в эксперименте 123. Для завершения скажите "выключи эксперимент".'
        assert layout.OutputSpeech == outputText
        assert layout.Cards[0].Text == outputText
        assert len(layout.Directives) == 1
        set_cookies_directive = layout.Directives[0].SetCookiesDirective
        assert set_cookies_directive.Value == '{"uaas_tests":[123]}'

        state = get_scenario_state(r.run_response)
        assert set(state.TestIds) == {123}

    def test_activate_two_exps(self, alice):
        r = alice(voice('Активируй эксперимент 123'))
        assert r.is_run_relevant_with_second_scenario_stage()

        r = alice(voice('Активируй эксперимент 456'))
        assert r.is_run_relevant_with_second_scenario_stage()
        layout = r.run_response.ResponseBody.Layout
        outputText = 'Активировано залипание в 2 экспериментах 123, 456. Для завершения скажите "выключи эксперимент".'
        assert layout.OutputSpeech == outputText
        assert layout.Cards[0].Text == outputText
        assert len(layout.Directives) == 1
        set_cookies_directive = layout.Directives[0].SetCookiesDirective
        assert set_cookies_directive.Value == '{"uaas_tests":[123,456]}'

        state = get_scenario_state(r.run_response)
        assert set(state.TestIds) == {123, 456}

    def test_activate_three_exps_with_one_twice(self, alice):
        r = alice(voice('Активируй эксперимент 123'))
        assert r.is_run_relevant_with_second_scenario_stage()

        r = alice(voice('Активируй эксперимент 456'))
        assert r.is_run_relevant_with_second_scenario_stage()

        r = alice(voice('Активируй эксперимент 123'))
        assert r.is_run_relevant_with_second_scenario_stage()
        layout = r.run_response.ResponseBody.Layout
        outputText = 'Активировано залипание в 2 экспериментах 123, 456. Для завершения скажите "выключи эксперимент".'
        assert layout.OutputSpeech == outputText
        assert layout.Cards[0].Text == outputText
        assert len(layout.Directives) == 1
        set_cookies_directive = layout.Directives[0].SetCookiesDirective
        assert set_cookies_directive.Value == '{"uaas_tests":[123,456]}'

        state = get_scenario_state(r.run_response)
        assert set(state.TestIds) == {123, 456}

    @pytest.mark.experiments('zero_testing_code=123qweRTY')
    def test_tell_me_code_success(self, alice):
        r = alice(voice('Скажи код эксперимента'))
        assert r.is_run_relevant_with_second_scenario_stage()
        layout = r.run_response.ResponseBody.Layout
        outputText = 'Текущий код 123qweRTY, эксперименты не найдены.'
        assert layout.OutputSpeech == outputText
        assert layout.Cards[0].Text == outputText

        assert not layout.Directives

    def test_tell_me_code_fail(self, alice):
        r = alice(voice('Скажи код эксперимента'))
        assert r.is_run_relevant_with_second_scenario_stage()
        layout = r.run_response.ResponseBody.Layout
        outputText = 'Код не найден, эксперименты не найдены.'
        assert layout.OutputSpeech == outputText
        assert layout.Cards[0].Text == outputText

        assert not layout.Directives

    @pytest.mark.experiments('zero_testing_code=123qweRTY')
    def test_tell_me_code_success_one_exp(self, alice):
        r = alice(voice('Активируй эксперимент 123'))
        assert r.is_run_relevant_with_second_scenario_stage()

        r = alice(voice('Скажи код эксперимента'))
        assert r.is_run_relevant_with_second_scenario_stage()
        layout = r.run_response.ResponseBody.Layout
        outputText = 'Текущий код 123qweRTY, эксперимент 123.'
        assert layout.OutputSpeech == outputText
        assert layout.Cards[0].Text == outputText

        assert not layout.Directives

    def test_tell_me_code_fail_one_exp(self, alice):
        r = alice(voice('Активируй эксперимент 123'))
        assert r.is_run_relevant_with_second_scenario_stage()

        r = alice(voice('Скажи код эксперимента'))
        assert r.is_run_relevant_with_second_scenario_stage()
        layout = r.run_response.ResponseBody.Layout
        outputText = 'Код не найден, эксперимент 123.'
        assert layout.OutputSpeech == outputText
        assert layout.Cards[0].Text == outputText

        assert not layout.Directives

    @pytest.mark.experiments('zero_testing_code=123qweRTY')
    def test_tell_me_code_success_two_exps(self, alice):
        r = alice(voice('Активируй эксперимент 123'))
        assert r.is_run_relevant_with_second_scenario_stage()

        r = alice(voice('Активируй эксперимент 456'))
        assert r.is_run_relevant_with_second_scenario_stage()

        r = alice(voice('Скажи код эксперимента'))
        assert r.is_run_relevant_with_second_scenario_stage()
        layout = r.run_response.ResponseBody.Layout
        outputText = 'Текущий код 123qweRTY, эксперименты 123, 456.'
        assert layout.OutputSpeech == outputText
        assert layout.Cards[0].Text == outputText

        assert not layout.Directives

    def test_tell_me_code_fail_two_exps(self, alice):
        r = alice(voice('Активируй эксперимент 123'))
        assert r.is_run_relevant_with_second_scenario_stage()

        r = alice(voice('Активируй эксперимент 456'))
        assert r.is_run_relevant_with_second_scenario_stage()

        r = alice(voice('Скажи код эксперимента'))
        assert r.is_run_relevant_with_second_scenario_stage()
        layout = r.run_response.ResponseBody.Layout
        outputText = 'Код не найден, эксперименты 123, 456.'
        assert layout.OutputSpeech == outputText
        assert layout.Cards[0].Text == outputText

        assert not layout.Directives

    def test_deactivate_exp(self, alice):
        r = alice(voice('Активируй эксперимент 123'))
        assert r.is_run_relevant_with_second_scenario_stage()

        state = get_scenario_state(r.run_response)
        assert set(state.TestIds) == {123}

        r = alice(voice('Выключи эксперимент'))
        assert r.is_run_relevant_with_second_scenario_stage()
        layout = r.run_response.ResponseBody.Layout
        outputText = 'Залипание в эксперименте выключено.'
        assert layout.OutputSpeech == outputText
        assert layout.Cards[0].Text == outputText
        assert len(layout.Directives) == 1
        set_cookies_directive = layout.Directives[0].SetCookiesDirective
        assert not set_cookies_directive.Value

        state = get_scenario_state(r.run_response)
        assert len(state.TestIds) == 0

    def test_deactivate_two_exps(self, alice):
        r = alice(voice('Активируй эксперимент 123'))
        assert r.is_run_relevant_with_second_scenario_stage()

        r = alice(voice('Активируй эксперимент 456'))
        assert r.is_run_relevant_with_second_scenario_stage()

        state = get_scenario_state(r.run_response)
        assert set(state.TestIds) == {123, 456}

        r = alice(voice('Выключи эксперимент'))
        assert r.is_run_relevant_with_second_scenario_stage()
        layout = r.run_response.ResponseBody.Layout
        outputText = 'Залипание в экспериментах выключено.'
        assert layout.OutputSpeech == outputText
        assert layout.Cards[0].Text == outputText
        assert len(layout.Directives) == 1
        set_cookies_directive = layout.Directives[0].SetCookiesDirective
        assert not set_cookies_directive.Value

        state = get_scenario_state(r.run_response)
        assert len(state.TestIds) == 0


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.scenario(name='ZeroTesting', handle='zero_testing')
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestsNonStaff:

    def _assert_irrel_run_response(self, run_response):
        layout = run_response.ResponseBody.Layout
        outputText = 'У Вас нет доступа к сценарию ZeroTesting.'
        assert layout.OutputSpeech == outputText
        assert layout.Cards[0].Text == outputText
        assert not layout.Directives

    def test_activate_exp(self, alice):
        r = alice(voice('Активируй эксперимент 123'))
        assert r.is_run_irrelevant()
        self._assert_irrel_run_response(r.run_response)

    @pytest.mark.experiments('zero_testing_code=123qweRTY')
    def test_tell_me_code_success(self, alice):
        r = alice(voice('Скажи код эксперимента'))
        assert r.is_run_irrelevant()
        self._assert_irrel_run_response(r.run_response)

    def test_tell_me_code_fail(self, alice):
        r = alice(voice('Скажи код эксперимента'))
        assert r.is_run_irrelevant()
        self._assert_irrel_run_response(r.run_response)

    def test_deactivate_exp(self, alice):
        r = alice(voice('Выключи эксперимент'))
        assert r.is_run_irrelevant()
        self._assert_irrel_run_response(r.run_response)
