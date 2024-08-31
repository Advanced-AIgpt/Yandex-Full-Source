import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


REMEMBER_ME_RESPONSE = (
    'Отличная идея! Это позволит мне понять, когда включить музыку просите именно вы. '
    'Друзья и близкие тоже смогут её включать — но это не повлияет на ваши рекомендации и подборки. '
    'Ваш голос будет связан с аккаунтом в Яндексе, с которого вы активировали это устройство. '
    'Если знакомство вас утомит, скажите: «Алиса, хватит». '
    'Чтобы начать, скажите: «Меня зовут...» — и добавьте своё имя.'
)

OK_MY_NAME_IS_RESPONSES = [
    (
        'Олег, если вы хотите называться по-другому, скажите: «Меня зовут...» — '
        'и добавьте новое имя. Если имя верное, скажите «я готов» или «я готова» — и мы продолжим знакомиться.'
    ),
    (
        'Олег, если вы хотите изменить имя, скажите: «Меня зовут...» — '
        'и добавьте новое имя. Если всё хорошо, скажите «я готов» или «я готова» — и мы продолжим знакомиться.'
    ),
]

CHECK_PHRASES = [
    'включи музыку',
    'поставь будильник на семь часов',
    'какие сейчас пробки?',
    'давай сыграем в города',
    'поставь звуки природы',
]


@pytest.mark.voice
@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments('personalization', 'enable_biometry_scoring', 'quasar_biometry')
@pytest.mark.parametrize('surface', [surface.station])
class TestPalmBiometryName(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-556
    """

    owners = ('nkodosov',)

    def test_alice_556(self, alice):
        response = alice('Давай познакомимся')
        assert response.scenario in {scenario.Vins, scenario.Voiceprint}
        assert response.intent == intent.VoiceprintEnroll
        assert response.directive.name == directives.names.PlayerPauseDirective
        assert not response.directive.payload.smooth
        assert response.text == REMEMBER_ME_RESPONSE

        response = alice('Меня зовут Олег')
        assert response.intent == intent.VoiceprintEnrollCollectVoice
        assert not response.directive
        assert response.text in OK_MY_NAME_IS_RESPONSES

        response = alice('Я готов')
        assert response.intent == intent.VoiceprintEnrollCollectVoice
        assert not response.directive

        # TODO finish this step. Here bass reports passport API error oauth_token.invalid
        MAX_REPEAT_COUNT = 2
        for i in range(len(CHECK_PHRASES) + MAX_REPEAT_COUNT):
            for phrase in CHECK_PHRASES:
                if phrase in response.text:
                    check_phrase = phrase
                    break
            else:
                assert False, ('cannot find next phrase for emulation request, check scenario answer', response.text)
            response = alice(check_phrase)
            if check_phrase == CHECK_PHRASES[-1]:
                assert response.intent in {intent.VoiceprintEnrollCollectVoice, intent.VoiceprintEnrollFinish}
                if response.intent == intent.VoiceprintEnrollFinish:
                    break
            else:
                assert response.intent == intent.VoiceprintEnrollCollectVoice
            assert not response.directive
        assert check_phrase == CHECK_PHRASES[-1] or response.intent == intent.VoiceprintEnrollFinish, \
            'expect last phrase on the end of requests or Finish intent'
