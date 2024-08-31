package steps

import (
	"testing"

	libnlg "a.yandex-team.ru/alice/library/go/nlg"
	"github.com/stretchr/testify/assert"
	"google.golang.org/protobuf/types/known/wrapperspb"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/frames"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	xtestdata "a.yandex-team.ru/alice/iot/bulbasaur/xtest/data"
	"a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	endpoint "a.yandex-team.ru/alice/protos/endpoint"
)

func Test_buildRecoveryAction(t *testing.T) {
	frame := frames.ScenarioStepActionsFrame{
		LaunchID:  "april",
		StepIndex: 12,
		Actions: []*common.TIoTDeviceActions{
			{
				DeviceId: "device-id",
				Actions: []*common.TIoTCapabilityAction{
					xtestdata.OnOffCapabilityAction(true).State().ToIotCapabilityAction(),
				},
				ExternalDeviceId: "external-device-id",
				SkillId:          model.YANDEXIO,
			},
			{
				DeviceId: "speaker-id",
				Actions: []*common.TIoTCapabilityAction{
					xtestdata.QuasarTTSCapabilityAction("nothing happened today").State().ToIotCapabilityAction(),
				},
				ExternalDeviceId: "external-speaker-id",
				SkillId:          model.QUASAR,
			},
		},
	}
	actualRecoveryActionCallbackDirective, actualErr := buildRecoveryAction(frame)
	assert.NoError(t, actualErr)
	expectedRecoveryCallback := ActionsResultCallback{
		LaunchID:  "april",
		StepIndex: 12,
		DeviceResults: []DeviceActionResult{
			{
				ID:     "speaker-id",
				Status: model.ErrorScenarioLaunchDeviceActionStatus,
			},
		},
	}
	expectedRecoveryActionDirective, err := expectedRecoveryCallback.ToCallbackDirective()
	assert.NoError(t, err)
	assert.Equal(t, expectedRecoveryActionDirective, actualRecoveryActionCallbackDirective)
}

func Test_buildDirectives(t *testing.T) {
	frame := frames.ScenarioStepActionsFrame{
		LaunchID:  "april",
		StepIndex: 12,
		Actions: []*common.TIoTDeviceActions{
			{
				DeviceId: "device-id",
				Actions: []*common.TIoTCapabilityAction{
					xtestdata.OnOffCapabilityAction(true).State().ToIotCapabilityAction(),
				},
				ExternalDeviceId: "external-device-id",
				SkillId:          model.YANDEXIO,
			},
			{
				DeviceId: "speaker-id",
				Actions: []*common.TIoTCapabilityAction{
					xtestdata.ColorSceneCapabilityAction(model.ColorSceneIDNight).State().ToIotCapabilityAction(),
					xtestdata.QuasarTTSCapabilityAction("nothing happened today").State().ToIotCapabilityAction(),
				},
				ExternalDeviceId: "external-speaker-id",
				SkillId:          model.QUASAR,
			},
		},
	}
	actualDirectives, actualErr := buildDirectives(frame)
	assert.NoError(t, actualErr)

	expectedDirectives := []*scenarios.TDirective{
		{
			EndpointId: wrapperspb.String("external-device-id"),
			Directive: &scenarios.TDirective_OnOffDirective{
				OnOffDirective: &endpoint.TOnOffCapability_TOnOffDirective{
					Name: "on_off_directive",
					On:   true,
				},
			},
		},
		{
			EndpointId: wrapperspb.String("external-speaker-id"),
			Directive: &scenarios.TDirective_SetColorSceneDirective{
				SetColorSceneDirective: &endpoint.TColorCapability_TSetColorSceneDirective{
					Name:       "set_color_scene_directive",
					ColorScene: endpoint.TColorCapability_NightScene,
				},
			},
		},
	}

	assert.Equal(t, expectedDirectives, actualDirectives)
}

