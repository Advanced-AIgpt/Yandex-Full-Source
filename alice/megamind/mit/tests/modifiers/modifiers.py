import pytest

from alice.megamind.mit.library.common.names.item_names import AH_ITEM_MODIFIER_REQUEST
from alice.megamind.mit.library.common.names.node_names import NODE_NAME_MODIFIERS_RUN, NODE_NAME_WALKER_FINALIZE
from alice.megamind.mit.library.request_builder import Voice
from alice.megamind.protos.modifiers.modifier_request_pb2 import TModifierRequest


WHISPER_TAG = 'is_whisper'


class TestModifiers(object):
    @pytest.mark.parametrize('is_request_whisper', [True, False])
    @pytest.mark.experiments('mm_default_whisper_config_for_unauthorized_users=true')
    def test_whisper_modifier(self, alice, apphost_stubber, is_request_whisper):
        def check_modifier_request(ctx):
            request = TModifierRequest()
            request.ParseFromString(ctx.get_protobuf_item(AH_ITEM_MODIFIER_REQUEST))
            assert request.Features.SoundSettings.IsWhisper == is_request_whisper
            assert request.ModifierBody.Layout.OutputSpeech
            assert request.BaseRequest

        def check_modifier_response(ctx):
            ctx.get_protobuf_item(b'mm_modifier_response') is not None

        apphost_stubber.assert_node_input(NODE_NAME_MODIFIERS_RUN, check_modifier_request)
        apphost_stubber.assert_node_input(NODE_NAME_WALKER_FINALIZE, check_modifier_response)
        response = alice(Voice('Привет!', whisper=is_request_whisper))
        assert (WHISPER_TAG in response.output_speech) == is_request_whisper
