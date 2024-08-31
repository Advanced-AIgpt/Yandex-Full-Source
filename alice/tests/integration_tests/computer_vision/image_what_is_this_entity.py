import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest

import computer_vision.div_card as div_card
import computer_vision.util as util


@pytest.mark.parametrize('surface', [surface.searchapp])
class TestImageWhatIsThisEntity(object):

    owners = ('polushkin', 'g:cv-search', )

    expected_more_info_buttons = {
        'Что это?': intent.ImageSimilar,
        **util.common_more_info_buttons
    }

    def _find_suggest_startswith(self, response, suggest_suffix):
        for suggest in response.suggests:
            if suggest.title.startswith(suggest_suffix):
                return suggest

    @pytest.mark.parametrize('photo', [
        'http://avatars.mds.yandex.net/get-images-similar-mturk/15681/2ea4e0233dff78e861f11a0f214cb28a/orig',
        'http://avatars.mds.yandex.net/get-images-similar-mturk/16267/210350be3ece70c67a8b22621204d21a/big',
    ])
    def test_check_entity_object(self, alice, photo):
        response = alice.search_by_photo(photo)
        assert response.scenario == scenario.ImageWhatIsThis
        assert response.intent == intent.ImageEntity
        assert not response.text_card
        assert response.has_voice_response()

        objects = response.scenario_analytics_info.objects
        assert objects['entity']['human_readable']

        subject = div_card.Entity(response.div_cards[0])
        assert subject.photo.image_url
        assert subject.name
        assert subject.description
        assert subject.source
        assert subject.action_url

        more_info_card = div_card.MoreInfo(response.div_cards[1])
        assert 'Ещё информация по этой картинке' in more_info_card.title
        util.assert_div_card_buttons(more_info_card.buttons, self.expected_more_info_buttons.keys())
        for title, expected_intent in self.expected_more_info_buttons.items():
            button_response = alice.click(more_info_card.button(title).image)
            assert button_response.intent == expected_intent

        search_suggest_suffix = '🔍 '
        search_suggest = self._find_suggest_startswith(response, search_suggest_suffix)
        assert search_suggest
        assert len(search_suggest.title) > len(search_suggest_suffix)

        util.assert_suggests(response.suggests, {search_suggest.title, 'Что это?'})
