import pytest

from .lib import (
    REQUEST_PHRASE, TRANSLATED_PHRASE,
    generate_polyglot_handler,
    not_visited_node_checker,
)
from alice.megamind.mit.library.common.names.item_names import AH_ITEM_BEGEMOT_REQUEST
from alice.megamind.mit.library.common.names.node_names import (
    NODE_NAME_UTTERANCE_POLYGLOT_HTTP,
    NODE_NAME_BEGEMOT_WORKER_MEGAMIND, NODE_NAME_BEGEMOT_POLYGLOT_WORKER_MEGAMIND,
    NODE_NAME_BEGEMOT_WORKER_BEGGINS, NODE_NAME_BEGEMOT_POLYGLOT_WORKER_BEGGINS,
    NODE_NAME_BEGEMOT_WORKER_MEGAMIND_MERGER, NODE_NAME_BEGEMOT_POLYGLOT_WORKER_MEGAMIND_MERGER,
    NODE_NAME_POLYGLOT_BEGEMOT_MERGER_MERGER
)
from alice.megamind.mit.library.request_builder import (
    MegamindRequestBuilder,
    Voice,
)


def generate_begemot_checker(lang, text):
    def handler(ctx):
        request = ctx.get_first_item(AH_ITEM_BEGEMOT_REQUEST)
        assert request['params']['uil'][0].decode() == lang
        assert request['params']['text'][0].decode() == text
    return handler


def generate_begemot_merger_checker(lang):
    def handler(ctx):
        request = ctx.get_first_item(AH_ITEM_BEGEMOT_REQUEST)
        assert request['params']['wizextra']
    return handler


def generate_polyglot_begemot_merger_merger_checker(lang):
    def handler(ctx):
        request = ctx.get_first_item(AH_ITEM_BEGEMOT_REQUEST)
        assert request['params']['wizextra']
    return handler


@pytest.mark.experiments('mm_allow_lang_ar')
class TestPolyglotBegemot(object):
    def test_ru_request(self, alice, apphost_stubber):
        apphost_stubber.mock_node(NODE_NAME_UTTERANCE_POLYGLOT_HTTP, not_visited_node_checker, True)

        apphost_stubber.assert_node_input(NODE_NAME_BEGEMOT_WORKER_MEGAMIND,
                                          generate_begemot_checker('ru', TRANSLATED_PHRASE))
        apphost_stubber.assert_node_input(NODE_NAME_BEGEMOT_WORKER_BEGGINS,
                                          generate_begemot_checker('ru', TRANSLATED_PHRASE))
        apphost_stubber.assert_node_input(NODE_NAME_BEGEMOT_WORKER_MEGAMIND_MERGER,
                                          generate_begemot_merger_checker('ru'))

        apphost_stubber.assert_node_input(NODE_NAME_BEGEMOT_POLYGLOT_WORKER_MEGAMIND,
                                          not_visited_node_checker, True)
        apphost_stubber.assert_node_input(NODE_NAME_BEGEMOT_POLYGLOT_WORKER_BEGGINS,
                                          not_visited_node_checker, True)
        apphost_stubber.assert_node_input(NODE_NAME_BEGEMOT_POLYGLOT_WORKER_MEGAMIND_MERGER,
                                          not_visited_node_checker, True)
        apphost_stubber.assert_node_input(NODE_NAME_POLYGLOT_BEGEMOT_MERGER_MERGER,
                                          not_visited_node_checker, True)

        request_builder = MegamindRequestBuilder(Voice(TRANSLATED_PHRASE)).set_lang('ru-RU')
        alice(request_builder)

    def test_ar_request(self, alice, apphost_stubber):
        apphost_stubber.mock_node(NODE_NAME_UTTERANCE_POLYGLOT_HTTP, generate_polyglot_handler())

        apphost_stubber.assert_node_input(NODE_NAME_BEGEMOT_WORKER_MEGAMIND,
                                          generate_begemot_checker('ru', TRANSLATED_PHRASE))
        apphost_stubber.assert_node_input(NODE_NAME_BEGEMOT_WORKER_BEGGINS,
                                          generate_begemot_checker('ru', TRANSLATED_PHRASE))
        apphost_stubber.assert_node_input(NODE_NAME_BEGEMOT_WORKER_MEGAMIND_MERGER,
                                          generate_begemot_merger_checker('ru'))

        apphost_stubber.assert_node_input(NODE_NAME_BEGEMOT_POLYGLOT_WORKER_MEGAMIND,
                                          generate_begemot_checker('ar', REQUEST_PHRASE))
        apphost_stubber.assert_node_input(NODE_NAME_BEGEMOT_POLYGLOT_WORKER_BEGGINS,
                                          generate_begemot_checker('ar', REQUEST_PHRASE))
        apphost_stubber.assert_node_input(NODE_NAME_BEGEMOT_POLYGLOT_WORKER_MEGAMIND_MERGER,
                                          generate_begemot_merger_checker('ar'))
        apphost_stubber.assert_node_input(NODE_NAME_POLYGLOT_BEGEMOT_MERGER_MERGER,
                                          generate_polyglot_begemot_merger_merger_checker('ar'))

        request_builder = MegamindRequestBuilder(Voice(REQUEST_PHRASE)).set_lang('ar-SA')
        alice(request_builder)
