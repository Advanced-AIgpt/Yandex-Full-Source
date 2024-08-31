package tuya

import (
	"encoding/json"
	"testing"

	"github.com/mitchellh/mapstructure"
	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/tuya"
	"a.yandex-team.ru/alice/library/go/tools"
)

func getColorLampSample() UserDevice {
	return UserDevice{
		ID:       "some-id-01",
		Name:     "Lamp name",
		Category: TuyaLightDeviceType.ToString(),
		Online:   true,
		Status: []TuyaCommand{
			{
				Code:  "work_mode",
				Value: "colour",
			},
			{
				Code:  "switch_led",
				Value: true,
			},
			{
				Code:  "bright_value",
				Value: 55,
			},
			{
				Code: "colour_data",
				Value: model.HSV{
					H: 360,
					S: 255,
					V: 255,
				},
			},
			{
				Code:  "temp_value",
				Value: 255,
			},
		},
		ProductID: LampYandexProductID,
		FirmwareInfo: DeviceFirmwareInfo{
			{
				CurrentVersion: "1.0",
				ModuleType:     FirmwareInfoWIFIModuleType,
			},
		},
	}
}

func getColorYeelightLampSample() UserDevice {
	return UserDevice{
		ID:       "some-id-01",
		Name:     "Lamp name",
		Category: TuyaLightDeviceType.ToString(),
		Online:   true,
		Status: []TuyaCommand{
			{
				Code:  "work_mode",
				Value: "colour",
			},
			{
				Code:  "switch_led",
				Value: true,
			},
			{
				Code:  "bright_value_v2",
				Value: 1000,
			},
			{
				Code: "colour_data_v2",
				Value: model.HSV{
					H: 360,
					S: 1000,
					V: 1000,
				},
			},
			{
				Code:  "temp_value_v2",
				Value: 1000,
			},
			{
				Code:  "scene_data_v2",
				Value: "{}",
			},
		},
		ProductID: LampE27Lemon2YandexProductID,
		FirmwareInfo: DeviceFirmwareInfo{
			{
				CurrentVersion: "1.0.15",
				ModuleType:     FirmwareInfoWIFIModuleType,
			},
		},
	}
}

func getColorYeelightLampV3Sample() UserDevice {
	return UserDevice{
		ID:       "some-id-01",
		Name:     "Lamp name",
		Category: TuyaLightDeviceType.ToString(),
		Online:   true,
		Status: []TuyaCommand{
			{
				Code:  "work_mode",
				Value: "colour",
			},
			{
				Code:  "switch_led",
				Value: true,
			},
			{
				Code:  "bright_value_v2",
				Value: 1000,
			},
			{
				Code: "colour_data_v2",
				Value: model.HSV{
					H: 360,
					S: 1000,
					V: 1000,
				},
			},
			{
				Code:  "temp_value_v2",
				Value: 1000,
			},
			{
				Code:  "scene_data_v2",
				Value: "{}",
			},
		},
		ProductID: LampE27Lemon3YandexProductID,
		FirmwareInfo: DeviceFirmwareInfo{
			{
				CurrentVersion: "1.0.2",
				ModuleType:     FirmwareInfoWIFIModuleType,
			},
		},
	}
}

func getYeelightTuyaCheapLampE27Sample() UserDevice {
	return UserDevice{
		ID:       "some-id-01",
		Name:     "Lamp name",
		Category: TuyaLightDeviceType.ToString(),
		Online:   true,
		Status: []TuyaCommand{
			{
				Code:  "work_mode",
				Value: "colour",
			},
			{
				Code:  "switch_led",
				Value: true,
			},
			{
				Code:  "bright_value_v2",
				Value: 1000,
			},
			{
				Code:  "temp_value_v2",
				Value: 1000,
			},
			{
				Code:  "scene_data_v2",
				Value: "{}",
			},
		},
		ProductID: LampE27A60YandexProductID,
		FirmwareInfo: DeviceFirmwareInfo{
			{
				CurrentVersion: "0.0.1",
				ModuleType:     FirmwareInfoWIFIModuleType,
			},
		},
	}
}

func getColorYeelightLampE14Sample() UserDevice {
	return UserDevice{
		ID:       "some-id-01",
		Name:     "Lamp name",
		Category: TuyaLightDeviceType.ToString(),
		Online:   true,
		Status: []TuyaCommand{
			{
				Code:  "work_mode",
				Value: "colour",
			},
			{
				Code:  "switch_led",
				Value: true,
			},
			{
				Code:  "bright_value_v2",
				Value: 1000,
			},
			{
				Code: "colour_data_v2",
				Value: model.HSV{
					H: 360,
					S: 1000,
					V: 1000,
				},
			},
			{
				Code:  "temp_value_v2",
				Value: 1000,
			},
			{
				Code:  "scene_data_v2",
				Value: "{}",
			},
		},
		ProductID: LampE14Test2YandexProductID,
		FirmwareInfo: DeviceFirmwareInfo{
			{
				CurrentVersion: "2.9.16",
				ModuleType:     FirmwareInfoWIFIModuleType,
			},
		},
	}
}

func getColorYeelightLamp3E14Sample() UserDevice {
	return UserDevice{
		ID:       "some-id-01",
		Name:     "Lamp name",
		Category: TuyaLightDeviceType.ToString(),
		Online:   true,
		Status: []TuyaCommand{
			{
				Code:  "work_mode",
				Value: "colour",
			},
			{
				Code:  "switch_led",
				Value: true,
			},
			{
				Code:  "bright_value_v2",
				Value: 1000,
			},
			{
				Code: "colour_data_v2",
				Value: model.HSV{
					H: 360,
					S: 1000,
					V: 1000,
				},
			},
			{
				Code:  "temp_value_v2",
				Value: 1000,
			},
			{
				Code:  "scene_data_v2",
				Value: "{}",
			},
		},
		ProductID: LampE14MPYandexProductID,
		FirmwareInfo: DeviceFirmwareInfo{
			{
				CurrentVersion: "1.0.2",
				ModuleType:     FirmwareInfoWIFIModuleType,
			},
		},
	}
}

func getColorYeelightLampGU10Sample() UserDevice {
	return UserDevice{
		ID:       "some-id-01",
		Name:     "Lamp name",
		Category: TuyaLightDeviceType.ToString(),
		Online:   true,
		Status: []TuyaCommand{
			{
				Code:  "work_mode",
				Value: "colour",
			},
			{
				Code:  "switch_led",
				Value: true,
			},
			{
				Code:  "bright_value_v2",
				Value: 1000,
			},
			{
				Code: "colour_data_v2",
				Value: model.HSV{
					H: 360,
					S: 1000,
					V: 1000,
				},
			},
			{
				Code:  "temp_value_v2",
				Value: 1000,
			},
			{
				Code:  "scene_data_v2",
				Value: "{}",
			},
		},
		ProductID: LampGU10TestYandexProductID,
		FirmwareInfo: DeviceFirmwareInfo{
			{
				CurrentVersion: "1.2.16",
				ModuleType:     FirmwareInfoWIFIModuleType,
			},
		},
	}
}

func getColorYeelightLamp2GU10Sample() UserDevice {
	return UserDevice{
		ID:       "some-id-01",
		Name:     "Lamp name",
		Category: TuyaLightDeviceType.ToString(),
		Online:   true,
		Status: []TuyaCommand{
			{
				Code:  "work_mode",
				Value: "colour",
			},
			{
				Code:  "switch_led",
				Value: true,
			},
			{
				Code:  "bright_value_v2",
				Value: 1000,
			},
			{
				Code: "colour_data_v2",
				Value: model.HSV{
					H: 360,
					S: 1000,
					V: 1000,
				},
			},
			{
				Code:  "temp_value_v2",
				Value: 1000,
			},
			{
				Code:  "scene_data_v2",
				Value: "{}",
			},
		},
		ProductID: LampGU10MPYandexProductID,
		FirmwareInfo: DeviceFirmwareInfo{
			{
				CurrentVersion: "1.2.16",
				ModuleType:     FirmwareInfoWIFIModuleType,
			},
		},
	}
}

func getColorSberE27LampSample() UserDevice {
	return UserDevice{
		ID:       "some-id-01",
		Name:     "Lamp name",
		Category: TuyaLightDeviceType.ToString(),
		Online:   true,
		Status: []TuyaCommand{
			{
				Code:  "work_mode",
				Value: "colour",
			},
			{
				Code:  "switch_led",
				Value: true,
			},
			{
				Code:  "bright_value_v2",
				Value: 1000,
			},
			{
				Code: "colour_data_v2",
				Value: model.HSV{
					H: 360,
					S: 1000,
					V: 1000,
				},
			},
			{
				Code:  "temp_value_v2",
				Value: 1000,
			},
		},
		ProductID: LampE27SberProductID,
		FirmwareInfo: DeviceFirmwareInfo{
			{
				CurrentVersion: "1.0",
				ModuleType:     FirmwareInfoWIFIModuleType,
			},
		},
	}
}

func getColorSberE27v2LampSample() UserDevice {
	return UserDevice{
		ID:       "some-id-01",
		Name:     "Lamp name",
		Category: TuyaLightDeviceType.ToString(),
		Online:   true,
		Status: []TuyaCommand{
			{
				Code:  "work_mode",
				Value: "colour",
			},
			{
				Code:  "switch_led",
				Value: true,
			},
			{
				Code:  "bright_value_v2",
				Value: 1000,
			},
			{
				Code: "colour_data_v2",
				Value: model.HSV{
					H: 360,
					S: 1000,
					V: 1000,
				},
			},
			{
				Code:  "temp_value_v2",
				Value: 1000,
			},
		},
		ProductID: Lamp2E27SberProductID,
		FirmwareInfo: DeviceFirmwareInfo{
			{
				CurrentVersion: "2.0",
				ModuleType:     FirmwareInfoWIFIModuleType,
			},
		},
	}
}

