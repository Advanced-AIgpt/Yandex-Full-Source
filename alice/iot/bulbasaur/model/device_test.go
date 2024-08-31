package model_test

import (
	"context"
	"sort"
	"testing"

	"github.com/stretchr/testify/assert"
	"golang.org/x/exp/slices"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/quasar"
	yandexiocd "a.yandex-team.ru/alice/iot/bulbasaur/model/yandexio"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/alice/megamind/protos/common"
	devicepb "a.yandex-team.ru/alice/protos/data/device"
	"a.yandex-team.ru/library/go/ptr"
)

func TestRoomIsEmpty(t *testing.T) {
	// Case 1: empty room struct
	emptyRoom := model.Room{}
	assert.True(t, emptyRoom.IsEmpty())

	// Case 2: non empty room (named)
	namedRoom := model.Room{Name: "My room"}
	assert.True(t, namedRoom.IsEmpty())

	// Case 3: filled room
	filledRoom := model.Room{ID: "room-id", Name: "My room name", Devices: []string{"device-id-1", "device-id-2"}}
	assert.False(t, filledRoom.IsEmpty())
}

func TestGroupIsEmpty(t *testing.T) {
	// Case 1: empty group struct
	emptyGroup := model.Group{}
	assert.True(t, emptyGroup.IsEmpty())

	// Case 2: non empty group (named)
	namedGroup := model.Group{Name: "My group"}
	assert.True(t, namedGroup.IsEmpty())

	// Case 3: filled group
	filledGroup := model.Group{ID: "group-id", Name: "My group name", Devices: []string{"device-id-1", "device-id-2"}}
	assert.False(t, filledGroup.IsEmpty())
}

func TestGetCapabilityByTypeAndInstance(t *testing.T) {
	brightnessCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	brightnessCapability.SetState(model.RangeCapabilityState{
		Instance: model.BrightnessRangeInstance,
		Value:    100.0,
	})
	brightnessCapability.SetParameters(model.RangeCapabilityParameters{
		Instance: model.BrightnessRangeInstance,
		Unit:     "percent",
	})

	volumeCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	volumeCapability.SetParameters(model.RangeCapabilityParameters{
		Instance: model.VolumeRangeInstance,
		Unit:     "volume",
	})

	device := model.Device{Capabilities: []model.ICapability{brightnessCapability, volumeCapability}}

	// Validate selection
	capability, exists := device.GetCapabilityByTypeAndInstance(model.RangeCapabilityType, string(model.BrightnessRangeInstance))
	assert.Equal(t, brightnessCapability, capability)
	assert.Equal(t, float64(100.0), capability.State().(model.RangeCapabilityState).Value)
	assert.True(t, exists)
	// Validate pointer changing selected capability state and checking device capabilities after that
	capability.SetState(model.RangeCapabilityState{
		Instance: model.BrightnessRangeInstance,
		Value:    55.0,
	})
	capability, _ = device.GetCapabilityByTypeAndInstance(model.RangeCapabilityType, string(model.BrightnessRangeInstance))
	assert.Equal(t, float64(55.0), capability.State().(model.RangeCapabilityState).Value)

	// Validate Range Volume selection
	capability, exists = device.GetCapabilityByTypeAndInstance(model.RangeCapabilityType, string(model.VolumeRangeInstance))
	assert.Equal(t, volumeCapability, capability)
	assert.True(t, exists)
}

func TestGetCapabilitiesByType(t *testing.T) {
	brightnessCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	brightnessCapability.SetParameters(model.RangeCapabilityParameters{
		Instance: model.BrightnessRangeInstance,
		Unit:     "percent",
	})

	volumeCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	volumeCapability.SetParameters(model.RangeCapabilityParameters{
		Instance: model.VolumeRangeInstance,
		Unit:     "volume",
	})

	colorSettingCapability := model.MakeCapabilityByType(model.ColorSettingCapabilityType)
	colorSettingCapability.SetParameters(model.ColorSettingCapabilityParameters{
		ColorSceneParameters: &model.ColorSceneParameters{
			Scenes: model.ColorScenes{
				{
					ID: model.ColorSceneIDParty,
				},
			},
		},
	})

	capabilities := []model.ICapability{brightnessCapability, volumeCapability, colorSettingCapability}
	device := model.Device{Capabilities: capabilities}

	assert.Equal(t, []model.ICapability{brightnessCapability, volumeCapability}, device.GetCapabilitiesByType(model.RangeCapabilityType))

	expectedCapabilities := []model.ICapability{colorSettingCapability}
	assert.Equal(t, expectedCapabilities, device.GetCapabilitiesByType(model.ColorSettingCapabilityType))

}

func TestDeviceCheckConfiguration(t *testing.T) {
	configuredDevice := model.Device{
		Name: "тест",
		Room: &model.Room{Name: "кухня"},
	}

	// Valid device cases
	validNames := []string{"однослово", "два слова", "целых три слова"}
	for _, name := range validNames {
		testDevice := configuredDevice
		testDevice.Name = name
		testDevice.Room.Name = name
		assert.NoError(t, testDevice.AssertSetup(), testDevice.Name)
	}

	// Invalid names cases
	invalidNames := []string{"словосцифрой1", "тутанглийское slovo", "english", "eng lang", "комбоlangслово"}
	for _, name := range invalidNames {
		testDevice := configuredDevice

		// invalid device name
		testDevice.Name = name
		assert.Equal(t, &model.NameCharError{}, testDevice.AssertSetup(), testDevice.Name)

		testDevice = configuredDevice
		testDevice.Room.Name = name
		assert.Equal(t, &model.NameCharError{}, testDevice.AssertSetup(), testDevice.Name)
	}

	// Empty room
	configuredDevice.Room = nil
	assert.Equal(t, &model.DeviceWithoutRoomError{}, configuredDevice.AssertSetup())
}

