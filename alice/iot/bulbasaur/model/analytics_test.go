package model

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestAnalyticDeviceTypes(t *testing.T) {
	for _, knownDeviceType := range KnownDeviceTypes {
		if DeviceType(knownDeviceType).IsSmartSpeakerOrModule() {
			continue
		}
		analyticsName := analyticsDeviceType(DeviceType(knownDeviceType))
		assert.Truef(t, len(analyticsName) > 0, "analytics name for device type %s should not be empty", knownDeviceType)
	}
}

func TestAnalyticNames(t *testing.T) {
	for knownRangeInstance := range KnownRangeInstanceNames {
		analyticsName, hasAnalyticsName := analyticsCapabilityNamesMap[analyticsCapabilityKey{
			Type:     RangeCapabilityType,
			Instance: string(knownRangeInstance),
		}]
		assert.Truef(t, hasAnalyticsName, "forgot analytics name for capability type range, instance %s", knownRangeInstance)
		assert.Truef(t, len(analyticsName) > 0, "analytics name for capability type range, instance %s should not be empty", knownRangeInstance)
	}
	for knownToggleInstance := range KnownToggleInstanceNames {
		analyticsName, hasAnalyticsName := analyticsCapabilityNamesMap[analyticsCapabilityKey{
			Type:     ToggleCapabilityType,
			Instance: string(knownToggleInstance),
		}]
		assert.Truef(t, hasAnalyticsName, "forgot analytics name for capability type toggle, instance %s", knownToggleInstance)
		assert.Truef(t, len(analyticsName) > 0, "analytics name for capability type toggle, instance %s should not be empty", knownToggleInstance)
	}
	for knownModeInstance := range KnownModeInstancesNames {
		analyticsName, hasAnalyticsName := analyticsCapabilityNamesMap[analyticsCapabilityKey{
			Type:     ModeCapabilityType,
			Instance: string(knownModeInstance),
		}]
		assert.Truef(t, hasAnalyticsName, "forgot analytics name for capability type mode, instance %s", knownModeInstance)
		assert.Truef(t, len(analyticsName) > 0, "analytics name for capability type mode, instance %s should not be empty", knownModeInstance)
	}

	analyticsName, hasAnalyticsName := analyticsCapabilityNamesMap[analyticsCapabilityKey{
		Type:     OnOffCapabilityType,
		Instance: string(OnOnOffCapabilityInstance),
	}]
	assert.True(t, hasAnalyticsName, "forgot analytics name for capability type on_off, instance on")
	assert.True(t, len(analyticsName) > 0, "analytics name for capability type on_off, instance on should not be empty")

	analyticsName, hasAnalyticsName = analyticsCapabilityNamesMap[analyticsCapabilityKey{
		Type:     ColorSettingCapabilityType,
		Instance: string(RgbColorCapabilityInstance),
	}]
	assert.True(t, hasAnalyticsName, "forgot analytics name for capability type color_setting, instance rgb")
	assert.True(t, len(analyticsName) > 0, "analytics name for capability type color_setting, instance rgb should not be empty")

	analyticsName, hasAnalyticsName = analyticsCapabilityNamesMap[analyticsCapabilityKey{
		Type:     ColorSettingCapabilityType,
		Instance: string(HsvColorCapabilityInstance),
	}]
	assert.True(t, hasAnalyticsName, "forgot analytics name for capability type color_setting, instance hsv")
	assert.True(t, len(analyticsName) > 0, "analytics name for capability type color_setting, instance hsv should not be empty")

	analyticsName, hasAnalyticsName = analyticsCapabilityNamesMap[analyticsCapabilityKey{
		Type:     ColorSettingCapabilityType,
		Instance: string(TemperatureKCapabilityInstance),
	}]
	assert.True(t, hasAnalyticsName, "forgot analytics name for capability type color_setting, instance temperature_k")
	assert.True(t, len(analyticsName) > 0, "analytics name for capability type color_setting, instance temperature_k should not be empty")
}
