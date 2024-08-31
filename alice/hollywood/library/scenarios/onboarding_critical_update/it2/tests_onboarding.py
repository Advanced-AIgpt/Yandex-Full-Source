import logging

import pytest
from alice.hollywood.library.python.testing.it2 import auth, surface
from alice.hollywood.library.python.testing.it2.input import voice


logger = logging.getLogger(__name__)


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['onboarding_critical_update']


@pytest.fixture(scope="function")
def srcrwr_params():
    return {}


def check_action(action, hint, frame):
    assert action is not None
    assert action.NluHint.FrameName == hint
    assert action.ParsedUtterance.TypedSemanticFrame.WhichOneof('Type') == frame


@pytest.mark.scenario(name='OnboardingCriticalUpdate', handle='onboarding_critical_update')
@pytest.mark.experiments('onboarding_debug_ignore_current_device')
@pytest.mark.experiments('bg_fresh_granet')
class _TestOnboarding:

    def _test(self, alice, success, speech=None, confirm_frame=None):
        r = alice(voice('приветствие станции'))
        response = r.run_response.ResponseBody

        layout = response.Layout
        directives = layout.Directives
        if success:
            assert speech is not None
            if isinstance(speech, list):
                assert layout.OutputSpeech in speech
            else:
                assert layout.OutputSpeech == speech
            assert len(directives) == 2
            assert directives[0].WhichOneof('Directive') == 'SuccessStartingOnboardingDirective'
            assert directives[0].SuccessStartingOnboardingDirective.Name == 'success_starting_onboarding'
            assert directives[1].WhichOneof('Directive') == 'TtsPlayPlaceholderDirective'
            assert directives[1].TtsPlayPlaceholderDirective.Name == 'tts_play_placeholder'
        else:
            assert speech is None
            assert 'попозже, пожалуйста' in layout.OutputSpeech  # part of error nlg
            assert len(directives) == 0
            assert confirm_frame is None

        analytics = response.AnalyticsInfo
        assert analytics.Intent == 'alice.onboarding.starting_configure_success'
        assert analytics.ProductScenarioName == 'onboarding_critical_update'

        frame_actions = response.FrameActions
        if confirm_frame is None:
            assert 'onboarding_confirm' not in frame_actions
            assert 'onboarding_decline' not in frame_actions
        else:
            check_action(frame_actions['onboarding_confirm'], 'alice.proactivity.confirm', confirm_frame)
            check_action(frame_actions['onboarding_decline'], 'alice.proactivity.decline', 'DoNothingSemanticFrame')
            assert layout.ShouldListen


@pytest.mark.parametrize('surface', [surface.loudspeaker])
@pytest.mark.experiments('onboarding_debug_override_FirstActivation=false')
class TestOnboardingSpeakerReconfigure(_TestOnboarding):
    @pytest.mark.oauth(auth.Yandex)
    def test_no_plus(self, alice):
        self._test(
            alice, success=True,
            speech='Поздравляю, вы успешно настроили колонку. Чтобы пользоваться всеми моими возможностями, '
                   'активируйте подписку Плюс. Рассказать, как это сделать?',
            confirm_frame='HowToSubscribeSemanticFrame')

    # HasPlus
    @pytest.mark.oauth(auth.YandexPlus)
    def test_has_plus(self, alice):
        self._test(alice, success=True, speech=[
            'Ваша колонка настроена и я готова к работе и общению.',
            'Ваша колонка настроена, давайте дружить!',
            'Ваша колонка настроена. Вы молодец.',
            'Ваша колонка настроена. Поехали.'
        ])


# FirstActivation
@pytest.mark.parametrize('surface', [surface.loudspeaker])
@pytest.mark.experiments('onboarding_debug_override_FirstActivation=true')
class TestOnboardingSoundOnlySpeakerActivation(_TestOnboarding):
    @pytest.mark.oauth(auth.Yandex)
    def test_no_plus(self, alice):
        self._test(
            alice, success=True,
            speech='Поздравляю, вы успешно настроили колонку. Чтобы пользоваться всеми моими возможностями, '
                   'активируйте подписку Плюс. Рассказать, как это сделать?',
            confirm_frame='HowToSubscribeSemanticFrame')

    # HasPlus
    @pytest.mark.oauth(auth.YandexPlus)
    def test_has_plus(self, alice):
        self._test(
            alice, success=True,
            speech='Поздравляю, вы успешно настроили колонку. Хотите включим зажигательную музыку, чтобы '
                   'отпраздновать это событие?',
            confirm_frame='MusicPlaySemanticFrame')

    # ManySpeakers
    @pytest.mark.oauth(auth.Yandex)
    @pytest.mark.experiments('onboarding_debug_override_ManySpeakers=true')
    def test_no_plus_many_speakers(self, alice):
        self._test(
            alice, success=True,
            speech='Поздравляю, ещё одна колонка настроена. Я заметила, что у вас нет подписки, а ведь так '
                   'классно слушать музыку сразу на всех устройствах по всему дому. Хотите расскажу как '
                   'подключить подписку?',
            confirm_frame='HowToSubscribeSemanticFrame')

    # HasPlus
    # ManySpeakers
    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.experiments('onboarding_debug_override_ManySpeakers=true')
    def test_has_plus_many_speakers(self, alice):
        self._test(
            alice, success=True,
            speech='Поздравляю, ещё одна колонка настроена. Теперь музыку можно слушать сразу на всех '
                   'устройствах, включим что-нибудь зажигательно везде?',
            confirm_frame='MusicPlaySemanticFrame')