func TestDeviceUpdateCapabilityStates(t *testing.T) {
	// Creating test device
	brightnessCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	brightnessCapability.SetRetrievable(true)
	brightnessCapability.SetState(model.RangeCapabilityState{
		Instance: model.BrightnessRangeInstance,
		Value:    100.0,
	})
	brightnessCapability.SetParameters(model.RangeCapabilityParameters{
		Instance: model.BrightnessRangeInstance,
		Unit:     "percent",
		Range: &model.Range{
			Min:       1,
			Max:       100,
			Precision: 1,
		},
	})

	volumeCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	volumeCapability.SetState(model.RangeCapabilityState{
		Instance: model.BrightnessRangeInstance,
		Value:    100.0,
	})
	volumeCapability.SetParameters(model.RangeCapabilityParameters{
		Instance: model.VolumeRangeInstance,
		Unit:     "percent",
		Range: &model.Range{
			Min:       1,
			Max:       100,
			Precision: 1,
		},
	})

	onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOffCapability.SetRetrievable(true)
	onOffCapability.SetState(model.OnOffCapabilityState{
		Instance: "on",
		Value:    false,
	})

	colorSettingCapability := model.MakeCapabilityByType(model.ColorSettingCapabilityType)
	colorSettingCapability.SetRetrievable(true)
	colorSettingCapability.SetParameters(model.ColorSettingCapabilityParameters{
		ColorModel: model.CM(model.HsvModelType),
	})

	toggleCapability := model.MakeCapabilityByType(model.ToggleCapabilityType)
	toggleCapability.SetRetrievable(false)
	toggleCapability.SetParameters(model.ToggleCapabilityParameters{
		Instance: model.MuteToggleCapabilityInstance,
	})

	originalDevice := model.Device{
		ID:           "Lamp",
		Name:         "Lamper",
		Type:         model.LightDeviceType,
		Capabilities: []model.ICapability{colorSettingCapability, brightnessCapability, onOffCapability, volumeCapability, toggleCapability},
	}

	// Creating device with new states received from provider
	newBrightnessCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	newBrightnessCapability.SetState(model.RangeCapabilityState{
		Instance: model.BrightnessRangeInstance,
		Value:    55.0,
	})

	newVolumeCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	newVolumeCapability.SetState(model.RangeCapabilityState{
		Instance: model.VolumeRangeInstance,
		Value:    15.0,
	})

	newOnOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
	newOnOffCapability.SetState(model.OnOffCapabilityState{
		Instance: "on",
		Value:    true,
	})

	newModeCapability := model.MakeCapabilityByType(model.ModeCapabilityType)
	newModeCapability.SetState(model.ModeCapabilityState{
		Instance: model.ThermostatModeInstance,
		Value:    "what",
	})

	newToggleCapability := model.MakeCapabilityByType(model.ToggleCapabilityType)
	newToggleCapability.SetState(model.ToggleCapabilityState{
		Instance: model.MuteToggleCapabilityInstance,
		Value:    true,
	})

	// Capabilities received from provider. Uses for updating originalDevice
	// There is no colorSettingCapability so we are expecting that colorSettingCapability from originalDevice will not be updated
	// Meanwhile there is ModeCapability within new states.
	donorCapabilities := []model.ICapability{newModeCapability, newBrightnessCapability, newOnOffCapability, newVolumeCapability, newToggleCapability}

	// Creating new device with new capabilities (will be used for update original device)
	donorDevice := originalDevice
	donorDevice.Capabilities = donorCapabilities

	originalDevice.UpdateState(donorDevice.Capabilities, nil)

	// Manually creating new expected device which we should get after the update originalDevice using PopulateCapabilitiesStates
	expectedBrightnessCapability := brightnessCapability
	expectedBrightnessCapability.SetState(newBrightnessCapability.State())

	expectedOnOffCapability := onOffCapability
	expectedOnOffCapability.SetState(newOnOffCapability.State())

	expectedVolumeCapability := volumeCapability
	expectedVolumeCapability.SetState(newVolumeCapability.State())

	expectedToggleCapability := toggleCapability // no changes expected here cause retrievable:false

	expectedCapabilities := []model.ICapability{colorSettingCapability, expectedBrightnessCapability, expectedOnOffCapability, expectedVolumeCapability, expectedToggleCapability}

	expectedDevice := originalDevice
	expectedDevice.Capabilities = expectedCapabilities

	assert.Equal(t, expectedDevice, originalDevice)
}

func TestDevice_UpdateCapabilityStates_withNilState(t *testing.T) {
	// Case: try to update from device that has capability with nil state
	onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOffCapability.SetRetrievable(true)
	onOffCapability.SetState(model.OnOffCapabilityState{
		Instance: "on",
		Value:    false,
	})

	colorSettingCapability := model.MakeCapabilityByType(model.ColorSettingCapabilityType)
	colorSettingCapability.SetRetrievable(true)
	colorSettingCapability.SetParameters(model.ColorSettingCapabilityParameters{
		ColorModel: model.CM(model.HsvModelType),
	})
	colorSettingCapability.SetState(model.ColorSettingCapabilityState{
		Instance: model.TemperatureKCapabilityInstance,
		Value:    model.TemperatureK(5400),
	})

	originalDevice := model.Device{
		ID:           "Lamp",
		Name:         "Lamper",
		Type:         model.LightDeviceType,
		Capabilities: []model.ICapability{colorSettingCapability, onOffCapability},
	}

	newOnOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
	newOnOffCapability.SetState(model.OnOffCapabilityState{
		Instance: "on",
		Value:    true,
	})

	newColorSettingCapability := model.MakeCapabilityByType(model.ColorSettingCapabilityType)

	donorDevice := originalDevice
	donorDevice.Capabilities = []model.ICapability{newColorSettingCapability, newOnOffCapability.WithLastUpdated(1)}

	expectedOnOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
	expectedOnOffCapability.SetRetrievable(true)
	expectedOnOffCapability.SetState(model.OnOffCapabilityState{
		Instance: "on",
		Value:    true,
	})
	expectedOnOffCapability.SetLastUpdated(1)

	expectedUpdatedDevice := originalDevice
	expectedUpdatedDevice.Capabilities = []model.ICapability{colorSettingCapability, expectedOnOffCapability}
	originalDevice.UpdateState(donorDevice.Capabilities, nil)
	assert.Equal(t, expectedUpdatedDevice, originalDevice)
}

func TestDevice_UpdateCapabilityStates_withRangeRelativeNotNil(t *testing.T) {
	// Case: try to update range capability state with non-nil relative
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
		Instance: "on",
		Value:    true,
	})

	originalDevice := model.Device{
		ID:           "Lamp",
		Name:         "Lamper",
		Type:         model.LightDeviceType,
		Capabilities: []model.ICapability{rangeCapability, onOffCapability},
	}

	rangeCapability.SetState(model.RangeCapabilityState{
		Instance: model.BrightnessRangeInstance,
		Relative: tools.AOB(true),
		Value:    10,
	})

	donorDevice := originalDevice.Clone()
	donorDevice.Capabilities = []model.ICapability{rangeCapability, onOffCapability}

	expectedDevice := originalDevice.Clone()

	originalDevice.UpdateState(donorDevice.Capabilities, nil)

	assert.Equal(t, expectedDevice, originalDevice)
}

func TestDevice_UpdateCapabilityStates_withOnOffRelativeNotNil(t *testing.T) {
	onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType).
		WithRetrievable(true).
		WithState(model.OnOffCapabilityState{
			Instance: model.OnOnOffCapabilityInstance,
			Value:    true,
		})

	originalDevice := model.Device{
		ID:           "Lamp",
		Name:         "Lamper",
		Type:         model.LightDeviceType,
		Capabilities: []model.ICapability{onOffCapability},
	}

	copiedCapability := onOffCapability.Clone()
	copiedCapability.SetState(model.OnOffCapabilityState{
		Instance: model.OnOnOffCapabilityInstance,
		Value:    true,
		Relative: ptr.Bool(true),
	})

	donorDevice := originalDevice.Clone()
	donorDevice.Capabilities = []model.ICapability{onOffCapability}
	expectedDevice := originalDevice.Clone()
	expectedDevice.Capabilities[0].SetState(model.OnOffCapabilityState{
		Instance: model.OnOnOffCapabilityInstance,
		Value:    true,
		Relative: nil,
	})

	originalDevice.UpdateState(donorDevice.Capabilities, nil)
	assert.Equal(t, expectedDevice, originalDevice)
}

