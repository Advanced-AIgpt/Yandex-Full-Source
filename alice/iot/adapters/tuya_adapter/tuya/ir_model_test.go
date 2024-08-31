package tuya

import (
	"testing"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/tuya"
	"a.yandex-team.ru/library/go/ptr"
	"github.com/stretchr/testify/assert"
)

func TestIRControl_ToDeviceInfoView(t *testing.T) {
	irControl := IRControl{
		ID:            "control-id",
		Name:          "My TV control",
		CategoryID:    TvIrCategoryID,
		BrandID:       "22",
		RemoteIndex:   "1122",
		TransmitterID: "my-transmitter-01",
		Keys: map[string]string{
			string(PowerKeyName):            "11",
			string(MuteKeyName):             "5",
			string(ChannelUpKeyName):        "3",
			string(PauseKeyName):            "166",
			string(ChannelDownKeyName):      "1",
			string(VolumeUpKeyName):         "33",
			string(VolumeDownKeyName):       "111",
			string(InputSourceKeyName):      "170",
			string(InputSourceHDMI1KeyName): "171",
			string(InputSourceHDMI2KeyName): "172",
			string(InputSourceTVKeyName):    "175",
		},
	}

	exceptedDeviceInfoView := adapter.DeviceInfoView{
		ID:   "control-id",
		Name: "My TV control",
		Type: model.TvDeviceDeviceType,
		DeviceInfo: &model.DeviceInfo{
			Manufacturer: ptr.String(YandexDevicesManufacturer),
			Model:        ptr.String(HubYandexModel),
		},
		CustomData: tuya.CustomData{
			DeviceType: model.TvDeviceDeviceType,
			InfraredData: &tuya.InfraredData{
				PresetID:      "1122",
				TransmitterID: "my-transmitter-01",
				Keys: map[tuya.IRKeyName]string{
					PowerKeyName:            "11",
					MuteKeyName:             "5",
					PauseKeyName:            "166",
					ChannelUpKeyName:        "3",
					ChannelDownKeyName:      "1",
					VolumeUpKeyName:         "33",
					VolumeDownKeyName:       "111",
					InputSourceOneKeyName:   "170",
					InputSourceTwoKeyName:   "171",
					InputSourceThreeKeyName: "172",
					InputSourceFourKeyName:  "175",
				},
			},
		},
		Capabilities: []adapter.CapabilityInfoView{
			{
				Type:        model.OnOffCapabilityType,
				Retrievable: false,
			},
			{
				Type:        model.ToggleCapabilityType,
				Retrievable: false,
				Parameters: model.ToggleCapabilityParameters{
					Instance: model.MuteToggleCapabilityInstance,
				},
			},
			{
				Type:        model.ToggleCapabilityType,
				Retrievable: false,
				Parameters: model.ToggleCapabilityParameters{
					Instance: model.PauseToggleCapabilityInstance,
				},
			},
			{
				Type:        model.ModeCapabilityType,
				Retrievable: false,
				Parameters: model.ModeCapabilityParameters{
					Instance: model.InputSourceModeInstance,
					Modes: []model.Mode{
						{Value: model.OneMode},
						{Value: model.TwoMode},
						{Value: model.ThreeMode},
						{Value: model.FourMode},
					},
				},
			},
			{
				Retrievable: false,
				Type:        model.RangeCapabilityType,
				Parameters: model.RangeCapabilityParameters{
					Instance:     model.VolumeRangeInstance,
					RandomAccess: false,
					Looped:       false,
				},
			},
			{
				Retrievable: false,
				Type:        model.RangeCapabilityType,
				Parameters: model.RangeCapabilityParameters{
					Instance:     model.ChannelRangeInstance,
					RandomAccess: false,
					Looped:       true,
				},
			},
		},
		Properties: []adapter.PropertyInfoView{},
	}

	productID := HubYandexProductID
	assert.EqualValues(t, exceptedDeviceInfoView, irControl.ToDeviceInfoView(productID))
}