func getColorSberE27v3LampSample() UserDevice {
	return UserDevice{
		ID:       "some-id-01",
		Name:     "Lamp name",
		Category: TuyaLightDeviceType.ToString(),
		Online:   true,
		Status: []TuyaCommand{
			{
				Code:  "work_mode",
				Value: "colour",
			},
			{
				Code:  "switch_led",
				Value: true,
			},
			{
				Code:  "bright_value_v2",
				Value: 1000,
			},
			{
				Code: "colour_data_v2",
				Value: model.HSV{
					H: 360,
					S: 1000,
					V: 1000,
				},
			},
			{
				Code:  "temp_value_v2",
				Value: 1000,
			},
		},
		ProductID: Lamp3E27SberProductID,
		FirmwareInfo: DeviceFirmwareInfo{
			{
				CurrentVersion: "2.0",
				ModuleType:     FirmwareInfoWIFIModuleType,
			},
		},
	}
}

func getColorSberE27v4LampSample() UserDevice {
	return UserDevice{
		ID:       "some-id-01",
		Name:     "Lamp name",
		Category: TuyaLightDeviceType.ToString(),
		Online:   true,
		Status: []TuyaCommand{
			{
				Code:  "work_mode",
				Value: "colour",
			},
			{
				Code:  "switch_led",
				Value: true,
			},
			{
				Code:  "bright_value_v2",
				Value: 1000,
			},
			{
				Code: "colour_data_v2",
				Value: model.HSV{
					H: 0,
					S: 1000,
					V: 1000,
				},
			},
			{
				Code:  "temp_value_v2",
				Value: 0,
			},
		},
		ProductID: Lamp4E27SberProductID,
		FirmwareInfo: DeviceFirmwareInfo{
			{
				CurrentVersion: "2.0",
				ModuleType:     FirmwareInfoWIFIModuleType,
			},
		},
	}
}

func getColorSberE27v5LampSample() UserDevice {
	return UserDevice{
		ID:       "some-id-01",
		Name:     "Lamp name",
		Category: TuyaLightDeviceType.ToString(),
		Online:   true,
		Status: []TuyaCommand{
			{
				Code:  "work_mode",
				Value: "colour",
			},
			{
				Code:  "switch_led",
				Value: true,
			},
			{
				Code:  "bright_value_v2",
				Value: 1000,
			},
			{
				Code: "colour_data_v2",
				Value: model.HSV{
					H: 0,
					S: 1000,
					V: 1000,
				},
			},
			{
				Code:  "temp_value_v2",
				Value: 0,
			},
		},
		ProductID: Lamp5E27SberProductID,
		FirmwareInfo: DeviceFirmwareInfo{
			{
				CurrentVersion: "3.0",
				ModuleType:     FirmwareInfoWIFIModuleType,
			},
		},
	}
}

func getColorSberE14LampSample() UserDevice {
	return UserDevice{
		ID:       "some-id-01",
		Name:     "Lamp name",
		Category: TuyaLightDeviceType.ToString(),
		Online:   true,
		Status: []TuyaCommand{
			{
				Code:  "work_mode",
				Value: "colour",
			},
			{
				Code:  "switch_led",
				Value: true,
			},
			{
				Code:  "bright_value_v2",
				Value: 1000,
			},
			{
				Code: "colour_data_v2",
				Value: model.HSV{
					H: 360,
					S: 1000,
					V: 1000,
				},
			},
			{
				Code:  "temp_value_v2",
				Value: 1000,
			},
		},
		ProductID: LampE14SberProductID,
		FirmwareInfo: DeviceFirmwareInfo{
			{
				CurrentVersion: "1.0",
				ModuleType:     FirmwareInfoWIFIModuleType,
			},
		},
	}
}

func getColorSberE14v2LampSample() UserDevice {
	return UserDevice{
		ID:       "some-id-01",
		Name:     "Lamp name",
		Category: TuyaLightDeviceType.ToString(),
		Online:   true,
		Status: []TuyaCommand{
			{
				Code:  "work_mode",
				Value: "colour",
			},
			{
				Code:  "switch_led",
				Value: true,
			},
			{
				Code:  "bright_value_v2",
				Value: 1000,
			},
			{
				Code: "colour_data_v2",
				Value: model.HSV{
					H: 360,
					S: 1000,
					V: 1000,
				},
			},
			{
				Code:  "temp_value_v2",
				Value: 1000,
			},
		},
		ProductID: Lamp2E14SberProductID,
		FirmwareInfo: DeviceFirmwareInfo{
			{
				CurrentVersion: "2.0",
				ModuleType:     FirmwareInfoWIFIModuleType,
			},
		},
	}
}

func getColorSberE14v3LampSample() UserDevice {
	return UserDevice{
		ID:       "some-id-01",
		Name:     "Lamp name",
		Category: TuyaLightDeviceType.ToString(),
		Online:   true,
		Status: []TuyaCommand{
			{
				Code:  "work_mode",
				Value: "colour",
			},
			{
				Code:  "switch_led",
				Value: true,
			},
			{
				Code:  "bright_value_v2",
				Value: 1000,
			},
			{
				Code: "colour_data_v2",
				Value: model.HSV{
					H: 360,
					S: 1000,
					V: 1000,
				},
			},
			{
				Code:  "temp_value_v2",
				Value: 1000,
			},
		},
		ProductID: Lamp3E14SberProductID,
		FirmwareInfo: DeviceFirmwareInfo{
			{
				CurrentVersion: "2.0",
				ModuleType:     FirmwareInfoWIFIModuleType,
			},
		},
	}
}

func getColorSberE14v4LampSample() UserDevice {
	return UserDevice{
		ID:       "some-id-01",
		Name:     "Lamp name",
		Category: TuyaLightDeviceType.ToString(),
		Online:   true,
		Status: []TuyaCommand{
			{
				Code:  "work_mode",
				Value: "colour",
			},
			{
				Code:  "switch_led",
				Value: true,
			},
			{
				Code:  "bright_value_v2",
				Value: 1000,
			},
			{
				Code: "colour_data_v2",
				Value: model.HSV{
					H: 360,
					S: 1000,
					V: 1000,
				},
			},
			{
				Code:  "temp_value_v2",
				Value: 1000,
			},
		},
		ProductID: Lamp4E14SberProductID,
		FirmwareInfo: DeviceFirmwareInfo{
			{
				CurrentVersion: "3.0",
				ModuleType:     FirmwareInfoWIFIModuleType,
			},
		},
	}
}

func getColorSberGU10LampSample() UserDevice {
	return UserDevice{
		ID:       "some-id-01",
		Name:     "Lamp name",
		Category: TuyaLightDeviceType.ToString(),
		Online:   true,
		Status: []TuyaCommand{
			{
				Code:  "work_mode",
				Value: "colour",
			},
			{
				Code:  "switch_led",
				Value: true,
			},
			{
				Code:  "bright_value_v2",
				Value: 1000,
			},
			{
				Code: "colour_data_v2",
				Value: model.HSV{
					H: 360,
					S: 1000,
					V: 1000,
				},
			},
			{
				Code:  "temp_value_v2",
				Value: 1000,
			},
		},
		ProductID: LampGU10SberProductID,
		FirmwareInfo: DeviceFirmwareInfo{
			{
				CurrentVersion: "2.0",
				ModuleType:     FirmwareInfoWIFIModuleType,
			},
		},
	}
}

func getWhiteLampSample() UserDevice {
	return UserDevice{
		ID:       "some-id-02",
		Name:     "Lamp name 2",
		Category: TuyaLightDeviceType.ToString(),
		Online:   true,
		Status: []TuyaCommand{
			{
				Code:  "work_mode",
				Value: "white",
			},
			{
				Code:  "led_switch",
				Value: false,
			},
			{
				Code:  "bright_value",
				Value: 55,
			},
			{
				Code:  "temp_value",
				Value: 120,
			},
		},
		ProductID: LampYandexProductID,
		FirmwareInfo: DeviceFirmwareInfo{
			{
				CurrentVersion: "1.0",
				ModuleType:     FirmwareInfoWIFIModuleType,
			},
		},
	}
}

func getSocketSample() UserDevice {
	return UserDevice{
		ID:       "some-id-03",
		Name:     "Lamp socket",
		Category: TuyaSocketDeviceType.ToString(),
		Online:   true,
		Status: []TuyaCommand{
			{
				Code:  "switch",
				Value: true,
			},
			{
				Code:  "switch_1",
				Value: false,
			},
			{
				Code:  "switch_2",
				Value: false,
			},
			{
				Code:  "cur_voltage",
				Value: 2200,
			},
			{
				Code:  "cur_current",
				Value: 30,
			},
			{
				Code:  "cur_power",
				Value: 35,
			},
		},
		ProductID: SocketYandexProductID,
		FirmwareInfo: DeviceFirmwareInfo{
			{
				CurrentVersion: "1.0",
				ModuleType:     FirmwareInfoMCUModuleType,
			},
		},
	}
}