type propertyStateWithLastUpdated struct {
	state       model.IPropertyState
	lastUpdated timestamp.PastTimestamp
}

func TestUpdateDeviceProperty(t *testing.T) {
	testCases := []struct {
		name           string
		existing       propertyStateWithLastUpdated
		updateProperty propertyStateWithLastUpdated
		expected       propertyStateWithLastUpdated
	}{
		{
			name: "update nil state with event",
			existing: propertyStateWithLastUpdated{
				state:       nil,
				lastUpdated: 0,
			},
			updateProperty: propertyStateWithLastUpdated{
				state: model.EventPropertyState{
					Instance: model.OpenPropertyInstance,
					Value:    model.OpenedEvent,
				},
				lastUpdated: 1647527855.200,
			},
			expected: propertyStateWithLastUpdated{
				state: model.EventPropertyState{
					Instance: model.OpenPropertyInstance,
					Value:    model.OpenedEvent,
				},
				lastUpdated: 1647527855.200,
			},
		},
		{
			name: "update existing state with the same value",
			existing: propertyStateWithLastUpdated{
				state: model.EventPropertyState{
					Instance: model.OpenPropertyInstance,
					Value:    model.OpenedEvent,
				},
				lastUpdated: 1647527322.100,
			},
			updateProperty: propertyStateWithLastUpdated{
				state: model.EventPropertyState{
					Instance: model.OpenPropertyInstance,
					Value:    model.OpenedEvent,
				},
				lastUpdated: 1647527855.200,
			},
			expected: propertyStateWithLastUpdated{
				state: model.EventPropertyState{
					Instance: model.OpenPropertyInstance,
					Value:    model.OpenedEvent,
				},
				lastUpdated: 1647527855.200,
			},
		},
		{
			name: "update existing state with with value changed ignore anti-flap",
			existing: propertyStateWithLastUpdated{
				state: model.EventPropertyState{
					Instance: model.OpenPropertyInstance,
					Value:    model.OpenedEvent,
				},
				lastUpdated: 1647527855.150,
			},
			updateProperty: propertyStateWithLastUpdated{
				state: model.EventPropertyState{
					Instance: model.OpenPropertyInstance,
					Value:    model.ClosedEvent,
				},
				lastUpdated: 1647527855.200,
			},
			expected: propertyStateWithLastUpdated{
				state: model.EventPropertyState{
					Instance: model.OpenPropertyInstance,
					Value:    model.ClosedEvent,
				},
				lastUpdated: 1647527855.200,
			},
		},
		{
			name: "do not update same state within anti-flap value",
			existing: propertyStateWithLastUpdated{
				state: model.EventPropertyState{
					Instance: model.OpenPropertyInstance,
					Value:    model.OpenedEvent,
				},
				lastUpdated: 1647527855.150,
			},
			updateProperty: propertyStateWithLastUpdated{
				state: model.EventPropertyState{
					Instance: model.OpenPropertyInstance,
					Value:    model.OpenedEvent,
				},
				lastUpdated: 1647527855.200,
			},
			expected: propertyStateWithLastUpdated{
				state: model.EventPropertyState{
					Instance: model.OpenPropertyInstance,
					Value:    model.OpenedEvent,
				},
				lastUpdated: 1647527855.150,
			},
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			existingProperty := model.MakePropertyByType(model.EventPropertyType).
				WithReportable(true).
				WithRetrievable(true).
				WithParameters(model.EventPropertyParameters{
					Instance: model.OpenPropertyInstance,
					Events: model.Events{
						model.KnownEvents[model.EventKey{Instance: model.OpenPropertyInstance, Value: model.OpenedEvent}],
						model.KnownEvents[model.EventKey{Instance: model.OpenPropertyInstance, Value: model.ClosedEvent}],
					},
				}).WithState(tc.existing.state).
				WithLastUpdated(tc.existing.lastUpdated)

			device := model.Device{
				ID:         "motion-sensor",
				Name:       "Motion sensor",
				Type:       model.SensorDeviceType,
				Properties: []model.IProperty{existingProperty},
			}
			updateProperty := model.MakePropertyByType(model.EventPropertyType).
				WithReportable(true).
				WithRetrievable(true).
				WithParameters(model.EventPropertyParameters{
					Instance: model.OpenPropertyInstance,
					Events: model.Events{
						model.KnownEvents[model.EventKey{Instance: model.OpenPropertyInstance, Value: model.OpenedEvent}],
						model.KnownEvents[model.EventKey{Instance: model.OpenPropertyInstance, Value: model.ClosedEvent}],
					},
				}).
				WithState(tc.updateProperty.state).
				WithLastUpdated(tc.updateProperty.lastUpdated)

			device.UpdateState(nil, []model.IProperty{updateProperty})

			assert.Equal(t, tc.expected.state, device.Properties[0].State())
			assert.Equal(t, tc.expected.lastUpdated, device.Properties[0].LastUpdated())
		})
	}
}

func TestDeviceValidateName(t *testing.T) {
	testCases := map[string]error{
		"invalid name":   &model.NameCharError{},
		"name":           &model.NameCharError{},
		"лампа":          nil,
		"моя лампа":      nil,
		"моя lampa":      &model.NameCharError{},
		"lampa моя":      &model.NameCharError{},
		"Моё устройство": nil,
		"Мой девайс":     nil,
		"моя лампа 1":    nil,
		"123":            &model.NameMinLettersError{},
		"слишком длинное название устройства": &model.NameLengthError{Limit: model.DeviceNameLength},
		"я":      &model.NameMinLettersError{}, // too short
		"лампа1": &model.NameCharError{},
		"":       &model.NameEmptyError{},
	}

	device := model.Device{}
	for k, v := range testCases {
		device.Name = k
		assert.Equal(t, v, device.AssertName(), device.Name)
	}
}

func TestRoomValidateName(t *testing.T) {
	testCases := map[string]error{
		"invalid name":  &model.NameCharError{},
		"name":          &model.NameCharError{},
		"спальня":       nil,
		"моя спальня":   nil,
		"моя spalnya":   &model.NameCharError{},
		"spalnya моя":   &model.NameCharError{},
		"моя спальня 1": nil,
		"123":           &model.NameMinLettersError{},
		"слишком длинное название комнаты": &model.NameLengthError{Limit: 20},
		"я":        &model.NameMinLettersError{}, // too short
		"Комната1": &model.NameCharError{},
		"":         &model.NameEmptyError{},
	}

	room := model.Room{}
	for k, v := range testCases {
		room.Name = k
		assert.Equal(t, v, room.AssertName(), room.Name)
	}
}

