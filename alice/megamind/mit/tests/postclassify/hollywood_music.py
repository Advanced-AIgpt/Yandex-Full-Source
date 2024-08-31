import pytest

from alice.megamind.mit.library.common.names.item_names import AH_ITEM_SCENARIO_RESPONSE
from alice.megamind.mit.library.common.names.node_names import scenario_node_name, Stage
from alice.megamind.mit.library.request_builder import ServerAction
import alice.megamind.mit.library.common.names.scenario_names as scenario_names

from alice.hollywood.library.framework.proto.framework_state_pb2 import TProtoHwSceneCCAArguments
from alice.hollywood.library.scenarios.music.proto.music_pb2 import TMusicScenarioSceneArgsFmRadio
from alice.megamind.protos.scenarios.response_pb2 import TScenarioRunResponse, TScenarioContinueResponse


def response_generator(response):
    def generator(ctx):
        ctx.add_protobuf_item(AH_ITEM_SCENARIO_RESPONSE, response)
    return generator


@pytest.mark.experiments('analytics_info')
class TestFmRadio:

    def _vins_run_response_general_conversation(self):
        response = TScenarioRunResponse()

        response.Features.Intent = 'personal_assistant.general_conversation.general_conversation'
        response.Features.VinsFeatures.SetInParent()  # initialize empty message
        response.Features.PlayerFeatures.SetInParent()  # initialize empty message
        response.Features.IgnoresExpectedRequest = True

        response.ResponseBody.Layout.OutputSpeech = 'включаю'
        response.ResponseBody.Layout.ShouldListen = True

        return response

    def _hollywood_music_run_response_fm_radio(self):
        scene_args = TMusicScenarioSceneArgsFmRadio()
        scene_args.RequestStatus = TMusicScenarioSceneArgsFmRadio.ERequestStatus.OK
        scene_args.ExplicitRequest.FmRadioId = 'silver_rain'

        args = TProtoHwSceneCCAArguments()
        args.ProtoHwScene.SceneArgs.Args.Pack(scene_args)
        args.ProtoHwScene.SceneArgs.SceneName = 'fm_radio'
        args.ProtoHwScene.RunFeatures.Intent = 'alice.music.fm_radio_play'
        args.ProtoHwScene.RunFeatures.PlayerFeatures.RestorePlayer = True

        response = TScenarioRunResponse()
        response.ContinueArguments.Pack(args)
        response.Features.Intent = 'alice.music.fm_radio_play'
        response.Features.PlayerFeatures.RestorePlayer = True  # this is the field that wins postclassification!

        return response

    def _hollywood_music_continue_response_fm_radio(self):
        response = TScenarioContinueResponse()
        response.ResponseBody.Layout.OutputSpeech = 'включаю радио шансон'
        return response

    def test_continue(self, alice, apphost_stubber):
        '''
        На "девайсе с экраном" нажимаем на кнопку продолжения, посылается TSF плеерной команды
        Надо чтобы выиграл HollywoodMusic, несмотря на ответы остальных сценариев на
        плеерную команду
        '''
        apphost_stubber.mock_node(scenario_node_name(scenario_names.VINS_SCENARIO, Stage.RUN),
                                  response_generator(self._vins_run_response_general_conversation()))
        apphost_stubber.mock_node(scenario_node_name(scenario_names.HOLLYWOODMUSIC_SCENARIO, Stage.RUN),
                                  response_generator(self._hollywood_music_run_response_fm_radio()))
        apphost_stubber.mock_node(scenario_node_name(scenario_names.HOLLYWOODMUSIC_SCENARIO, Stage.CONTINUE),
                                  response_generator(self._hollywood_music_continue_response_fm_radio()))

        payload = {
            'typed_semantic_frame': {
                'player_continue_semantic_frame': {
                },
            },
            'analytics': {
                'purpose': 'player_continue',
                'origin': 'SmartSpeaker',
            },
        }

        response = alice(ServerAction(name='@@mm_semantic_frame', payload=payload))

        assert response.winner_scenario == scenario_names.HOLLYWOODMUSIC_SCENARIO


@pytest.mark.experiments('analytics_info')
class TestFrameRedirect:

    def _hollywood_music_run_response_start_multiroom(self):
        response = TScenarioRunResponse()

        # add empty clear_queue directive
        response.ResponseBody.Layout.Directives.add().ClearQueueDirective.SetInParent()

        # add multiroom_semantic_frame directive
        tsf_directive = response.ResponseBody.Layout.Directives.add().MultiroomSemanticFrameDirective
        tsf_directive.DeviceId = 'master_device_id'
        tsf_directive.Body.TypedSemanticFrame.MusicPlaySemanticFrame.SetInParent()

        # this is the field that wins postclassification!
        response.Features.PlayerFeatures.RestorePlayer = True

        return response

    def _commands_run_response_start_multiroom(self):
        response = TScenarioRunResponse()

        # add start_multiroom directive
        mr_directive = response.ResponseBody.Layout.Directives.add().StartMultiroomDirective
        mr_directive.LocationInfo.RoomsIds.append('test_room_id')
        mr_directive.LocationInfo.IncludeCurrentDeviceId = True

        # set frame
        response.ResponseBody.SemanticFrame.Name = 'alice.multiroom.start_multiroom'

        return response

    def test_start_multiroom_in_location(self, alice, apphost_stubber):
        '''
        При запросе "продолжи в комнате XXX" матчится фрейм "start_multiroom",
        надо чтобы выигрывал HollywoodMusic, так как он лучше обрабатывает
        запросы в контексте музыки
        '''
        self._commands_run_response_start_multiroom()
        apphost_stubber.mock_node(scenario_node_name(scenario_names.HOLLYWOODMUSIC_SCENARIO, Stage.RUN),
                                  response_generator(self._hollywood_music_run_response_start_multiroom()))
        apphost_stubber.mock_node(scenario_node_name(scenario_names.COMMANDS_SCENARIO, Stage.RUN),
                                  response_generator(self._commands_run_response_start_multiroom()))

        payload = {
            'typed_semantic_frame': {
                'start_multiroom_semantic_frame': {
                    'location_room': {
                        'user_iot_room_value': 'a6c04487-a487-47b4-8e7e-76d3a45fc5d6',
                    },
                },
            },
            'analytics': {
                'purpose': 'player_continue',
                'origin': 'SmartSpeaker',
            },
        }

        response = alice(ServerAction(name='@@mm_semantic_frame', payload=payload))

        assert response.winner_scenario == scenario_names.HOLLYWOODMUSIC_SCENARIO