func TestIRControlTVFullView_ToDeviceInfoView(t *testing.T) {
	irControl := IRControl{
		ID:            "control-id",
		Name:          "My TV control",
		CategoryID:    TvIrCategoryID,
		BrandID:       "22",
		RemoteIndex:   "1122",
		TransmitterID: "my-transmitter-01",
		Keys: map[string]string{
			string(PowerKeyName):                 "11",
			string(MuteKeyName):                  "5",
			string(PauseKeyName):                 "166",
			string(ChannelUpKeyName):             "3",
			string(ChannelDownKeyName):           "1",
			string(DigitOneKeyName):              "01",
			string(DigitTwoKeyName):              "02",
			string(DigitThreeKeyName):            "03",
			string(DigitFourKeyName):             "04",
			string(DigitFiveKeyName):             "05",
			string(DigitSixKeyName):              "06",
			string(DigitSevenKeyName):            "07",
			string(DigitEightKeyName):            "08",
			string(DigitNineKeyName):             "09",
			string(DigitZeroKeyName):             "00",
			string(InputSourceKeyName):           "170",
			string(InputSourceHDMI1KeyName):      "171",
			string(InputSourceHDMI2KeyName):      "172",
			string(InputSourceHDMI3KeyName):      "173",
			string(InputSourceHDMI4KeyName):      "200",
			string(InputSourceComponent1KeyName): "175",
			string(InputSourceComponent2KeyName): "176",
			string(InputSourceTVVIDKeyName):      "177",
			string(InputSourceTVKeyName):         "178",
			string(InputSourceVideoKeyName):      "179",
			string(InputSourceVideo2KeyName):     "180",
			string(InputSourceVideo3KeyName):     "181",
			string(InputSourceDVIKeyName):        "182",
			string(InputSourceDVI1KeyName):       "183",
			string(InputSourceDVI2KeyName):       "184",
		},
	}

	expectedDeviceInfoView := adapter.DeviceInfoView{
		ID:   "control-id",
		Name: "My TV control",
		Type: model.TvDeviceDeviceType,
		DeviceInfo: &model.DeviceInfo{
			Manufacturer: ptr.String(YandexDevicesManufacturer),
			Model:        ptr.String(HubYandexModel),
		},
		CustomData: tuya.CustomData{
			DeviceType: model.TvDeviceDeviceType,
			InfraredData: &tuya.InfraredData{
				PresetID:      "1122",
				TransmitterID: "my-transmitter-01",
				Keys: map[tuya.IRKeyName]string{
					PowerKeyName:            "11",
					MuteKeyName:             "5",
					PauseKeyName:            "166",
					ChannelUpKeyName:        "3",
					ChannelDownKeyName:      "1",
					DigitOneKeyName:         "01",
					DigitTwoKeyName:         "02",
					DigitThreeKeyName:       "03",
					DigitFourKeyName:        "04",
					DigitFiveKeyName:        "05",
					DigitSixKeyName:         "06",
					DigitSevenKeyName:       "07",
					DigitEightKeyName:       "08",
					DigitNineKeyName:        "09",
					DigitZeroKeyName:        "00",
					InputSourceOneKeyName:   "170",
					InputSourceTwoKeyName:   "171",
					InputSourceThreeKeyName: "172",
					InputSourceFourKeyName:  "173",
					InputSourceFiveKeyName:  "200",
					InputSourceSixKeyName:   "175",
					InputSourceSevenKeyName: "176",
					InputSourceEightKeyName: "177",
					InputSourceNineKeyName:  "178",
					InputSourceTenKeyName:   "179",
				},
			},
		},
		Capabilities: []adapter.CapabilityInfoView{
			{
				Type:        model.OnOffCapabilityType,
				Retrievable: false,
			},
			{
				Type:        model.ToggleCapabilityType,
				Retrievable: false,
				Parameters: model.ToggleCapabilityParameters{
					Instance: model.MuteToggleCapabilityInstance,
				},
			},
			{
				Type:        model.ToggleCapabilityType,
				Retrievable: false,
				Parameters: model.ToggleCapabilityParameters{
					Instance: model.PauseToggleCapabilityInstance,
				},
			},
			{
				Type:        model.ModeCapabilityType,
				Retrievable: false,
				Parameters: model.ModeCapabilityParameters{
					Instance: model.InputSourceModeInstance,
					Modes: []model.Mode{
						{Value: model.OneMode},
						{Value: model.TwoMode},
						{Value: model.ThreeMode},
						{Value: model.FourMode},
						{Value: model.FiveMode},
						{Value: model.SixMode},
						{Value: model.SevenMode},
						{Value: model.EightMode},
						{Value: model.NineMode},
						{Value: model.TenMode},
					},
				},
			},
			{
				Retrievable: false,
				Type:        model.RangeCapabilityType,
				Parameters: model.RangeCapabilityParameters{
					Instance:     model.ChannelRangeInstance,
					RandomAccess: true,
					Looped:       true,
				},
			},
		},
		Properties: []adapter.PropertyInfoView{},
	}

	productID := Hub2YandexProductID
	assert.EqualValues(t, expectedDeviceInfoView, irControl.ToDeviceInfoView(productID))
}

