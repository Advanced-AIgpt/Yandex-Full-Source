import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.voice
@pytest.mark.parametrize('surface', [surface.taximeter])
@pytest.mark.experiments(f'mm_enable_protocol_scenario={scenario.Taximeter}')
class TestTaximeterRequestconfirm(object):

    owners = ('artfulvampire', 'g:developersyandextaxi')

    @pytest.mark.parametrize('command', ['принимаю', 'берём заказ', 'поехали'])
    def test_positive_answer(self, alice, command):
        response = alice(command)

        assert response.scenario == scenario.Taximeter
        assert response.intent == intent.TaximeterRequestconfirm
        assert response.directive.name == directives.names.OpenUriDirective
        assert response.directive.payload.uri == 'taximeter://income_order?action=accept'

    @pytest.mark.parametrize('command', ['не надо', 'пропусти заказ'])
    def test_negative_answer(self, alice, command):
        response = alice(command)

        assert response.scenario == scenario.Taximeter
        assert response.intent == intent.TaximeterRequestconfirm
        assert response.directive.name == directives.names.OpenUriDirective
        assert response.directive.payload.uri == 'taximeter://income_order?action=decline'
