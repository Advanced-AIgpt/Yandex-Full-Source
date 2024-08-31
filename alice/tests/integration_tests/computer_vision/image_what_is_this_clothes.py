import alice.tests.library.intent as intent
import alice.tests.library.region as region
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest

import computer_vision.div_card as div_card
import computer_vision.util as util


@pytest.mark.region(region.Moscow, user_defined_region_id=True)
@pytest.mark.experiments(
    'vins_add_irrelevant_intents=personal_assistant.scenarios.image_what_is_this',
)
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestImageWhatIsThisClothes(object):

    owners = ('polushkin', 'g:cv-search', )

    expected_more_info_buttons = {
        'Что это?': intent.ImageSimilar,
        **util.common_more_info_buttons
    }

    def test_clothes(self, alice):
        photo = 'https://avatars.mds.yandex.net/get-alice/3939230/test_KRmAUzEaHmKNxZE_0kFj6A/big'
        response = alice.search_by_photo(photo)
        assert response.scenario == scenario.ImageWhatIsThis
        assert response.intent == intent.ImageClothes
        assert response.product_scenario == 'images_what_is_this'
        assert response.text_card
        assert 'сумк' in response.text.lower() or 'юбк' in response.text.lower()

        glothes = div_card.Clothes(response.div_cards[0])

        glothes_titles = {g.title for g in glothes}
        assert glothes_titles == {'Сумка', 'Юбка', 'Обувь', 'Все'}
        assert glothes.first.title == 'Все'
        assert len(glothes.first) == len(glothes) - 1
        for glothes_gallery in glothes:
            for item in glothes_gallery:
                assert item.image_url
                util.assert_price(item.price)
                assert item.name
                assert item.shop
                assert item.action_url

        more_info_card = div_card.MoreInfo(response.div_cards[1])
        assert 'Ещё информация по этой картинке' in more_info_card.title
        util.assert_div_card_buttons(more_info_card.buttons, self.expected_more_info_buttons.keys())
        for title, expected_intent in self.expected_more_info_buttons.items():
            button_response = alice.click(more_info_card.button(title).image)
            assert button_response.intent == expected_intent

        objects = response.scenario_analytics_info.objects
        tag = objects['tag']['human_readable']
        util.assert_suggests(response.suggests, {f'🔍 "{tag}"', 'Что это?'})
