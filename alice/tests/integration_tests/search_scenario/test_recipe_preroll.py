import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [
    surface.loudspeaker,
    surface.station(is_tv_plugged_in=False),
])
@pytest.mark.experiments('factoid_recipe_preroll')
class TestRecipePrerollInSearchScenario(object):

    owners = ('abc:yandexdialogs2', )

    prerolls = (
        'Нашла рецепт в интернете, но сама его ещё не пробовала. Если хотите узнать, какие есть проверенные рецепты, скажите: «Что приготовить?».',
    )

    @pytest.mark.parametrize('command', [
        'как приготовить плова',
        'рецепт салата цезарь',
        'рецепт греческого салата',
        'как варить какао',
    ])
    def test_preroll(self, alice, command):
        response = alice(command)
        # recipe preroll works only on whitelist websites
        # currently only povar.ru and m.povar.ru are in the whitelist
        if 'povar.ru' in response.text:
            assert any(preroll in response.text for preroll in self.prerolls)
