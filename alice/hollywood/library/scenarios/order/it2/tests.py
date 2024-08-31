import pytest

from alice.hollywood.library.python.testing.it2 import auth, surface
from alice.hollywood.library.python.testing.it2.input import voice
from alice.hollywood.library.python.testing.it2.stubber import create_stubber_fixture
from alice.hollywood.library.python.testing.it2.input import server_action
from alice.hollywood.library.python.testing.it2.stubber import StubberEndpoint
from alice.hollywood.library.scenarios.order.proto.order_pb2 import TOrderState # noqa

order_stubber = create_stubber_fixture(
    'grocery-alice-gateway.lavka.tst.yandex.net', 80, [StubberEndpoint('/internal/v1/orders/v1/state', ['POST'])], stubs_subdir='order'
)

SEMANTIC_FRAME = {
    "typed_semantic_frame": {
        "order_notification_semantic_frame": {
            "provider_name": {
                "string_value": "lavka"
            },
            "order_id": {
                "string_value": "123456"
            },
            "order_notification_type": {
                "order_notification_type_value": 1
            }
        }
    },
    "analytics": {"origin": "Push", "purpose": "order_notification", "origin_info": "lavka"}
}

HIDE_NAMES_MEMENTO = {
    "UserConfigs": {
        "OrderStatusConfig": {
            "HideItemNames": True
        }
    }
}


@pytest.fixture(scope="function")
def srcrwr_params(order_stubber):
    return {'ORDER_PROXY': f'localhost:{order_stubber.port}'}


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['order']


@pytest.mark.experiments('bg_fresh_granet_form=alice.order.get_status')
@pytest.mark.scenario(name='Order', handle='order')
@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.searchapp])
class Tests:
    skills_request_node_name = 'ORDER_PROXY'

    def test_order(self, alice):
        r = alice(voice('статус заказа из лавки'))
        req = r.sources_dump.get_http_request(self.skills_request_node_name)
        assert req
        resp = r.sources_dump.get_http_response(self.skills_request_node_name)
        assert resp
        analytics_info = r.run_response.ResponseBody.AnalyticsInfo
        assert analytics_info.ProductScenarioName == 'order'
        assert analytics_info.Intent == 'alice.order.get_status'
        layout = r.run_response.ResponseBody.Layout
        assert len(layout.Directives) == 0
        assert layout.Cards[0].Text in ['К сожалению, у вас нет активных заказов в Яндекс Лавке.']
        assert layout.OutputSpeech == layout.Cards[0].Text

    def test_notification_scene(self, alice):
        payload = SEMANTIC_FRAME
        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        analytics_info = r.run_response.ResponseBody.AnalyticsInfo
        assert analytics_info.ProductScenarioName == 'order'
        assert analytics_info.Intent == 'alice.order.notification'
        req = r.sources_dump.get_http_request(self.skills_request_node_name)
        assert req
        resp = r.sources_dump.get_http_response(self.skills_request_node_name)
        assert resp
        assert r.scenario_stages() == {'run'}

    @pytest.mark.experiments('hw_order_test', "hw_order_number_test=1")
    def test_two_joint_order(self, alice):
        # Тест на два полностью пересекающихся заказа, проговариваем уникальные элементы заказа.
        r = alice(voice('статус заказа из лавки'))
        return str(r)

    @pytest.mark.memento(HIDE_NAMES_MEMENTO)
    @pytest.mark.experiments('hw_order_test', "hw_order_number_test=1")
    def test_two_joint_order_dont_call_items(self, alice):
        # Тест на два полностью пересекающихся заказа, не проговариваем уникальные элементы заказа.
        r = alice(voice('статус заказа из лавки'))
        return str(r)

    @pytest.mark.experiments('hw_order_test', "hw_order_number_test=2")
    def test_two_disjoint_orders(self, alice):
        # Тест на два полностью непересекающихся заказа, проговариваем уникальные элементы заказа.
        r = alice(voice('статус заказа из лавки'))
        return str(r)

    @pytest.mark.memento(HIDE_NAMES_MEMENTO)
    @pytest.mark.experiments('hw_order_test', "hw_order_number_test=2")
    def test_two_disjoint_orders_dont_call_items(self, alice):
        # Тест на два полностью непересекающихся заказа, проговариваем уникальные элементы заказа.
        r = alice(voice('статус заказа из лавки'))
        return str(r)

    @pytest.mark.experiments('hw_order_test', "hw_order_number_test=3")
    def test_two_large_joint_orders(self, alice):
        r = alice(voice('статус заказа из лавки'))
        return str(r)

    @pytest.mark.experiments('hw_order_test', "hw_order_number_test=4")
    def test_three_large_joint_orders(self, alice):
        r = alice(voice('статус заказа из лавки'))
        return str(r)

    @pytest.mark.experiments('hw_order_test', "hw_order_number_test=5")
    def test_tree_large_innerjoint_orders(self, alice):
        r = alice(voice('статус заказа из лавки'))
        return str(r)

    @pytest.mark.experiments('hw_order_test', "hw_order_number_test=6")
    def test_one_order(self, alice):
        r = alice(voice('статус заказа из лавки'))
        return str(r)

    def test_market_request(self, alice):
        r = alice(voice('статус заказа из маркета'))
        layout = r.run_response.ResponseBody.Layout
        assert r.run_response.Features.IsIrrelevant
        assert layout.Cards[0].Text in ['Произошла досадная ошибка!']

    def test_wildberries_request(self, alice):
        r = alice(voice('статус заказа из wildberries'))
        layout = r.run_response.ResponseBody.Layout
        assert not r.run_response.Features.IsIrrelevant
        assert layout.Cards[0].Text in ['Я пока не умею этого делать.']

    @pytest.mark.no_oauth
    def test_unauthorized_user(self, alice):
        r = alice(voice('статус заказа из лавки'))
        layout = r.run_response.ResponseBody.Layout
        assert not r.run_response.Features.IsIrrelevant
        assert layout.Cards[0].Text in ['Для того, чтобы я могла овтетить на этот вопрос, вам надо сначала авторизоваться.']
