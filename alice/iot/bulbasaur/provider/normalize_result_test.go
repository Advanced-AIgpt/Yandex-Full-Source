package provider

import (
	"context"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	xtestadapter "a.yandex-team.ru/alice/iot/bulbasaur/xtest/adapter"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/zaplogger"
)

func TestStatesResult_NormalizePayloadDevices(t *testing.T) {
	normalizer := Normalizer{
		logger: zaplogger.NewNop(),
	}

	type testCase struct {
		request      adapter.StatesRequest
		result       adapter.StatesResult
		defaultError adapter.ErrorCode
		expect       adapter.StatesResult
	}

	check := func(tc testCase) func(*testing.T) {
		return func(t *testing.T) {
			ctx := timestamp.ContextWithTimestamper(context.Background(), timestamp.NewMockTimestamper())
			result := normalizer.normalizeStatesResult(ctx, tc.request, tc.result, tc.defaultError)
			assert.Equal(t, tc.expect, result)
		}
	}

	t.Run("sort result as request", check(testCase{
		request: adapter.StatesRequest{Devices: []adapter.StatesRequestDevice{
			{ID: "ext-id-1"},
			{ID: "ext-id-2"},
			{ID: "ext-id-3"},
		}},
		result: adapter.StatesResult{Payload: adapter.StatesResultPayload{Devices: []adapter.DeviceStateView{
			{ID: "ext-id-2"},
			{ID: "ext-id-3"},
			{ID: "ext-id-1"},
		}}},
		defaultError: adapter.InternalError,
		expect: adapter.StatesResult{Payload: adapter.StatesResultPayload{Devices: []adapter.DeviceStateView{
			{ID: "ext-id-1"},
			{ID: "ext-id-2"},
			{ID: "ext-id-3"},
		}}},
	}))

	t.Run("drop extra devices", check(testCase{
		request: adapter.StatesRequest{Devices: []adapter.StatesRequestDevice{
			{ID: "ext-id-1"},
			{ID: "ext-id-2"},
		}},
		result: adapter.StatesResult{Payload: adapter.StatesResultPayload{Devices: []adapter.DeviceStateView{
			{ID: "ext-id-2"},
			{ID: "ext-id-3"},
			{ID: "ext-id-1"},
		}}},
		defaultError: adapter.InternalError,
		expect: adapter.StatesResult{Payload: adapter.StatesResultPayload{Devices: []adapter.DeviceStateView{
			{ID: "ext-id-1"},
			{ID: "ext-id-2"},
		}}},
	}))

	t.Run("fill missing devices with default error", check(testCase{
		request: adapter.StatesRequest{Devices: []adapter.StatesRequestDevice{
			{ID: "ext-id-1"},
			{ID: "ext-id-2"},
			{ID: "ext-id-3"},
		}},
		result: adapter.StatesResult{Payload: adapter.StatesResultPayload{Devices: []adapter.DeviceStateView{
			{ID: "ext-id-1"},
		}}},
		defaultError: adapter.InternalError,
		expect: adapter.StatesResult{Payload: adapter.StatesResultPayload{Devices: []adapter.DeviceStateView{
			{ID: "ext-id-1"},
			{ID: "ext-id-2", ErrorCode: adapter.InternalError},
			{ID: "ext-id-3", ErrorCode: adapter.InternalError},
		}}},
	}))

	t.Run("fill unsupported errors with default error", check(testCase{
		request: adapter.StatesRequest{Devices: []adapter.StatesRequestDevice{
			{ID: "ext-id-1"},
		}},
		result: adapter.StatesResult{Payload: adapter.StatesResultPayload{Devices: []adapter.DeviceStateView{
			{ID: "ext-id-1", ErrorCode: adapter.NotSupportedInCurrentMode},
		}}},
		defaultError: adapter.InternalError,
		expect: adapter.StatesResult{Payload: adapter.StatesResultPayload{Devices: []adapter.DeviceStateView{
			{ID: "ext-id-1", ErrorCode: adapter.InternalError},
		}}},
	}))

	t.Run("fill timestamps where empty", check(testCase{
		request: adapter.StatesRequest{Devices: []adapter.StatesRequestDevice{
			{ID: "ext-id-1"},
		}},
		result: adapter.StatesResult{
			Payload: adapter.StatesResultPayload{Devices: []adapter.DeviceStateView{
				{
					ID: "ext-id-1",
					Capabilities: []adapter.CapabilityStateView{
						xtestadapter.OnOffState(true, 25),
						xtestadapter.ColorSceneState(model.ColorSceneIDNight, 0),
					},
				},
			}},
		},
		defaultError: adapter.InternalError,
		expect: adapter.StatesResult{
			Payload: adapter.StatesResultPayload{Devices: []adapter.DeviceStateView{
				{
					ID: "ext-id-1",
					Capabilities: []adapter.CapabilityStateView{
						xtestadapter.OnOffState(true, 25),                        // this is not changed
						xtestadapter.ColorSceneState(model.ColorSceneIDNight, 1), // this is now 1
					},
				},
			}},
		},
	}))
}