func getSocketSberSample() UserDevice {
	return UserDevice{
		ID:       "some-id-03",
		Name:     "Lamp socket",
		Category: TuyaSocketDeviceType.ToString(),
		Online:   true,
		Status: []TuyaCommand{
			{
				Code:  "switch",
				Value: true,
			},
			{
				Code:  "switch_1",
				Value: false,
			},
			{
				Code:  "switch_2",
				Value: false,
			},
			{
				Code:  "cur_voltage",
				Value: 2200,
			},
			{
				Code:  "cur_current",
				Value: 30,
			},
			{
				Code:  "cur_power",
				Value: 35,
			},
		},
		ProductID: SocketSberProductID,
		FirmwareInfo: DeviceFirmwareInfo{
			{
				CurrentVersion: "1.0",
				ModuleType:     FirmwareInfoMCUModuleType,
			},
		},
	}
}

func getIRSample() UserDevice {
	return UserDevice{
		ID:       "some-ir-id",
		Name:     "ir hub",
		Category: TuyaIRDeviceType.ToString(),
		Online:   true,
		Status: []TuyaCommand{
			{
				Code:  "key_study4",
				Value: "",
			},
			{
				Code:  "ir_code",
				Value: "",
			},
			{
				Code:  "control",
				Value: "study_key",
			},
		},
		ProductID: HubYandexProductID,
		FirmwareInfo: DeviceFirmwareInfo{
			{
				CurrentVersion: "1.0",
				ModuleType:     FirmwareInfoMCUModuleType,
			},
		},
	}
}

func getIR2Sample() UserDevice {
	return UserDevice{
		ID:       "some-ir-id",
		Name:     "ir hub",
		Category: TuyaIRDeviceType2.ToString(),
		Online:   true,
		Status: []TuyaCommand{
			{
				Code:  "key_study4",
				Value: "",
			},
			{
				Code:  "ir_code",
				Value: "",
			},
			{
				Code:  "control",
				Value: "study_key",
			},
		},
		ProductID: Hub2YandexProductID,
		FirmwareInfo: DeviceFirmwareInfo{
			{
				CurrentVersion: "1.0",
				ModuleType:     FirmwareInfoMCUModuleType,
			},
		},
	}
}

func TestUserDevice_GetMainSwitchStatusItem(t *testing.T) {
	// Basic socket with `switch` command
	testDevice := getSocketSample()
	expectedCommand := TuyaCommand{
		Code:  "switch",
		Value: true,
	}

	switchCommand, _ := testDevice.GetMainSwitchStatusItem()
	assert.Equal(t, expectedCommand, switchCommand)

	// Lamp with `switch_led` command
	testDevice = getColorLampSample()
	expectedCommand = TuyaCommand{
		Code:  "switch_led",
		Value: true,
	}

	switchCommand, _ = testDevice.GetMainSwitchStatusItem()
	assert.Equal(t, expectedCommand, switchCommand)

	// Lamp with `led_switch` command
	testDevice = getWhiteLampSample()
	expectedCommand = TuyaCommand{
		Code:  "led_switch",
		Value: false,
	}

	switchCommand, _ = testDevice.GetMainSwitchStatusItem()
	assert.Equal(t, expectedCommand, switchCommand)

	// IR without `switch` command
	testDevice = getIRSample()
	_, exists := testDevice.GetMainSwitchStatusItem()
	assert.False(t, exists)
}

