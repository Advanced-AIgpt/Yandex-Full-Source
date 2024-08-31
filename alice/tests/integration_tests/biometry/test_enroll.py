import alice.tests.library.auth as auth
import alice.tests.library.intent as intent
import alice.tests.library.surface as surface
import pytest


@pytest.mark.voice
@pytest.mark.oauth(auth.Yandex)
@pytest.mark.parametrize('surface', [surface.station])
class TestEnroll(object):

    owners = ('tolyandex', )

    def test(self, alice):
        # This request is used to initiate session on uniproxy
        alice('привет')
        response = alice('запомни мой голос')
        assert response.intent == intent.VoiceprintEnroll
        response = alice('алиса, включи музыку')
        assert response.intent == intent.VoiceprintEnrollCollectVoice


@pytest.mark.experiments('mm_formula=search_with_pre')
class TestEnrollExp(TestEnroll):
    pass
