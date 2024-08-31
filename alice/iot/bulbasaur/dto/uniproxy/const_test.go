package uniproxy

import (
	"testing"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"github.com/stretchr/testify/assert"
)

func TestAnalyticDeviceTypes(t *testing.T) {
	for _, knownDeviceType := range model.KnownDeviceTypes {
		if model.DeviceType(knownDeviceType).IsSmartSpeakerOrModule() {
			continue
		}
		analyticsName := getAnalyticsType(model.DeviceType(knownDeviceType))
		assert.Truef(t, len(analyticsName) > 0, "Analytics name for device type %s should not be empty", knownDeviceType)
	}
}

func TestAnalyticNames(t *testing.T) {
	for knownRangeInstance := range model.KnownRangeInstanceNames {
		analyticsName, hasAnalyticsName := analyticsNamesMap[CapabilityUserInfoKey{
			Type:     model.RangeCapabilityType,
			Instance: string(knownRangeInstance),
		}]
		assert.Truef(t, hasAnalyticsName, "Forgot analytics name for capability type range, instance %s", knownRangeInstance)
		assert.Truef(t, len(analyticsName) > 0, "Analytics name for capability type range, instance %s should not be empty", knownRangeInstance)
	}
	for knownToggleInstance := range model.KnownToggleInstanceNames {
		analyticsName, hasAnalyticsName := analyticsNamesMap[CapabilityUserInfoKey{
			Type:     model.ToggleCapabilityType,
			Instance: string(knownToggleInstance),
		}]
		assert.Truef(t, hasAnalyticsName, "Forgot analytics name for capability type toggle, instance %s", knownToggleInstance)
		assert.Truef(t, len(analyticsName) > 0, "Analytics name for capability type toggle, instance %s should not be empty", knownToggleInstance)
	}
	for knownModeInstance := range model.KnownModeInstancesNames {
		analyticsName, hasAnalyticsName := analyticsNamesMap[CapabilityUserInfoKey{
			Type:     model.ModeCapabilityType,
			Instance: string(knownModeInstance),
		}]
		assert.Truef(t, hasAnalyticsName, "Forgot analytics name for capability type mode, instance %s", knownModeInstance)
		assert.Truef(t, len(analyticsName) > 0, "Analytics name for capability type mode, instance %s should not be empty", knownModeInstance)
	}

	analyticsName, hasAnalyticsName := analyticsNamesMap[CapabilityUserInfoKey{
		Type:     model.OnOffCapabilityType,
		Instance: string(model.OnOnOffCapabilityInstance),
	}]
	assert.True(t, hasAnalyticsName, "Forgot analytics name for capability type on_off, instance on")
	assert.True(t, len(analyticsName) > 0, "Analytics name for capability type on_off, instance on should not be empty")

	analyticsName, hasAnalyticsName = analyticsNamesMap[CapabilityUserInfoKey{
		Type:     model.ColorSettingCapabilityType,
		Instance: string(model.RgbColorCapabilityInstance),
	}]
	assert.True(t, hasAnalyticsName, "Forgot analytics name for capability type color_setting, instance rgb")
	assert.True(t, len(analyticsName) > 0, "Analytics name for capability type color_setting, instance rgb should not be empty")

	analyticsName, hasAnalyticsName = analyticsNamesMap[CapabilityUserInfoKey{
		Type:     model.ColorSettingCapabilityType,
		Instance: string(model.HsvColorCapabilityInstance),
	}]
	assert.True(t, hasAnalyticsName, "Forgot analytics name for capability type color_setting, instance hsv")
	assert.True(t, len(analyticsName) > 0, "Analytics name for capability type color_setting, instance hsv should not be empty")

	analyticsName, hasAnalyticsName = analyticsNamesMap[CapabilityUserInfoKey{
		Type:     model.ColorSettingCapabilityType,
		Instance: string(model.TemperatureKCapabilityInstance),
	}]
	assert.True(t, hasAnalyticsName, "Forgot analytics name for capability type color_setting, instance temperature_k")
	assert.True(t, len(analyticsName) > 0, "Analytics name for capability type color_setting, instance temperature_k should not be empty")
}