func TestUserDevice_ToDeviceStateView(t *testing.T) {
	// CASE 1: Color lamp
	tuyaColorLamp := getColorLampSample()

	expectedColorLampState := adapter.DeviceStateView{
		ID: "some-id-01",
		Capabilities: []adapter.CapabilityStateView{
			{
				Type: model.RangeCapabilityType,
				State: model.RangeCapabilityState{
					Instance: model.BrightnessRangeInstance,
					Value:    100,
				},
			},
			{
				Type: model.ColorSettingCapabilityType,
				State: model.ColorSettingCapabilityState{
					Instance: model.HsvColorCapabilityInstance,
					Value: model.HSV{
						H: 360,
						S: 100,
						V: 100,
					},
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
		Properties: []adapter.PropertyStateView{},
	}

	assert.Equal(t, expectedColorLampState, tuyaColorLamp.ToDeviceStateView())

	// CASE 2: White lamp
	tuyaWhiteLamp := getWhiteLampSample()

	expectedWhiteLampState := adapter.DeviceStateView{
		ID: "some-id-02",
		Capabilities: []adapter.CapabilityStateView{
			{
				Type: model.RangeCapabilityType,
				State: model.RangeCapabilityState{
					Instance: model.BrightnessRangeInstance,
					Value:    17,
				},
			},
			{
				Type: model.ColorSettingCapabilityType,
				State: model.ColorSettingCapabilityState{
					Instance: model.TemperatureKCapabilityInstance,
					Value:    model.TemperatureK(4500),
				},
			},
			{
				Type: model.OnOffCapabilityType,
				State: model.OnOffCapabilityState{
					Instance: model.OnOnOffCapabilityInstance,
					Value:    false,
				},
			},
		},
		Properties: []adapter.PropertyStateView{},
	}

	assert.Equal(t, expectedWhiteLampState, tuyaWhiteLamp.ToDeviceStateView())

	// CASE 3: Socket
	tuyaSocket := getSocketSample()

	expectedSocketState := adapter.DeviceStateView{
		ID: "some-id-03",
		Capabilities: []adapter.CapabilityStateView{
			{
				Type: model.OnOffCapabilityType,
				State: model.OnOffCapabilityState{
					Instance: model.OnOnOffCapabilityInstance,
					Value:    true,
				},
			},
		},
		Properties: []adapter.PropertyStateView{
			{
				Type: model.FloatPropertyType,
				State: model.FloatPropertyState{
					Instance: model.VoltagePropertyInstance,
					Value:    220,
				},
			},
			{
				Type: model.FloatPropertyType,
				State: model.FloatPropertyState{
					Instance: model.PowerPropertyInstance,
					Value:    3.5,
				},
			},
			{
				Type: model.FloatPropertyType,
				State: model.FloatPropertyState{
					Instance: model.AmperagePropertyInstance,
					Value:    0.03,
				},
			},
		},
	}

	assert.Equal(t, expectedSocketState, tuyaSocket.ToDeviceStateView())

	// CASE 4: IR Transmitter
	tuyaIR := getIRSample()

	expectedIRState := adapter.DeviceStateView{
		ID:           "some-ir-id",
		Capabilities: []adapter.CapabilityStateView{},
		Properties:   []adapter.PropertyStateView{},
	}

	assert.Equal(t, expectedIRState, tuyaIR.ToDeviceStateView())

	// CASE 5: Color Yeelight lamp
	yeelightColorLamp := getColorYeelightLampSample()

	expectedYeelightColorLampState := adapter.DeviceStateView{
		ID: "some-id-01",
		Capabilities: []adapter.CapabilityStateView{
			{
				Type: model.RangeCapabilityType,
				State: model.RangeCapabilityState{
					Instance: model.BrightnessRangeInstance,
					Value:    100,
				},
			},
			{
				Type: model.ColorSettingCapabilityType,
				State: model.ColorSettingCapabilityState{
					Instance: model.HsvColorCapabilityInstance,
					Value: model.HSV{
						H: 360,
						S: 100,
						V: 100,
					},
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
		Properties: []adapter.PropertyStateView{},
	}

	assert.Equal(t, expectedYeelightColorLampState, yeelightColorLamp.ToDeviceStateView())
}

func TestUserDevice_ToDeviceInfoView(t *testing.T) {
	// CASE 1: Color lamp
	tuyaColorLamp := getColorLampSample()

	expectedColorLampView := adapter.DeviceInfoView{
		ID:   "some-id-01",
		Name: "Лампочка",
		Type: model.LightDeviceType,
		Capabilities: []adapter.CapabilityInfoView{
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.RangeCapabilityType,
				Parameters: model.RangeCapabilityParameters{
					Instance:     model.BrightnessRangeInstance,
					Unit:         model.UnitPercent,
					RandomAccess: true,
					Range: &model.Range{
						Min:       1,
						Max:       100,
						Precision: 1,
					},
				},
			},
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.ColorSettingCapabilityType,
				Parameters: model.ColorSettingCapabilityParameters{
					TemperatureK: &model.TemperatureKParameters{
						Min: 2700,
						Max: 6500,
					},
					ColorModel: model.CM(model.HsvModelType),
				},
			},
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.OnOffCapabilityType,
			},
		},
		Properties: []adapter.PropertyInfoView{},
		CustomData: tuya.CustomData{
			DeviceType:    model.LightDeviceType,
			SwitchCommand: string(SwitchLedCommand),
			ProductID:     tools.AOS(string(LampYandexProductID)),
			FwVersion:     tools.AOS("1.0"),
		},
		DeviceInfo: &model.DeviceInfo{
			Manufacturer: tools.AOS("Yandex Services AG"),
			Model:        tools.AOS("YNDX-0005"),
			SwVersion:    tools.AOS("1.0"),
			HwVersion:    tools.AOS("1.0"),
		},
	}

	assert.EqualValues(t, expectedColorLampView, tuyaColorLamp.ToDeviceInfoView())

	// CASE 2: White lamp
	tuyaWhiteLamp := getWhiteLampSample()

	expectedWhiteLampView := adapter.DeviceInfoView{
		ID:   "some-id-02",
		Name: "Лампочка",
		Type: model.LightDeviceType,
		Capabilities: []adapter.CapabilityInfoView{
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.RangeCapabilityType,
				Parameters: model.RangeCapabilityParameters{
					Instance:     model.BrightnessRangeInstance,
					Unit:         model.UnitPercent,
					RandomAccess: true,
					Range: &model.Range{
						Min:       1,
						Max:       100,
						Precision: 1,
					},
				},
			},
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.ColorSettingCapabilityType,
				Parameters: model.ColorSettingCapabilityParameters{
					TemperatureK: &model.TemperatureKParameters{
						Min: 2700,
						Max: 6500,
					},
				},
			},
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.OnOffCapabilityType,
			},
		},
		Properties: []adapter.PropertyInfoView{},
		CustomData: tuya.CustomData{
			DeviceType:    model.LightDeviceType,
			SwitchCommand: string(LedSwitchCommand),
			ProductID:     tools.AOS(string(LampYandexProductID)),
			FwVersion:     tools.AOS("1.0"),
		},
		DeviceInfo: &model.DeviceInfo{
			Manufacturer: tools.AOS("Yandex Services AG"),
			Model:        tools.AOS("YNDX-0005"),
			SwVersion:    tools.AOS("1.0"),
			HwVersion:    tools.AOS("1.0"),
		},
	}

	assert.EqualValues(t, expectedWhiteLampView, tuyaWhiteLamp.ToDeviceInfoView())

	// CASE 3: Socket
	tuyaSocket := getSocketSample()

	expectedSocketView := adapter.DeviceInfoView{
		ID:   "some-id-03",
		Name: "Розетка",
		Type: model.SocketDeviceType,
		Capabilities: []adapter.CapabilityInfoView{
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.OnOffCapabilityType,
			},
		},
		Properties: []adapter.PropertyInfoView{
			{
				Type:        model.FloatPropertyType,
				Retrievable: true,
				Reportable:  true,
				Parameters: model.FloatPropertyParameters{
					Instance: model.VoltagePropertyInstance,
					Unit:     model.UnitVolt,
				},
			},
			{
				Type:        model.FloatPropertyType,
				Retrievable: true,
				Reportable:  true,
				Parameters: model.FloatPropertyParameters{
					Instance: model.PowerPropertyInstance,
					Unit:     model.UnitWatt,
				},
			},
			{
				Type:        model.FloatPropertyType,
				Retrievable: true,
				Reportable:  true,
				Parameters: model.FloatPropertyParameters{
					Instance: model.AmperagePropertyInstance,
					Unit:     model.UnitAmpere,
				},
			},
		},
		CustomData: tuya.CustomData{
			DeviceType:    model.SocketDeviceType,
			SwitchCommand: string(SwitchCommand),
			ProductID:     tools.AOS(string(SocketYandexProductID)),
			FwVersion:     tools.AOS("1.0"),
		},
		DeviceInfo: &model.DeviceInfo{
			Manufacturer: tools.AOS("Yandex Services AG"),
			Model:        tools.AOS("YNDX-0007"),
			SwVersion:    tools.AOS("1.0"),
			HwVersion:    tools.AOS("1.0"),
		},
	}

	assert.EqualValues(t, expectedSocketView, tuyaSocket.ToDeviceInfoView())

	// CASE 4: Hub
	tuyaIR := getIRSample()

	expectedIRView := adapter.DeviceInfoView{
		ID:           "some-ir-id",
		Name:         "Пульт",
		Type:         model.HubDeviceType,
		Capabilities: []adapter.CapabilityInfoView{},
		Properties:   []adapter.PropertyInfoView{},
		CustomData: tuya.CustomData{
			DeviceType: model.HubDeviceType,
			ProductID:  tools.AOS(string(HubYandexProductID)),
			FwVersion:  tools.AOS("1.0"),
		},
		DeviceInfo: &model.DeviceInfo{
			Manufacturer: tools.AOS("Yandex Services AG"),
			Model:        tools.AOS("YNDX-0006"),
			SwVersion:    tools.AOS("1.0"),
			HwVersion:    tools.AOS("1.0"),
		},
	}
	assert.EqualValues(t, expectedIRView, tuyaIR.ToDeviceInfoView())

	// CASE 5: Not Yandex Socket
	tuyaSocket = getSocketSample()
	tuyaSocket.ProductID = "blablablbalbalbl"

	expectedSocketView = adapter.DeviceInfoView{
		ID:   "some-id-03",
		Name: "Розетка",
		Type: model.SocketDeviceType,
		Capabilities: []adapter.CapabilityInfoView{
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.OnOffCapabilityType,
			},
		},
		Properties: []adapter.PropertyInfoView{
			{
				Type:        model.FloatPropertyType,
				Retrievable: true,
				Reportable:  true,
				Parameters: model.FloatPropertyParameters{
					Instance: model.VoltagePropertyInstance,
					Unit:     model.UnitVolt,
				},
			},
			{
				Type:        model.FloatPropertyType,
				Retrievable: true,
				Reportable:  true,
				Parameters: model.FloatPropertyParameters{
					Instance: model.PowerPropertyInstance,
					Unit:     model.UnitWatt,
				},
			},
			{
				Type:        model.FloatPropertyType,
				Retrievable: true,
				Reportable:  true,
				Parameters: model.FloatPropertyParameters{
					Instance: model.AmperagePropertyInstance,
					Unit:     model.UnitAmpere,
				},
			},
		},
		CustomData: tuya.CustomData{
			DeviceType:    model.SocketDeviceType,
			SwitchCommand: string(SwitchCommand),
			ProductID:     tools.AOS("blablablbalbalbl"),
			FwVersion:     tools.AOS("1.0"),
		},
		DeviceInfo: &model.DeviceInfo{
			Manufacturer: tools.AOS(UnknownDeviceManufacturer),
			Model:        tools.AOS(UnknownDeviceModel),
			SwVersion:    tools.AOS("1.0"),
		},
	}
	assert.EqualValues(t, expectedSocketView, tuyaSocket.ToDeviceInfoView())

	// CASE 6: No Firmware Info in Lamp
	tuyaColorLamp = getColorLampSample()
	tuyaColorLamp.FirmwareInfo = make(DeviceFirmwareInfo, 0)

	expectedColorLampView = adapter.DeviceInfoView{
		ID:   "some-id-01",
		Name: "Лампочка",
		Type: model.LightDeviceType,
		Capabilities: []adapter.CapabilityInfoView{
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.RangeCapabilityType,
				Parameters: model.RangeCapabilityParameters{
					Instance:     model.BrightnessRangeInstance,
					Unit:         model.UnitPercent,
					RandomAccess: true,
					Range: &model.Range{
						Min:       1,
						Max:       100,
						Precision: 1,
					},
				},
			},
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.ColorSettingCapabilityType,
				Parameters: model.ColorSettingCapabilityParameters{
					TemperatureK: &model.TemperatureKParameters{
						Min: 2700,
						Max: 6500,
					},
					ColorModel: model.CM(model.HsvModelType),
				},
			},
			{
				Retrievable: true,
				Type:        model.OnOffCapabilityType,
			},
		},
		CustomData: tuya.CustomData{
			DeviceType:    model.LightDeviceType,
			SwitchCommand: string(SwitchLedCommand),
			ProductID:     tools.AOS(string(LampYandexProductID)),
			FwVersion:     tools.AOS("1.0"),
		},
		DeviceInfo: &model.DeviceInfo{
			Manufacturer: tools.AOS("Yandex Services AG"),
			Model:        tools.AOS("YNDX-0005"),
			HwVersion:    tools.AOS("1.0"),
		},
	}
	assert.EqualValues(t, expectedIRView, tuyaIR.ToDeviceInfoView())

	// CASE 7: Yandex Yeelight Color lamp
	yeelightColorLamp := getColorYeelightLampSample()

	expectedYeelightColorLampView := adapter.DeviceInfoView{
		ID:   "some-id-01",
		Name: "Лампочка",
		Type: model.LightDeviceType,
		Capabilities: []adapter.CapabilityInfoView{
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.RangeCapabilityType,
				Parameters: model.RangeCapabilityParameters{
					Instance:     model.BrightnessRangeInstance,
					Unit:         model.UnitPercent,
					RandomAccess: true,
					Range: &model.Range{
						Min:       1,
						Max:       100,
						Precision: 1,
					},
				},
			},
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.ColorSettingCapabilityType,
				Parameters: model.ColorSettingCapabilityParameters{
					TemperatureK: &model.TemperatureKParameters{
						Min: 1500,
						Max: 6500,
					},
					ColorModel:           model.CM(model.HsvModelType),
					ColorSceneParameters: GetColorSceneParameters(YeelightScenePool),
				},
			},
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.OnOffCapabilityType,
			},
		},
		Properties: []adapter.PropertyInfoView{},
		CustomData: tuya.CustomData{
			DeviceType:    model.LightDeviceType,
			SwitchCommand: string(SwitchLedCommand),
			ProductID:     tools.AOS(string(LampE27Lemon2YandexProductID)),
			FwVersion:     tools.AOS("1.0.15"),
		},
		DeviceInfo: &model.DeviceInfo{
			Manufacturer: tools.AOS("Yandex Services AG"),
			Model:        tools.AOS("YNDX-00010"),
			SwVersion:    tools.AOS("1.0.15"),
			HwVersion:    tools.AOS("2.0"),
		},
	}

	assert.EqualValues(t, expectedYeelightColorLampView, yeelightColorLamp.ToDeviceInfoView())

	// CASE 8: Yandex Yeelight Color lamp V3
	yeelightColorLampV3 := getColorYeelightLampV3Sample()

	expectedYeelightColorLampV3View := adapter.DeviceInfoView{
		ID:   "some-id-01",
		Name: "Лампочка",
		Type: model.LightDeviceType,
		Capabilities: []adapter.CapabilityInfoView{
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.RangeCapabilityType,
				Parameters: model.RangeCapabilityParameters{
					Instance:     model.BrightnessRangeInstance,
					Unit:         model.UnitPercent,
					RandomAccess: true,
					Range: &model.Range{
						Min:       1,
						Max:       100,
						Precision: 1,
					},
				},
			},
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.ColorSettingCapabilityType,
				Parameters: model.ColorSettingCapabilityParameters{
					TemperatureK: &model.TemperatureKParameters{
						Min: 1500,
						Max: 6500,
					},
					ColorModel:           model.CM(model.HsvModelType),
					ColorSceneParameters: GetColorSceneParameters(YeelightScenePool),
				},
			},
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.OnOffCapabilityType,
			},
		},
		Properties: []adapter.PropertyInfoView{},
		CustomData: tuya.CustomData{
			DeviceType:    model.LightDeviceType,
			SwitchCommand: string(SwitchLedCommand),
			ProductID:     tools.AOS(string(LampE27Lemon3YandexProductID)),
			FwVersion:     tools.AOS("1.0.2"),
		},
		DeviceInfo: &model.DeviceInfo{
			Manufacturer: tools.AOS("Yandex Services AG"),
			Model:        tools.AOS("YNDX-00018"),
			SwVersion:    tools.AOS("1.0.2"),
			HwVersion:    tools.AOS("3.0"),
		},
	}

	assert.EqualValues(t, expectedYeelightColorLampV3View, yeelightColorLampV3.ToDeviceInfoView())

	// CASE 9: Sber E27 Color lamp
	sberE27ColorLamp := getColorSberE27LampSample()

	expectedSberE27ColorLampView := adapter.DeviceInfoView{
		ID:   "some-id-01",
		Name: "Лампочка",
		Type: model.LightDeviceType,
		Capabilities: []adapter.CapabilityInfoView{
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.RangeCapabilityType,
				Parameters: model.RangeCapabilityParameters{
					Instance:     model.BrightnessRangeInstance,
					Unit:         model.UnitPercent,
					RandomAccess: true,
					Range: &model.Range{
						Min:       1,
						Max:       100,
						Precision: 1,
					},
				},
			},
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.ColorSettingCapabilityType,
				Parameters: model.ColorSettingCapabilityParameters{
					TemperatureK: &model.TemperatureKParameters{
						Min: 1500,
						Max: 6500,
					},
					ColorModel: model.CM(model.HsvModelType),
				},
			},
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.OnOffCapabilityType,
			},
		},
		Properties: []adapter.PropertyInfoView{},
		CustomData: tuya.CustomData{
			DeviceType:    model.LightDeviceType,
			SwitchCommand: string(SwitchLedCommand),
			ProductID:     tools.AOS(string(LampE27SberProductID)),
			FwVersion:     tools.AOS("1.0"),
		},
		DeviceInfo: &model.DeviceInfo{
			Manufacturer: tools.AOS("SberDevices"),
			Model:        tools.AOS("SBDV-00019"),
			SwVersion:    tools.AOS("1.0"),
			HwVersion:    tools.AOS("1.0"),
		},
	}

	assert.EqualValues(t, expectedSberE27ColorLampView, sberE27ColorLamp.ToDeviceInfoView())

	// CASE 10: Sber E14 Color lamp
	sberE14ColorLamp := getColorSberE14LampSample()

	expectedSberE14ColorLampView := adapter.DeviceInfoView{
		ID:   "some-id-01",
		Name: "Лампочка",
		Type: model.LightDeviceType,
		Capabilities: []adapter.CapabilityInfoView{
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.RangeCapabilityType,
				Parameters: model.RangeCapabilityParameters{
					Instance:     model.BrightnessRangeInstance,
					Unit:         model.UnitPercent,
					RandomAccess: true,
					Range: &model.Range{
						Min:       1,
						Max:       100,
						Precision: 1,
					},
				},
			},
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.ColorSettingCapabilityType,
				Parameters: model.ColorSettingCapabilityParameters{
					TemperatureK: &model.TemperatureKParameters{
						Min: 1500,
						Max: 6500,
					},
					ColorModel: model.CM(model.HsvModelType),
				},
			},
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.OnOffCapabilityType,
			},
		},
		Properties: []adapter.PropertyInfoView{},
		CustomData: tuya.CustomData{
			DeviceType:    model.LightDeviceType,
			SwitchCommand: string(SwitchLedCommand),
			ProductID:     tools.AOS(string(LampE14SberProductID)),
			FwVersion:     tools.AOS("1.0"),
		},
		DeviceInfo: &model.DeviceInfo{
			Manufacturer: tools.AOS("SberDevices"),
			Model:        tools.AOS("SBDV-00020"),
			SwVersion:    tools.AOS("1.0"),
			HwVersion:    tools.AOS("1.0"),
		},
	}

	assert.EqualValues(t, expectedSberE14ColorLampView, sberE14ColorLamp.ToDeviceInfoView())

	// CASE 11: Sber E14 v2 Color lamp
	sberE14v2ColorLamp := getColorSberE14v2LampSample()

	expectedSberE14v2ColorLampView := adapter.DeviceInfoView{
		ID:   "some-id-01",
		Name: "Лампочка",
		Type: model.LightDeviceType,
		Capabilities: []adapter.CapabilityInfoView{
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.RangeCapabilityType,
				Parameters: model.RangeCapabilityParameters{
					Instance:     model.BrightnessRangeInstance,
					Unit:         model.UnitPercent,
					RandomAccess: true,
					Range: &model.Range{
						Min:       1,
						Max:       100,
						Precision: 1,
					},
				},
			},
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.ColorSettingCapabilityType,
				Parameters: model.ColorSettingCapabilityParameters{
					TemperatureK: &model.TemperatureKParameters{
						Min: 1500,
						Max: 6500,
					},
					ColorModel: model.CM(model.HsvModelType),
				},
			},
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.OnOffCapabilityType,
			},
		},
		Properties: []adapter.PropertyInfoView{},
		CustomData: tuya.CustomData{
			DeviceType:    model.LightDeviceType,
			SwitchCommand: string(SwitchLedCommand),
			ProductID:     tools.AOS(string(Lamp2E14SberProductID)),
			FwVersion:     tools.AOS("2.0"),
		},
		DeviceInfo: &model.DeviceInfo{
			Manufacturer: tools.AOS("SberDevices"),
			Model:        tools.AOS("SBDV-00020"),
			SwVersion:    tools.AOS("2.0"),
			HwVersion:    tools.AOS("2.0"),
		},
	}

	assert.EqualValues(t, expectedSberE14v2ColorLampView, sberE14v2ColorLamp.ToDeviceInfoView())

	// CASE 12: Sber Socket
	sberSocket := getSocketSberSample()

	expectedSberSocketView := adapter.DeviceInfoView{
		ID:   "some-id-03",
		Name: "Розетка",
		Type: model.SocketDeviceType,
		Capabilities: []adapter.CapabilityInfoView{
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.OnOffCapabilityType,
			},
		},
		Properties: []adapter.PropertyInfoView{
			{
				Type:        model.FloatPropertyType,
				Retrievable: true,
				Reportable:  true,
				Parameters: model.FloatPropertyParameters{
					Instance: model.VoltagePropertyInstance,
					Unit:     model.UnitVolt,
				},
			},
			{
				Type:        model.FloatPropertyType,
				Retrievable: true,
				Reportable:  true,
				Parameters: model.FloatPropertyParameters{
					Instance: model.PowerPropertyInstance,
					Unit:     model.UnitWatt,
				},
			},
			{
				Type:        model.FloatPropertyType,
				Retrievable: true,
				Reportable:  true,
				Parameters: model.FloatPropertyParameters{
					Instance: model.AmperagePropertyInstance,
					Unit:     model.UnitAmpere,
				},
			},
		},
		CustomData: tuya.CustomData{
			DeviceType:    model.SocketDeviceType,
			SwitchCommand: string(SwitchCommand),
			ProductID:     tools.AOS(string(SocketSberProductID)),
			FwVersion:     tools.AOS("1.0"),
		},
		DeviceInfo: &model.DeviceInfo{
			Manufacturer: tools.AOS(SberDevicesManufacturer),
			Model:        tools.AOS(SocketSberModel),
			SwVersion:    tools.AOS("1.0"),
			HwVersion:    tools.AOS("1.0"),
		},
	}
	assert.EqualValues(t, expectedSberSocketView, sberSocket.ToDeviceInfoView())

	// CASE 13: Hub with other category
	tuyaIR2 := getIR2Sample()

	expectedIR2View := adapter.DeviceInfoView{
		ID:           "some-ir-id",
		Name:         "Пульт",
		Type:         model.HubDeviceType,
		Capabilities: []adapter.CapabilityInfoView{},
		Properties:   []adapter.PropertyInfoView{},
		CustomData: tuya.CustomData{
			DeviceType: model.HubDeviceType,
			ProductID:  tools.AOS(string(Hub2YandexProductID)),
			FwVersion:  tools.AOS("1.0"),
		},
		DeviceInfo: &model.DeviceInfo{
			Manufacturer: tools.AOS("Yandex Services AG"),
			Model:        tools.AOS("YNDX-0006"),
			SwVersion:    tools.AOS("1.0"),
			HwVersion:    tools.AOS("1.0"),
		},
	}
	assert.EqualValues(t, expectedIR2View, tuyaIR2.ToDeviceInfoView())

	// CASE 14: Yandex Yeelight Color lamp E14
	yeelightColorLampE14 := getColorYeelightLampE14Sample()

	expectedYeelightColorLampE14View := adapter.DeviceInfoView{
		ID:   "some-id-01",
		Name: "Лампочка",
		Type: model.LightDeviceType,
		Capabilities: []adapter.CapabilityInfoView{
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.RangeCapabilityType,
				Parameters: model.RangeCapabilityParameters{
					Instance:     model.BrightnessRangeInstance,
					Unit:         model.UnitPercent,
					RandomAccess: true,
					Range: &model.Range{
						Min:       1,
						Max:       100,
						Precision: 1,
					},
				},
			},
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.ColorSettingCapabilityType,
				Parameters: model.ColorSettingCapabilityParameters{
					TemperatureK: &model.TemperatureKParameters{
						Min: 1500,
						Max: 6500,
					},
					ColorModel:           model.CM(model.HsvModelType),
					ColorSceneParameters: GetColorSceneParameters(TuyaScenePool),
				},
			},
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.OnOffCapabilityType,
			},
		},
		Properties: []adapter.PropertyInfoView{},
		CustomData: tuya.CustomData{
			DeviceType:    model.LightDeviceType,
			SwitchCommand: string(SwitchLedCommand),
			ProductID:     tools.AOS(string(LampE14Test2YandexProductID)),
			FwVersion:     tools.AOS("2.9.16"),
		},
		DeviceInfo: &model.DeviceInfo{
			Manufacturer: tools.AOS("Yandex Services AG"),
			Model:        tools.AOS("YNDX-00017"),
			SwVersion:    tools.AOS("2.9.16"),
			HwVersion:    tools.AOS("2.0"),
		},
	}
	assert.EqualValues(t, expectedYeelightColorLampE14View, yeelightColorLampE14.ToDeviceInfoView())

	// CASE 15: Sber GU 10 Color lamp
	sberGU10ColorLamp := getColorSberGU10LampSample()

	expectedSberGU10ColorLampView := adapter.DeviceInfoView{
		ID:   "some-id-01",
		Name: "Лампочка",
		Type: model.LightDeviceType,
		Capabilities: []adapter.CapabilityInfoView{
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.RangeCapabilityType,
				Parameters: model.RangeCapabilityParameters{
					Instance:     model.BrightnessRangeInstance,
					Unit:         model.UnitPercent,
					RandomAccess: true,
					Range: &model.Range{
						Min:       1,
						Max:       100,
						Precision: 1,
					},
				},
			},
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.ColorSettingCapabilityType,
				Parameters: model.ColorSettingCapabilityParameters{
					TemperatureK: &model.TemperatureKParameters{
						Min: 1500,
						Max: 6500,
					},
					ColorModel: model.CM(model.HsvModelType),
				},
			},
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.OnOffCapabilityType,
			},
		},
		Properties: []adapter.PropertyInfoView{},
		CustomData: tuya.CustomData{
			DeviceType:    model.LightDeviceType,
			SwitchCommand: string(SwitchLedCommand),
			ProductID:     tools.AOS(string(LampGU10SberProductID)),
			FwVersion:     tools.AOS("2.0"),
		},
		DeviceInfo: &model.DeviceInfo{
			Manufacturer: tools.AOS("SberDevices"),
			Model:        tools.AOS("SBDV-00024"),
			SwVersion:    tools.AOS("2.0"),
			HwVersion:    tools.AOS("2.0"),
		},
	}

	assert.EqualValues(t, expectedSberGU10ColorLampView, sberGU10ColorLamp.ToDeviceInfoView())
	// CASE 16: Sber E27 V2 Lamp Sample
	sberE27v2ColorLamp := getColorSberE27v2LampSample()

	expectedSberE27v2ColorLampView := adapter.DeviceInfoView{
		ID:   "some-id-01",
		Name: "Лампочка",
		Type: model.LightDeviceType,
		Capabilities: []adapter.CapabilityInfoView{
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.RangeCapabilityType,
				Parameters: model.RangeCapabilityParameters{
					Instance:     model.BrightnessRangeInstance,
					Unit:         model.UnitPercent,
					RandomAccess: true,
					Range: &model.Range{
						Min:       1,
						Max:       100,
						Precision: 1,
					},
				},
			},
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.ColorSettingCapabilityType,
				Parameters: model.ColorSettingCapabilityParameters{
					TemperatureK: &model.TemperatureKParameters{
						Min: 1500,
						Max: 6500,
					},
					ColorModel: model.CM(model.HsvModelType),
				},
			},
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.OnOffCapabilityType,
			},
		},
		Properties: []adapter.PropertyInfoView{},
		CustomData: tuya.CustomData{
			DeviceType:    model.LightDeviceType,
			SwitchCommand: string(SwitchLedCommand),
			ProductID:     tools.AOS(string(Lamp2E27SberProductID)),
			FwVersion:     tools.AOS("2.0"),
		},
		DeviceInfo: &model.DeviceInfo{
			Manufacturer: tools.AOS("SberDevices"),
			Model:        tools.AOS("SBDV-00019"),
			SwVersion:    tools.AOS("2.0"),
			HwVersion:    tools.AOS("2.0"),
		},
	}

	assert.EqualValues(t, expectedSberE27v2ColorLampView, sberE27v2ColorLamp.ToDeviceInfoView())

	// CASE 17: Yandex Yeelight Color lamp E14
	yeelightColorLamp3E14 := getColorYeelightLamp3E14Sample()

	expectedYeelightColorLamp3E14View := adapter.DeviceInfoView{
		ID:   "some-id-01",
		Name: "Лампочка",
		Type: model.LightDeviceType,
		Capabilities: []adapter.CapabilityInfoView{
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.RangeCapabilityType,
				Parameters: model.RangeCapabilityParameters{
					Instance:     model.BrightnessRangeInstance,
					Unit:         model.UnitPercent,
					RandomAccess: true,
					Range: &model.Range{
						Min:       1,
						Max:       100,
						Precision: 1,
					},
				},
			},
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.ColorSettingCapabilityType,
				Parameters: model.ColorSettingCapabilityParameters{
					TemperatureK: &model.TemperatureKParameters{
						Min: 1500,
						Max: 6500,
					},
					ColorModel:           model.CM(model.HsvModelType),
					ColorSceneParameters: GetColorSceneParameters(TuyaScenePool),
				},
			},
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.OnOffCapabilityType,
			},
		},
		Properties: []adapter.PropertyInfoView{},
		CustomData: tuya.CustomData{
			DeviceType:    model.LightDeviceType,
			SwitchCommand: string(SwitchLedCommand),
			ProductID:     tools.AOS(string(LampE14MPYandexProductID)),
			FwVersion:     tools.AOS("1.0.2"),
		},
		DeviceInfo: &model.DeviceInfo{
			Manufacturer: tools.AOS("Yandex Services AG"),
			Model:        tools.AOS("YNDX-00017"),
			SwVersion:    tools.AOS("1.0.2"),
			HwVersion:    tools.AOS("3.0"),
		},
	}
	assert.EqualValues(t, expectedYeelightColorLamp3E14View, yeelightColorLamp3E14.ToDeviceInfoView())

	// CASE 18: Yandex Yeelight Color lamp GU10
	yeelightColorLampGU10 := getColorYeelightLampGU10Sample()

	expectedYeelightColorLampGU10View := adapter.DeviceInfoView{
		ID:   "some-id-01",
		Name: "Лампочка",
		Type: model.LightDeviceType,
		Capabilities: []adapter.CapabilityInfoView{
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.RangeCapabilityType,
				Parameters: model.RangeCapabilityParameters{
					Instance:     model.BrightnessRangeInstance,
					Unit:         model.UnitPercent,
					RandomAccess: true,
					Range: &model.Range{
						Min:       1,
						Max:       100,
						Precision: 1,
					},
				},
			},
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.ColorSettingCapabilityType,
				Parameters: model.ColorSettingCapabilityParameters{
					TemperatureK: &model.TemperatureKParameters{
						Min: 1500,
						Max: 6500,
					},
					ColorModel:           model.CM(model.HsvModelType),
					ColorSceneParameters: GetColorSceneParameters(TuyaScenePool),
				},
			},
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.OnOffCapabilityType,
			},
		},
		Properties: []adapter.PropertyInfoView{},
		CustomData: tuya.CustomData{
			DeviceType:    model.LightDeviceType,
			SwitchCommand: string(SwitchLedCommand),
			ProductID:     tools.AOS(string(LampGU10TestYandexProductID)),
			FwVersion:     tools.AOS("1.2.16"),
		},
		DeviceInfo: &model.DeviceInfo{
			Manufacturer: tools.AOS("Yandex Services AG"),
			Model:        tools.AOS("YNDX-00019"),
			SwVersion:    tools.AOS("1.2.16"),
			HwVersion:    tools.AOS("1.0"),
		},
	}
	assert.EqualValues(t, expectedYeelightColorLampGU10View, yeelightColorLampGU10.ToDeviceInfoView())

	// CASE 19: Sber E27 V3 Lamp Sample
	sberE27v3ColorLamp := getColorSberE27v3LampSample()

	expectedSberE27v3ColorLampView := adapter.DeviceInfoView{
		ID:   "some-id-01",
		Name: "Лампочка",
		Type: model.LightDeviceType,
		Capabilities: []adapter.CapabilityInfoView{
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.RangeCapabilityType,
				Parameters: model.RangeCapabilityParameters{
					Instance:     model.BrightnessRangeInstance,
					Unit:         model.UnitPercent,
					RandomAccess: true,
					Range: &model.Range{
						Min:       1,
						Max:       100,
						Precision: 1,
					},
				},
			},
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.ColorSettingCapabilityType,
				Parameters: model.ColorSettingCapabilityParameters{
					TemperatureK: &model.TemperatureKParameters{
						Min: 1500,
						Max: 6500,
					},
					ColorModel: model.CM(model.HsvModelType),
				},
			},
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.OnOffCapabilityType,
			},
		},
		Properties: []adapter.PropertyInfoView{},
		CustomData: tuya.CustomData{
			DeviceType:    model.LightDeviceType,
			SwitchCommand: string(SwitchLedCommand),
			ProductID:     tools.AOS(string(Lamp3E27SberProductID)),
			FwVersion:     tools.AOS("2.0"),
		},
		DeviceInfo: &model.DeviceInfo{
			Manufacturer: tools.AOS("SberDevices"),
			Model:        tools.AOS("SBDV-00019"),
			SwVersion:    tools.AOS("2.0"),
			HwVersion:    tools.AOS("2.0"),
		},
	}

	assert.EqualValues(t, expectedSberE27v3ColorLampView, sberE27v3ColorLamp.ToDeviceInfoView())

	// CASE 20: Yandex Yeelight Color lamp GU10 MP
	yeelightColorLamp2GU10 := getColorYeelightLamp2GU10Sample()
	expectedYeelightColorLamp2GU10View := adapter.DeviceInfoView{
		ID:   "some-id-01",
		Name: "Лампочка",
		Type: model.LightDeviceType,
		Capabilities: []adapter.CapabilityInfoView{
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.RangeCapabilityType,
				Parameters: model.RangeCapabilityParameters{
					Instance:     model.BrightnessRangeInstance,
					Unit:         model.UnitPercent,
					RandomAccess: true,
					Range: &model.Range{
						Min:       1,
						Max:       100,
						Precision: 1,
					},
				},
			},
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.ColorSettingCapabilityType,
				Parameters: model.ColorSettingCapabilityParameters{
					TemperatureK: &model.TemperatureKParameters{
						Min: 1500,
						Max: 6500,
					},
					ColorModel:           model.CM(model.HsvModelType),
					ColorSceneParameters: GetColorSceneParameters(TuyaScenePool),
				},
			},
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.OnOffCapabilityType,
			},
		},
		Properties: []adapter.PropertyInfoView{},
		CustomData: tuya.CustomData{
			DeviceType:    model.LightDeviceType,
			SwitchCommand: string(SwitchLedCommand),
			ProductID:     tools.AOS(string(LampGU10MPYandexProductID)),
			FwVersion:     tools.AOS("1.2.16"),
		},
		DeviceInfo: &model.DeviceInfo{
			Manufacturer: tools.AOS("Yandex Services AG"),
			Model:        tools.AOS("YNDX-00019"),
			SwVersion:    tools.AOS("1.2.16"),
			HwVersion:    tools.AOS("2.0"),
		},
	}
	assert.EqualValues(t, expectedYeelightColorLamp2GU10View, yeelightColorLamp2GU10.ToDeviceInfoView())

	// CASE 21: Yandex Yeelight And Tuya Cheap lamp E27
	yeelightTuyaCheapLampE27 := getYeelightTuyaCheapLampE27Sample()
	expectedYeelightTuyaCheapLampE27View := adapter.DeviceInfoView{
		ID:   "some-id-01",
		Name: "Лампочка",
		Type: model.LightDeviceType,
		Capabilities: []adapter.CapabilityInfoView{
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.RangeCapabilityType,
				Parameters: model.RangeCapabilityParameters{
					Instance:     model.BrightnessRangeInstance,
					Unit:         model.UnitPercent,
					RandomAccess: true,
					Range: &model.Range{
						Min:       1,
						Max:       100,
						Precision: 1,
					},
				},
			},
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.ColorSettingCapabilityType,
				Parameters: model.ColorSettingCapabilityParameters{
					TemperatureK: &model.TemperatureKParameters{
						Min: 1500,
						Max: 6500,
					},
				},
			},
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.OnOffCapabilityType,
			},
		},
		Properties: []adapter.PropertyInfoView{},
		CustomData: tuya.CustomData{
			DeviceType:    model.LightDeviceType,
			SwitchCommand: string(SwitchLedCommand),
			ProductID:     tools.AOS(string(LampE27A60YandexProductID)),
			FwVersion:     tools.AOS("0.0.1"),
		},
		DeviceInfo: &model.DeviceInfo{
			Manufacturer: tools.AOS("Yandex Services AG"),
			Model:        tools.AOS("YNDX-00501"),
			SwVersion:    tools.AOS("0.0.1"),
			HwVersion:    tools.AOS("4.0"),
		},
	}
	assert.EqualValues(t, expectedYeelightTuyaCheapLampE27View, yeelightTuyaCheapLampE27.ToDeviceInfoView())

	// CASE 22: Sber E27 V4 Lamp Sample
	sberE27v4ColorLamp := getColorSberE27v4LampSample()

	expectedSberE27v4ColorLampView := adapter.DeviceInfoView{
		ID:   "some-id-01",
		Name: "Лампочка",
		Type: model.LightDeviceType,
		Capabilities: []adapter.CapabilityInfoView{
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.RangeCapabilityType,
				Parameters: model.RangeCapabilityParameters{
					Instance:     model.BrightnessRangeInstance,
					Unit:         model.UnitPercent,
					RandomAccess: true,
					Range: &model.Range{
						Min:       1,
						Max:       100,
						Precision: 1,
					},
				},
			},
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.ColorSettingCapabilityType,
				Parameters: model.ColorSettingCapabilityParameters{
					TemperatureK: &model.TemperatureKParameters{
						Min: 1500,
						Max: 6500,
					},
					ColorModel: model.CM(model.HsvModelType),
				},
			},
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.OnOffCapabilityType,
			},
		},
		Properties: []adapter.PropertyInfoView{},
		CustomData: tuya.CustomData{
			DeviceType:    model.LightDeviceType,
			SwitchCommand: string(SwitchLedCommand),
			ProductID:     tools.AOS(string(Lamp4E27SberProductID)),
			FwVersion:     tools.AOS("2.0"),
		},
		DeviceInfo: &model.DeviceInfo{
			Manufacturer: tools.AOS("SberDevices"),
			Model:        tools.AOS("SBDV-00019"),
			SwVersion:    tools.AOS("2.0"),
			HwVersion:    tools.AOS("2.0"),
		},
	}

	assert.EqualValues(t, expectedSberE27v4ColorLampView, sberE27v4ColorLamp.ToDeviceInfoView())

	// CASE 23: Sber E14 V3 Lamp Sample
	sberE14v3ColorLamp := getColorSberE14v3LampSample()

	expectedSberE14v3ColorLampView := adapter.DeviceInfoView{
		ID:   "some-id-01",
		Name: "Лампочка",
		Type: model.LightDeviceType,
		Capabilities: []adapter.CapabilityInfoView{
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.RangeCapabilityType,
				Parameters: model.RangeCapabilityParameters{
					Instance:     model.BrightnessRangeInstance,
					Unit:         model.UnitPercent,
					RandomAccess: true,
					Range: &model.Range{
						Min:       1,
						Max:       100,
						Precision: 1,
					},
				},
			},
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.ColorSettingCapabilityType,
				Parameters: model.ColorSettingCapabilityParameters{
					TemperatureK: &model.TemperatureKParameters{
						Min: 1500,
						Max: 6500,
					},
					ColorModel: model.CM(model.HsvModelType),
				},
			},
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.OnOffCapabilityType,
			},
		},
		Properties: []adapter.PropertyInfoView{},
		CustomData: tuya.CustomData{
			DeviceType:    model.LightDeviceType,
			SwitchCommand: string(SwitchLedCommand),
			ProductID:     tools.AOS(string(Lamp3E14SberProductID)),
			FwVersion:     tools.AOS("2.0"),
		},
		DeviceInfo: &model.DeviceInfo{
			Manufacturer: tools.AOS("SberDevices"),
			Model:        tools.AOS("SBDV-00020"),
			SwVersion:    tools.AOS("2.0"),
			HwVersion:    tools.AOS("2.0"),
		},
	}

	assert.EqualValues(t, expectedSberE14v3ColorLampView, sberE14v3ColorLamp.ToDeviceInfoView())

	// CASE 24: Sber E14 V4 Lamp Sample
	sberE14v4ColorLamp := getColorSberE14v4LampSample()

	expectedSberE14v4ColorLampView := adapter.DeviceInfoView{
		ID:   "some-id-01",
		Name: "Лампочка",
		Type: model.LightDeviceType,
		Capabilities: []adapter.CapabilityInfoView{
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.RangeCapabilityType,
				Parameters: model.RangeCapabilityParameters{
					Instance:     model.BrightnessRangeInstance,
					Unit:         model.UnitPercent,
					RandomAccess: true,
					Range: &model.Range{
						Min:       1,
						Max:       100,
						Precision: 1,
					},
				},
			},
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.ColorSettingCapabilityType,
				Parameters: model.ColorSettingCapabilityParameters{
					TemperatureK: &model.TemperatureKParameters{
						Min: 1500,
						Max: 6500,
					},
					ColorModel: model.CM(model.HsvModelType),
				},
			},
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.OnOffCapabilityType,
			},
		},
		Properties: []adapter.PropertyInfoView{},
		CustomData: tuya.CustomData{
			DeviceType:    model.LightDeviceType,
			SwitchCommand: string(SwitchLedCommand),
			ProductID:     tools.AOS(string(Lamp4E14SberProductID)),
			FwVersion:     tools.AOS("3.0"),
		},
		DeviceInfo: &model.DeviceInfo{
			Manufacturer: tools.AOS("SberDevices"),
			Model:        tools.AOS("SBDV-00020"),
			SwVersion:    tools.AOS("3.0"),
			HwVersion:    tools.AOS("3.0"),
		},
	}

	assert.EqualValues(t, expectedSberE14v4ColorLampView, sberE14v4ColorLamp.ToDeviceInfoView())

	// CASE 25: Sber E27 V5 Lamp Sample
	sberE27v5ColorLamp := getColorSberE27v5LampSample()

	expectedSberE27v5ColorLampView := adapter.DeviceInfoView{
		ID:   "some-id-01",
		Name: "Лампочка",
		Type: model.LightDeviceType,
		Capabilities: []adapter.CapabilityInfoView{
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.RangeCapabilityType,
				Parameters: model.RangeCapabilityParameters{
					Instance:     model.BrightnessRangeInstance,
					Unit:         model.UnitPercent,
					RandomAccess: true,
					Range: &model.Range{
						Min:       1,
						Max:       100,
						Precision: 1,
					},
				},
			},
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.ColorSettingCapabilityType,
				Parameters: model.ColorSettingCapabilityParameters{
					TemperatureK: &model.TemperatureKParameters{
						Min: 1500,
						Max: 6500,
					},
					ColorModel: model.CM(model.HsvModelType),
				},
			},
			{
				Retrievable: true,
				Reportable:  true,
				Type:        model.OnOffCapabilityType,
			},
		},
		Properties: []adapter.PropertyInfoView{},
		CustomData: tuya.CustomData{
			DeviceType:    model.LightDeviceType,
			SwitchCommand: string(SwitchLedCommand),
			ProductID:     tools.AOS(string(Lamp5E27SberProductID)),
			FwVersion:     tools.AOS("3.0"),
		},
		DeviceInfo: &model.DeviceInfo{
			Manufacturer: tools.AOS("SberDevices"),
			Model:        tools.AOS("SBDV-00019"),
			SwVersion:    tools.AOS("3.0"),
			HwVersion:    tools.AOS("3.0"),
		},
	}

	assert.EqualValues(t, expectedSberE27v5ColorLampView, sberE27v5ColorLamp.ToDeviceInfoView())
}

