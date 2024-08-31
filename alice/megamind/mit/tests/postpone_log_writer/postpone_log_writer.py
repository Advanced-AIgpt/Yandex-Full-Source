from alice.megamind.library.apphost_request.protos.analytics_logs_context_pb2 import TAnalyticsLogsContext
from alice.megamind.protos.common.events_pb2 import TEvent
from alice.megamind.mit.library.common.names.item_names import (
    AH_ITEM_ANALYTICS_LOG_CONTEXT,
    AH_ITEM_SKR_EVENT,
)
from alice.megamind.mit.library.common.names.node_names import (
    NODE_NAME_POSTPONE_LOG_WRITER,
    NODE_NAME_WALKER_RUN_FINALIZE,
)
from alice.megamind.mit.library.request_builder import (
    Text,
    Voice,
)


class TestPostponeLogWriter(object):
    def test_postpone_log_writer(self, alice, apphost_stubber):
        def check_analytics(ctx):
            analytics = TAnalyticsLogsContext()
            analytics.ParseFromString(ctx.get_protobuf_item(AH_ITEM_ANALYTICS_LOG_CONTEXT))
            assert analytics.AnalyticsInfo.WinnerScenario.Name == 'RandomNumber'
            assert analytics.QualityStorage
            assert int(analytics.HttpCode) == 200
            assert len(analytics.SpeechKitResponse.VoiceResponse.OutputSpeech.Text)

        apphost_stubber.assert_node_input(NODE_NAME_POSTPONE_LOG_WRITER, check_analytics)
        alice(Voice('Рандомное число от 1 до 124'))

    def test_postpone_log_writer_on_error(self, alice, apphost_stubber):
        query = 'покажи мои штрафы'

        def do_nothing(_):
            pass

        apphost_stubber.mock_node(NODE_NAME_WALKER_RUN_FINALIZE, do_nothing)

        def check_analytics_on_error(ctx):
            try:
                analytics = ctx.get_protobuf_items(AH_ITEM_ANALYTICS_LOG_CONTEXT)
                assert analytics is None, f'Item {AH_ITEM_ANALYTICS_LOG_CONTEXT} should not be in context'
            except KeyError:
                pass
            event = TEvent()
            event.ParseFromString(ctx.get_protobuf_item(AH_ITEM_SKR_EVENT))
            assert event.Text == query

        apphost_stubber.assert_node_input(NODE_NAME_POSTPONE_LOG_WRITER, check_analytics_on_error)
        alice(Text(query))
