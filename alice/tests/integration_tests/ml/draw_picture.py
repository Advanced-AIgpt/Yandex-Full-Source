import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


class TestDrawPicture(object):

    owners = ('valerymal', 'g:milab', )

    commands = [
        'алиса нарисуй мне картину',
        'алиса нарисуй себя',
        'нарисуй картинку про пищу богов',
        'создайте полотно в стиле ван гога',
        'напиши шедевр про трактор',
    ]

    def check_response_validity(self, response, expected_num_blocks):
        assert response.scenario == scenario.DrawPicture
        assert response.suggests

        assert response.text_card
        assert response.text_card.text == response.text

        assert response.div_card
        assert len(response.div_card) == expected_num_blocks
        assert response.div_card.image
        assert response.div_card.image.action_url
        assert response.div_card.image.image_url

    @pytest.mark.parametrize('surface', [
        surface.launcher,
        surface.searchapp,
    ])
    @pytest.mark.parametrize('command', commands)
    def test_draw_picture(self, alice, command):
        response = alice(command)
        self.check_response_validity(response, 4)

    @pytest.mark.parametrize('surface', [
        surface.yabro_win,
    ])
    @pytest.mark.parametrize('command', commands)
    def test_draw_picture_yabro(self, alice, command):
        response = alice(command)
        self.check_response_validity(response, 1)

    @pytest.mark.parametrize('surface', [
        surface.automotive,
        surface.navi,
        surface.watch,
    ])
    @pytest.mark.parametrize('command', commands)
    def test_unsupported_surface(self, alice, command):
        response = alice(command)
        assert response.scenario != scenario.DrawPicture

    @pytest.mark.parametrize('surface', [
        surface.loudspeaker,
        surface.smart_tv,
        surface.station,
    ])
    @pytest.mark.parametrize('command', commands)
    def test_station_surface(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.DrawPicture
        assert response.text_card
        assert not response.div_card
        assert response.text == 'Я бы с удовольствием, но не могу показать вам картину на этом устройстве. Попросите меня об этом в приложении Яндекса, и я с радостью!'
