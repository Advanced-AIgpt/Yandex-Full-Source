import alice.tests.library.intent as intent
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [surface.watch])
class TestPalmGeneralConversation(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-3
    """

    owners = ('leletko',)

    def test_alice_3(self, alice):
        test_phrase = 'Что ты думаешь о быстром беге'

        response = alice(test_phrase)
        assert response.intent == intent.GeneralConversation
        assert response.text
        first_reply = response.text

        response = alice(test_phrase)
        assert response.intent == intent.GeneralConversation
        assert response.text
        second_reply = response.text
        assert first_reply != second_reply

        response = alice(test_phrase)
        assert response.intent == intent.GeneralConversation
        assert response.text
        third_reply = response.text
        assert first_reply != third_reply
        assert second_reply != third_reply
