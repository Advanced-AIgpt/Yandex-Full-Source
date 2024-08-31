import alice.tests.library.auth as auth
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.oauth(auth.YandexPlus)
class TestVideo(object):

    owners = ('ran1s',)

    @pytest.mark.parametrize('surface', [surface.station])
    def test_play_video_after_vins_change_form(self, alice):
        response = alice('включи фильм терминатор')
        assert response.scenario == scenario.Video
        response = alice('перемотай на 1 минуту 13 секунд')
        assert response.intent == intent.PlayerRewind
        response = alice('сири включи смешариков')
        assert response.scenario == scenario.Video
        assert response.intent == intent.MmVideoPlay
