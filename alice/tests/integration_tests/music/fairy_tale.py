import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [surface.searchapp])
class TestPlayFairyTale(object):

    owners = ('abc:alice_scenarios_music',)

    @pytest.mark.oauth(auth.YandexPlus)
    def test_play_humpty_dumpty(self, alice):
        response = alice('сказку шалтая болтая')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.MusicFairyTale
        assert response.directive.name == directives.names.OpenUriDirective
        assert any(phrase in response.text for phrase in [
            # The purpose of this test is to run throgh a changeform chain and veryfy that we do not crash
            # We get changeform chain in case when a fairy tale about 'шалтая болтая' is not found.
            # Alice suggests us some another fairy tale:
            'такой сказки у меня пока нет',
            'такой сказки у меня нет',
            'такой сказки нет',
            'Включаю',  # BUT sometimes our search founds such a fairy tale...
        ])

    @pytest.mark.oauth(auth.YandexPlus)
    def test_new_default(self, alice):
        response = alice('включи сказку')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicFairyTale
        assert response.directive.name == directives.names.OpenUriDirective
        assert '1039' in response.directive.payload.uri

    @pytest.mark.oauth(auth.Yandex)
    def test_without_plus(self, alice):
        response = alice('включи сказку о рыбаке и рыбке')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.MusicFairyTale
        assert response.directive.name == directives.names.OpenUriDirective
        assert response.text in [
            'Без подписки доступен только отрывок сказки. Это неплохо, но с подпиской было бы идеально.\nВключаю \"Сказка о рыбаке и рыбке\"',
            'Без подписки можно слушать только часть сказки, но самое интересное впереди!\nВключаю \"Сказка о рыбаке и рыбке\"',
            'Сказка прервется в самом интересном месте! Оформите подписку - слушайте целиком.\nВключаю \"Сказка о рыбаке и рыбке\"'
        ]

    @pytest.mark.no_oauth
    def test_unauthorized(self, alice):
        response = alice('включи сказку о рыбаке и рыбке')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.MusicFairyTale
        assert response.directive.name == directives.names.OpenUriDirective
        assert response.text in [
            'Пожалуйста, войдите в свой аккаунт на Яндексе, чтобы я могла включать вам сказки целиком. А пока послушайте отрывок.\nВключаю \"Сказка о рыбаке и рыбке\"',
            'Пожалуйста, войдите в аккаунт, чтобы я могла включать вам сказки, которые вы любите, полностью. А пока - отрывок.\nВключаю \"Сказка о рыбаке и рыбке\"'
        ]


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.station])
class TestPalmPlayFairyTale(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-2065
    """

    owners = ('abc:alice_scenarios_music',)

    @pytest.mark.parametrize('command, track_id', [
        ('поставь сказку от антона комолова', 43228976),
        ('поставь сказку про царевну и лягушку от сергея лазарева', 43228977),
        ('поставь сказку новое платье короля от татьяны лазаревой', 43228978),
    ])
    def test_play_tales_with_stars(self, alice, command, track_id):
        response = alice(command)
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicFairyTale
        assert response.text.startswith('Включаю')
        assert 'сказку' in response.text or 'Сказка' in response.text
        assert response.directive.name == directives.names.AudioPlayDirective
        assert response.directive.payload.stream.id == str(track_id)


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.smart_tv])
class TestPlayFairyTaleUnsupported(object):
    '''
    https://testpalm.yandex-team.ru/testcase/alice-2865
    '''

    owners = ('abc:alice_scenarios_music',)

    @pytest.mark.parametrize('command, response_text', [
        ('Включи сказку', 'Включаю подборку "Плейлист сказок для Алисы".'),
        ('Расскажи сказку о красной шапочке', 'Включаю сказку "Красная Шапочка"'),
    ])
    def test_alice_2865(self, alice, command, response_text):
        response = alice(command)
        assert response.scenario in [scenario.HollywoodMusic, scenario.Vins]
        assert response.intent == intent.MusicFairyTale or intent.MusicPlay
        assert response.directive.name == directives.names.MusicPlayDirective
        assert response.text == response_text
