import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest

import computer_vision.util as util


@pytest.mark.parametrize('surface', [surface.searchapp])
class TestImageWhatIsThisTags(object):

    owners = ('polushkin', 'g:cv-search', )

    def test_similar_with_tag(self, alice):
        photo = 'https://avatars.mds.yandex.net/get-images-similar-mturk/13615/1LGkKbIG4nDc/orig'
        response = alice.search_by_photo(photo)
        assert response.scenario == scenario.ImageWhatIsThis
        assert response.intent == intent.ImageSimilar
        assert response.text_card

        objects = response.scenario_analytics_info.objects
        tag = objects['tag']['human_readable']
        assert tag and tag in response.text
        util.assert_suggests(response.suggests, {
            'Похожая картина', f'🔍 "{tag}"',
        })
