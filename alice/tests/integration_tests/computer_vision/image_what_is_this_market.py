import alice.tests.library.intent as intent
import alice.tests.library.region as region
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest

import computer_vision.div_card as div_card
import computer_vision.util as util


@pytest.mark.region(region.Moscow, user_defined_region_id=True)
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestImageWhatIsThisMarket(object):

    owners = ('polushkin', 'g:cv-search', )

    expected_more_info_buttons = {
        'Текст': intent.ImageOcr,
        'Озвучить': intent.ImageOcrVoice,
        **util.common_more_info_buttons
    }

    def test_market(self, alice):
        photo = 'http://avatars.mds.yandex.net/get-images-similar-mturk/41142/1404020eee13f63c354cacb3c5d13263/orig'
        response = alice.search_by_photo(photo)
        assert response.scenario == scenario.ImageWhatIsThis
        assert response.intent == intent.ImageMarket
        assert response.text_card
        assert 'шоколад' in response.text.lower() or 'ritter sport' in response.text.lower()

        market_card = div_card.Market(response.div_cards)
        if market_card.exact_goods:
            assert market_card.exact_goods.image_url
            assert market_card.exact_goods.name
            assert market_card.exact_goods.price
            if market_card.exact_goods.source is not None:
                assert market_card.exact_goods.source
            assert 'отзыв' in market_card.exact_goods.reviews

        assert len(market_card.goods_gallery) > 3
        for goods in market_card.goods_gallery:
            assert goods.image_url
            assert goods.name
            util.assert_price(goods.price)
            assert goods.action_url

        more_info_card = div_card.MoreInfo(response.div_cards[-1])
        assert 'Ещё информация по этой картинке' in more_info_card.title
        util.assert_div_card_buttons(more_info_card.buttons, self.expected_more_info_buttons.keys())
        for title, expected_intent in self.expected_more_info_buttons.items():
            button_response = alice.click(more_info_card.button(title).image)
            assert button_response.intent == expected_intent

        objects = response.scenario_analytics_info.objects
        tag = objects['tag']['human_readable']
        util.assert_suggests(response.suggests, {f'🔍 "{tag}"', 'Зачитай этот текст', 'Распознанный текст'})
