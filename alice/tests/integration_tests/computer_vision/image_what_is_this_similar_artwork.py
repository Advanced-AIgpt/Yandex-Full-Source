import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest

import computer_vision.div_card as div_card
import computer_vision.util as util


@pytest.mark.parametrize('surface', [surface.searchapp])
class TestImageWhatIsThisSimilarArtwork(object):

    owners = ('polushkin', 'g:cv-search', )

    expected_more_info_buttons = {
        'Что это?': intent.ImageSimilar,
        'Текст': intent.ImageOcr,
        'Озвучить': intent.ImageOcrVoice,
        'Похожие товары': intent.ImageMarket,
        **util.common_more_info_buttons
    }

    def test_similar(self, alice):
        url = 'https://avatars.mds.yandex.net/get-alice/4387275/test_KxP-HPDcyxzMIwiafObiWg/fullocr'
        alice('На какую картину это похоже')
        response = alice.search_by_photo(url)
        assert response.scenario == scenario.ImageWhatIsThis
        assert response.intent == intent.ImageSimilarArtwork
        assert response.text_card

        objects = response.scenario_analytics_info.objects
        assert objects['similar_artwork']['human_readable']

        similar_card = div_card.SimilarArtwork(response.div_cards[0])
        assert similar_card.title
        assert similar_card.source
        assert similar_card.action_url
        assert similar_card.similar_photo.image_url
        assert similar_card.similar_photo.action_url
        assert similar_card.original_photo.image_url == url

        more_info_card = div_card.MoreInfo(response.div_cards[1])
        assert 'Ещё информация по этой картинке' in more_info_card.title
        util.assert_div_card_buttons(more_info_card.buttons, self.expected_more_info_buttons.keys())
        for title, expected_intent in self.expected_more_info_buttons.items():
            button_response = alice.click(more_info_card.button(title).image)
            assert button_response.intent == expected_intent

        util.assert_suggests(response.suggests, {
            'Попробовать еще раз', 'Что это?', 'Похожие товары', 'Зачитай этот текст', 'Распознанный текст',
        })