func TestDevice_RemoveCapabilityByTypeAndInstance(t *testing.T) {
	// Creating test device
	brightnessCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	brightnessCapability.SetRetrievable(true)
	brightnessCapability.SetState(model.RangeCapabilityState{
		Instance: model.BrightnessRangeInstance,
		Value:    100.0,
	})
	brightnessCapability.SetParameters(model.RangeCapabilityParameters{
		Instance: model.BrightnessRangeInstance,
		Unit:     "percent",
		Range: &model.Range{
			Min:       1,
			Max:       100,
			Precision: 1,
		},
	})

	volumeCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	volumeCapability.SetRetrievable(true)
	volumeCapability.SetParameters(model.RangeCapabilityParameters{
		Instance: model.VolumeRangeInstance,
		Unit:     "percent",
		Range: &model.Range{
			Min:       1,
			Max:       100,
			Precision: 1,
		},
	})

	onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOffCapability.SetRetrievable(true)
	onOffCapability.SetState(model.OnOffCapabilityState{
		Instance: "on",
		Value:    false,
	})

	colorSettingCapability := model.MakeCapabilityByType(model.ColorSettingCapabilityType)
	colorSettingCapability.SetRetrievable(true)
	colorSettingCapability.SetParameters(model.ColorSettingCapabilityParameters{
		ColorModel: model.CM(model.HsvModelType),
	})

	toggleCapability := model.MakeCapabilityByType(model.ToggleCapabilityType)
	toggleCapability.SetRetrievable(false)
	toggleCapability.SetParameters(model.ToggleCapabilityParameters{
		Instance: model.MuteToggleCapabilityInstance,
	})

	device := model.Device{
		ID:           "Lamp",
		Name:         "Lamper",
		Type:         model.LightDeviceType,
		Capabilities: []model.ICapability{colorSettingCapability, brightnessCapability, onOffCapability, volumeCapability, toggleCapability},
	}

	_, existsBefore := device.GetCapabilityByTypeAndInstance(model.OnOffCapabilityType, string(model.OnOnOffCapabilityInstance))
	assert.True(t, existsBefore)
	assert.Len(t, device.Capabilities, 5)

	device.RemoveCapabilityByTypeAndInstance(model.OnOffCapabilityType, string(model.OnOnOffCapabilityInstance))
	_, existsAfter := device.GetCapabilityByTypeAndInstance(model.OnOffCapabilityType, string(model.OnOnOffCapabilityInstance))
	assert.False(t, existsAfter)
	assert.Len(t, device.Capabilities, 4)
}

func TestDeviceCloneEqual(t *testing.T) {
	brightnessCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	brightnessCapability.SetRetrievable(true)
	brightnessCapability.SetState(model.RangeCapabilityState{
		Instance: model.BrightnessRangeInstance,
		Value:    100.0,
	})
	brightnessCapability.SetParameters(model.RangeCapabilityParameters{
		Instance: model.BrightnessRangeInstance,
		Unit:     "percent",
		Range: &model.Range{
			Min:       1,
			Max:       100,
			Precision: 1,
		},
	})

	volumeCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	volumeCapability.SetRetrievable(true)
	volumeCapability.SetParameters(model.RangeCapabilityParameters{
		Instance: model.VolumeRangeInstance,
		Unit:     model.UnitPercent,
		Range: &model.Range{
			Min:       1,
			Max:       100,
			Precision: 1,
		},
	})

	onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOffCapability.SetRetrievable(true)
	onOffCapability.SetState(model.OnOffCapabilityState{
		Instance: model.OnOnOffCapabilityInstance,
		Value:    false,
	})

	colorSettingCapability := model.MakeCapabilityByType(model.ColorSettingCapabilityType)
	colorSettingCapability.SetRetrievable(true)
	colorSettingCapability.SetParameters(model.ColorSettingCapabilityParameters{
		ColorModel: model.CM(model.HsvModelType),
	})

	toggleCapability := model.MakeCapabilityByType(model.ToggleCapabilityType)
	toggleCapability.SetRetrievable(false)
	toggleCapability.SetParameters(model.ToggleCapabilityParameters{
		Instance: model.MuteToggleCapabilityInstance,
	})

	device := model.Device{
		ID:           "Lamp",
		Name:         "Lamper",
		Type:         model.LightDeviceType,
		Capabilities: []model.ICapability{colorSettingCapability, brightnessCapability, onOffCapability, volumeCapability, toggleCapability},
	}

	deviceClone := device.Clone()
	assert.True(t, device.Equals(deviceClone))
	assert.Equal(t, device, deviceClone)

	device2 := model.Device{
		ID:           "Lamp",
		Name:         "Lamper",
		Type:         model.LightDeviceType,
		Capabilities: []model.ICapability{colorSettingCapability, onOffCapability, volumeCapability, toggleCapability},
	}

	assert.False(t, device2.Equals(device))
}

func TestTuyaACDeviceWeirdFix(t *testing.T) {
	onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOffCapability.SetRetrievable(true)

	temperatureCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	temperatureCapability.SetRetrievable(true)
	temperatureCapability.SetParameters(model.RangeCapabilityParameters{
		Instance: model.TemperatureRangeInstance,
		Unit:     model.UnitTemperatureCelsius,
		Range: &model.Range{
			Min:       1,
			Max:       100,
			Precision: 1,
		},
	})

	muteCapability := model.MakeCapabilityByType(model.ToggleCapabilityType)
	muteCapability.SetRetrievable(true)
	muteCapability.SetParameters(model.ToggleCapabilityParameters{
		Instance: model.MuteToggleCapabilityInstance,
	})

	backlightCapability := model.MakeCapabilityByType(model.ToggleCapabilityType)
	backlightCapability.SetRetrievable(false)
	backlightCapability.SetParameters(
		model.ToggleCapabilityParameters{Instance: model.BacklightToggleCapabilityInstance},
	)

	// check filling with default states and filtrating unretrievable
	// mute does not have state
	// backlight is unretrievable

	device := model.Device{
		Capabilities: []model.ICapability{
			onOffCapability,
			temperatureCapability,
			muteCapability,
			backlightCapability,
		},
	}

	// preparing capabilities for newStateContainer

	onOff2 := onOffCapability.Clone()
	onOff2.SetState(model.OnOffCapabilityState{
		Instance: model.OnOnOffCapabilityInstance,
		Value:    true,
	})

	temperature2 := temperatureCapability.Clone()
	temperature2.SetState(model.RangeCapabilityState{
		Instance: model.TemperatureRangeInstance,
		Relative: tools.AOB(false),
		Value:    30.0,
	})

	mute2 := muteCapability.Clone()

	backlight2 := backlightCapability.Clone()
	backlight2.SetState(model.ToggleCapabilityState{
		Instance: model.BacklightToggleCapabilityInstance,
		Value:    true,
	})

	newStateContainer := model.Device{
		Capabilities: []model.ICapability{
			onOff2,
			temperature2,
			mute2,
			backlight2,
		},
	}

	// expected results

	expectedOnOff := onOff2.Clone()

	expectedTemperature := temperature2.Clone()

	expectedMute := mute2.Clone()
	expectedMute.SetState(expectedMute.DefaultState())

	expectedBacklight := backlight2.Clone()
	expectedBacklight.SetState(nil)

	expected := model.Device{
		Capabilities: []model.ICapability{
			expectedOnOff,
			expectedTemperature,
			expectedMute,
			expectedBacklight,
		},
	}

	actual := model.TuyaACDeviceWeirdFix(newStateContainer, device)
	assert.Equal(t, expected, actual)
}

