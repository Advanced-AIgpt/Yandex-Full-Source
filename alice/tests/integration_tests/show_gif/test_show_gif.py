import alice.tests.library.directives as directives
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


class _TestShowGif(object):
    owners = ('jan-fazli', 'olegator',)


@pytest.mark.parametrize('surface', [
    surface.automotive,
    surface.launcher,
    surface.navi,
    surface.smart_tv,
    surface.station,
    surface.watch,
    surface.yabro_win,
])
class TestCantShowGif(_TestShowGif):

    @pytest.mark.parametrize('command', [
        'гифку',
        'покажи гифку',
        'хочу посмотреть смешную гифку',
        'cкинь седьмую gif',
    ])
    def test(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.ShowGif
        assert response.text_card
        assert not response.div_cards
        assert response.text in [
            'Простите, я пока не могу показать здесь гифку:( Но я обязательно научусь.',
        ]


class TestShowGif(_TestShowGif):

    show_commands = [
        ('скинь гифку', 0),
        ('покажи первую гифку', 1),
        ('гифка номер пять', 5),
    ]

    show_responses = (
        'Ловите!',
        'Гифка для вас!',
        'Осторожно, она шевелится!',
        'Держите, красивая.',
        'Нашла для вас кое-что.',
        'Смотрите, кое-что нашла.',
        'Нашла. Нравится?',
        'Как вам такое?',
        'Вот гифка.',
        'Специально для вас!',
        'Любой каприз за ваши деньги! Шучу - это бесплатно.',
        'Нашла. Вот я молодец!',
        'Вот гифка. Самой нравится.',
        'Вот классная гифка.',
        'Нашла для вас нечто великолепное.',
        'Держите.',
        'Как вам такой колор?',
        'Вот. Правда, классная?',
    )

    @pytest.mark.parametrize('surface', [surface.searchapp])
    @pytest.mark.parametrize('command, number', show_commands)
    def test_show_gif(self, alice, command, number):
        response = alice(command)
        assert response.scenario == scenario.ShowGif

        assert len(response.cards) == 2
        assert response.text_card
        assert response.text in self.show_responses

        assert response.div_card
        assert response.div_card.gif_card.type == 'gif_card'

    @pytest.mark.parametrize('surface', [surface.searchapp])
    @pytest.mark.parametrize('command, number', show_commands)
    @pytest.mark.experiments('hw_debug_gif_to_text')
    def test_show_gif_text(self, alice, command, number):
        response = alice(command)
        assert response.scenario == scenario.ShowGif

        assert len(response.cards) == 2
        assert len(response.cards) == len(response.text_cards)
        assert response.text in self.show_responses

        gif_text_card = response.text_cards[1]
        assert gif_text_card.text.startswith('Показываю гифку:')

    @pytest.mark.parametrize('surface', [surface.station_pro])
    @pytest.mark.parametrize('command, number', show_commands)
    def test_led_show_gif(self, alice, command, number):
        response = alice(command)
        assert response.scenario == scenario.ShowGif
        assert response.text in self.show_responses

        assert len(response.directives) == 2
        assert response.directives[0].name == directives.names.ForceDisplayCardsDirective

        led_directive = response.directives[1]
        assert led_directive.name == directives.names.DrawLedScreenDirective
        assert led_directive.payload.animation_sequence[0].frontal_led_image.endswith(f'{number}.gif')
