package model_test

import (
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	xtestdata "a.yandex-team.ru/alice/iot/bulbasaur/xtest/data"
)

func TestDevice_GetUpdatedState(t *testing.T) {
	originalStates := model.Device{
		Capabilities: []model.ICapability{
			xtestdata.OnOffCapabilityWithState(false, 0),
			xtestdata.ToggleCapabilityWithState(model.BacklightToggleCapabilityInstance, false, 0),
		},
		Properties: []model.IProperty{
			xtestdata.FloatPropertyWithState(model.BatteryLevelPropertyInstance, 0, 0),
			xtestdata.FloatPropertyWithState(model.WaterLevelPropertyInstance, 0, 0),
			xtestdata.EventPropertyWithState(model.OpenPropertyInstance, model.ClosedEvent, 0),
		},
	}
	newStates := model.Device{
		Capabilities: []model.ICapability{
			xtestdata.OnOffCapabilityWithState(true, 1905),
			xtestdata.OnOffCapabilityWithState(false, 1917),
			xtestdata.ToggleCapabilityWithState(model.BacklightToggleCapabilityInstance, true, 1917),
			xtestdata.OnOffCapabilityWithState(false, 2086),
			xtestdata.ToggleCapabilityWithState(model.BacklightToggleCapabilityInstance, false, 2100),
		},
		Properties: []model.IProperty{
			xtestdata.FloatPropertyWithState(model.BatteryLevelPropertyInstance, 5, 1905),
			xtestdata.FloatPropertyWithState(model.BatteryLevelPropertyInstance, 17, 1917),
			xtestdata.FloatPropertyWithState(model.WaterLevelPropertyInstance, 17, 1917),
			xtestdata.FloatPropertyWithState(model.BatteryLevelPropertyInstance, 85, 2086),
			xtestdata.FloatPropertyWithState(model.WaterLevelPropertyInstance, 0, 2100),

			xtestdata.EventPropertyWithState(model.OpenPropertyInstance, model.OpenedEvent, 1905),
			xtestdata.EventPropertyWithState(model.OpenPropertyInstance, model.ClosedEvent, 1917),
			xtestdata.EventPropertyWithState(model.OpenPropertyInstance, model.OpenedEvent, 2086),
		},
	}
	actualUpdatedDevice, actualUpdatesSnapshot, actualChangedStates := originalStates.GetUpdatedState(newStates)
	expectedUpdatedDevice := model.Device{
		Capabilities: []model.ICapability{
			xtestdata.OnOffCapabilityWithState(false, 2086),
			xtestdata.ToggleCapabilityWithState(model.BacklightToggleCapabilityInstance, false, 2100),
		},
		Properties: []model.IProperty{
			xtestdata.FloatPropertyWithState(model.BatteryLevelPropertyInstance, 85, 2086).WithStateChangedAt(2086),
			xtestdata.FloatPropertyWithState(model.WaterLevelPropertyInstance, 0, 2100).WithStateChangedAt(2100),
			xtestdata.EventPropertyWithState(model.OpenPropertyInstance, model.OpenedEvent, 2086).WithStateChangedAt(2086),
		},
	}
	assert.ElementsMatch(t, expectedUpdatedDevice.Capabilities, actualUpdatedDevice.Capabilities)
	assert.ElementsMatch(t, expectedUpdatedDevice.Properties, actualUpdatedDevice.Properties)

	// note that in updatesSnapshot after GetUpdatedState all properties/capabilities are always sorted by lastUpdated key
	expectedUpdatesSnapshot := model.DeviceUpdatesSnapshot{
		UpdatedCapabilitiesMap: map[string]model.Capabilities{
			model.CapabilityKey(model.OnOffCapabilityType, model.OnOnOffCapabilityInstance.String()): {
				xtestdata.OnOffCapabilityWithState(true, 1905),
				xtestdata.OnOffCapabilityWithState(false, 1917),
				xtestdata.OnOffCapabilityWithState(false, 2086),
			},
			model.CapabilityKey(model.ToggleCapabilityType, model.BacklightToggleCapabilityInstance.String()): {
				xtestdata.ToggleCapabilityWithState(model.BacklightToggleCapabilityInstance, true, 1917),
				xtestdata.ToggleCapabilityWithState(model.BacklightToggleCapabilityInstance, false, 2100),
			},
		},
		UpdatedPropertiesMap: map[string]model.Properties{
			model.PropertyKey(model.FloatPropertyType, model.BatteryLevelPropertyInstance.String()): {
				xtestdata.FloatPropertyWithState(model.BatteryLevelPropertyInstance, 5, 1905).WithStateChangedAt(1905),
				xtestdata.FloatPropertyWithState(model.BatteryLevelPropertyInstance, 17, 1917).WithStateChangedAt(1917),
				xtestdata.FloatPropertyWithState(model.BatteryLevelPropertyInstance, 85, 2086).WithStateChangedAt(2086),
			},
			model.PropertyKey(model.FloatPropertyType, model.WaterLevelPropertyInstance.String()): {
				xtestdata.FloatPropertyWithState(model.WaterLevelPropertyInstance, 17, 1917).WithStateChangedAt(1917),
				xtestdata.FloatPropertyWithState(model.WaterLevelPropertyInstance, 0, 2100).WithStateChangedAt(2100),
			},
			model.PropertyKey(model.EventPropertyType, model.OpenPropertyInstance.String()): {
				xtestdata.EventPropertyWithState(model.OpenPropertyInstance, model.OpenedEvent, 1905).WithStateChangedAt(1905),
				xtestdata.EventPropertyWithState(model.OpenPropertyInstance, model.ClosedEvent, 1917).WithStateChangedAt(1917),
				xtestdata.EventPropertyWithState(model.OpenPropertyInstance, model.OpenedEvent, 2086).WithStateChangedAt(2086),
			},
		},
	}
	assert.Equal(t, expectedUpdatesSnapshot, actualUpdatesSnapshot)

	expectedChangedStates := model.PropertiesChangedStates{
		model.PropertyChangedStates{
			Previous: xtestdata.FloatPropertyWithState(model.BatteryLevelPropertyInstance, 0, 0).State(),
			Current:  xtestdata.FloatPropertyWithState(model.BatteryLevelPropertyInstance, 85, 2086).State(),
		},
		model.PropertyChangedStates{
			Previous: xtestdata.EventPropertyWithState(model.OpenPropertyInstance, model.ClosedEvent, 0).State(),
			Current:  xtestdata.EventPropertyWithState(model.OpenPropertyInstance, model.OpenedEvent, 2086).State(),
		},
		model.PropertyChangedStates{
			Previous: xtestdata.FloatPropertyWithState(model.WaterLevelPropertyInstance, 0, 0).State(),
			Current:  xtestdata.FloatPropertyWithState(model.WaterLevelPropertyInstance, 0, 2100).State(),
		},
	}
	assert.ElementsMatch(t, expectedChangedStates, actualChangedStates)
}