func TestIRCustomControlTVFullView_ToDeviceInfoView(t *testing.T) {
	irControl := IRControl{
		ID:            "control-id",
		Name:          "My customized control",
		CategoryID:    CustomIrCategoryID,
		BrandID:       "0",
		RemoteIndex:   "some_index",
		TransmitterID: "my-transmitter-01",
		CustomControlData: &IRCustomControl{
			ID:         "control-id",
			Name:       "Мой любимый телек",
			DeviceType: model.KettleDeviceType,
			Buttons: []IRCustomButton{
				{
					Key:  "first_channel",
					Name: "Первый канал",
				},
				{
					Key:  "second_channel",
					Name: "Второй канал",
				},
				{
					Key:  "cool_infrared_signal",
					Name: "Мой крутой сигнал",
				},
			},
		},
	}

	expectedDeviceInfoView := adapter.DeviceInfoView{
		ID:   "control-id",
		Name: "Мой любимый телек",
		Type: model.KettleDeviceType,
		DeviceInfo: &model.DeviceInfo{
			Manufacturer: ptr.String(UnknownDeviceManufacturer),
			Model:        ptr.String(UnknownDeviceModel),
		},
		CustomData: tuya.CustomData{
			DeviceType: model.KettleDeviceType,
			InfraredData: &tuya.InfraredData{
				TransmitterID: "my-transmitter-01",
				Keys: map[tuya.IRKeyName]string{
					"first_channel":        "first_channel",
					"second_channel":       "second_channel",
					"cool_infrared_signal": "cool_infrared_signal",
				},
				Learned: true,
			},
		},
		Capabilities: []adapter.CapabilityInfoView{
			{
				Type:        model.CustomButtonCapabilityType,
				Retrievable: false,
				Parameters: model.CustomButtonCapabilityParameters{
					Instance:      "first_channel",
					InstanceNames: []string{"Первый канал"},
				},
			},
			{
				Type:        model.CustomButtonCapabilityType,
				Retrievable: false,
				Parameters: model.CustomButtonCapabilityParameters{
					Instance:      "second_channel",
					InstanceNames: []string{"Второй канал"},
				},
			},
			{
				Type:        model.CustomButtonCapabilityType,
				Retrievable: false,
				Parameters: model.CustomButtonCapabilityParameters{
					Instance:      "cool_infrared_signal",
					InstanceNames: []string{"Мой крутой сигнал"},
				},
			},
		},
		Properties: []adapter.PropertyInfoView{},
	}

	productID := TuyaDeviceProductID("blabla")
	assert.EqualValues(t, expectedDeviceInfoView, irControl.ToDeviceInfoView(productID))
}