func TestCustomData_fromDeviceActionView(t *testing.T) {
	rawCustomData := `{"device_type":"devices.types.light","switch_command":"switch_led","infrared_data":{"transmitter_id":"some-ir-tx-id","brand_name":"Samsung","preset_id":"1122"}}`

	var iCustomDataVar interface{}
	if err := json.Unmarshal([]byte(rawCustomData), &iCustomDataVar); err != nil {
		t.Error(err)
	}

	adapterActionView := adapter.DeviceActionRequestView{
		CustomData: iCustomDataVar,
	}

	var customData tuya.CustomData
	if err := mapstructure.Decode(adapterActionView.CustomData, &customData); err != nil {
		t.Error(err)
	}

	expectedCustomData := tuya.CustomData{
		DeviceType:    model.LightDeviceType,
		SwitchCommand: string(SwitchLedCommand),
		InfraredData: &tuya.InfraredData{
			BrandName:     "Samsung",
			PresetID:      "1122",
			TransmitterID: "some-ir-tx-id",
		},
	}

	assert.Equal(t, expectedCustomData, customData)
}

func TestCustomData_FromDeleteRequest(t *testing.T) {
	rawCustomData := `{"device_type":"devices.types.light","switch_command":"switch_led","infrared_data":{"transmitter_id":"some-ir-tx-id","brand_name":"Samsung","preset_id":"1122"}}`

	var iCustomDataVar interface{}
	if err := json.Unmarshal([]byte(rawCustomData), &iCustomDataVar); err != nil {
		t.Error(err)
	}

	adapterActionView := adapter.DeleteRequest{
		CustomData: iCustomDataVar,
	}

	var customData tuya.CustomData
	if err := mapstructure.Decode(adapterActionView.CustomData, &customData); err != nil {
		t.Error(err)
	}

	expectedCustomData := tuya.CustomData{
		DeviceType:    model.LightDeviceType,
		SwitchCommand: string(SwitchLedCommand),
		InfraredData: &tuya.InfraredData{
			BrandName:     "Samsung",
			PresetID:      "1122",
			TransmitterID: "some-ir-tx-id",
		},
	}

	assert.Equal(t, expectedCustomData, customData)
}