func TestDeviceTypeToProto(t *testing.T) {
	for _, deviceType := range model.KnownDeviceTypes {
		testFunc := func() {
			device := model.Device{
				Type:         model.DeviceType(deviceType),
				OriginalType: model.DeviceType(deviceType),
			}
			_ = device.ToProto()
		}
		assert.NotPanicsf(t, testFunc, "should add device type %s to toProto map", deviceType)
	}
}

func TestGenerateDeviceName(t *testing.T) {
	for _, d := range model.KnownDeviceTypes {
		assert.NotPanics(t, func() {
			_ = model.DeviceType(d).GenerateDeviceName()
		})
	}

	for _, d := range model.KnownDeviceTypes {
		if slices.Contains(model.KnownQuasarDeviceTypes, d) {
			continue
		}
		d := model.Device{
			Name: model.DeviceType(d).GenerateDeviceName(),
		}
		err := d.AssertName()
		assert.NoError(t, err, "failed at \"%s\"", d.Name)
	}
}

func TestDeviceTypeIconsAreNotForgotten(t *testing.T) {
	for _, dt := range model.KnownDeviceTypes {
		assert.NotPanics(t, func() {
			_ = model.DeviceType(dt).IconURL(model.OriginalIconFormat)
		})
	}
}

func TestDevicesAggregatedDeviceType(t *testing.T) {
	multiroomSpeakers := model.Devices{{Type: model.YandexStationDeviceType}, {Type: model.YandexStationMini2DeviceType}}
	notSameTypeDevices := model.Devices{{Type: model.LightDeviceType}, {Type: model.SocketDeviceType}}
	lightDevices := model.Devices{{Type: model.LightDeviceType}, {Type: model.LightDeviceType}}
	mixedSpeakers := model.Devices{{Type: model.DexpSmartBoxDeviceType}, {Type: model.YandexStationMini2DeviceType}}
	type testCase struct {
		name               string
		devices            model.Devices
		expectedDeviceType model.DeviceType
	}
	testCases := []testCase{
		{
			name:               "multiroom",
			devices:            multiroomSpeakers,
			expectedDeviceType: model.SmartSpeakerDeviceType,
		},
		{
			name:               "not same type",
			devices:            notSameTypeDevices,
			expectedDeviceType: "",
		},
		{
			name:               "light",
			devices:            lightDevices,
			expectedDeviceType: model.LightDeviceType,
		},
		{
			name:               "mixed",
			devices:            mixedSpeakers,
			expectedDeviceType: "",
		},
	}
	for _, tc := range testCases {
		dt := tc.devices.AggregatedDeviceType()
		assert.Equal(t, tc.expectedDeviceType, dt, tc.name)
	}
}

func TestDevicesGetRooms(t *testing.T) {
	devices := model.Devices{
		{
			ID: "1",
			Room: &model.Room{
				ID:      "room-1",
				Name:    "Конура",
				Devices: []string{"1"},
			},
		},
		{
			ID: "2",
			Room: &model.Room{
				ID:   "room-1",
				Name: "Конура",
			},
		},
		{
			ID: "3",
			Room: &model.Room{
				ID:   "room-2",
				Name: "Клетка",
			},
		},
		{
			ID: "4",
			Room: &model.Room{
				ID:   "room-2",
				Name: "Клетка",
			},
		},
		{
			ID: "5",
			Room: &model.Room{
				ID:   "room-3",
				Name: "Моя крепость",
			},
		},
	}
	rooms := devices.GetRooms()
	sort.Sort(model.RoomsSorting(rooms))
	expected := model.Rooms{
		{
			ID:      "room-2",
			Name:    "Клетка",
			Devices: []string{"3", "4"},
		},
		{
			ID:      "room-1",
			Name:    "Конура",
			Devices: []string{"1", "2"},
		},
		{
			ID:      "room-3",
			Name:    "Моя крепость",
			Devices: []string{"5"},
		},
	}
	assert.Equal(t, expected, rooms)
}

func TestIsTandemCompatibleWith(t *testing.T) {
	type testCase struct {
		DisplayType model.DeviceType
		SpeakerType model.DeviceType
		Compatible  bool
	}
	testCases := []testCase{
		{
			DisplayType: model.YandexModule2DeviceType,
			SpeakerType: model.YandexStationMini2NoClockDeviceType,
			Compatible:  true,
		},
		{
			DisplayType: model.YandexModuleDeviceType,
			SpeakerType: model.YandexStationMini2NoClockDeviceType,
			Compatible:  true,
		},
		{
			DisplayType: model.YandexModuleDeviceType,
			SpeakerType: model.IrbisADeviceType,
			Compatible:  true,
		},
		{
			DisplayType: model.YandexModule2DeviceType,
			SpeakerType: model.IrbisADeviceType,
			Compatible:  false,
		},
		{
			DisplayType: model.YandexModuleDeviceType,
			SpeakerType: model.YandexStationMicroDeviceType,
			Compatible:  false,
		},
		{
			DisplayType: model.YandexModule2DeviceType,
			SpeakerType: model.YandexStationMicroDeviceType,
			Compatible:  true,
		},
		{
			DisplayType: model.TvDeviceDeviceType,
			SpeakerType: model.YandexStationMini2NoClockDeviceType,
			Compatible:  true,
		},
		{
			DisplayType: model.TvDeviceDeviceType,
			SpeakerType: model.IrbisADeviceType,
			Compatible:  false,
		},
		{
			DisplayType: model.TvDeviceDeviceType,
			SpeakerType: model.YandexStationMicroDeviceType,
			Compatible:  true,
		},
	}
	for _, tc := range testCases {
		displayDevice := model.Device{SkillID: model.QUASAR, Type: tc.DisplayType}
		speakerDevice := model.Device{SkillID: model.QUASAR, Type: tc.SpeakerType}
		assert.Equal(t, displayDevice.IsTandemCompatibleWith(speakerDevice), tc.Compatible)
		assert.Equal(t, speakerDevice.IsTandemCompatibleWith(displayDevice), tc.Compatible)
	}
}

