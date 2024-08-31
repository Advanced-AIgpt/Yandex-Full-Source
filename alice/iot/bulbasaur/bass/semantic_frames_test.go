package bass

import (
	"encoding/json"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/frames"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	megamindcommonpb "a.yandex-team.ru/alice/megamind/protos/common"
)

func TestSemanticFramesMarshalling(t *testing.T) {
	type testCase struct {
		frame    ITypedSemanticFrame
		expected string
	}
	testCases := []testCase{
		{
			frame: IoTBroadcastSuccessTypedSemanticFrame{
				DevicesID:  TypedSemanticFrameStringSlot{StringValue: "id"},
				ProductIDs: TypedSemanticFrameStringSlot{StringValue: "id"},
			},
			expected: `{"iot_broadcast_success":{"devices_id":{"string_value":"id"},"product_ids":{"string_value":"id"}}}`,
		},
		{
			frame: IoTBroadcastFailureTypedSemanticFrame{
				TimeoutMs: TypedSemanticFrameUInt32Slot{UInt32Value: 60000},
				Reason:    TypedSemanticFrameStringSlot{StringValue: "some reason"},
			},
			expected: `{"iot_broadcast_failure":{"timeout_ms":{"uint32_value":60000},"reason":{"string_value":"some reason"}}}`,
		},
		{
			frame: IoTDiscoverySuccessTypedSemanticFrame{
				DeviceIDs:  TypedSemanticFrameStringSlot{StringValue: "device"},
				ProductIDs: TypedSemanticFrameStringSlot{StringValue: "product"},
				DeviceType: TypedSemanticFrameStringSlot{StringValue: "type"},
			},
			expected: `{"iot_discovery_success":{"device_ids":{"string_value":"device"},"product_ids":{"string_value":"product"},"device_type":{"string_value":"type"}}}`,
		},
		{
			frame: IoTSpeakerActionTypedSemanticFrame{
				LaunchID:  "some-id",
				StepIndex: 5,
				CapabilityAction: frames.SpeakerActionCapabilityValue{
					Type: model.QuasarCapabilityType,
					State: model.QuasarCapabilityState{
						Instance: model.TTSCapabilityInstance,
						Value: model.TTSQuasarCapabilityValue{
							Text: "фразочка",
						},
					},
				},
			},
			expected: `{"iot_scenario_speaker_action_semantic_frame":{"launch_id":{"string_value":"some-id"},"step_index":{"uint32_value":5},"capability_action":{"capability_action_value":{"type":"QuasarCapabilityType","quasar_capability_state":{"instance":"tts","tts_value":{"text":"фразочка"}}}}}}`,
		},
		{
			frame: IoTSpeakerActionTypedSemanticFrame{
				LaunchID:  "some-id",
				StepIndex: 5,
				CapabilityAction: frames.SpeakerActionCapabilityValue{
					Type: model.QuasarCapabilityType,
					State: model.QuasarCapabilityState{
						Instance: model.MusicPlayCapabilityInstance,
						Value: model.MusicPlayQuasarCapabilityValue{
							SearchText:       "kek",
							PlayInBackground: true,
						},
					},
				},
			},
			expected: `{"iot_scenario_speaker_action_semantic_frame":{"launch_id":{"string_value":"some-id"},"step_index":{"uint32_value":5},"capability_action":{"capability_action_value":{"type":"QuasarCapabilityType","quasar_capability_state":{"instance":"music_play","music_play_value":{"search_text":"kek","play_in_background":true}}}}}}`,
		},
		{
			frame: IoTSpeakerActionTypedSemanticFrame{
				LaunchID:  "some-id",
				StepIndex: 5,
				CapabilityAction: frames.SpeakerActionCapabilityValue{
					Type: model.QuasarCapabilityType,
					State: model.QuasarCapabilityState{
						Instance: model.NewsCapabilityInstance,
						Value: model.NewsQuasarCapabilityValue{
							Topic:            model.IndexSpeakerNewsTopic,
							PlayInBackground: true,
						},
					},
				},
			},
			expected: `{"iot_scenario_speaker_action_semantic_frame":{"launch_id":{"string_value":"some-id"},"step_index":{"uint32_value":5},"capability_action":{"capability_action_value":{"type":"QuasarCapabilityType","quasar_capability_state":{"instance":"news","news_value":{"topic":"index","play_in_background":true}}}}}}`,
		},
		{
			frame: IoTSpeakerActionTypedSemanticFrame{
				LaunchID:  "some-id",
				StepIndex: 5,
				CapabilityAction: frames.SpeakerActionCapabilityValue{
					Type: model.QuasarCapabilityType,
					State: model.QuasarCapabilityState{
						Instance: model.SoundPlayCapabilityInstance,
						Value: model.SoundPlayQuasarCapabilityValue{
							Sound: "chainsaw-1",
						},
					},
				},
			},
			expected: `{"iot_scenario_speaker_action_semantic_frame":{"launch_id":{"string_value":"some-id"},"step_index":{"uint32_value":5},"capability_action":{"capability_action_value":{"type":"QuasarCapabilityType","quasar_capability_state":{"instance":"sound_play","sound_play_value":{"sound":"chainsaw-1"}}}}}}`,
		},
		{
			frame: IoTSpeakerActionTypedSemanticFrame{
				LaunchID:  "some-id",
				StepIndex: 5,
				CapabilityAction: frames.SpeakerActionCapabilityValue{
					Type: model.QuasarCapabilityType,
					State: model.QuasarCapabilityState{
						Instance: model.VolumeCapabilityInstance,
						Value: model.VolumeQuasarCapabilityValue{
							Value:    1,
							Relative: true,
						},
					},
				},
			},
			expected: `{"iot_scenario_speaker_action_semantic_frame":{"launch_id":{"string_value":"some-id"},"step_index":{"uint32_value":5},"capability_action":{"capability_action_value":{"type":"QuasarCapabilityType","quasar_capability_state":{"instance":"volume","volume_value":{"value":1,"relative":true}}}}}}`,
		},
		{
			frame: IoTSpeakerActionTypedSemanticFrame{
				LaunchID:  "some-id",
				StepIndex: 5,
				CapabilityAction: frames.SpeakerActionCapabilityValue{
					Type: model.QuasarCapabilityType,
					State: model.QuasarCapabilityState{
						Instance: model.StopEverythingCapabilityInstance,
						Value:    model.StopEverythingQuasarCapabilityValue{},
					},
				},
			},
			expected: `{"iot_scenario_speaker_action_semantic_frame":{"launch_id":{"string_value":"some-id"},"step_index":{"uint32_value":5},"capability_action":{"capability_action_value":{"type":"QuasarCapabilityType","quasar_capability_state":{"instance":"stop_everything","stop_everything_value":{}}}}}}`,
		},
		{
			frame: IoTSpeakerActionTypedSemanticFrame{
				LaunchID:  "some-id",
				StepIndex: 5,
				CapabilityAction: frames.SpeakerActionCapabilityValue{
					Type: model.QuasarCapabilityType,
					State: model.QuasarCapabilityState{
						Instance: model.WeatherCapabilityInstance,
						Value:    model.WeatherQuasarCapabilityValue{},
					},
				},
			},
			expected: `{"iot_scenario_speaker_action_semantic_frame":{"launch_id":{"string_value":"some-id"},"step_index":{"uint32_value":5},"capability_action":{"capability_action_value":{"type":"QuasarCapabilityType","quasar_capability_state":{"instance":"weather","weather_value":{}}}}}}`,
		},
		{
			frame: IoTSpeakerActionTypedSemanticFrame{
				LaunchID:  "some-id",
				StepIndex: 5,
				CapabilityAction: frames.SpeakerActionCapabilityValue{
					Type: model.QuasarCapabilityType,
					State: model.QuasarCapabilityState{
						Instance: model.AliceShowCapabilityInstance,
						Value:    model.AliceShowQuasarCapabilityValue{},
					},
				},
			},
			expected: `{"iot_scenario_speaker_action_semantic_frame":{"launch_id":{"string_value":"some-id"},"step_index":{"uint32_value":5},"capability_action":{"capability_action_value":{"type":"QuasarCapabilityType","quasar_capability_state":{"instance":"alice_show","alice_show_value":{}}}}}}`,
		},
		{
			frame: YandexIOActionTypedSemanticFrame{
				Actions: []*megamindcommonpb.TIoTDeviceActions{
					{
						DeviceId:         "lamp-device-id-1",
						ExternalDeviceId: "lamp-device-ext-id-1",
						SkillId:          model.YANDEXIO,
						Actions: []*megamindcommonpb.TIoTCapabilityAction{
							{
								Type: megamindcommonpb.TIoTUserInfo_TCapability_OnOffCapabilityType,
								State: &megamindcommonpb.TIoTCapabilityAction_OnOffCapabilityState{
									OnOffCapabilityState: &megamindcommonpb.TIoTUserInfo_TCapability_TOnOffCapabilityState{
										Instance: model.OnOnOffCapabilityInstance.String(),
										Value:    true,
									},
								},
							},
						},
					},
				},
			},
			expected: `{"iot_yandex_io_action_semantic_frame":{"request":{"request_value":{"endpoint_actions":[{"device_id":"lamp-device-id-1","actions":[{"type":"OnOffCapabilityType","on_off_capability_state":{"instance":"on","value":true}}],"external_device_id":"lamp-device-ext-id-1","skill_id":"YANDEX_IO"}]}}}}`,
		},
	}
	for _, tc := range testCases {
		bytes, err := json.Marshal(tc.frame)
		assert.NoError(t, err)
		assert.Equal(t, tc.expected, string(bytes), "invalid marshalling: typed frame %q, expected: %s, actual: %s", tc.frame.Type(), tc.expected, string(bytes))
	}
}
