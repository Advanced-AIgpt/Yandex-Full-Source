package provider

import (
	"context"
	"fmt"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	xtestadapter "a.yandex-team.ru/alice/iot/bulbasaur/xtest/adapter"
	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/alice/library/go/timestamp"
)

func TestJSONRPCProvider_Unlink(t *testing.T) {
	p := NewJSONRPCServerMock().WithUnlinkResponses(map[string]adapter.UnlinkResult{
		"1": {RequestID: "1"},
	})

	testCases := []struct {
		name      string
		requestID string
	}{
		{
			name:      "unlink",
			requestID: "1",
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			ctx := requestid.WithRequestID(context.Background(), tc.requestID)
			err := p.provider.Unlink(ctx)
			assert.NoError(t, err)

			expectRequest := adapter.JSONRPCRequest{
				Headers: adapter.JSONRPCHeaders{
					Authorization: fmt.Sprintf("Bearer %s", p.testToken),
					RequestID:     tc.requestID,
				},
				RequestType: adapter.Unlink,
			}
			assert.Equal(t, expectRequest, p.requests[tc.requestID])
		})
	}
}

func TestJSONRPCProvider_Discover(t *testing.T) {
	p := NewJSONRPCServerMock().WithDiscoveryResponses(map[string]adapter.DiscoveryResult{
		"3": {
			RequestID: "3",
			Payload: adapter.DiscoveryPayload{
				UserID: "MultiUser",
				Devices: []adapter.DeviceInfoView{
					xtestadapter.SwitchDeviceDiscoveryResult("all", "SuperDevice"),
				},
			},
		},
	})

	testCases := []struct {
		name           string
		requestID      string
		expectResponse adapter.DiscoveryResult
	}{
		{
			name:      "discover_several_devices",
			requestID: "3",
			expectResponse: adapter.DiscoveryResult{
				RequestID: "3",
				Payload: adapter.DiscoveryPayload{
					UserID: "MultiUser",
					Devices: []adapter.DeviceInfoView{
						xtestadapter.SwitchDeviceDiscoveryResult("all", "SuperDevice"),
					},
				},
			},
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			ctx := requestid.WithRequestID(context.Background(), tc.requestID)
			res, err := p.provider.Discover(ctx)

			assert.NoError(t, err)
			assert.Equal(t, tc.expectResponse, res)

			expectRequest := adapter.JSONRPCRequest{
				Headers: adapter.JSONRPCHeaders{
					Authorization: fmt.Sprintf("Bearer %s", p.testToken),
					RequestID:     tc.requestID,
				},
				RequestType: adapter.Discovery,
			}
			assert.Equal(t, expectRequest, p.requests[tc.requestID])
		})
	}
}

func TestJSONRPCProvider_Query(t *testing.T) {
	p := NewJSONRPCServerMock().WithQueryResponses(map[string]adapter.StatesResult{
		"4": {
			RequestID: "4",
			Payload: adapter.StatesResultPayload{
				Devices: []adapter.DeviceStateView{
					{
						ID: "testID1",
						Capabilities: []adapter.CapabilityStateView{
							xtestadapter.OnOffState(true, 25),
						},
					},
					{
						ID: "testID2",
						Capabilities: []adapter.CapabilityStateView{
							xtestadapter.OnOffState(true, 25),
						},
					},
					{
						ID:        "testID3",
						ErrorCode: adapter.DeviceNotFound,
					},
				},
			},
		},
	})

	testCases := []struct {
		name           string
		requestID      string
		statesRequest  adapter.StatesRequest
		expectResponse adapter.StatesResult
	}{
		{
			name:      "query",
			requestID: "4",
			expectResponse: adapter.StatesResult{
				RequestID: "4",
				Payload: adapter.StatesResultPayload{
					Devices: []adapter.DeviceStateView{
						{
							ID: "testID1",
							Capabilities: []adapter.CapabilityStateView{
								xtestadapter.OnOffState(true, 25),
							},
						},
						{
							ID: "testID2",
							Capabilities: []adapter.CapabilityStateView{
								xtestadapter.OnOffState(true, 25),
							},
						},
						{
							ID:        "testID3",
							ErrorCode: adapter.DeviceNotFound,
						},
					},
				},
			},
			statesRequest: adapter.StatesRequest{
				Devices: []adapter.StatesRequestDevice{
					{
						ID:         "testID1",
						CustomData: "mew",
					},
					{
						ID:         "testID2",
						CustomData: "mur",
					},
					{
						ID:         "testID3",
						CustomData: "mew",
					},
				},
			},
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			ctx := timestamp.ContextWithTimestamper(requestid.WithRequestID(context.Background(), tc.requestID), timestamp.NewMockTimestamper())
			res, err := p.provider.Query(ctx, tc.statesRequest)
			assert.NoError(t, err)

			assert.Equal(t, tc.expectResponse, res)

			expectRequest := adapter.JSONRPCRequest{
				Headers: adapter.JSONRPCHeaders{
					Authorization: fmt.Sprintf("Bearer %s", p.testToken),
					RequestID:     tc.requestID,
				},
				RequestType: adapter.Query,
				Payload:     tc.statesRequest,
			}
			assert.Equal(t, expectRequest, p.requests[tc.requestID])
		})
	}
}

