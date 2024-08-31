import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


BEG_YOUR_PARDON = {
    'Üzgünüm, daha iyi anlamam için başka türlü söyleyebilir misin?',
    'Tam olarak anlayamadım. Daha açık bir şekilde anlatabilir misin?',
    'Ne dediğinden emin olamadım. Biraz daha farklı bir şekilde söyleyebilir misin?',
}

GENERAL_CONVERSATION_DUMMY = {
    'Konuyu değiştirmeye ne dersin?',
    'Bu konuya girmeyelim lütfen.',
    'Bu konuda söyleyecek pek bir şeyim yok.',
    'Konuşacak milyonlarca farklı konu var. Onlardan konuşalım.',
    'Bu konuyu pas geçiyorum. Sıradaki konu lütfen.',
    'Seninle çok daha farklı konulardan konuşabiliriz.',
    'Bu konuyu başka bir zamana bırakalım.',
}


@pytest.mark.parametrize('surface', [surface.navi_tr])
@pytest.mark.experiments(f'mm_scenario={scenario.GeneralConversationTr}')
class TestGeneralConversationTr(object):

    owners = ('ardulat',)

    def test_gc_not_banned(self, alice):
        response = alice('nasılsın')
        assert response.scenario == scenario.GeneralConversationTr
        assert response.intent == intent.GeneralConversationVinslessPardon
        assert response.text in BEG_YOUR_PARDON

    @pytest.mark.experiments('gc_not_banned',)
    @pytest.mark.parametrize('command', [
        'abdul ahat andican',
        'ahmet haluk koç',
    ])
    def test_gc_dummy(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.GeneralConversationTr
        assert response.intent == intent.GeneralConversationVinslessDummy
        assert response.text in GENERAL_CONVERSATION_DUMMY

    @pytest.mark.experiments('gc_not_banned',)
    def test_general_conversation(self, alice):
        response = alice('nasılsın')
        assert response.scenario == scenario.GeneralConversationTr
        assert response.text


@pytest.mark.experiments('gc_not_banned')
@pytest.mark.parametrize('surface', [surface.navi_tr])
@pytest.mark.xfail(reason='ALICE-9604')
class TestPalmGeneralConversationTr(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-2184
    """

    owners = ('ardulat', 'zhigan')

    def test_alice_2184(self, alice):
        response = alice('Alisa, hızlı koşuş yapsak mı yapmasak mı')
        assert response.scenario == scenario.GeneralConversationTr
        assert response.text

        response = alice('Alisa, Recep Erdoğanı ne düşünüyorsun?')
        assert response.scenario == scenario.GeneralConversationTr
        assert response.intent == intent.GeneralConversationVinslessDummy
        assert response.text in GENERAL_CONVERSATION_DUMMY