func TestIRControlSendKeysView_FromDeviceActionView(t *testing.T) {
	customData := tuya.CustomData{
		DeviceType: model.TvDeviceDeviceType,
		InfraredData: &tuya.InfraredData{
			BrandName:     "Samsung",
			PresetID:      "1122",
			TransmitterID: "my-transmitter-01",
			Keys: map[tuya.IRKeyName]string{
				PowerKeyName:            "11",
				MuteKeyName:             "5",
				PauseKeyName:            "166",
				ChannelUpKeyName:        "3",
				ChannelDownKeyName:      "1",
				DigitOneKeyName:         "01",
				DigitTwoKeyName:         "02",
				DigitThreeKeyName:       "03",
				DigitFourKeyName:        "04",
				DigitFiveKeyName:        "05",
				DigitSixKeyName:         "06",
				DigitSevenKeyName:       "07",
				DigitEightKeyName:       "08",
				DigitNineKeyName:        "09",
				DigitZeroKeyName:        "00",
				VolumeUpKeyName:         "33",
				VolumeDownKeyName:       "111",
				InputSourceOneKeyName:   "170",
				InputSourceTwoKeyName:   "171",
				InputSourceThreeKeyName: "172",
				InputSourceFourKeyName:  "173",
			},
		},
	}

	deviceActionView := adapter.DeviceActionRequestView{
		ID:         "test-id",
		CustomData: customData,
		Capabilities: []adapter.CapabilityActionView{
			{
				Type: model.OnOffCapabilityType,
				State: model.OnOffCapabilityState{
					Instance: model.OnOnOffCapabilityInstance,
					Value:    true,
				},
			},
			{
				Type: model.RangeCapabilityType,
				State: model.RangeCapabilityState{
					Instance: model.VolumeRangeInstance,
					Relative: ptr.Bool(true),
					Value:    40,
				},
			},
			{
				Type: model.RangeCapabilityType,
				State: model.RangeCapabilityState{
					Instance: model.ChannelRangeInstance,
					Value:    3,
				},
			},
			{
				Type: model.RangeCapabilityType,
				State: model.RangeCapabilityState{
					Instance: model.ChannelRangeInstance,
					Value:    215,
				},
			},
			{
				Type: model.RangeCapabilityType,
				State: model.RangeCapabilityState{
					Instance: model.ChannelRangeInstance,
					Relative: ptr.Bool(true),
					Value:    -1,
				},
			},
			{
				Type: model.ModeCapabilityType,
				State: model.ModeCapabilityState{
					Instance: model.InputSourceModeInstance,
					Value:    model.OneMode,
				},
			},
		},
	}

	testView := IRControlSendKeysView{CustomData: customData}
	if err := testView.FromDeviceActionView(deviceActionView); err != nil {
		t.Error(err)
	}

	expectedView := IRControlSendKeysView{
		ID:         "test-id",
		CustomData: customData,
		Commands: []IRCommand{
			{
				PresetID: "1122",
				KeyID:    "11",
			},
			{
				PresetID: "1122",
				KeyID:    "03",
			},
			{
				PresetID: "1122",
				KeyID:    "02",
			},
			{
				PresetID: "1122",
				KeyID:    "01",
			},
			{
				PresetID: "1122",
				KeyID:    "05",
			},
			{
				PresetID: "1122",
				KeyID:    "1",
			},
			{
				PresetID: "1122",
				KeyID:    "170",
			},
		},
		CustomCommands: []IRCustomCommand{},
		BatchCommands: []IRBatchCommand{
			{
				InfraredID: "my-transmitter-01",
				RemoteID:   "test-id",
				Key:        BatchVolumeUpKey,
				Value:      "40",
			},
		},
	}

	assert.EqualValues(t, expectedView, testView)
}

func TestIRCustomPresetControlSendKeysView_FromDeviceActionView(t *testing.T) {
	customData := tuya.CustomData{
		DeviceType: model.TvDeviceDeviceType,
		InfraredData: &tuya.InfraredData{
			TransmitterID: "my-transmitter-01",
			Keys: map[tuya.IRKeyName]string{
				PowerKeyName:                 "11",
				MuteKeyName:                  "5",
				PauseKeyName:                 "166",
				"somebody_once_told_me":      "some_key",
				"the_world_is_gonna_roll_me": "super_tnt_channel_key",
			},
			Learned: true,
		},
	}

	deviceActionView := adapter.DeviceActionRequestView{
		ID:         "test-id",
		CustomData: customData,
		Capabilities: []adapter.CapabilityActionView{
			{
				Type: model.CustomButtonCapabilityType,
				State: model.CustomButtonCapabilityState{
					Instance: "somebody_once_told_me",
					Value:    true,
				},
			},
			{
				Type: model.CustomButtonCapabilityType,
				State: model.CustomButtonCapabilityState{
					Instance: "the_world_is_gonna_roll_me",
					Value:    true,
				},
			},
		},
	}

	testView := IRControlSendKeysView{CustomData: customData}
	if err := testView.FromDeviceActionView(deviceActionView); err != nil {
		t.Error(err)
	}

	expectedView := IRControlSendKeysView{
		ID:         "test-id",
		CustomData: customData,
		CustomCommands: []IRCustomCommand{
			{
				RemoteID: "test-id",
				Key:      "some_key",
			},
			{
				RemoteID: "test-id",
				Key:      "super_tnt_channel_key",
			},
		},
		Commands:      []IRCommand{},
		BatchCommands: []IRBatchCommand{},
	}

	assert.EqualValues(t, expectedView, testView)
}

