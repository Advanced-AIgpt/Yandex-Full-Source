import re

import alice.tests.library.auth as auth
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.station])
class TestPalmVideoRater(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-2570
    """

    owners = ('flimsywhimsy', )
    title_re = re.compile(r'(фильм|мультфильм|сериал) ".+?"')
    rate_re = re.compile(
        r'(А как вам|А|Спасибо, а что вы скажете про|Хорошо! Что вы думаете про|Ясно. А про|Отлично. А|Записала. А)'
        r' (фильм|мультфильм|сериал) ".+?"( что думаете)?\?'
    )
    greetings_re = re.compile(r'Давайте начнем! Что вы думаете про (фильм|мультфильм|сериал) ".+?"\?')
    goodbye_text = 'Окей! Вы можете в любой момент продолжить оценивать фильмы, сказав "Алиса, хочу оценить фильм".'
    dont_understand_text = 'Простите, не понимаю вашу оценку. Если устали, скажите "Хватит".'

    @pytest.mark.experiments('hw_video_rater_clear_history')
    def test_cleanup(self, alice):
        response = alice('хочу оценить фильм')
        assert response.scenario == scenario.VideoRater
        assert self.greetings_re.match(response.text) is not None

        response = alice('Лайк')
        assert response.scenario == scenario.VideoRater
        assert self.rate_re.match(response.text) is not None

    def test_alice_2570(self, alice):
        titles = []

        response = alice('Хочу оценить фильм')
        assert response.scenario == scenario.VideoRater
        assert self.greetings_re.match(response.text) is not None
        titles.append(
            self.title_re.search(response.text).group(0)
        )

        response = alice('Лайк')
        assert response.scenario == scenario.VideoRater
        assert self.rate_re.match(response.text) is not None
        titles.append(
            self.title_re.search(response.text).group(0)
        )

        response = alice('5')
        assert response.scenario == scenario.VideoRater
        assert self.rate_re.match(response.text) is not None

        response = alice('хватит')
        assert response.scenario == scenario.VideoRater
        assert response.text == self.goodbye_text

        response = alice('Хочу оценить фильм')
        assert response.scenario == scenario.VideoRater
        assert self.greetings_re.match(response.text) is not None
        titles.append(
            self.title_re.search(response.text).group(0)
        )

        response = alice('Дизлайк')
        assert response.scenario == scenario.VideoRater
        assert self.rate_re.match(response.text) is not None

        for i in range(2):
            response = alice('абракадабра')
            assert response.scenario == scenario.VideoRater
            assert response.text == 'Простите, не понимаю вашу оценку. Если устали, скажите "Хватит".'

        response = alice('абракадабра')
        assert response.scenario != scenario.VideoRater

        response = alice('Хочу оценить фильм')
        assert response.scenario == scenario.VideoRater
        assert self.greetings_re.match(response.text) is not None

        assert len(titles) == len(set(titles))  # All rated movies are different
