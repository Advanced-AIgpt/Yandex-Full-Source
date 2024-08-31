import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest

import computer_vision.div_card as div_card
import computer_vision.util as util


@pytest.mark.parametrize('surface', [surface.searchapp])
class TestImageWhatIsThisStubAnswers(object):

    owners = ('polushkin', 'g:cv-search', )

    expected_more_info_buttons = {
        'Что это?': intent.ImageSimilar,
        **util.common_more_info_buttons
    }

    def test_dark_answer(self, alice):
        response = alice.search_by_photo('http://avatars.mds.yandex.net/get-images-similar-mturk/40560/XcvbdjZJJjmZd/orig')
        assert response.scenario == scenario.ImageWhatIsThis
        assert response.intent == intent.ImageDark
        assert response.text_card

    def test_porn_answer(self, alice):
        response = alice.search_by_photo('http://avatars.mds.yandex.net/get-images-similar-mturk/38061/Xh5e_9je4aM-v/orig')
        assert response.scenario == scenario.ImageWhatIsThis
        assert response.intent == intent.ImagePorn
        assert response.text_card

    def test_gruesome_answer(self, alice):
        response = alice.search_by_photo('https://avatars.mds.yandex.net/get-images-similar-mturk/16267/X5h_PCU5IBpWYpmh7SoSOw/orig')
        assert response.scenario == scenario.ImageWhatIsThis
        assert response.intent == intent.ImageGruesome
        assert response.text_card

    def test_barcode_answer(self, alice):
        response = alice.search_by_photo('https://avatars.mds.yandex.net/get-pdb/199965/5b112b6d-a046-45df-bbd5-37f1d5905601/s1200')
        assert response.scenario == scenario.ImageWhatIsThis
        assert response.intent == intent.ImageBarcode
        assert response.text_card

        objects = response.scenario_analytics_info.objects
        assert 'barcode_text' in objects
        assert objects['phone_contact']['human_readable'] == '+7 495 212-13-96'
        assert objects['url_contact']['human_readable'] == 'http://kitaiki.ru'

    def test_museum_answer(self, alice):
        response = alice.search_by_photo('http://avatars.mds.yandex.net/get-images-similar-mturk/15681/c445b6e90e3ca7b06dca29ff36ef3a5b/orig')
        assert response.scenario == scenario.ImageWhatIsThis
        assert response.intent == intent.ImageMuseum
        assert not response.text_card
        assert response.has_voice_response()

        objects = response.scenario_analytics_info.objects
        assert 'museum' in objects
        assert objects['museum']['human_readable'] == '"Московский дворик"'

        art_work = div_card.MuseumArtwork(response.div_cards[0])
        assert art_work.photo
        assert art_work.name
        assert art_work.description
        assert art_work.source
        assert art_work.listen_audio
        assert art_work.action_url

        more_info_card = div_card.MoreInfo(response.div_cards[1])
        assert 'Ещё информация по этой картинке' in more_info_card.title
        util.assert_div_card_buttons(more_info_card.buttons, self.expected_more_info_buttons.keys())
        for title, expected_intent in self.expected_more_info_buttons.items():
            button_response = alice.click(more_info_card.button(title).image)
            assert button_response.intent == expected_intent

        util.assert_suggests(response.suggests, {'Что это?'})
