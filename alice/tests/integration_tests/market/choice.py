import alice.tests.library.surface as surface
import pytest

from market import div_card


@pytest.mark.parametrize('surface', [surface.searchapp])
class TestMarketChoice(object):

    owners = ('mllnr',)

    def test_offer_not_in_history(self, alice):
        alice('помоги выбрать товар')
        response = alice('чайник')
        gallery = div_card.MarketGallery(response.div_card)
        assert 'adult=0' in gallery.market_url

    @pytest.mark.experiments('market_choice_ext_gallery_open_market')
    def test_open_market_for_item(self, alice):
        alice('помоги выбрать товар')
        response = alice('платье')
        gallery = div_card.MarketGallery(response.div_card)
        for item in gallery:
            assert item.action_url.startswith((
                'https://m.market.yandex.ru/product',
                'https://m.market.yandex.ru/offer/',
            ))

    def test_open_shop_for_item(self, alice):
        alice('помоги выбрать товар')
        response = alice('платье')
        gallery = div_card.MarketGallery(response.div_card)
        # Если товар поставляется маркетом, а не партнёром, то у него не будет кликового урла.
        # Но в данном случае должен найтись хоть один товар от партнёра
        has_click_url = False
        for item in gallery:
            has_click_url |= item.action_url.startswith('https://market-click2.yandex.ru')
            assert item.action_url.startswith((
                'https://m.market.yandex.ru/product',
                'https://market-click2.yandex.ru',
            ))
        assert has_click_url, 'Expected at least one item with market-click2.yandex.ru url'