func TestAcIRDeviceStateView_FromDeviceActionView(t *testing.T) {
	customData := tuya.CustomData{
		DeviceType: model.TvDeviceDeviceType,
		InfraredData: &tuya.InfraredData{
			BrandName:     "Samsung",
			PresetID:      "1122",
			TransmitterID: "my-transmitter-01",
		},
	}

	deviceActionView := adapter.DeviceActionRequestView{
		ID:         "test-id",
		CustomData: customData,
		Capabilities: []adapter.CapabilityActionView{
			{
				Type: model.OnOffCapabilityType,
				State: model.OnOffCapabilityState{
					Instance: model.OnOnOffCapabilityInstance,
					Value:    true,
				},
			},
			{
				Type: model.RangeCapabilityType,
				State: model.RangeCapabilityState{
					Instance: model.TemperatureRangeInstance,
					Relative: ptr.Bool(false),
					Value:    18,
				},
			},
			{
				Type: model.ModeCapabilityType,
				State: model.ModeCapabilityState{
					Instance: model.ThermostatModeInstance,
					Value:    model.HeatMode,
				},
			},
			{
				Type: model.ModeCapabilityType,
				State: model.ModeCapabilityState{
					Instance: model.FanSpeedModeInstance,
					Value:    model.LowMode,
				},
			},
		},
	}

	testView := AcIRDeviceStateView{
		ID:            "test-id",
		TransmitterID: "my-transmitter-01",
		RemoteIndex:   "1122",
	}
	if err := testView.FromDeviceActionView(deviceActionView); err != nil {
		t.Error(err)
	}

	expectedView := AcIRDeviceStateView{
		ID:            "test-id",
		TransmitterID: "my-transmitter-01",
		RemoteIndex:   "1122",
		Power:         ptr.String("1"),
		Mode:          ptr.String("1"),
		Wind:          ptr.String("1"),
		Temp:          ptr.String("18"),
	}

	assert.Equal(t, expectedView, testView)
}

func TestAcIRDeviceStateView_ToAcStateView(t *testing.T) {
	acIRDeviceStateView := AcIRDeviceStateView{
		ID:            "test-id",
		TransmitterID: "my-transmitter-01",
		RemoteIndex:   "1122",
		Power:         ptr.String("1"),
		Mode:          ptr.String("1"),
		Wind:          ptr.String("1"),
		Temp:          ptr.String("18"),
	}

	expectedAcStateView := AcStateView{
		RemoteID:    "test-id",
		RemoteIndex: "1122",
		Power:       ptr.String("1"),
		Mode:        ptr.String("1"),
		Wind:        ptr.String("1"),
		Temp:        ptr.String("18"),
	}

	assert.Equal(t, expectedAcStateView, acIRDeviceStateView.ToAcStateView())
}

func TestAcStateView_ToString(t *testing.T) {
	original := AcStateView{
		RemoteID:    "test-id",
		RemoteIndex: "55",
		Power:       ptr.String("1"),
		Mode:        ptr.String("0"),
		Temp:        ptr.String("18"),
	}
	test := original

	expected := "{remote_id: test-id, remote_index:55 power:1 mode:0 wind:nil temp:18}"

	assert.Equal(t, expected, test.String())
	assert.Equal(t, original, test)
}

