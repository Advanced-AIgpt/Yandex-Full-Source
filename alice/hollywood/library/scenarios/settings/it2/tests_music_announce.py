import pytest
from alice.hollywood.library.python.testing.it2 import auth, surface
from alice.hollywood.library.python.testing.it2.input import voice
from alice.memento.proto.api_pb2 import EConfigKey
from alice.protos.data.scenario.music.config_pb2 import TUserConfig


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['settings']


@pytest.mark.scenario(name='Settings', handle='settings')
@pytest.mark.experiments('hw_music_announce')
class TestBase:
    pass


MEMENTO_ANNOUNCE_ENABLED = {
    "UserConfigs": {
        "MusicConfig": {
            "AnnounceTracks": True,
        }
    }
}


def _assert_memento_directive(r, enabled):
    directives = r.run_response.ResponseBody.ServerDirectives
    assert len(directives) == 1
    assert directives[0].HasField('MementoChangeUserObjectsDirective')
    memento = directives[0].MementoChangeUserObjectsDirective
    kv = memento.UserObjects.UserConfigs[0]
    assert kv.Key == EConfigKey.CK_MUSIC
    music_config = TUserConfig()
    kv.Value.Unpack(music_config)
    assert music_config.AnnounceTracks == enabled


def _assert_no_memento_directive(r):
    directives = r.run_response.ResponseBody.ServerDirectives
    assert len(directives) == 0


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.parametrize('surface', [
    surface.station,
])
class TestMusic(TestBase):
    def test_announce_enable(self, alice):
        r = alice(voice('включи режим диджей'))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.Layout.OutputSpeech
        _assert_memento_directive(r, True)
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'alice.music.announce.enable'
        assert r.run_response.ResponseBody.AnalyticsInfo.Actions[0].Id == 'alice.music.announce.enable'

    @pytest.mark.memento(MEMENTO_ANNOUNCE_ENABLED)
    def test_announce_disable(self, alice):
        r = alice(voice('выключи режим диджей'))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.Layout.OutputSpeech
        _assert_memento_directive(r, False)
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'alice.music.announce.disable'
        assert r.run_response.ResponseBody.AnalyticsInfo.Actions[0].Id == 'alice.music.announce.disable'

    @pytest.mark.memento(MEMENTO_ANNOUNCE_ENABLED)
    def test_announce_enabled_already(self, alice):
        r = alice(voice('включи анонс треков'))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.Layout.OutputSpeech
        _assert_no_memento_directive(r)
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'alice.music.announce.enable'

    def test_announce_disabled_already(self, alice):
        r = alice(voice('не надо объявлять музыку'))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.Layout.OutputSpeech
        _assert_no_memento_directive(r)
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'alice.music.announce.disable'


@pytest.mark.parametrize('surface', [
    surface.station,
])
class TestMusicNoAuth(TestBase):
    def test_announce_enable(self, alice):
        r = alice(voice('включи режим объявления музыки'))
        assert r.scenario_stages() == {'run'}
        assert 'авториз' in r.run_response.ResponseBody.Layout.OutputSpeech
        _assert_no_memento_directive(r)


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.parametrize('surface', [
    surface.searchapp,
])
class TestMusicNoAudioClient(TestBase):
    def test_announce_enable(self, alice):
        r = alice(voice('говори какую музыку ты включаешь'))
        assert r.scenario_stages() == {'run'}
        assert 'нельзя' in r.run_response.ResponseBody.Layout.OutputSpeech
        _assert_no_memento_directive(r)
