import alice.tests.library.surface as surface
import pytest
from alice.protos.analytics.dummy_response.response_pb2 import TResponse
import alice.tests.library.directives as directives


class TestWebtouchShazamScenario(object):

    owners = ('bvdvlg', )

    @pytest.mark.parametrize('surface', [surface.webtouch])
    def test_show_promo(self, alice):
        response = alice('что сейчас играет')
        assert response.scenario_analytics_info.object("dummy_response").reason == TResponse.EReason.SurfaceInability
        assert response.directive.name == directives.names.ShowPromoDirective
