import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.surface as surface
import pytest


def get_log_id(directive):
    return directive.payload.div2_card.body.log_id


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.experiments('mm_enable_combinators', 'mm_enable_combinator=CentaurCombinator', 'mm_enable_protocol_scenario=PhotoFrame')
@pytest.mark.parametrize('surface', [surface.smart_display])
class TestSmartDisplayTeasers(object):
    owners = ('nkodosov', )

    TEST_CAROUSEL_ID = 'test_carousel_id'
    CAROUSEL_SHOW_TIME_SEC = 300
    NEWS_TEASER_LOG_ID = 'news_scenario'
    WEATHER_TEASER_LOG_ID = 'weather.teasers'
    PHOTO_TEASER_LOG_ID = 'gallery-screensaver'

    def test_collect_teasers(self, alice):
        response = alice.collect_teasers(self.TEST_CAROUSEL_ID)
        assert response.combinator_product_name == 'CentaurTeasersCombinator'

        assert response.directives[0].name == directives.names.UpdateSpaceActionsDirective

        add_card_directives = response.directives[1:-1]
        assert all(directive.name == directives.names.AddCardDirective for directive in add_card_directives)

        news_count = len([d for d in add_card_directives if get_log_id(d) == self.NEWS_TEASER_LOG_ID])
        assert news_count > 3
        weather_count = len([d for d in add_card_directives if get_log_id(d) == self.WEATHER_TEASER_LOG_ID])
        assert weather_count == 1
        photo_count = len([d for d in add_card_directives if get_log_id(d) == self.PHOTO_TEASER_LOG_ID])
        assert photo_count > 3

        roteta_cards_directive = response.directives[-1]
        assert roteta_cards_directive.name == directives.names.RotateCardsDirective
        assert roteta_cards_directive.payload.carousel_id == self.TEST_CAROUSEL_ID
        assert roteta_cards_directive.payload.carousel_show_time_sec == self.CAROUSEL_SHOW_TIME_SEC
