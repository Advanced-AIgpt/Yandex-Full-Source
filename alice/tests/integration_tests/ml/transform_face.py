import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


class TestTransformFace(object):

    owners = ('valerymal', 'g:milab', )

    commands = [
        'состарь меня',
        'измени мою внешность',
        'как я выглядела в молодости',
        'сделай меня женщиной',
        'как бы я выглядела если бы была мужчиной',
    ]

    def assert_response(self, response):
        assert response.text_card
        assert response.text_card.text == response.text

        cards = response.div_cards
        assert len(cards) == 2

        output = cards[0]
        assert len(output) == 4

        gallery = cards[1].gallery
        assert len(gallery) == 8

    @pytest.mark.parametrize('surface', [
        surface.searchapp,
    ])
    @pytest.mark.parametrize('command', commands)
    def test_supported_surfaces(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.TransformFace

        input_url = 'https://avatars.mds.yandex.net/get-alice/4336950/test_ZQ9GHiqCbZo6OsN1TlPZDQ/big'
        response = alice.search_by_photo(input_url)
        self.assert_response(response)

        suggesed_transform_image = response.div_cards[1].gallery.first
        response = alice.click(suggesed_transform_image)
        self.assert_response(response)

    @pytest.mark.parametrize('surface', [
        surface.automotive,
        surface.loudspeaker,
        surface.navi,
        surface.smart_tv,
        surface.station,
        surface.watch,
        surface.yabro_win,
    ])
    @pytest.mark.parametrize('command', commands)
    def test_unsupported_surfaces(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.TransformFace
        assert response.text_card
        assert not response.div_card
        assert response.text == 'Я бы с удовольствием, но не могу трансформировать внешность на этом устройстве. Попросите меня об этом в приложении Яндекса, и я с радостью!'