func TestJSONRPCProvider_Action(t *testing.T) {
	p := NewJSONRPCServerMock().WithActionResponses(map[string]adapter.ActionResult{
		"6": {
			RequestID: "6",
			Payload: adapter.ActionResultPayload{
				Devices: []adapter.DeviceActionResultView{
					{
						ID: "testID1",
						Capabilities: []adapter.CapabilityActionResultView{
							xtestadapter.OnOffActionSuccessResult(25),
						},
					},
					{
						ID: "testID2",
						Capabilities: []adapter.CapabilityActionResultView{
							xtestadapter.ColorSettingActionErrorResult(model.TemperatureKCapabilityInstance, adapter.InternalError, 25),
						},
					},
				},
			},
		},
	})

	testCases := []struct {
		name           string
		requestID      string
		expectResponse adapter.ActionResult
		actionRequest  adapter.ActionRequest
	}{
		{
			name:      "action",
			requestID: "6",
			expectResponse: adapter.ActionResult{
				RequestID: "6",
				Payload: adapter.ActionResultPayload{
					Devices: []adapter.DeviceActionResultView{
						{
							ID: "testID1",
							Capabilities: []adapter.CapabilityActionResultView{
								xtestadapter.OnOffActionSuccessResult(25),
							},
						},
						{
							ID: "testID2",
							Capabilities: []adapter.CapabilityActionResultView{
								xtestadapter.ColorSettingActionErrorResult(model.SceneCapabilityInstance, adapter.InternalError, 25),
							},
						},
					},
				},
			},
			actionRequest: adapter.ActionRequest{
				Payload: adapter.ActionRequestPayload{
					Devices: []adapter.DeviceActionRequestView{
						{
							ID: "testID1",
							Capabilities: []adapter.CapabilityActionView{
								xtestadapter.OnOffAction(false),
							},
							CustomData: "mur",
						},
						{
							ID: "testID2",
							Capabilities: []adapter.CapabilityActionView{
								xtestadapter.ColorSceneAction(model.ColorSceneIDNight),
							},
							CustomData: "mur",
						},
					},
				},
			},
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			ctx := timestamp.ContextWithTimestamper(requestid.WithRequestID(context.Background(), tc.requestID), timestamp.NewMockTimestamper())
			res, err := p.provider.Action(ctx, tc.actionRequest)
			assert.NoError(t, err)

			assert.Equal(t, tc.expectResponse, res)

			expectRequest := adapter.JSONRPCRequest{
				Headers: adapter.JSONRPCHeaders{
					Authorization: fmt.Sprintf("Bearer %s", p.testToken),
					RequestID:     tc.requestID,
				},
				RequestType: adapter.Action,
				Payload:     tc.actionRequest.Payload,
			}
			assert.Equal(t, expectRequest, p.requests[tc.requestID])
		})
	}
}