func TestActionResult_NormalizePayloadDevices(t *testing.T) {
	normalizer := Normalizer{
		logger: zaplogger.NewNop(),
	}

	type testCase struct {
		request      adapter.ActionRequest
		result       adapter.ActionResult
		defaultError adapter.ErrorCode
		expect       adapter.ActionResult
	}

	check := func(tc testCase) func(*testing.T) {
		return func(subT *testing.T) {
			ctx := timestamp.ContextWithTimestamper(context.Background(), timestamp.NewMockTimestamper())
			result := normalizer.normalizeActionResult(ctx, tc.request, tc.result, tc.defaultError)
			assert.Equal(subT, tc.expect, result)
			for _, device := range result.Payload.Devices {
				assert.Nil(subT, device.ActionResult)
			}
		}
	}

	t.Run("sort result as request", check(testCase{
		request: adapter.ActionRequest{Payload: adapter.ActionRequestPayload{Devices: []adapter.DeviceActionRequestView{
			{
				ID: "ext-dev-1",
				Capabilities: []adapter.CapabilityActionView{
					{
						Type:  model.OnOffCapabilityType,
						State: model.OnOffCapabilityState{Instance: model.OnOnOffCapabilityInstance, Value: false},
					},
				},
			},
			{
				ID: "ext-dev-2",
				Capabilities: []adapter.CapabilityActionView{
					{
						Type:  model.OnOffCapabilityType,
						State: model.OnOffCapabilityState{Instance: model.OnOnOffCapabilityInstance, Value: true},
					},
					{
						Type:  model.ModeCapabilityType,
						State: model.ModeCapabilityState{Instance: model.FanSpeedModeInstance, Value: model.MediumMode},
					},
				},
			},
		}}},
		result: adapter.ActionResult{Payload: adapter.ActionResultPayload{Devices: []adapter.DeviceActionResultView{
			{
				ID: "ext-dev-2",
				Capabilities: []adapter.CapabilityActionResultView{
					{
						Type: model.ModeCapabilityType,
						State: adapter.CapabilityStateActionResultView{
							Instance: string(model.FanSpeedModeInstance),
							ActionResult: adapter.StateActionResult{
								Status:    adapter.ERROR,
								ErrorCode: adapter.InvalidValue,
							},
						},
					},
					{
						Type: model.OnOffCapabilityType,
						State: adapter.CapabilityStateActionResultView{
							Instance: string(model.OnOnOffCapabilityInstance),
							ActionResult: adapter.StateActionResult{
								Status: adapter.DONE,
							},
						},
					},
				},
			},
			{
				ID: "ext-dev-1",
				Capabilities: []adapter.CapabilityActionResultView{
					{
						Type: model.OnOffCapabilityType,
						State: adapter.CapabilityStateActionResultView{
							Instance: string(model.OnOnOffCapabilityInstance),
							ActionResult: adapter.StateActionResult{
								Status:    adapter.ERROR,
								ErrorCode: adapter.DeviceBusy,
							},
						},
					},
				},
			},
		}}},
		defaultError: "WTF",
		expect: adapter.ActionResult{Payload: adapter.ActionResultPayload{Devices: []adapter.DeviceActionResultView{
			{
				ID: "ext-dev-1",
				Capabilities: []adapter.CapabilityActionResultView{
					{
						Type: model.OnOffCapabilityType,
						State: adapter.CapabilityStateActionResultView{
							Instance: string(model.OnOnOffCapabilityInstance),
							ActionResult: adapter.StateActionResult{
								Status:    adapter.ERROR,
								ErrorCode: adapter.DeviceBusy,
							},
						},
						Timestamp: 1,
					},
				},
			},
			{
				ID: "ext-dev-2",
				Capabilities: []adapter.CapabilityActionResultView{
					{
						Type: model.OnOffCapabilityType,
						State: adapter.CapabilityStateActionResultView{
							Instance: string(model.OnOnOffCapabilityInstance),
							ActionResult: adapter.StateActionResult{
								Status: adapter.DONE,
							},
						},
						Timestamp: 1,
					},
					{
						Type: model.ModeCapabilityType,
						State: adapter.CapabilityStateActionResultView{
							Instance: string(model.FanSpeedModeInstance),
							ActionResult: adapter.StateActionResult{
								Status:    adapter.ERROR,
								ErrorCode: adapter.InvalidValue,
							},
						},
						Timestamp: 1,
					},
				},
			},
		}}},
	}))

	t.Run("fill timestamps where empty", check(testCase{
		request: adapter.ActionRequest{Payload: adapter.ActionRequestPayload{Devices: []adapter.DeviceActionRequestView{
			{
				ID: "ext-dev-1",
				Capabilities: []adapter.CapabilityActionView{
					{
						Type:  model.OnOffCapabilityType,
						State: model.OnOffCapabilityState{Instance: model.OnOnOffCapabilityInstance, Value: false},
					},
				},
			},
			{
				ID: "ext-dev-2",
				Capabilities: []adapter.CapabilityActionView{
					{
						Type:  model.OnOffCapabilityType,
						State: model.OnOffCapabilityState{Instance: model.OnOnOffCapabilityInstance, Value: true},
					},
				},
			},
		}}},
		result: adapter.ActionResult{Payload: adapter.ActionResultPayload{Devices: []adapter.DeviceActionResultView{
			{
				ID: "ext-dev-2",
				Capabilities: []adapter.CapabilityActionResultView{
					{
						Type: model.OnOffCapabilityType,
						State: adapter.CapabilityStateActionResultView{
							Instance: string(model.OnOnOffCapabilityInstance),
							ActionResult: adapter.StateActionResult{
								Status: adapter.DONE,
							},
						},
						Timestamp: 100500,
					},
				},
			},
			{
				ID: "ext-dev-1",
				Capabilities: []adapter.CapabilityActionResultView{
					{
						Type: model.OnOffCapabilityType,
						State: adapter.CapabilityStateActionResultView{
							Instance: string(model.OnOnOffCapabilityInstance),
							ActionResult: adapter.StateActionResult{
								Status:    adapter.ERROR,
								ErrorCode: adapter.DeviceBusy,
							},
						},
						Timestamp: 2086,
					},
				},
			},
		}}},
		defaultError: "WTF",
		expect: adapter.ActionResult{Payload: adapter.ActionResultPayload{Devices: []adapter.DeviceActionResultView{
			{
				ID: "ext-dev-1",
				Capabilities: []adapter.CapabilityActionResultView{
					{
						Type: model.OnOffCapabilityType,
						State: adapter.CapabilityStateActionResultView{
							Instance: string(model.OnOnOffCapabilityInstance),
							ActionResult: adapter.StateActionResult{
								Status:    adapter.ERROR,
								ErrorCode: adapter.DeviceBusy,
							},
						},
						Timestamp: 2086,
					},
				},
			},
			{
				ID: "ext-dev-2",
				Capabilities: []adapter.CapabilityActionResultView{
					{
						Type: model.OnOffCapabilityType,
						State: adapter.CapabilityStateActionResultView{
							Instance: string(model.OnOnOffCapabilityInstance),
							ActionResult: adapter.StateActionResult{
								Status: adapter.DONE,
							},
						},
						Timestamp: 100500,
					},
				},
			},
		}}},
	}))

	t.Run("drop extra devices and capabilities", check(testCase{
		request: adapter.ActionRequest{Payload: adapter.ActionRequestPayload{Devices: []adapter.DeviceActionRequestView{
			{
				ID: "ext-dev-2",
				Capabilities: []adapter.CapabilityActionView{
					{
						Type:  model.ModeCapabilityType,
						State: model.ModeCapabilityState{Instance: model.FanSpeedModeInstance, Value: model.MediumMode},
					},
				},
			},
		}}},
		result: adapter.ActionResult{Payload: adapter.ActionResultPayload{Devices: []adapter.DeviceActionResultView{
			{
				ID: "ext-dev-2",
				Capabilities: []adapter.CapabilityActionResultView{
					{
						Type: model.ModeCapabilityType,
						State: adapter.CapabilityStateActionResultView{
							Instance: string(model.FanSpeedModeInstance),
							ActionResult: adapter.StateActionResult{
								Status:    adapter.ERROR,
								ErrorCode: adapter.InvalidValue,
							},
						},
					},
					{
						Type: model.OnOffCapabilityType,
						State: adapter.CapabilityStateActionResultView{
							Instance: string(model.OnOnOffCapabilityInstance),
							ActionResult: adapter.StateActionResult{
								Status: adapter.DONE,
							},
						},
					},
				},
			},
			{
				ID: "ext-dev-1",
				Capabilities: []adapter.CapabilityActionResultView{
					{
						Type: model.OnOffCapabilityType,
						State: adapter.CapabilityStateActionResultView{
							Instance: string(model.OnOnOffCapabilityInstance),
							ActionResult: adapter.StateActionResult{
								Status:    adapter.ERROR,
								ErrorCode: adapter.DeviceBusy,
							},
						},
					},
				},
			},
		}}},
		defaultError: "WTF",
		expect: adapter.ActionResult{Payload: adapter.ActionResultPayload{Devices: []adapter.DeviceActionResultView{
			{
				ID: "ext-dev-2",
				Capabilities: []adapter.CapabilityActionResultView{
					{
						Type: model.ModeCapabilityType,
						State: adapter.CapabilityStateActionResultView{
							Instance: string(model.FanSpeedModeInstance),
							ActionResult: adapter.StateActionResult{
								Status:    adapter.ERROR,
								ErrorCode: adapter.InvalidValue,
							},
						},
						Timestamp: 1,
					},
				},
			},
		}}},
	}))

	t.Run("fill missing devices and capabilities with default error", check(testCase{
		request: adapter.ActionRequest{Payload: adapter.ActionRequestPayload{Devices: []adapter.DeviceActionRequestView{
			{
				ID: "ext-dev-1",
				Capabilities: []adapter.CapabilityActionView{
					{
						Type:  model.OnOffCapabilityType,
						State: model.OnOffCapabilityState{Instance: model.OnOnOffCapabilityInstance, Value: false},
					},
				},
			},
			{
				ID: "ext-dev-2",
				Capabilities: []adapter.CapabilityActionView{
					{
						Type:  model.OnOffCapabilityType,
						State: model.OnOffCapabilityState{Instance: model.OnOnOffCapabilityInstance, Value: true},
					},
					{
						Type:  model.ModeCapabilityType,
						State: model.ModeCapabilityState{Instance: model.FanSpeedModeInstance, Value: model.MediumMode},
					},
				},
			},
		}}},
		result: adapter.ActionResult{Payload: adapter.ActionResultPayload{Devices: []adapter.DeviceActionResultView{
			{
				ID: "ext-dev-2",
				Capabilities: []adapter.CapabilityActionResultView{
					{
						Type: model.ModeCapabilityType,
						State: adapter.CapabilityStateActionResultView{
							Instance: string(model.FanSpeedModeInstance),
							ActionResult: adapter.StateActionResult{
								Status:    adapter.ERROR,
								ErrorCode: adapter.InvalidValue,
							},
						},
						Timestamp: 1,
					},
				},
			},
		}}},
		defaultError: "WTF",
		expect: adapter.ActionResult{Payload: adapter.ActionResultPayload{Devices: []adapter.DeviceActionResultView{
			{
				ID: "ext-dev-1",
				Capabilities: []adapter.CapabilityActionResultView{
					{
						Type: model.OnOffCapabilityType,
						State: adapter.CapabilityStateActionResultView{
							Instance: string(model.OnOnOffCapabilityInstance),
							ActionResult: adapter.StateActionResult{
								Status:    adapter.ERROR,
								ErrorCode: "WTF",
							},
						},
						Timestamp: 1,
					},
				},
			},
			{
				ID: "ext-dev-2",
				Capabilities: []adapter.CapabilityActionResultView{
					{
						Type: model.OnOffCapabilityType,
						State: adapter.CapabilityStateActionResultView{
							Instance: string(model.OnOnOffCapabilityInstance),
							ActionResult: adapter.StateActionResult{
								Status:    adapter.ERROR,
								ErrorCode: "WTF",
							},
						},
						Timestamp: 1,
					},
					{
						Type: model.ModeCapabilityType,
						State: adapter.CapabilityStateActionResultView{
							Instance: string(model.FanSpeedModeInstance),
							ActionResult: adapter.StateActionResult{
								Status:    adapter.ERROR,
								ErrorCode: adapter.InvalidValue,
							},
						},
						Timestamp: 1,
					},
				},
			},
		}}},
	}))

	t.Run("use adapter.DeviceActionResultView.ActionResult to fill every capability", check(testCase{
		request: adapter.ActionRequest{Payload: adapter.ActionRequestPayload{Devices: []adapter.DeviceActionRequestView{
			{
				ID: "ext-dev-1",
				Capabilities: []adapter.CapabilityActionView{
					{
						Type:  model.OnOffCapabilityType,
						State: model.OnOffCapabilityState{Instance: model.OnOnOffCapabilityInstance, Value: false},
					},
					{
						Type:  model.ModeCapabilityType,
						State: model.ModeCapabilityState{Instance: model.CleanUpModeInstance, Value: model.AutoMode},
					},
				},
			},
			{
				ID: "ext-dev-2",
				Capabilities: []adapter.CapabilityActionView{
					{
						Type:  model.OnOffCapabilityType,
						State: model.OnOffCapabilityState{Instance: model.OnOnOffCapabilityInstance, Value: true},
					},
					{
						Type:  model.ModeCapabilityType,
						State: model.ModeCapabilityState{Instance: model.FanSpeedModeInstance, Value: model.MediumMode},
					},
				},
			},
		}}},
		result: adapter.ActionResult{Payload: adapter.ActionResultPayload{Devices: []adapter.DeviceActionResultView{
			{
				ID: "ext-dev-1",
				ActionResult: &adapter.StateActionResult{
					Status:    adapter.ERROR,
					ErrorCode: adapter.DeviceNotFound,
				},
				Capabilities: []adapter.CapabilityActionResultView{
					{
						Type: model.OnOffCapabilityType,
						State: adapter.CapabilityStateActionResultView{
							Instance: string(model.OnOnOffCapabilityInstance),
							ActionResult: adapter.StateActionResult{
								Status:    adapter.ERROR,
								ErrorCode: adapter.DeviceBusy,
							},
						},
					},
					{
						Type: model.ModeCapabilityType,
						State: adapter.CapabilityStateActionResultView{
							Instance: string(model.CleanUpModeInstance),
							ActionResult: adapter.StateActionResult{
								Status:    adapter.ERROR,
								ErrorCode: adapter.HumanInvolvementNeeded,
							},
						},
					},
				},
			},
			{
				ID: "ext-dev-2",
				ActionResult: &adapter.StateActionResult{
					Status: adapter.DONE,
				},
			},
		}}},
		defaultError: "WTF",
		expect: adapter.ActionResult{Payload: adapter.ActionResultPayload{Devices: []adapter.DeviceActionResultView{
			{
				ID: "ext-dev-1",
				Capabilities: []adapter.CapabilityActionResultView{
					{
						Type: model.OnOffCapabilityType,
						State: adapter.CapabilityStateActionResultView{
							Instance: string(model.OnOnOffCapabilityInstance),
							ActionResult: adapter.StateActionResult{
								Status:    adapter.ERROR,
								ErrorCode: adapter.DeviceBusy,
							},
						},
						Timestamp: 1,
					},
					{
						Type: model.ModeCapabilityType,
						State: adapter.CapabilityStateActionResultView{
							Instance: string(model.CleanUpModeInstance),
							ActionResult: adapter.StateActionResult{
								Status:    adapter.ERROR,
								ErrorCode: adapter.HumanInvolvementNeeded,
							},
						},
						Timestamp: 1,
					},
				},
			},
			{
				ID: "ext-dev-2",
				Capabilities: []adapter.CapabilityActionResultView{
					{
						Type: model.OnOffCapabilityType,
						State: adapter.CapabilityStateActionResultView{
							Instance: string(model.OnOnOffCapabilityInstance),
							ActionResult: adapter.StateActionResult{
								Status: adapter.DONE,
							},
						},
						Timestamp: 1,
					},
					{
						Type: model.ModeCapabilityType,
						State: adapter.CapabilityStateActionResultView{
							Instance: string(model.FanSpeedModeInstance),
							ActionResult: adapter.StateActionResult{
								Status: adapter.DONE,
							},
						},
						Timestamp: 1,
					},
				},
			},
		}}},
	}))

	t.Run("fix adapter.ActionResult stupids", check(testCase{
		request: adapter.ActionRequest{Payload: adapter.ActionRequestPayload{Devices: []adapter.DeviceActionRequestView{
			{
				ID: "ext-dev-1",
				Capabilities: []adapter.CapabilityActionView{
					{
						Type:  model.OnOffCapabilityType,
						State: model.OnOffCapabilityState{Instance: model.OnOnOffCapabilityInstance, Value: false},
					},
				},
			},
			{
				ID: "ext-dev-2",
				Capabilities: []adapter.CapabilityActionView{
					{
						Type:  model.OnOffCapabilityType,
						State: model.OnOffCapabilityState{Instance: model.OnOnOffCapabilityInstance, Value: true},
					},
					{
						Type:  model.ModeCapabilityType,
						State: model.ModeCapabilityState{Instance: model.FanSpeedModeInstance, Value: model.MediumMode},
					},
				},
			},
		}}},
		result: adapter.ActionResult{Payload: adapter.ActionResultPayload{Devices: []adapter.DeviceActionResultView{
			{
				ID: "ext-dev-1",
				// NB: adapter.DeviceActionResultView.ActionResult overrides adapter.DeviceActionResultView.Capabilities[i].ActionResult
				ActionResult: &adapter.StateActionResult{
					Status:    adapter.DONE,
					ErrorCode: adapter.DeviceNotFound,
				},
			},
			{
				ID: "ext-dev-2",
				Capabilities: []adapter.CapabilityActionResultView{
					{
						Type: model.OnOffCapabilityType,
						State: adapter.CapabilityStateActionResultView{
							Instance:     string(model.OnOnOffCapabilityInstance),
							ActionResult: adapter.StateActionResult{},
						},
					},
					{
						Type: model.ModeCapabilityType,
						State: adapter.CapabilityStateActionResultView{
							Instance: string(model.FanSpeedModeInstance),
							ActionResult: adapter.StateActionResult{
								Status: adapter.ERROR,
							},
						},
					},
				},
			},
		}}},
		defaultError: "WTF",
		expect: adapter.ActionResult{Payload: adapter.ActionResultPayload{Devices: []adapter.DeviceActionResultView{
			{
				ID: "ext-dev-1",
				Capabilities: []adapter.CapabilityActionResultView{
					{
						Type: model.OnOffCapabilityType,
						State: adapter.CapabilityStateActionResultView{
							Instance: string(model.OnOnOffCapabilityInstance),
							ActionResult: adapter.StateActionResult{
								Status:    adapter.ERROR,
								ErrorCode: adapter.DeviceNotFound,
							},
						},
						Timestamp: 1,
					},
				},
			},
			{
				ID: "ext-dev-2",
				Capabilities: []adapter.CapabilityActionResultView{
					{
						Type: model.OnOffCapabilityType,
						State: adapter.CapabilityStateActionResultView{
							Instance: string(model.OnOnOffCapabilityInstance),
							ActionResult: adapter.StateActionResult{
								Status:    adapter.ERROR,
								ErrorCode: "WTF",
							},
						},
						Timestamp: 1,
					},
					{
						Type: model.ModeCapabilityType,
						State: adapter.CapabilityStateActionResultView{
							Instance: string(model.FanSpeedModeInstance),
							ActionResult: adapter.StateActionResult{
								Status:    adapter.ERROR,
								ErrorCode: "WTF",
							},
						},
						Timestamp: 1,
					},
				},
			},
		}}},
	}))

	t.Run("capability result vs device result", check(testCase{
		request: adapter.ActionRequest{Payload: adapter.ActionRequestPayload{Devices: []adapter.DeviceActionRequestView{
			{
				ID: "ext-dev-1",
				Capabilities: []adapter.CapabilityActionView{
					{
						Type:  model.OnOffCapabilityType,
						State: model.OnOffCapabilityState{Instance: model.OnOnOffCapabilityInstance, Value: false},
					},
				},
			},
			{
				ID: "ext-dev-2",
				Capabilities: []adapter.CapabilityActionView{
					{
						Type:  model.OnOffCapabilityType,
						State: model.OnOffCapabilityState{Instance: model.OnOnOffCapabilityInstance, Value: true},
					},
					{
						Type:  model.ModeCapabilityType,
						State: model.ModeCapabilityState{Instance: model.FanSpeedModeInstance, Value: model.MediumMode},
					},
				},
			},
		}}},
		result: adapter.ActionResult{Payload: adapter.ActionResultPayload{Devices: []adapter.DeviceActionResultView{
			{
				ID: "ext-dev-1",
				Capabilities: []adapter.CapabilityActionResultView{
					{
						Type: model.OnOffCapabilityType,
						State: adapter.CapabilityStateActionResultView{
							Instance: string(model.OnOnOffCapabilityInstance),
							ActionResult: adapter.StateActionResult{
								Status:    adapter.ERROR,
								ErrorCode: adapter.DeviceBusy,
							},
						},
					},
				},
				ActionResult: &adapter.StateActionResult{
					Status: adapter.DONE,
				},
			},
			{
				ID: "ext-dev-2",
				Capabilities: []adapter.CapabilityActionResultView{
					{
						Type: model.ModeCapabilityType,
						State: adapter.CapabilityStateActionResultView{
							Instance: string(model.FanSpeedModeInstance),
							ActionResult: adapter.StateActionResult{
								Status: adapter.ERROR,
							},
						},
					},
				},
				ActionResult: &adapter.StateActionResult{
					Status: adapter.DONE,
				},
			},
		}}},
		defaultError: "WTF",
		expect: adapter.ActionResult{Payload: adapter.ActionResultPayload{Devices: []adapter.DeviceActionResultView{
			{
				ID: "ext-dev-1",
				Capabilities: []adapter.CapabilityActionResultView{
					{
						Type: model.OnOffCapabilityType,
						State: adapter.CapabilityStateActionResultView{
							Instance: string(model.OnOnOffCapabilityInstance),
							ActionResult: adapter.StateActionResult{
								Status:    adapter.ERROR,
								ErrorCode: adapter.DeviceBusy,
							},
						},
						Timestamp: 1,
					},
				},
			},
			{
				ID: "ext-dev-2",
				Capabilities: []adapter.CapabilityActionResultView{
					{
						Type: model.OnOffCapabilityType,
						State: adapter.CapabilityStateActionResultView{
							Instance: string(model.OnOnOffCapabilityInstance),
							ActionResult: adapter.StateActionResult{
								Status: adapter.DONE,
							},
						},
						Timestamp: 1,
					},
					{
						Type: model.ModeCapabilityType,
						State: adapter.CapabilityStateActionResultView{
							Instance: string(model.FanSpeedModeInstance),
							ActionResult: adapter.StateActionResult{
								Status:    adapter.ERROR,
								ErrorCode: "WTF",
							},
						},
						Timestamp: 1,
					},
				},
			},
		}}},
	}))
}