func Test_buildStackEngine(t *testing.T) {
	t.Run("with_completion", func(t *testing.T) {
		frame := frames.ScenarioStepActionsFrame{
			LaunchID:  "april",
			StepIndex: 12,
			Actions: []*common.TIoTDeviceActions{
				{
					DeviceId: "device-id",
					Actions: []*common.TIoTCapabilityAction{
						xtestdata.OnOffCapabilityAction(true).State().ToIotCapabilityAction(),
					},
					ExternalDeviceId: "external-device-id",
					SkillId:          model.YANDEXIO,
				},
				{
					DeviceId: "speaker-id",
					Actions: []*common.TIoTCapabilityAction{
						xtestdata.ColorSceneCapabilityAction(model.ColorSceneIDNight).State().ToIotCapabilityAction(),
						xtestdata.QuasarTTSCapabilityAction("nothing happened today").State().ToIotCapabilityAction(),
					},
					ExternalDeviceId: "external-speaker-id",
					SkillId:          model.QUASAR,
				},
			},
		}

		sideEffects, err := buildSpeakerActions(frame)
		assert.NoError(t, err)
		assert.Len(t, sideEffects.Directives, 0)
		expectedCompletionCallback := ActionsResultCallback{
			LaunchID:  "april",
			StepIndex: 12,
			DeviceResults: []DeviceActionResult{
				{
					ID:     "speaker-id",
					Status: model.DoneScenarioLaunchDeviceActionStatus,
				},
			},
		}
		expectedCallbackDirective, err := expectedCompletionCallback.ToCallbackDirective()
		assert.NoError(t, err)
		expectedEffects := []*scenarios.TStackEngineEffect{
			{
				Effect: &scenarios.TStackEngineEffect_Callback{Callback: expectedCallbackDirective},
			},
		}
		assert.Equal(t, expectedEffects, sideEffects.StackEngineEffects)
		expectedNLG := libnlg.FromVariant("nothing happened today")
		assert.Equal(t, expectedNLG, sideEffects.NLG)
	})

	t.Run("without_completion", func(t *testing.T) {
		frame := frames.ScenarioStepActionsFrame{
			LaunchID:  "april",
			StepIndex: 12,
			Actions: []*common.TIoTDeviceActions{
				{
					DeviceId: "device-id",
					Actions: []*common.TIoTCapabilityAction{
						xtestdata.OnOffCapabilityAction(true).State().ToIotCapabilityAction(),
					},
					ExternalDeviceId: "external-device-id",
					SkillId:          model.YANDEXIO,
				},
				{
					DeviceId: "speaker-id",
					Actions: []*common.TIoTCapabilityAction{
						xtestdata.ColorSceneCapabilityAction(model.ColorSceneIDNight).State().ToIotCapabilityAction(),
						xtestdata.QuasarAliceShowCapabilityAction().State().ToIotCapabilityAction(),
					},
					ExternalDeviceId: "external-speaker-id",
					SkillId:          model.QUASAR,
				},
			},
		}
		sideEffects, err := buildSpeakerActions(frame)
		assert.NoError(t, err)
		assert.Len(t, sideEffects.Directives, 0)
		expectedEffects := []*scenarios.TStackEngineEffect{
			{
				Effect: &scenarios.TStackEngineEffect_ParsedUtterance{
					ParsedUtterance: &scenarios.TParsedUtterance{
						TypedSemanticFrame: xtestdata.QuasarAliceShowCapabilityAction().State().(model.QuasarCapabilityState).Value.ToTypedSemanticFrame(),
						Analytics: &common.TAnalyticsTrackingModule{
							ProductScenario: "iot",
							Origin:          common.TAnalyticsTrackingModule_Scenario,
							Purpose:         "iot_scenarios_speakers_actions",
							OriginInfo:      "april",
						},
						Params: &common.TFrameRequestParams{
							DisableShouldListen: true,
							DisableOutputSpeech: false,
						},
					},
				},
			},
		}
		assert.Equal(t, expectedEffects, sideEffects.StackEngineEffects)
	})

	t.Run("sound set level directive", func(t *testing.T) {
		frame := frames.ScenarioStepActionsFrame{
			LaunchID:  "april",
			StepIndex: 25,
			Actions: []*common.TIoTDeviceActions{
				{
					DeviceId: "device-id",
					Actions: []*common.TIoTCapabilityAction{
						xtestdata.OnOffCapabilityAction(true).State().ToIotCapabilityAction(),
					},
					ExternalDeviceId: "external-device-id",
					SkillId:          model.YANDEXIO,
				},
				{
					DeviceId: "speaker-id",
					Actions: []*common.TIoTCapabilityAction{
						xtestdata.ColorSceneCapabilityAction(model.ColorSceneIDNight).State().ToIotCapabilityAction(),
						xtestdata.QuasarVolumeCapabilityAction(7, false).State().ToIotCapabilityAction(),
					},
					ExternalDeviceId: "external-speaker-id",
					SkillId:          model.QUASAR,
				},
			},
		}
		sideEffects, err := buildSpeakerActions(frame)
		assert.NoError(t, err)
		expectedDirectives := []*scenarios.TDirective{
			{
				Directive: &scenarios.TDirective_SoundSetLevelDirective{
					SoundSetLevelDirective: &scenarios.TSoundSetLevelDirective{
						NewLevel: int32(7),
					},
				},
			},
		}
		assert.Equal(t, expectedDirectives, sideEffects.Directives)
	})
}
