package action_test

import (
	"net/http"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/action"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/provider"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func TestDevicesResult_Err(t *testing.T) {
	t.Run("TestRendererXerrorsAs", func(t *testing.T) {
		result := action.DevicesResult{
			ProviderResults: []action.ProviderDevicesResult{
				{
					SkillInfo: provider.SkillInfo{},
					DeviceResults: []action.ProviderDeviceResult{
						{
							ActionResults: map[string]adapter.CapabilityActionResultView{
								"does-not-matter": {
									Type: model.OnOffCapabilityType,
									State: adapter.CapabilityStateActionResultView{
										Instance: string(model.OnOnOffCapabilityInstance),
										ActionResult: adapter.StateActionResult{
											Status:       adapter.ERROR,
											ErrorCode:    adapter.DeviceUnreachable,
											ErrorMessage: "alarm this is bad uh oh",
										},
									},
								},
							},
						},
					},
				},
			},
		}
		err := result.Err()

		var providerErr provider.Error
		assert.True(t, xerrors.As(err, &providerErr))
		assert.EqualValues(t, http.StatusOK, providerErr.HTTPStatus())
		assert.EqualValues(t, adapter.DeviceUnreachable, providerErr.ErrorCode())
	})
}

func TestPolicyLatency(t *testing.T) {
	retryPolicy := action.RetryPolicy{
		RetryCount: 5,
		LatencyMs:  400,
		Type:       action.ExponentialRetryPolicyType,
	}
	assert.Equal(t, 0*time.Millisecond, retryPolicy.GetLatencyMs(0))
	assert.Equal(t, 400*time.Millisecond, retryPolicy.GetLatencyMs(1))
	assert.Equal(t, 800*time.Millisecond, retryPolicy.GetLatencyMs(2))
	assert.Equal(t, 1600*time.Millisecond, retryPolicy.GetLatencyMs(3))
	assert.Equal(t, 3200*time.Millisecond, retryPolicy.GetLatencyMs(4))
	assert.Equal(t, 6400*time.Millisecond, retryPolicy.GetLatencyMs(5))

	retryPolicy.Type = action.ProgressionRetryPolicyType
	assert.Equal(t, 0*time.Millisecond, retryPolicy.GetLatencyMs(0))
	assert.Equal(t, 400*time.Millisecond, retryPolicy.GetLatencyMs(1))
	assert.Equal(t, 800*time.Millisecond, retryPolicy.GetLatencyMs(2))
	assert.Equal(t, 1200*time.Millisecond, retryPolicy.GetLatencyMs(3))
	assert.Equal(t, 1600*time.Millisecond, retryPolicy.GetLatencyMs(4))
	assert.Equal(t, 2000*time.Millisecond, retryPolicy.GetLatencyMs(5))

	retryPolicy.Type = action.UniformRetryPolicyType
	assert.Equal(t, 0*time.Millisecond, retryPolicy.GetLatencyMs(0))
	assert.Equal(t, 400*time.Millisecond, retryPolicy.GetLatencyMs(1))
	assert.Equal(t, 400*time.Millisecond, retryPolicy.GetLatencyMs(2))
	assert.Equal(t, 400*time.Millisecond, retryPolicy.GetLatencyMs(3))
	assert.Equal(t, 400*time.Millisecond, retryPolicy.GetLatencyMs(4))
	assert.Equal(t, 400*time.Millisecond, retryPolicy.GetLatencyMs(5))

	retryPolicy.Type = action.UniformParallelRetryPolicyType
	assert.Equal(t, 0*time.Millisecond, retryPolicy.GetLatencyMs(0))
	assert.Equal(t, 400*time.Millisecond, retryPolicy.GetLatencyMs(1))
	assert.Equal(t, 400*time.Millisecond, retryPolicy.GetLatencyMs(2))
	assert.Equal(t, 400*time.Millisecond, retryPolicy.GetLatencyMs(3))
	assert.Equal(t, 400*time.Millisecond, retryPolicy.GetLatencyMs(4))
	assert.Equal(t, 400*time.Millisecond, retryPolicy.GetLatencyMs(5))
}

func TestProviderDeviceResult_hasStateUpdates(t *testing.T) {
	testCases := []struct {
		name                 string
		deviceResult         action.ProviderDeviceResult
		expectedStateUpdates bool
	}{
		{
			name: "online_with_capabilities_leads_to_state_update",
			deviceResult: action.ProviderDeviceResult{
				Status:              model.OnlineDeviceStatus,
				UpdatedCapabilities: []model.ICapability{model.MakeCapabilityByType(model.OnOffCapabilityType)},
			},
			expectedStateUpdates: true,
		},
		{
			name: "online_with_no_capabilities_leads_to_skip_update",
			deviceResult: action.ProviderDeviceResult{
				Status: model.OnlineDeviceStatus,
			},
			expectedStateUpdates: false,
		},
		{
			name: "not_online_leads_to_state_update",
			deviceResult: action.ProviderDeviceResult{
				Status: model.OfflineDeviceStatus,
			},
			expectedStateUpdates: true,
		},
	}
	for _, testCase := range testCases {
		t.Run(testCase.name, func(t *testing.T) {
			assert.Equal(t, testCase.expectedStateUpdates, testCase.deviceResult.HasStateUpdates())
		})
	}
}