func TestMidiToUserInfoProto(t *testing.T) {
	midi := model.Device{
		Type: model.YandexStationMidiDeviceType,
	}

	colorScenes := make(model.ColorScenes, 0, len(model.KnownYandexmidiColorScenes))
	for _, colorSceneID := range model.KnownYandexmidiColorScenes {
		colorScenes = append(colorScenes, model.KnownColorScenes[model.ColorSceneID(colorSceneID)])
	}
	sort.Sort(model.ColorSceneSorting(colorScenes))
	midiColorSetting := model.MakeCapabilityByType(model.ColorSettingCapabilityType).
		WithParameters(model.ColorSettingCapabilityParameters{
			ColorSceneParameters: &model.ColorSceneParameters{
				Scenes: colorScenes,
			},
		})
	expected := &common.TIoTUserInfo_TDevice{
		Type: devicepb.EUserDeviceType_LightDeviceType,
		Capabilities: []*common.TIoTUserInfo_TCapability{
			model.MakeCapabilityByType(model.OnOffCapabilityType).ToUserInfoProto(),
			midiColorSetting.ToUserInfoProto(),
		},
		Properties:    []*common.TIoTUserInfo_TProperty{},
		GroupIds:      make([]string, 0),
		AnalyticsType: "devices.types.light",
		AnalyticsName: "Осветительный прибор",
		IconURL:       "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.smart_speaker.yandex.station.midi.png",
		OriginalType:  devicepb.EUserDeviceType_LightDeviceType,
	}
	ctxWithMidi := experiments.ContextWithManager(context.Background(), experiments.MockManager{
		experiments.MidiUserInfoColorSetting: true,
	})
	assert.Equal(t, expected, midi.ToUserInfoProto(ctxWithMidi))
	expected = &common.TIoTUserInfo_TDevice{
		Type:          devicepb.EUserDeviceType_YandexStationMidiDeviceType,
		Capabilities:  []*common.TIoTUserInfo_TCapability{},
		Properties:    []*common.TIoTUserInfo_TProperty{},
		GroupIds:      make([]string, 0),
		AnalyticsType: "devices.types.smart_speaker.yandex.station.midi",
		AnalyticsName: "Умное устройство",
		IconURL:       "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.smart_speaker.yandex.station.midi.png",
	}
	assert.Equal(t, expected, midi.ToUserInfoProto(context.Background()))
}

func TestYandexIODevice_GetParent(t *testing.T) {
	type args struct {
		userDevices model.Devices
	}
	tests := []struct {
		name         string
		d            model.YandexIODevice
		args         args
		parentDevice model.Device
		hasParent    bool
	}{
		{
			name: "find success",
			d: model.YandexIODevice{
				ID:         "yandexio-lamp",
				SkillID:    model.YANDEXIO,
				CustomData: yandexiocd.CustomData{ParentEndpointID: "parent-speaker-external-id"},
			},
			args: args{
				userDevices: model.Devices{
					{
						ID:         "yandexio-lamp",
						SkillID:    model.YANDEXIO,
						CustomData: yandexiocd.CustomData{ParentEndpointID: "parent-speaker-external-id"},
					},
					{
						ID:         "parent-speaker",
						SkillID:    model.QUASAR,
						ExternalID: "parent-speaker-external-id",
						CustomData: quasar.CustomData{DeviceID: "parent-speaker-external-id"},
					},
				},
			},
			parentDevice: model.Device{
				ID:         "parent-speaker",
				SkillID:    model.QUASAR,
				ExternalID: "parent-speaker-external-id",
				CustomData: quasar.CustomData{DeviceID: "parent-speaker-external-id"},
			},
			hasParent: true,
		},
		{
			name: "find failure",
			d: model.YandexIODevice{
				ID:         "yandexio-lamp",
				SkillID:    model.YANDEXIO,
				CustomData: yandexiocd.CustomData{ParentEndpointID: "some-unknown-id"},
			},
			args: args{
				userDevices: model.Devices{
					{
						ID:         "yandexio-lamp",
						SkillID:    model.YANDEXIO,
						CustomData: yandexiocd.CustomData{ParentEndpointID: "some-unknown-id"},
					},
					{
						ID:         "some-other-speaker",
						SkillID:    model.QUASAR,
						ExternalID: "other-speaker-external-id",
						CustomData: quasar.CustomData{DeviceID: "other-speaker-external-id"},
					},
				},
			},
			parentDevice: model.Device{},
			hasParent:    false,
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			got, got1 := tt.d.GetParent(tt.args.userDevices)
			assert.Equalf(t, tt.parentDevice, got, "GetParent(%v)", tt.args.userDevices)
			assert.Equalf(t, tt.hasParent, got1, "GetParent(%v)", tt.args.userDevices)
		})
	}
}

func TestYandexIODevices_GetParentChildRelations(t *testing.T) {
	type args struct {
		userDevices model.Devices
	}
	tests := []struct {
		name                   string
		devices                model.YandexIODevices
		args                   args
		wantParentsMap         map[string]model.Device
		wantParentRelationsMap map[string]model.Devices
	}{
		{
			name: "multiple speakers, multiple yandexio devices",
			devices: model.YandexIODevices{
				{
					ID:         "lamp-1",
					SkillID:    model.YANDEXIO,
					CustomData: yandexiocd.CustomData{ParentEndpointID: "parent-speaker-ext-1"},
				},
				{
					ID:         "lamp-2",
					SkillID:    model.YANDEXIO,
					CustomData: yandexiocd.CustomData{ParentEndpointID: "parent-speaker-ext-1"},
				},
				{
					ID:         "sensor-1",
					SkillID:    model.YANDEXIO,
					CustomData: yandexiocd.CustomData{ParentEndpointID: "parent-speaker-ext-2"},
				},
			},
			args: args{
				userDevices: model.Devices{
					{
						ID:         "lamp-1",
						SkillID:    model.YANDEXIO,
						CustomData: yandexiocd.CustomData{ParentEndpointID: "parent-speaker-ext-1"},
					},
					{
						ID:         "lamp-2",
						SkillID:    model.YANDEXIO,
						CustomData: yandexiocd.CustomData{ParentEndpointID: "parent-speaker-ext-1"},
					},
					{
						ID:         "sensor-1",
						SkillID:    model.YANDEXIO,
						CustomData: yandexiocd.CustomData{ParentEndpointID: "parent-speaker-ext-2"},
					},
					{
						ID:         "parent-speaker-1",
						SkillID:    model.QUASAR,
						ExternalID: "parent-speaker-ext-1",
						CustomData: quasar.CustomData{DeviceID: "parent-speaker-ext-1"},
					},
					{
						ID:         "parent-speaker-2",
						SkillID:    model.QUASAR,
						ExternalID: "parent-speaker-ext-2",
						CustomData: quasar.CustomData{DeviceID: "parent-speaker-ext-2"},
					},
				},
			},
			wantParentsMap: map[string]model.Device{
				"parent-speaker-1": {
					ID:         "parent-speaker-1",
					SkillID:    model.QUASAR,
					ExternalID: "parent-speaker-ext-1",
					CustomData: quasar.CustomData{DeviceID: "parent-speaker-ext-1"},
				},
				"parent-speaker-2": {
					ID:         "parent-speaker-2",
					SkillID:    model.QUASAR,
					ExternalID: "parent-speaker-ext-2",
					CustomData: quasar.CustomData{DeviceID: "parent-speaker-ext-2"},
				},
			},
			wantParentRelationsMap: map[string]model.Devices{
				"parent-speaker-1": {
					{
						ID:         "lamp-1",
						SkillID:    model.YANDEXIO,
						CustomData: yandexiocd.CustomData{ParentEndpointID: "parent-speaker-ext-1"},
					},
					{
						ID:         "lamp-2",
						SkillID:    model.YANDEXIO,
						CustomData: yandexiocd.CustomData{ParentEndpointID: "parent-speaker-ext-1"},
					},
				},
				"parent-speaker-2": {
					{
						ID:         "sensor-1",
						SkillID:    model.YANDEXIO,
						CustomData: yandexiocd.CustomData{ParentEndpointID: "parent-speaker-ext-2"},
					},
				},
			},
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			gotParentsMap, gotParentRelationsMap := tt.devices.GetParentChildRelations(tt.args.userDevices)
			assert.Equalf(t, tt.wantParentsMap, gotParentsMap, "GetParentChildRelations(%v)", tt.args.userDevices)
			assert.Equalf(t, tt.wantParentRelationsMap, gotParentRelationsMap, "GetParentChildRelations(%v)", tt.args.userDevices)
		})
	}
}

