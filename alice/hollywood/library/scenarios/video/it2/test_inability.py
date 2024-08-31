import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice


@pytest.mark.scenario(name='Video', handle='video')
@pytest.mark.experiments('video_use_pure_plugs')
class TestInability:
    @pytest.mark.device_state(is_tv_plugged_in=False)
    @pytest.mark.parametrize('surface', [surface.station, surface.station_pro])
    def test_no_tv_plugged(self, alice):
        r = alice(voice("Найди фильмы"))
        assert "Чтобы смотреть видеоролики, фильмы и сериалы," in r.run_response.ResponseBody.Layout.OutputSpeech
        return str(r)

    @pytest.mark.xfail()
    @pytest.mark.device_state(is_tv_plugged_in=False)
    @pytest.mark.parametrize('surface', [surface.station_lite_red, surface.station_midi, surface.loudspeaker])
    def test_fail_no_tv_plugged(self, alice):
        """
        Станции, у которых нет HDMI выхода, не могут быть с экраном.
        """
        r = alice(voice("Найди фильмы"))
        assert "Чтобы смотреть видеоролики, фильмы и сериалы," in r.run_response.ResponseBody.Layout.OutputSpeech

    @pytest.mark.parametrize('surface', [surface.station_lite_red, surface.station_midi, surface.loudspeaker])
    def test_not_supported(self, alice):
        r = alice(voice("Найди фильмы"))
        assert r.run_response.ResponseBody.Layout.OutputSpeech == "Воспроизведение видео не поддерживается на этом устройстве."
        return str(r)
