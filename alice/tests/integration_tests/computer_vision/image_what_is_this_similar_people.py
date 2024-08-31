import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest

import computer_vision.div_card as div_card
import computer_vision.util as util


@pytest.mark.parametrize('surface', [surface.searchapp])
class TestImageWhatIsThisSimilarPeople(object):

    owners = ('polushkin', 'g:cv-search', )

    expected_more_info_buttons = {
        'Кто это?': intent.ImageSimilar,
        'Похожая картина': intent.ImageSimilarArtwork,
        'Похожая одежда': intent.ImageClothes,
        **util.common_more_info_buttons
    }

    @pytest.mark.parametrize('command, expected_intent', [
        ('На кого он похож', intent.ImageSimilarPeople),
        ('На кого я похож', intent.ImageSimilarPeopleFrontal),
    ])
    def test(self, alice, command, expected_intent):
        url = 'https://avatars.mds.yandex.net/get-alice/4336950/test_ZQ9GHiqCbZo6OsN1TlPZDQ/big'
        alice(command)
        response = alice.search_by_photo(url)
        assert response.scenario == scenario.ImageWhatIsThis
        assert response.intent == expected_intent
        assert response.text_card

        objects = response.scenario_analytics_info.objects
        assert len(objects['similar_people']['human_readable'].split(' ')) in [2, 3]
        assert 'similar_gallery' in objects

        similar_card = div_card.SimilarPeople(response.div_cards[0])
        assert similar_card.title
        assert similar_card.action_url
        assert similar_card.similar_photo.image_url
        assert similar_card.similar_photo.action_url
        assert similar_card.original_photo.image_url == url

        more_info_card = div_card.MoreInfo(response.div_cards[2])
        assert 'Ещё информация по этой картинке' in more_info_card.title
        util.assert_div_card_buttons(more_info_card.buttons, self.expected_more_info_buttons.keys())
        for title, expected_intent in self.expected_more_info_buttons.items():
            button_response = alice.click(more_info_card.button(title).image)
            assert button_response.intent == expected_intent

        util.assert_suggests(response.suggests, {
            'Похожая картина', 'Попробовать еще раз', 'Кто это?', 'Похожая одежда',
        })