func TestYandexIODevices_GetChildParentRelations(t *testing.T) {
	type args struct {
		userDevices model.Devices
	}
	tests := []struct {
		name                  string
		devices               model.YandexIODevices
		args                  args
		wantChildrenMap       map[string]model.Device
		wantChildRelationsMap map[string]model.Device
	}{
		{
			name: "multiple speakers, multiple yandexio devices",
			devices: model.YandexIODevices{
				{
					ID:         "lamp-1",
					SkillID:    model.YANDEXIO,
					CustomData: yandexiocd.CustomData{ParentEndpointID: "parent-speaker-ext-1"},
				},
				{
					ID:         "lamp-2",
					SkillID:    model.YANDEXIO,
					CustomData: yandexiocd.CustomData{ParentEndpointID: "parent-speaker-ext-1"},
				},
				{
					ID:         "sensor-1",
					SkillID:    model.YANDEXIO,
					CustomData: yandexiocd.CustomData{ParentEndpointID: "parent-speaker-ext-2"},
				},
			},
			args: args{
				userDevices: model.Devices{
					{
						ID:         "lamp-1",
						SkillID:    model.YANDEXIO,
						CustomData: yandexiocd.CustomData{ParentEndpointID: "parent-speaker-ext-1"},
					},
					{
						ID:         "lamp-2",
						SkillID:    model.YANDEXIO,
						CustomData: yandexiocd.CustomData{ParentEndpointID: "parent-speaker-ext-1"},
					},
					{
						ID:         "sensor-1",
						SkillID:    model.YANDEXIO,
						CustomData: yandexiocd.CustomData{ParentEndpointID: "parent-speaker-ext-2"},
					},
					{
						ID:         "parent-speaker-1",
						SkillID:    model.QUASAR,
						ExternalID: "parent-speaker-ext-1",
						CustomData: quasar.CustomData{DeviceID: "parent-speaker-ext-1"},
					},
					{
						ID:         "parent-speaker-2",
						SkillID:    model.QUASAR,
						ExternalID: "parent-speaker-ext-2",
						CustomData: quasar.CustomData{DeviceID: "parent-speaker-ext-2"},
					},
				},
			},
			wantChildrenMap: map[string]model.Device{
				"lamp-1": {
					ID:         "lamp-1",
					SkillID:    model.YANDEXIO,
					CustomData: yandexiocd.CustomData{ParentEndpointID: "parent-speaker-ext-1"},
				},
				"lamp-2": {
					ID:         "lamp-2",
					SkillID:    model.YANDEXIO,
					CustomData: yandexiocd.CustomData{ParentEndpointID: "parent-speaker-ext-1"},
				},
				"sensor-1": {
					ID:         "sensor-1",
					SkillID:    model.YANDEXIO,
					CustomData: yandexiocd.CustomData{ParentEndpointID: "parent-speaker-ext-2"},
				},
			},
			wantChildRelationsMap: map[string]model.Device{
				"lamp-1": {
					ID:         "parent-speaker-1",
					SkillID:    model.QUASAR,
					ExternalID: "parent-speaker-ext-1",
					CustomData: quasar.CustomData{DeviceID: "parent-speaker-ext-1"},
				},
				"lamp-2": {
					ID:         "parent-speaker-1",
					SkillID:    model.QUASAR,
					ExternalID: "parent-speaker-ext-1",
					CustomData: quasar.CustomData{DeviceID: "parent-speaker-ext-1"},
				},
				"sensor-1": {
					ID:         "parent-speaker-2",
					SkillID:    model.QUASAR,
					ExternalID: "parent-speaker-ext-2",
					CustomData: quasar.CustomData{DeviceID: "parent-speaker-ext-2"},
				},
			},
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			gotChildrenMap, gotChildRelationsMap := tt.devices.GetChildParentRelations(tt.args.userDevices)
			assert.Equalf(t, tt.wantChildrenMap, gotChildrenMap, "GetChildParentRelations(%v)", tt.args.userDevices)
			assert.Equalf(t, tt.wantChildRelationsMap, gotChildRelationsMap, "GetChildParentRelations(%v)", tt.args.userDevices)
		})
	}
}

