import pytest
import re

from .lib import (
    REQUEST_PHRASE, TRANSLATED_PHRASE,
    generate_polyglot_handler,
    not_visited_node_checker
)

from alice.megamind.mit.library.common.names.item_names import (
    AH_ITEM_SCENARIO_REQUEST, AH_ITEM_SCENARIO_RESPONSE,
    AH_ITEM_MODIFIER_REQUEST, AH_ITEM_MODIFIER_RESPONSE,
)
from alice.megamind.mit.library.common.names.node_names import (
    scenario_node_name, Stage,
    NODE_NAME_UTTERANCE_POLYGLOT_HTTP, NODE_NAME_MODIFIERS_RUN
)
from alice.megamind.mit.library.request_builder import (
    MegamindRequestBuilder,
    Voice,
)
from alice.megamind.protos.modifiers.modifier_request_pb2 import TModifierRequest
from alice.megamind.protos.modifiers.modifier_response_pb2 import TModifierResponse
from alice.megamind.protos.scenarios.response_pb2 import TScenarioRunResponse
from alice.megamind.protos.scenarios.request_pb2 import TScenarioRunRequest
from alice.protos.data.language.language_pb2 import ELang


TEST_PRODUCT_SCENARIO_NAME = 'test_product_scenario_name'
SCENARIO_RESPONSE_PHRASE = 'scenario response phrase'

MODIFIER_RESPONSE_PHRASE = 'modifier response phrase'


def assert_is_arabic_utterance(utterance):
    assert not re.search('[а-я]', utterance, re.IGNORECASE)
    assert re.search('[\u0621-\u064A\u0660-\u0669]', utterance, re.IGNORECASE)


def assert_is_russian_utterance(utterance):
    assert re.search('[а-я]', utterance, re.IGNORECASE)
    assert not re.search('[\u0621-\u064A\u0660-\u0669]', utterance, re.IGNORECASE)


def generate_scenario_handler(request_lang, assert_utterance):
    def handler(ctx):
        request = TScenarioRunRequest()
        request.ParseFromString(ctx.get_protobuf_item(AH_ITEM_SCENARIO_REQUEST))

        assert request.BaseRequest.UserLanguage == request_lang
        assert request.Input.Voice.AsrData
        assert_utterance(request.Input.Voice.AsrData[0].Utterance)
        assert_utterance(request.Input.Voice.Utterance)

        response = TScenarioRunResponse()
        response.ResponseBody.AnalyticsInfo.ProductScenarioName = TEST_PRODUCT_SCENARIO_NAME
        response.ResponseBody.Layout.OutputSpeech = SCENARIO_RESPONSE_PHRASE

        ctx.add_protobuf_item(AH_ITEM_SCENARIO_RESPONSE, response)

    return handler


def generate_modifier_handler(request_lang):
    def handler(ctx):
        request = TModifierRequest()
        request.ParseFromString(ctx.get_protobuf_item(AH_ITEM_MODIFIER_REQUEST))

        assert request.BaseRequest.UserLanguage == ELang.L_ARA
        assert request.Features.ProductScenarioName == TEST_PRODUCT_SCENARIO_NAME
        assert request.Features.ScenarioLanguage == request_lang
        assert request.ModifierBody.Layout.OutputSpeech == SCENARIO_RESPONSE_PHRASE

        response = TModifierResponse()
        response.ModifierBody.Layout.OutputSpeech = MODIFIER_RESPONSE_PHRASE

        ctx.add_protobuf_item(AH_ITEM_MODIFIER_RESPONSE, response)

    return handler


@pytest.mark.experiments('mm_allow_lang_ar')
class TestPolyglotPipeline(object):
    @pytest.mark.experiments('mm_subscribe_to_frame=alice.random_number:Vins')
    @pytest.mark.experiments('mm_scenario=Vins')
    def test_ru_only_scenario(self, alice, apphost_stubber):
        request_builder = MegamindRequestBuilder(Voice(REQUEST_PHRASE)).set_lang('ar-SA')

        apphost_stubber.mock_node(NODE_NAME_UTTERANCE_POLYGLOT_HTTP, generate_polyglot_handler())
        apphost_stubber.mock_node(
            scenario_node_name('Vins', Stage.RUN),
            generate_scenario_handler(ELang.L_RUS, assert_is_russian_utterance))
        apphost_stubber.mock_node(NODE_NAME_MODIFIERS_RUN, generate_modifier_handler(ELang.L_RUS))

        response = alice(request_builder)
        assert response.output_speech == MODIFIER_RESPONSE_PHRASE

    @pytest.mark.experiments('mm_subscribe_to_frame=alice.random_number:Commands')
    @pytest.mark.experiments('mm_scenario=Commands')
    def test_ar_friendly_scenario(self, alice, apphost_stubber):
        request_builder = MegamindRequestBuilder(Voice(REQUEST_PHRASE)).set_lang('ar-SA')

        apphost_stubber.mock_node(NODE_NAME_UTTERANCE_POLYGLOT_HTTP, generate_polyglot_handler())
        apphost_stubber.mock_node(
            scenario_node_name('Commands', Stage.RUN),
            generate_scenario_handler(ELang.L_ARA, assert_is_arabic_utterance))
        apphost_stubber.mock_node(NODE_NAME_MODIFIERS_RUN, generate_modifier_handler(ELang.L_ARA))

        response = alice(request_builder)
        assert response.output_speech == MODIFIER_RESPONSE_PHRASE

    @pytest.mark.experiments('mm_subscribe_to_frame=alice.random_number:GetTime')
    @pytest.mark.experiments('mm_scenario=GetTime')
    def test_ar_only_scenario(self, alice, apphost_stubber):
        request_builder = MegamindRequestBuilder(Voice(TRANSLATED_PHRASE)).set_lang('ru-RU')

        apphost_stubber.assert_node_input(NODE_NAME_UTTERANCE_POLYGLOT_HTTP,
                                          not_visited_node_checker, True)
        apphost_stubber.assert_node_input(scenario_node_name('GetTime', Stage.RUN),
                                          not_visited_node_checker, True)
        apphost_stubber.assert_node_input(NODE_NAME_MODIFIERS_RUN,
                                          not_visited_node_checker, True)

        response = alice(request_builder)
        assert response.error and response.error.get('code') == 512
        assert response.output_speech