func TestTuyaAcState_ToDeviceStateView(t *testing.T) {
	acState := TuyaAcState{
		RemoteID: "my-remote-id-01",
		Power:    ptr.String("1"),
		Mode:     ptr.String("0"),
		Temp:     ptr.String("18"),
		Wind:     ptr.String("1"),
	}

	expected := adapter.DeviceStateView{
		ID: "my-remote-id-01",
		Capabilities: []adapter.CapabilityStateView{
			{
				Type: model.ModeCapabilityType,
				State: model.ModeCapabilityState{
					Instance: model.ThermostatModeInstance,
					Value:    model.CoolMode,
				},
			},
			{
				Type: model.ModeCapabilityType,
				State: model.ModeCapabilityState{
					Instance: model.FanSpeedModeInstance,
					Value:    model.LowMode,
				},
			},
			{
				Type: model.RangeCapabilityType,
				State: model.RangeCapabilityState{
					Instance: model.TemperatureRangeInstance,
					Value:    18,
				},
			},
			{
				Type: model.OnOffCapabilityType,
				State: model.OnOffCapabilityState{
					Instance: model.OnOnOffCapabilityInstance,
					Value:    true,
				},
			},
		},
	}

	assert.Equal(t, expected, acState.ToDeviceStateView())
}

func TestIRPresetsIntersect(t *testing.T) {
	presets1 := MatchedPresets{
		{
			BrandID:   "haha",
			BrandName: "notFunny",
			PresetID:  "1",
		},
		{
			BrandID:   "12",
			BrandName: "Samsung",
			PresetID:  "2",
		},
		{
			BrandID:   "105",
			BrandName: "Xiaomi",
			PresetID:  "3",
		},
	}

	presets2 := MatchedPresets{
		{
			BrandID:   "serious",
			BrandName: "verySerious",
			PresetID:  "100",
		},
		{
			BrandID:   "12",
			BrandName: "Samsung",
			PresetID:  "2",
		},
		{
			BrandID:   "105",
			BrandName: "Xiaomi",
			PresetID:  "34",
		},
	}

	expected := MatchedPresets{
		{
			BrandID:   "12",
			BrandName: "Samsung",
			PresetID:  "2",
		},
	}

	actual := presets1.Intersect(presets2)
	assert.Equal(t, expected, actual)

	actual2 := presets2.Intersect(presets1)
	assert.Equal(t, expected, actual2)
}

func TestIRPresetsBrandsDuplicating(t *testing.T) {
	// prepare data
	resultSet := matchingRemotesResultSet{
		Brands: []matchingRemotesBrand{
			{
				BrandID:   "first, heh",
				BrandName: "Best brand",
			},
			{
				BrandID:   "second, meh",
				BrandName: "Some brand",
			},
		},
		RemoteIndex: "1111",
	}
	firstPreset, ok := resultSet.toMatchedPreset(AcIrCategoryID)
	assert.True(t, ok)
	assert.Equal(t, firstPreset.BrandName, "Best brand")
	assert.Equal(t, firstPreset.BrandID, "first, heh")
	assert.Equal(t, firstPreset.PresetID, "1111")
	assert.Equal(t, firstPreset.CategoryID, AcIrCategoryID)

	resultSet2 := matchingRemotesResultSet{
		Brands: []matchingRemotesBrand{
			{
				BrandID:   "second, meh",
				BrandName: "Some brand",
			},
			{
				BrandID:   "first, meh",
				BrandName: "Best brand",
			},
		},
		RemoteIndex: "1111",
	}
	secondPreset, ok := resultSet2.toMatchedPreset(SetTopBoxIrCategoryID)
	assert.True(t, ok)
	assert.Equal(t, secondPreset.BrandName, "Some brand")
	assert.Equal(t, secondPreset.BrandID, "second, meh")
	assert.Equal(t, secondPreset.PresetID, "1111")
	assert.Equal(t, secondPreset.CategoryID, SetTopBoxIrCategoryID)

	emptyResultSet := matchingRemotesResultSet{
		Brands:      []matchingRemotesBrand{},
		RemoteIndex: "3333",
	}
	_, ok = emptyResultSet.toMatchedPreset(SetTopBoxIrCategoryID)
	assert.False(t, ok)

	// intersect it
	presets1 := MatchedPresets{
		firstPreset,
	}

	presets2 := MatchedPresets{
		secondPreset,
	}

	expected := MatchedPresets{
		secondPreset,
	}
	actual := presets1.Intersect(presets2)
	assert.Equal(t, expected, actual)

	expected = MatchedPresets{
		firstPreset,
	}
	actual2 := presets2.Intersect(presets1)
	assert.Equal(t, expected, actual2)
}