func TestQuasarDevice_ChildDevices(t *testing.T) {
	type args struct {
		userDevices model.Devices
	}
	tests := []struct {
		name string
		d    model.QuasarDevice
		args args
		want model.Devices
	}{
		{
			name: "multiple speakers, multiple yandexio devices",
			d: model.QuasarDevice{
				ID:         "parent-speaker-1",
				SkillID:    model.QUASAR,
				ExternalID: "parent-speaker-ext-1",
				CustomData: quasar.CustomData{DeviceID: "parent-speaker-ext-1"},
			},
			args: args{
				userDevices: model.Devices{
					{
						ID:         "lamp-2",
						Name:       "Яркая лампа",
						SkillID:    model.YANDEXIO,
						CustomData: yandexiocd.CustomData{ParentEndpointID: "parent-speaker-ext-1"},
					},
					{
						ID:         "lamp-1",
						Name:       "Лампа 1",
						SkillID:    model.YANDEXIO,
						CustomData: yandexiocd.CustomData{ParentEndpointID: "parent-speaker-ext-1"},
					},
					{
						ID:         "lamp-3",
						Name:       "Моргающая лампа",
						SkillID:    model.YANDEXIO,
						CustomData: yandexiocd.CustomData{ParentEndpointID: "parent-speaker-ext-1"},
					},
					{
						ID:         "sensor-1",
						SkillID:    model.YANDEXIO,
						CustomData: yandexiocd.CustomData{ParentEndpointID: "parent-speaker-ext-2"},
					},
					{
						ID:         "parent-speaker-1",
						SkillID:    model.QUASAR,
						ExternalID: "parent-speaker-ext-1",
						CustomData: quasar.CustomData{DeviceID: "parent-speaker-ext-1"},
					},
					{
						ID:         "parent-speaker-2",
						SkillID:    model.QUASAR,
						ExternalID: "parent-speaker-ext-2",
						CustomData: quasar.CustomData{DeviceID: "parent-speaker-ext-2"},
					},
				},
			},
			want: model.Devices{
				{
					ID:         "lamp-1",
					Name:       "Лампа 1",
					SkillID:    model.YANDEXIO,
					CustomData: yandexiocd.CustomData{ParentEndpointID: "parent-speaker-ext-1"},
				},
				{
					ID:         "lamp-3",
					Name:       "Моргающая лампа",
					SkillID:    model.YANDEXIO,
					CustomData: yandexiocd.CustomData{ParentEndpointID: "parent-speaker-ext-1"},
				},
				{
					ID:         "lamp-2",
					Name:       "Яркая лампа",
					SkillID:    model.YANDEXIO,
					CustomData: yandexiocd.CustomData{ParentEndpointID: "parent-speaker-ext-1"},
				},
			},
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			assert.Equalf(t, tt.want, tt.d.ChildDevices(tt.args.userDevices), "ChildDevices(%v)", tt.args.userDevices)
		})
	}
}

func TestSharedDevices(t *testing.T) {
	devices := model.Devices{
		{
			ID: "1",
			SharingInfo: &model.SharingInfo{
				OwnerID:     1,
				HouseholdID: "1",
			},
		},
		{ID: "2"},
		{ID: "3"},
	}
	expectedNonShared := model.Devices{
		{ID: "2"},
		{ID: "3"},
	}
	expectedShared := model.Devices{
		{
			ID: "1",
			SharingInfo: &model.SharingInfo{
				OwnerID:     1,
				HouseholdID: "1",
			},
		},
	}
	assert.Equal(t, expectedNonShared, devices.NonSharedDevices())
	assert.Equal(t, expectedShared, devices.SharedDevices())
}

func TestDevice_PopulateAsStateContainer(t *testing.T) {
	t.Run("off + brightness -> off", func(t *testing.T) {
		var stateContainer model.Device
		userDevice := model.Device{
			Capabilities: model.Capabilities{
				model.MakeCapabilityByType(model.OnOffCapabilityType).
					WithState(model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    true,
					}),
				model.MakeCapabilityByType(model.RangeCapabilityType).
					WithParameters(model.RangeCapabilityParameters{
						Instance: model.BrightnessRangeInstance,
						Range:    &model.Range{Min: 0, Max: 100, Precision: 1},
					}).
					WithState(model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    50,
					}),
			},
		}
		actions := model.Capabilities{
			model.MakeCapabilityByType(model.OnOffCapabilityType).
				WithState(model.OnOffCapabilityState{
					Instance: model.OnOnOffCapabilityInstance,
					Value:    false,
				}),
			model.MakeCapabilityByType(model.RangeCapabilityType).
				WithState(model.RangeCapabilityState{
					Instance: model.BrightnessRangeInstance,
					Value:    100,
				}),
		}
		stateContainer.PopulateAsStateContainer(userDevice, actions)
		expectedActions := model.Capabilities{
			model.MakeCapabilityByType(model.OnOffCapabilityType).
				WithState(model.OnOffCapabilityState{
					Instance: model.OnOnOffCapabilityInstance,
					Value:    false,
				}),
		}
		assert.Equal(t, expectedActions, stateContainer.Capabilities)
	})
	t.Run("invert to off + brightness -> off", func(t *testing.T) {
		var stateContainer model.Device
		userDevice := model.Device{
			Capabilities: model.Capabilities{
				model.MakeCapabilityByType(model.OnOffCapabilityType).
					WithState(model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    true,
					}),
				model.MakeCapabilityByType(model.RangeCapabilityType).
					WithParameters(model.RangeCapabilityParameters{
						Instance: model.BrightnessRangeInstance,
						Range:    &model.Range{Min: 0, Max: 100, Precision: 1},
					}).
					WithState(model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    50,
					}),
			},
		}
		actions := model.Capabilities{
			model.MakeCapabilityByType(model.OnOffCapabilityType).
				WithState(model.OnOffCapabilityState{
					Instance: model.OnOnOffCapabilityInstance,
					Relative: ptr.Bool(true),
				}),
			model.MakeCapabilityByType(model.RangeCapabilityType).
				WithState(model.RangeCapabilityState{
					Instance: model.BrightnessRangeInstance,
					Value:    100,
				}),
		}
		stateContainer.PopulateAsStateContainer(userDevice, actions)
		expectedActions := model.Capabilities{
			model.MakeCapabilityByType(model.OnOffCapabilityType).
				WithState(model.OnOffCapabilityState{
					Instance: model.OnOnOffCapabilityInstance,
					Value:    false,
				}),
		}
		assert.Equal(t, expectedActions, stateContainer.Capabilities)
	})
	t.Run("invert to true + brightness -> on+brightness", func(t *testing.T) {
		var stateContainer model.Device
		userDevice := model.Device{
			Capabilities: model.Capabilities{
				model.MakeCapabilityByType(model.OnOffCapabilityType).
					WithState(model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    false,
					}),
				model.MakeCapabilityByType(model.RangeCapabilityType).
					WithParameters(model.RangeCapabilityParameters{
						Instance: model.BrightnessRangeInstance,
						Range:    &model.Range{Min: 0, Max: 100, Precision: 1},
					}).
					WithState(model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    50,
					}),
			},
		}
		actions := model.Capabilities{
			model.MakeCapabilityByType(model.OnOffCapabilityType).
				WithState(model.OnOffCapabilityState{
					Instance: model.OnOnOffCapabilityInstance,
					Relative: ptr.Bool(true),
				}),
			model.MakeCapabilityByType(model.RangeCapabilityType).
				WithState(model.RangeCapabilityState{
					Instance: model.BrightnessRangeInstance,
					Value:    100,
				}),
		}
		stateContainer.PopulateAsStateContainer(userDevice, actions)
		expectedActions := model.Capabilities{
			model.MakeCapabilityByType(model.OnOffCapabilityType).
				WithState(model.OnOffCapabilityState{
					Instance: model.OnOnOffCapabilityInstance,
					Value:    true,
				}),
			model.MakeCapabilityByType(model.RangeCapabilityType).
				WithParameters(model.RangeCapabilityParameters{
					Instance: model.BrightnessRangeInstance,
					Range:    &model.Range{Min: 0, Max: 100, Precision: 1},
				}).
				WithState(model.RangeCapabilityState{
					Instance: model.BrightnessRangeInstance,
					Value:    100,
				}),
		}
		assert.Equal(t, expectedActions, stateContainer.Capabilities)
	})
}