# FirstActivation
# SupportsVideo
@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.experiments('onboarding_debug_override_FirstActivation=true')
@pytest.mark.device_state(is_tv_plugged_in=False)
class TestOnboardingUnpluggedVideoSpeakerActivation(_TestOnboarding):
    @pytest.mark.oauth(auth.Yandex)
    def test_no_plus(self, alice):
        self._test(
            alice, success=True,
            speech='Поздравляю, вы успешно настроили колонку. Чтобы пользоваться всеми моими возможностями, '
                   'активируйте подписку Плюс. Рассказать, как это сделать?',
            confirm_frame='HowToSubscribeSemanticFrame')

    # HasPlus
    @pytest.mark.oauth(auth.YandexPlus)
    def test_has_plus(self, alice):
        self._test(
            alice, success=True,
            speech='Поздравляю, вы успешно настроили колонку. Подключите станцию к телевизору. Дальше на экране появится '
                   'подборка фильмов специально для вас. Выбирайте и наслаждайтесь.')

    # ManySpeakers
    @pytest.mark.oauth(auth.Yandex)
    @pytest.mark.experiments('onboarding_debug_override_ManySpeakers=true')
    def test_no_plus_many_speakers(self, alice):
        self._test(
            alice, success=True,
            speech='Поздравляю, ещё одна колонка настроена. Я заметила, что у вас нет подписки, а ведь так '
                   'классно слушать музыку сразу на всех устройствах по всему дому. Хотите расскажу как '
                   'подключить подписку?',
            confirm_frame='HowToSubscribeSemanticFrame')

    # HasPlus
    # ManySpeakers
    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.experiments('onboarding_debug_override_ManySpeakers=true')
    def test_has_plus_many_speakers(self, alice):
        self._test(
            alice, success=True,
            speech='Поздравляю, ещё одна колонка настроена. Подключите станцию к телевизору. Дальше на экране '
                   'появится подборка фильмов специально для вас. Выбирайте и наслаждайтесь.')


# FirstActivation
# SupportsVideo
# IsTvPlugged
@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.experiments('onboarding_debug_override_FirstActivation=true')
@pytest.mark.device_state(is_tv_plugged_in=True)
class TestOnboardingPluggedVideoSpeakerActivation(_TestOnboarding):
    @pytest.mark.oauth(auth.Yandex)
    def test_no_plus(self, alice):
        self._test(
            alice, success=True,
            speech='Поздравляю, вы успешно настроили колонку. Чтобы пользоваться всеми моими возможностями, '
                   'активируйте подписку Плюс. Рассказать, как это сделать?',
            confirm_frame='HowToSubscribeSemanticFrame')

    # HasPlus
    @pytest.mark.oauth(auth.YandexPlus)
    def test_has_plus_cant_connect_remote(self, alice):
        self._test(
            alice, success=True,
            speech='Поздравляю, вы успешно настроили колонку. У меня есть много фильмов и сериалов специально для вас. '
                   'Хотите посмотреть новинки?',
            confirm_frame='VideoPlaySemanticFrame')

    # HasPlus
    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.supported_features('bluetooth_rcu')
    @pytest.mark.device_state(rcu={"is_rcu_connected": True})
    def test_has_plus_remote_already_connected(self, alice):
        self._test(
            alice, success=True,
            speech='Поздравляю, вы успешно настроили колонку. У меня есть много фильмов и сериалов специально для вас. '
                   'Хотите посмотреть новинки?',
            confirm_frame='VideoPlaySemanticFrame')

    # HasPlus
    # CanConnectRemote
    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.supported_features('bluetooth_rcu')
    def test_has_plus_can_connect_remote(self, alice):
        self._test(
            alice, success=True,
            speech='Поздравляю, вы успешно настроили колонку. Чтобы выбирать фильмы было удобно, советую подключить '
                   'пульт. Просто скажите "Алиса, настрой пульт".')