func TestUnmarshalTuyaFirmwareInfoResponse(t *testing.T) {
	rawTuyaFirmwareInfo := `
	{
		"result": [
			{
				"current_version": "1.0",
				"last_upgrade_time": 0,
				"module_desc": "${v1,{\"code\":\"FIRMWARE_UPGRADE_TYPE_DESC_4\"}}",
				"module_type": 4,
				"upgrade_status": 0
			}
		],
		"success": true,
		"t": 1580399517143
	}`

	tuyaResponse := TuyaFirmwareInfoResponse{}
	err := json.Unmarshal([]byte(rawTuyaFirmwareInfo), &tuyaResponse)

	assert.NoError(t, err)
	assert.Equal(t, "1.0", tuyaResponse.Result[0].CurrentVersion)
	assert.Equal(t, FirmwareModuleType(4), tuyaResponse.Result[0].ModuleType)
}

func TestTuyaCommand_UnmarshalJSON(t *testing.T) {
	// generating test cases: key - raw command json, value - expected result

	//  {"code":"colour_data_v2","value":""}]
	testCases := map[string]TuyaCommand{
		`{"code":"switch_led","value":"true"}`:                               {Code: SwitchLedCommand.ToString(), Value: true},
		`{"code":"work_mode","value":"white"}`:                               {Code: WorkModeCommand.ToString(), Value: WhiteWorkMode.ToString()},
		`{"code":"bright_value_v2","value":"1000"}`:                          {Code: Brightness2Command.ToString(), Value: 1000},
		`{"code":"bright_value","value":"255"}`:                              {Code: BrightnessCommand.ToString(), Value: 255},
		`{"code":"temp_value_v2","value":"1000"}`:                            {Code: TempValue2Command.ToString(), Value: 1000},
		`{"code":"temp_value","value":"255"}`:                                {Code: TempValueCommand.ToString(), Value: 255},
		`{"code":"colour_data_v2","value":"{\"h\":0,\"s\":960,\"v\":1000}"}`: {Code: ColorData2Command.ToString(), Value: model.HSV{H: 0, S: 960, V: 1000}},
		`{"code":"colour_data","value":"{\"h\":0,\"s\":255,\"v\":255}"}`:     {Code: ColorDataCommand.ToString(), Value: model.HSV{H: 0, S: 255, V: 255}},
		`{"code":"countdown_1","value":"0"}`:                                 {Code: "unknown", Value: ""},
		`{"code":"control_data","value":""}`:                                 {Code: "control_data", Value: nil},
		`{"code":"colour_data_v2","value":""}`:                               {Code: ColorData2Command.ToString(), Value: model.HSV{H: 55, S: 55, V: 55}},
		`{"code":"colour_data","value":""}`:                                  {Code: ColorDataCommand.ToString(), Value: model.HSV{H: 55, S: 55, V: 55}},
		`{"code":"switch_led","value":""}`:                                   {Code: SwitchLedCommand.ToString(), Value: nil},
		`{"code":"work_mode","value":""}`:                                    {Code: WorkModeCommand.ToString(), Value: nil},
	}

	for rawCommand, expected := range testCases {
		unmarshalled := TuyaCommand{}
		err := json.Unmarshal([]byte(rawCommand), &unmarshalled)
		assert.NoError(t, err)
		assert.Equal(t, expected, unmarshalled, rawCommand)
	}
}

func TestDeviceFirmwareInfoToMap(t *testing.T) {
	// case 1: upgrade exception
	firmwareInfo := DeviceFirmwareInfo{
		{
			ModuleType:    FirmwareInfoWIFIModuleType,
			UpgradeStatus: FirmwareHardwareIsReadyStatus,
		},
		{
			ModuleType:    FirmwareInfoMCUModuleType,
			UpgradeStatus: FirmwareUpgradeExceptionStatus,
		},
	}
	upgradeStatus, moduleType := firmwareInfo.GetStatusAndModuleType()
	assert.Equal(t, FirmwareUpgradeExceptionStatus, upgradeStatus)
	assert.Equal(t, FirmwareInfoMCUModuleType, moduleType)

	// case 2: module upgrading
	firmwareInfo[0].UpgradeStatus = FirmwareUpgradingStatus
	firmwareInfo[1].UpgradeStatus = FirmwareHardwareIsReadyStatus
	upgradeStatus, moduleType = firmwareInfo.GetStatusAndModuleType()
	assert.Equal(t, FirmwareUpgradingStatus, upgradeStatus)
	assert.Equal(t, FirmwareInfoWIFIModuleType, moduleType)
}
