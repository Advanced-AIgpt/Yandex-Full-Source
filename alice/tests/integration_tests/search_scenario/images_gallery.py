import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [
    surface.launcher,
    surface.searchapp,
])
class TestImagesGalleryInSearchScenario(object):

    owners = ('tolyandex', )

    @pytest.mark.parametrize('command', [
        'картинки котов',
        'поздравление открыткой',
    ])
    def test_gallery(self, alice, command):
        response = alice(command)
        assert response.product_scenario == 'images_gallery'
        assert len(response.cards) > 0
