package localscenarios

import (
	"testing"

	"github.com/stretchr/testify/assert"
	"google.golang.org/protobuf/types/known/anypb"

	"a.yandex-team.ru/alice/iot/bulbasaur/model/directives"
	"a.yandex-team.ru/alice/library/go/xproto"
	iotpb "a.yandex-team.ru/alice/protos/data/iot"
	endpointpb "a.yandex-team.ru/alice/protos/endpoint"
)

func TestDirectivesMarshalling(t *testing.T) {
	type testCase struct {
		frame    directives.SpeechkitDirective
		expected string
	}
	lightDirective1, _ := directives.NewProtoSpeechkitDirective(directives.NewOnOffDirective("light-ext-1", true))
	lightDirective2, _ := directives.NewProtoSpeechkitDirective(directives.NewOnOffDirective("light-ext-2", false))

	expectedLocalScenarios := []*iotpb.TLocalScenario{
		{
			ID: "localScenarioID",
			Condition: &iotpb.TLocalScenarioCondition{
				Condition: &iotpb.TLocalScenarioCondition_AnyOfCondition{
					AnyOfCondition: &iotpb.TLocalScenarioCondition_TAnyOfCondition{
						Conditions: []*iotpb.TLocalScenarioCondition{
							{
								Condition: &iotpb.TLocalScenarioCondition_CapabilityEventCondition{
									CapabilityEventCondition: &iotpb.TLocalScenarioCondition_TCapabilityEventCondition{
										EndpointID: "motion-sensor-ext-id",
										EventCondition: xproto.MustAny(anypb.New(
											&endpointpb.TMotionCapability_TCondition{Events: []endpointpb.TCapability_EEventType{
												endpointpb.TCapability_MotionDetectedEventType,
											}},
										)),
									},
								},
							},
							{
								Condition: &iotpb.TLocalScenarioCondition_CapabilityEventCondition{
									CapabilityEventCondition: &iotpb.TLocalScenarioCondition_TCapabilityEventCondition{
										EndpointID: "button-sensor-ext-id",
										EventCondition: xproto.MustAny(anypb.New(
											&endpointpb.TButtonCapability_TCondition{Events: []endpointpb.TCapability_EEventType{
												endpointpb.TCapability_ButtonClickEventType,
											}},
										)),
									},
								},
							},
						},
					},
				},
			},
			Steps: []*iotpb.TLocalScenario_TStep{
				{
					Step: &iotpb.TLocalScenario_TStep_DirectivesStep{
						DirectivesStep: &iotpb.TLocalScenario_TStep_TDirectivesStep{
							Directives: []*anypb.Any{
								xproto.MustAny(anypb.New(lightDirective1)),
								xproto.MustAny(anypb.New(lightDirective2)),
							},
						},
					},
				},
			},
			EffectiveTime: nil,
		},
	}
	testCases := []testCase{
		{
			frame:    NewAddScenariosSpeechkitDirective("endpoint_id", expectedLocalScenarios),
			expected: `{"name":"add_iot_scenarios_directive", "scenarios":[{"id":"localScenarioID", "condition":{"any_of_condition":{"conditions":[{"capability_event_condition":{"endpoint_id":"motion-sensor-ext-id", "event_condition":{"@type":"type.googleapis.com/NAlice.TMotionCapability.TCondition", "events":["MotionDetectedEventType"]}}}, {"capability_event_condition":{"endpoint_id":"button-sensor-ext-id", "event_condition":{"@type":"type.googleapis.com/NAlice.TButtonCapability.TCondition", "events":["ButtonClickEventType"]}}}]}}, "steps":[{"directives_step":{"directives":[{"@type":"type.googleapis.com/NAlice.NSpeechKit.TDirective", "type":"client_action", "name":"on_off_directive", "sub_name":"on_off_directive", "payload":{"on":true}, "endpoint_id":"light-ext-1"}, {"@type":"type.googleapis.com/NAlice.NSpeechKit.TDirective", "type":"client_action", "name":"on_off_directive", "sub_name":"on_off_directive", "payload":{"on":false}, "endpoint_id":"light-ext-2"}]}}]}]}`,
		},
		{
			frame:    NewRemoveScenariosSpeechkitDirective("endpoint_id", []string{"delete-me"}),
			expected: `{"ids":["delete-me"]}`,
		},
	}
	for _, tc := range testCases {
		bytes, err := tc.frame.MarshalJSONPayload()
		assert.NoError(t, err)
		assert.Equal(t, tc.expected, string(bytes), "invalid marshalling: directive %q, expected: %s, actual: %s", tc.frame.SpeechkitName(), tc.expected, string(bytes))
	}
}
