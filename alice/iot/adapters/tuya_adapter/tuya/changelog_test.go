package tuya

import (
	"context"
	"testing"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	btuya "a.yandex-team.ru/alice/iot/bulbasaur/model/tuya"
	"a.yandex-team.ru/library/go/ptr"
	"github.com/stretchr/testify/assert"
)

func TestComputeFirmwareUpgrade(t *testing.T) {
	type testCase struct {
		customData          btuya.CustomData
		swVersion           *string
		expected            FirmwareChangelogID
		overrideExperiments experiments.MockManager
		name                string
	}

	testCases := []testCase{
		{
			customData: btuya.CustomData{
				ProductID: ptr.String(string(LampE14MPYandexProductID)),
			},
			swVersion: ptr.String("2.9.17"),
			expected:  Lamp3To5AndScenesFirmwareUpgradeID,
			name:      "e14 2.9.17",
		},
		{
			customData: btuya.CustomData{
				ProductID: ptr.String(string(LampE14MPYandexProductID)),
			},
			swVersion: ptr.String("2.9.16"),
			expected:  Lamp3To5AndScenesFirmwareUpgradeID,
			name:      "e14 2.9.16",
		},
		{
			customData: btuya.CustomData{
				ProductID: ptr.String(string(LampE14MPYandexProductID)),
			},
			swVersion: ptr.String("2.9.18"),
			name:      "e14 2.9.18",
		},
		{
			customData: btuya.CustomData{
				ProductID: ptr.String(string(LampGU10MPYandexProductID)),
			},
			swVersion: ptr.String("1.0.6"),
			expected:  Lamp3To5AndScenesFirmwareUpgradeID,
			name:      "gu10 1.0.6",
		},
		{
			customData: btuya.CustomData{
				ProductID: ptr.String(string(LampGU10MPYandexProductID)),
			},
			swVersion: ptr.String("1.0.5"),
			expected:  Lamp3To5AndScenesFirmwareUpgradeID,
			name:      "gu10 1.0.5",
		},
		{
			customData: btuya.CustomData{
				ProductID: ptr.String(string(LampGU10MPYandexProductID)),
			},
			swVersion:           ptr.String("1.0.5"),
			overrideExperiments: experiments.MockManager{},
			name:                "gu10 1.0.5",
		},
		{
			customData: btuya.CustomData{
				ProductID: ptr.String(string(LampGU10MPYandexProductID)),
			},
			swVersion: ptr.String("1.0.7"),
			name:      "gu10 1.0.7",
		},
		{
			customData: btuya.CustomData{
				ProductID: ptr.String(string(LampE27Lemon2YandexProductID)),
			},
			swVersion: ptr.String("1.0.18"),
			expected:  Lamp3To5AndScenesFirmwareUpgradeID,
			name:      "e27 lamp2 1.0.18",
		},
		{
			customData: btuya.CustomData{
				ProductID: ptr.String(string(LampE27Lemon2YandexProductID)),
			},
			swVersion:           ptr.String("1.0.18"),
			overrideExperiments: experiments.MockManager{},
			name:                "e27 lamp2 1.0.18 without experiment",
		},
		{
			customData: btuya.CustomData{
				ProductID: ptr.String(string(LampE27Lemon2YandexProductID)),
			},
			swVersion: ptr.String("1.0.20"),
			name:      "e27 lamp2 1.0.20",
		},
		{
			customData: btuya.CustomData{
				ProductID: ptr.String(string(LampE27Lemon3YandexProductID)),
			},
			swVersion: ptr.String("1.0.7"),
			expected:  Lamp3To5AndScenesFirmwareUpgradeID,
			name:      "e27 lamp3 1.0.7",
		},
		{
			customData: btuya.CustomData{
				ProductID: ptr.String(string(LampE27Lemon3YandexProductID)),
			},
			swVersion: ptr.String("1.0.8"),
			name:      "e27 lamp3 1.0.8",
		},
		{
			customData: btuya.CustomData{
				ProductID: ptr.String("scam"),
			},
			swVersion: ptr.String("1.0.100500"),
			name:      "scam 100500",
		},
	}
	for _, tc := range testCases {
		expManager := experiments.MockManager{
			experiments.ShowAvailableChangelog:          true,
			experiments.ShowAvailableChangelogE27Lemon2: true,
		}
		if tc.overrideExperiments != nil {
			expManager = tc.overrideExperiments
		}
		expCtx := experiments.ContextWithManager(context.Background(), expManager)
		upgradeID := KnownFirmwareChangelogs.GetAvailableChangelog(expCtx, tc.customData, tc.swVersion)
		if tc.expected == "" {
			assert.Nil(t, upgradeID, tc.name)
			continue
		}
		assert.NotNil(t, upgradeID, tc.name)
		if upgradeID != nil {
			assert.Equal(t, tc.expected, *upgradeID, tc.name)
		}
	}

}
