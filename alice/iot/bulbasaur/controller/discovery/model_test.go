package discovery

import (
	"testing"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"github.com/stretchr/testify/assert"
)

func Test_UserDiffInfo(t *testing.T) {
	// Prepare capabilities and properties
	rangeCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	rangeCapability.SetRetrievable(true)
	rangeCapability.SetParameters(
		model.RangeCapabilityParameters{
			Instance:     model.BrightnessRangeInstance,
			Unit:         model.UnitPercent,
			RandomAccess: true,
			Looped:       false,
			Range: &model.Range{
				Min:       1,
				Max:       100,
				Precision: 1,
			},
		})

	rangeCapability.SetState(model.RangeCapabilityState{
		Instance: model.BrightnessRangeInstance,
		Value:    50,
	})

	onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOffCapability.SetRetrievable(true)
	onOffCapability.SetState(model.OnOffCapabilityState{
		Instance: model.OnOnOffCapabilityInstance,
		Value:    true,
	})

	backlight := model.MakeCapabilityByType(model.ToggleCapabilityType)
	backlight.SetParameters(model.ToggleCapabilityParameters{Instance: model.BacklightToggleCapabilityInstance})
	backlight.SetRetrievable(true)

	humidity := model.MakePropertyByType(model.FloatPropertyType)
	humidity.SetParameters(model.FloatPropertyParameters{
		Instance: model.HumidityPropertyInstance,
		Unit:     model.UnitPercent,
	})
	humidity.SetRetrievable(true)

	// Prepare devices
	beforeLamp := model.Device{
		ID:           "Internal Lamp",
		ExternalID:   "External Lamp",
		Name:         "Lamper",
		OriginalType: model.LightDeviceType,
		Type:         model.LightDeviceType,
		Capabilities: []model.ICapability{rangeCapability, onOffCapability},
		Properties:   model.Properties{},
	}

	// Provider added toggle: backlight and humidity to this lamp
	// Provider also made range unretrievable
	afterRange := rangeCapability.Clone()
	afterRange.SetRetrievable(false)

	afterLamp := model.Device{
		ExternalID:   "External Lamp",
		Name:         "Lamper",
		OriginalType: model.LightDeviceType,
		Type:         model.LightDeviceType,
		Capabilities: []model.ICapability{afterRange, onOffCapability, backlight},
		Properties:   model.Properties{humidity},
	}

	// humidifier remained the same
	humidifier := model.Device{
		ID:           "Internal Humid",
		ExternalID:   "External Humid",
		Name:         "Humidifier",
		OriginalType: model.HumidifierDeviceType,
		Type:         model.HumidifierDeviceType,
		Capabilities: []model.ICapability{onOffCapability},
		Properties:   model.Properties{humidity},
	}

	// User acquiring socket for the first time
	socket := model.Device{
		ExternalID:   "External Socket",
		Name:         "Socket",
		OriginalType: model.SocketDeviceType,
		Type:         model.SocketDeviceType,
		Capabilities: []model.ICapability{onOffCapability},
		Properties:   model.Properties{humidity},
	}

	beforeDevices := model.Devices{beforeLamp, humidifier}
	afterDevices := model.Devices{afterLamp, humidifier, socket}

	expected := UserDiffInfo{
		Devices: []DeviceDiffInfo{
			{
				ID:           "Internal Lamp",
				ExternalID:   "External Lamp",
				Name:         "Lamper",
				OriginalType: model.LightDeviceType,
				Status:       UpdatedDiffStatus,
				PropertiesDiffInfo: []PropertyDiffInfo{
					{
						Retrievable: true,
						Parameters: model.FloatPropertyParameters{
							Instance: model.HumidityPropertyInstance,
							Unit:     model.UnitPercent,
						},
						Type:   model.FloatPropertyType,
						Status: NewDiffStatus,
					},
				},
				CapabilitiesDiffInfo: []CapabilityDiffInfo{
					{
						Type:        model.RangeCapabilityType,
						Retrievable: false,
						Parameters: model.RangeCapabilityParameters{
							Instance:     model.BrightnessRangeInstance,
							Unit:         model.UnitPercent,
							RandomAccess: true,
							Looped:       false,
							Range: &model.Range{
								Min:       1,
								Max:       100,
								Precision: 1,
							},
						},

						Status: UpdatedDiffStatus,
					},
					{
						Type:        model.ToggleCapabilityType,
						Retrievable: true,
						Parameters: model.ToggleCapabilityParameters{
							Instance: model.BacklightToggleCapabilityInstance,
						},
						Status: NewDiffStatus,
					},
				},
			},
			{
				ID:           "Internal Socket",
				ExternalID:   "External Socket",
				Name:         "Socket",
				OriginalType: model.SocketDeviceType,
				Status:       NewDiffStatus,
				CapabilitiesDiffInfo: []CapabilityDiffInfo{
					{
						Type:        model.OnOffCapabilityType,
						Retrievable: true,
						Parameters:  model.OnOffCapabilityParameters{Split: false},
						Status:      NewDiffStatus,
					},
				},
				PropertiesDiffInfo: []PropertyDiffInfo{
					{
						Type:        model.FloatPropertyType,
						Retrievable: true,
						Parameters: model.FloatPropertyParameters{
							Instance: model.HumidityPropertyInstance,
							Unit:     model.UnitPercent,
						},
						Status: NewDiffStatus,
					},
				},
			},
		},
	}

	storeResult := []model.DeviceStoreResult{
		{
			Device: model.Device{
				ID:         "Internal Lamp",
				ExternalID: "External Lamp",
				Name:       "Lamper",
				Type:       model.LightDeviceType,
			},
			Result: model.StoreResultUpdated,
		},
		{
			Device: model.Device{
				ID:         "Internal Socket",
				ExternalID: "External Socket",
				Name:       "Socket",
				Type:       model.SocketDeviceType,
			},
			Result: model.StoreResultNew,
		},
		{
			Device: model.Device{
				ID:         "Internal Humid",
				ExternalID: "External Humid",
				Name:       "Humidifier",
				Type:       model.HumidifierDeviceType,
			},
			Result: model.StoreResultUpdated,
		},
	}

	var actual UserDiffInfo
	actual.FromOldAndDiscoveredDevices(beforeDevices, afterDevices, storeResult)
	assert.Equal(t, expected, actual)
}
