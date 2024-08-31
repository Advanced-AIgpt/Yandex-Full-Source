package action

import (
	"context"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/action"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/common"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/frames"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/nlg"
	xtestlogs "a.yandex-team.ru/alice/iot/bulbasaur/xtest/logs"
	xtestmegamind "a.yandex-team.ru/alice/iot/bulbasaur/xtest/megamind"
	"a.yandex-team.ru/alice/library/go/inflector"
	libnlg "a.yandex-team.ru/alice/library/go/nlg"
)

func TestNlgFromDevicesResult(t *testing.T) {
	deviceID1 := "id-1"
	deviceID2 := "id-2"
	defaultNLG := nlg.OK

	inputs := []struct {
		name          string
		devicesResult action.DevicesResult
		expectedNLG   libnlg.NLG
	}{
		{
			name: "one_device_one_error",
			devicesResult: action.DevicesResult{
				ProviderResults: []action.ProviderDevicesResult{
					{
						DeviceResults: []action.ProviderDeviceResult{
							errorDeviceResult(deviceID1, adapter.DeviceOff),
						},
					},
				},
			},
			expectedNLG: nlg.DeviceOffError,
		},
		{
			name: "two_devices_same_errors",
			devicesResult: action.DevicesResult{
				ProviderResults: []action.ProviderDevicesResult{
					{
						DeviceResults: []action.ProviderDeviceResult{
							errorDeviceResult(deviceID1, adapter.InvalidAction),
							errorDeviceResult(deviceID2, adapter.InvalidAction),
						},
					},
				},
			},
			expectedNLG: megamind.MultipleDevicesErrorCodeNLG(adapter.InvalidAction),
		},
		{
			name: "two_devices_different_errors",
			devicesResult: action.DevicesResult{
				ProviderResults: []action.ProviderDevicesResult{
					{
						DeviceResults: []action.ProviderDeviceResult{
							errorDeviceResult(deviceID1, adapter.InvalidAction),
							errorDeviceResult(deviceID2, adapter.DeviceBusy),
						},
					},
				},
			},
			expectedNLG: nlg.AllActionsFailedError,
		},
		{
			name: "two_devices_one_error",
			devicesResult: action.DevicesResult{
				ProviderResults: []action.ProviderDevicesResult{
					{
						DeviceResults: []action.ProviderDeviceResult{
							errorDeviceResult(deviceID1, adapter.DeviceOff),
							doneDeviceResult(deviceID2),
						},
					},
				},
			},
			expectedNLG: defaultNLG,
		},
	}

	for _, input := range inputs {
		t.Run(input.name, func(t *testing.T) {
			actualNLG := nlgFromDevicesResult(defaultNLG, input.devicesResult)
			assert.Equal(t, input.expectedNLG, actualNLG)
		})
	}
}

func TestNlgByIntentParameters(t *testing.T) {
	deviceID1 := "id-1"
	deviceID2 := "id-2"
	oneDoneDeviceDevicesResult := action.DevicesResult{
		ProviderResults: []action.ProviderDevicesResult{
			{
				DeviceResults: []action.ProviderDeviceResult{
					doneDeviceResult(deviceID2),
				},
			},
		},
	}
	inputs := []struct {
		name             string
		devices          model.Devices
		intentParameters frames.ActionIntentParametersSlot
		devicesResult    action.DevicesResult
		frame            frames.ActionFrameV2
		userInfo         model.UserInfo
		expectedNLG      libnlg.NLG

		applyArguments ApplyArguments
	}{
		{
			name:          "turn_on",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.OnOffCapabilityType),
				CapabilityInstance: string(model.OnOnOffCapabilityInstance),
			},
			frame: frames.ActionFrameV2{
				OnOffValue: &frames.OnOffValueSlot{
					Value: true,
				},
			},
			expectedNLG: nlg.TurnOn,
		},
		{
			name:          "turn_off",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.OnOffCapabilityType),
				CapabilityInstance: string(model.OnOnOffCapabilityInstance),
			},
			frame: frames.ActionFrameV2{
				OnOffValue: &frames.OnOffValueSlot{
					Value: false,
				},
			},
			expectedNLG: nlg.TurnOff,
		},
		{
			name:          "open",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.OnOffCapabilityType),
				CapabilityInstance: string(model.OnOnOffCapabilityInstance),
			},
			frame: frames.ActionFrameV2{
				OnOffValue: &frames.OnOffValueSlot{
					Value: true,
				},
				RequiredDeviceType: frames.RequiredDeviceTypesSlot{
					DeviceTypes: []string{
						string(model.OpenableDeviceType),
						string(model.CurtainDeviceType),
					},
				},
			},
			expectedNLG: nlg.Open,
		},
		{
			name:          "make_tea",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.OnOffCapabilityType),
				CapabilityInstance: string(model.OnOnOffCapabilityInstance),
			},
			frame: frames.ActionFrameV2{
				OnOffValue: &frames.OnOffValueSlot{
					Value: true,
				},
				RequiredDeviceType: frames.RequiredDeviceTypesSlot{
					DeviceTypes: []string{
						string(model.KettleDeviceType),
					},
				},
			},
			expectedNLG: nlg.Boil,
		},
		{
			name:          "finish_cleaning",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.OnOffCapabilityType),
				CapabilityInstance: string(model.OnOnOffCapabilityInstance),
			},
			frame: frames.ActionFrameV2{
				OnOffValue: &frames.OnOffValueSlot{
					Value: false,
				},
				RequiredDeviceType: frames.RequiredDeviceTypesSlot{
					DeviceTypes: []string{
						string(model.VacuumCleanerDeviceType),
					},
				},
			},
			expectedNLG: nlg.OK,
		},
		{
			name:          "brighter",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.RangeCapabilityType),
				CapabilityInstance: string(model.BrightnessRangeInstance),
				RelativityType:     string(model.Increase),
			},
			frame: frames.ActionFrameV2{
				RangeValue: &frames.RangeValueSlot{
					SlotType: string(frames.NumSlotType),
					NumValue: 10,
				},
			},
			expectedNLG: nlg.Brighten,
		},
		{
			name:          "brighter_no_value",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.RangeCapabilityType),
				CapabilityInstance: string(model.BrightnessRangeInstance),
				RelativityType:     string(model.Increase),
			},
			expectedNLG: nlg.Brighten,
		},
		{
			name:          "darker",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.RangeCapabilityType),
				CapabilityInstance: string(model.BrightnessRangeInstance),
				RelativityType:     string(model.Decrease),
			},
			frame: frames.ActionFrameV2{
				RangeValue: &frames.RangeValueSlot{
					SlotType: string(frames.NumSlotType),
					NumValue: 10,
				},
			},
			expectedNLG: nlg.Dim,
		},
		{
			name:          "darker_no_value",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.RangeCapabilityType),
				CapabilityInstance: string(model.BrightnessRangeInstance),
				RelativityType:     string(model.Decrease),
			},
			expectedNLG: nlg.Dim,
		},
		{
			name:          "max_brightness_with_relative",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.RangeCapabilityType),
				CapabilityInstance: string(model.BrightnessRangeInstance),
				RelativityType:     string(model.Increase),
			},
			frame: frames.ActionFrameV2{
				RangeValue: &frames.RangeValueSlot{
					SlotType:    string(frames.StringSlotType),
					StringValue: frames.MaxRangeValue,
				},
			},
			expectedNLG: nlg.MaxBrightness,
		},
		{
			name:          "max_brighness",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.RangeCapabilityType),
				CapabilityInstance: string(model.BrightnessRangeInstance),
			},
			frame: frames.ActionFrameV2{
				RangeValue: &frames.RangeValueSlot{
					SlotType:    string(frames.StringSlotType),
					StringValue: frames.MaxRangeValue,
				},
			},
			expectedNLG: nlg.MaxBrightness,
		},
		{
			name:          "min_brighness",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.RangeCapabilityType),
				CapabilityInstance: string(model.BrightnessRangeInstance),
			},
			frame: frames.ActionFrameV2{
				RangeValue: &frames.RangeValueSlot{
					SlotType:    string(frames.StringSlotType),
					StringValue: frames.MinRangeValue,
				},
			},
			expectedNLG: nlg.MaxDim,
		},
		{
			name:          "set_brightness",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.RangeCapabilityType),
				CapabilityInstance: string(model.BrightnessRangeInstance),
			},
			frame: frames.ActionFrameV2{
				RangeValue: &frames.RangeValueSlot{
					SlotType: string(frames.NumSlotType),
					NumValue: 5,
				},
			},
			expectedNLG: nlg.ChangeBrightness,
		},
		{
			name:          "set_color",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.ColorSettingCapabilityType),
				CapabilityInstance: model.HypothesisColorCapabilityInstance,
			},
			frame: frames.ActionFrameV2{
				ColorSettingValue: &frames.ColorSettingValueSlot{
					SlotType: string(frames.ColorSlotType),
					Color:    model.ColorIDYellow,
				},
			},
			expectedNLG: nlg.ChangeColor,
		},
		{
			name:          "set_color_scene",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.ColorSettingCapabilityType),
				CapabilityInstance: model.HypothesisColorSceneCapabilityInstance,
			},
			frame: frames.ActionFrameV2{
				ColorSettingValue: &frames.ColorSettingValueSlot{
					SlotType:   string(frames.ColorSceneSlotType),
					ColorScene: model.ColorSceneIDParty,
				},
			},
			expectedNLG: nlg.OK,
		},
		{
			name:          "warmer_color",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.ColorSettingCapabilityType),
				CapabilityInstance: string(model.TemperatureKCapabilityInstance),
				RelativityType:     string(common.Decrease),
			},
			expectedNLG: nlg.DecreaseTemperatureK,
		},
		{
			name:          "colder_color",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.ColorSettingCapabilityType),
				CapabilityInstance: string(model.TemperatureKCapabilityInstance),
				RelativityType:     string(common.Increase),
			},
			expectedNLG: nlg.IncreaseTemperatureK,
		},
		{
			name:          "set_temperature",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.RangeCapabilityType),
				CapabilityInstance: string(model.TemperatureRangeInstance),
			},
			frame: frames.ActionFrameV2{
				RangeValue: &frames.RangeValueSlot{
					SlotType: string(frames.NumSlotType),
					NumValue: 5,
				},
			},
			expectedNLG: nlg.ChangeTemperature,
		},
		{
			name:          "max_temperature",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.RangeCapabilityType),
				CapabilityInstance: string(model.TemperatureRangeInstance),
			},
			frame: frames.ActionFrameV2{
				RangeValue: &frames.RangeValueSlot{
					SlotType:    string(frames.StringSlotType),
					StringValue: frames.MaxRangeValue,
				},
			},
			expectedNLG: nlg.MaxTemperature,
		},
		{
			name:          "min_temperature",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.RangeCapabilityType),
				CapabilityInstance: string(model.TemperatureRangeInstance),
			},
			frame: frames.ActionFrameV2{
				RangeValue: &frames.RangeValueSlot{
					SlotType:    string(frames.StringSlotType),
					StringValue: frames.MinRangeValue,
				},
			},
			expectedNLG: nlg.MinTemperature,
		},
		{
			name:          "increase_temperature",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.RangeCapabilityType),
				CapabilityInstance: string(model.TemperatureRangeInstance),
				RelativityType:     string(common.Increase),
			},
			frame: frames.ActionFrameV2{
				RangeValue: &frames.RangeValueSlot{
					SlotType: string(frames.NumSlotType),
					NumValue: 5,
				},
			},
			expectedNLG: nlg.IncreaseTemperature,
		},
		{
			name:          "decrease_temperature",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.RangeCapabilityType),
				CapabilityInstance: string(model.TemperatureRangeInstance),
				RelativityType:     string(common.Decrease),
			},
			frame: frames.ActionFrameV2{
				RangeValue: &frames.RangeValueSlot{
					SlotType: string(frames.NumSlotType),
					NumValue: 5,
				},
			},
			expectedNLG: nlg.DecreaseTemperature,
		},
		{
			name:          "decrease_temperature_no_value",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.RangeCapabilityType),
				CapabilityInstance: string(model.TemperatureRangeInstance),
				RelativityType:     string(common.Decrease),
			},
			expectedNLG: nlg.DecreaseTemperature,
		},
		{
			name:          "set_humidity",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.RangeCapabilityType),
				CapabilityInstance: string(model.HumidityRangeInstance),
			},
			frame: frames.ActionFrameV2{
				RangeValue: &frames.RangeValueSlot{
					SlotType: string(frames.NumSlotType),
					NumValue: 5,
				},
			},
			expectedNLG: nlg.ChangeHumidity,
		},
		{
			name:          "max_humidity",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.RangeCapabilityType),
				CapabilityInstance: string(model.HumidityRangeInstance),
			},
			frame: frames.ActionFrameV2{
				RangeValue: &frames.RangeValueSlot{
					SlotType:    string(frames.StringSlotType),
					StringValue: frames.MaxRangeValue,
				},
			},
			expectedNLG: nlg.MaxHumidity,
		},
		{
			name:          "min_humidity",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.RangeCapabilityType),
				CapabilityInstance: string(model.HumidityRangeInstance),
			},
			frame: frames.ActionFrameV2{
				RangeValue: &frames.RangeValueSlot{
					SlotType:    string(frames.StringSlotType),
					StringValue: frames.MinRangeValue,
				},
			},
			expectedNLG: nlg.MinHumidity,
		},
		{
			name:          "increase_humidity",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.RangeCapabilityType),
				CapabilityInstance: string(model.HumidityRangeInstance),
				RelativityType:     string(common.Increase),
			},
			frame: frames.ActionFrameV2{
				RangeValue: &frames.RangeValueSlot{
					SlotType: string(frames.NumSlotType),
					NumValue: 5,
				},
			},
			expectedNLG: nlg.IncreaseHumidity,
		},
		{
			name:          "decrease_humidity",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.RangeCapabilityType),
				CapabilityInstance: string(model.HumidityRangeInstance),
				RelativityType:     string(common.Decrease),
			},
			frame: frames.ActionFrameV2{
				RangeValue: &frames.RangeValueSlot{
					SlotType: string(frames.NumSlotType),
					NumValue: 5,
				},
			},
			expectedNLG: nlg.DecreaseHumidity,
		},
		{
			name:          "decrease_humidity_no_value",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.RangeCapabilityType),
				CapabilityInstance: string(model.HumidityRangeInstance),
				RelativityType:     string(common.Decrease),
			},
			expectedNLG: nlg.DecreaseHumidity,
		},
		{
			name:          "set_volume",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.RangeCapabilityType),
				CapabilityInstance: string(model.VolumeRangeInstance),
			},
			frame: frames.ActionFrameV2{
				RangeValue: &frames.RangeValueSlot{
					SlotType: string(frames.NumSlotType),
					NumValue: 5,
				},
			},
			expectedNLG: nlg.ChangeVolume,
		},
		{
			name:          "max_volume",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.RangeCapabilityType),
				CapabilityInstance: string(model.VolumeRangeInstance),
			},
			frame: frames.ActionFrameV2{
				RangeValue: &frames.RangeValueSlot{
					SlotType:    string(frames.StringSlotType),
					StringValue: frames.MaxRangeValue,
				},
			},
			expectedNLG: nlg.MaxVolume,
		},
		{
			name:          "min_volume",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.RangeCapabilityType),
				CapabilityInstance: string(model.VolumeRangeInstance),
			},
			frame: frames.ActionFrameV2{
				RangeValue: &frames.RangeValueSlot{
					SlotType:    string(frames.StringSlotType),
					StringValue: frames.MinRangeValue,
				},
			},
			expectedNLG: nlg.MinVolume,
		},
		{
			name:          "increase_volume",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.RangeCapabilityType),
				CapabilityInstance: string(model.VolumeRangeInstance),
				RelativityType:     string(common.Increase),
			},
			frame: frames.ActionFrameV2{
				RangeValue: &frames.RangeValueSlot{
					SlotType: string(frames.NumSlotType),
					NumValue: 5,
				},
			},
			expectedNLG: nlg.IncreaseVolume,
		},
		{
			name:          "decrease_volume",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.RangeCapabilityType),
				CapabilityInstance: string(model.VolumeRangeInstance),
				RelativityType:     string(common.Decrease),
			},
			frame: frames.ActionFrameV2{
				RangeValue: &frames.RangeValueSlot{
					SlotType: string(frames.NumSlotType),
					NumValue: 5,
				},
			},
			expectedNLG: nlg.DecreaseVolume,
		},
		{
			name:          "decrease_volume_no_value",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.RangeCapabilityType),
				CapabilityInstance: string(model.VolumeRangeInstance),
				RelativityType:     string(common.Decrease),
			},
			expectedNLG: nlg.DecreaseVolume,
		},
		{
			name:          "set_channel",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.RangeCapabilityType),
				CapabilityInstance: string(model.ChannelRangeInstance),
			},
			frame: frames.ActionFrameV2{
				RangeValue: &frames.RangeValueSlot{
					SlotType: string(frames.NumSlotType),
					NumValue: 5,
				},
			},
			expectedNLG: nlg.ChangeChannel,
		},
		{
			name:          "increase_channel",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.RangeCapabilityType),
				CapabilityInstance: string(model.ChannelRangeInstance),
				RelativityType:     string(common.Increase),
			},
			expectedNLG: nlg.IncreaseChannel,
		},
		{
			name:          "decrease_channel",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.RangeCapabilityType),
				CapabilityInstance: string(model.ChannelRangeInstance),
				RelativityType:     string(common.Decrease),
			},
			expectedNLG: nlg.DecreaseChannel,
		},
		{
			name:          "decrease_mode_thermostat",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.ModeCapabilityType),
				CapabilityInstance: string(model.ThermostatModeInstance),
				RelativityType:     string(common.Decrease),
			},
			expectedNLG: nlg.PreviousWorkingMode,
		},
		{
			name:          "increase_mode_thermostat",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.ModeCapabilityType),
				CapabilityInstance: string(model.ThermostatModeInstance),
				RelativityType:     string(common.Increase),
			},
			expectedNLG: nlg.NextWorkingMode,
		},
		{
			name:          "decrease_mode",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.ModeCapabilityType),
				CapabilityInstance: string(model.CoffeeModeInstance),
				RelativityType:     string(common.Decrease),
			},
			expectedNLG: nlg.PreviousMode,
		},
		{
			name:          "increase_mode",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.ModeCapabilityType),
				CapabilityInstance: string(model.CoffeeModeInstance),
				RelativityType:     string(common.Increase),
			},
			expectedNLG: nlg.NextMode,
		},
		{
			name:          "fan_only_mode",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.ModeCapabilityType),
				CapabilityInstance: string(model.ThermostatModeInstance),
			},
			frame: frames.ActionFrameV2{
				ModeValue: &frames.ModeValueSlot{
					ModeValue: model.FanOnlyMode,
				},
			},
			expectedNLG: SwitchToMode(model.FanOnlyMode),
		},
		{
			name:          "heat_mode",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.ModeCapabilityType),
				CapabilityInstance: string(model.ThermostatModeInstance),
			},
			frame: frames.ActionFrameV2{
				ModeValue: &frames.ModeValueSlot{
					ModeValue: model.HeatMode,
				},
			},
			expectedNLG: nlg.SwitchToHeatMode,
		},
		{
			name:          "cool_mode",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.ModeCapabilityType),
				CapabilityInstance: string(model.ThermostatModeInstance),
			},
			frame: frames.ActionFrameV2{
				ModeValue: &frames.ModeValueSlot{
					ModeValue: model.CoolMode,
				},
			},
			expectedNLG: nlg.SwitchToCoolMode,
		},
		{
			name:          "toggle_mute_on",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.ToggleCapabilityType),
				CapabilityInstance: string(model.MuteToggleCapabilityInstance),
			},
			frame: frames.ActionFrameV2{
				ToggleValue: &frames.ToggleValueSlot{
					Value: true,
				},
			},
			expectedNLG: nlg.PressMuteButton,
		},
		{
			name:          "toggle_mute_off",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.ToggleCapabilityType),
				CapabilityInstance: string(model.MuteToggleCapabilityInstance),
			},
			frame: frames.ActionFrameV2{
				ToggleValue: &frames.ToggleValueSlot{
					Value: false,
				},
			},
			expectedNLG: nlg.PressMuteButton,
		},
		{
			name:          "household_specified_nlg_1",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.ToggleCapabilityType),
				CapabilityInstance: string(model.MuteToggleCapabilityInstance),
			},
			frame: frames.ActionFrameV2{
				Devices: frames.DeviceSlots{
					{
						DeviceIDs: []string{deviceID1},
						SlotType:  string(frames.DeviceSlotType),
					},
				},
				ToggleValue: &frames.ToggleValueSlot{
					Value: true,
				},
			},
			devices: model.Devices{
				{
					ID:          deviceID1,
					Name:        "лампочка",
					HouseholdID: "h-id-1",
				},
			},
			userInfo: model.UserInfo{
				Devices: model.Devices{
					{
						ID:          deviceID1,
						Name:        "лампочка",
						HouseholdID: "h-id-1",
					},
					{
						ID:          deviceID2,
						Name:        "торшер",
						HouseholdID: "h-id-1",
					},
				},
				Households: model.Households{
					{
						ID:   "h-id-1",
						Name: "Дача",
					},
					{
						ID:   "h-id-2",
						Name: "Домик",
					},
				},
			},
			expectedNLG: libnlg.FromVariants([]string{
				"Окей, сделала на даче",
				"Сделала на даче",
				"Выполнила на даче",
			}),
		},
		{
			name:          "household_specified_nlg_2",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.VideoStreamCapabilityType),
				CapabilityInstance: string(model.GetStreamCapabilityInstance),
			},
			frame: frames.ActionFrameV2{
				Devices: frames.DeviceSlots{
					{
						DeviceIDs: []string{deviceID1},
						SlotType:  string(frames.DeviceSlotType),
					},
				},
			},
			devices: model.Devices{
				{
					ID:          deviceID1,
					Name:        "лампочка",
					HouseholdID: "h-id-2",
				},
			},
			userInfo: model.UserInfo{
				Devices: model.Devices{
					{
						ID:          deviceID1,
						Name:        "лампочка",
						HouseholdID: "h-id-2",
					},
					{
						ID:          deviceID2,
						Name:        "торшер",
						HouseholdID: "h-id-2",
					},
				},
				Households: model.Households{
					{
						ID:   "h-id-1",
						Name: "Дача",
					},
					{
						ID:   "h-id-2",
						Name: "Домик",
					},
				},
			},
			expectedNLG: libnlg.FromVariants([]string{
				"Окей, сделала в домике",
				"Сделала в домике",
				"Выполнила в домике",
			}),
		},
		{
			name:          "household_specified_nlg_3",
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.OnOffCapabilityType),
				CapabilityInstance: string(model.OnOnOffCapabilityInstance),
			},
			frame: frames.ActionFrameV2{
				Devices: frames.DeviceSlots{
					{
						DeviceIDs: []string{deviceID1},
						SlotType:  string(frames.DeviceSlotType),
					},
				},
			},
			devices: model.Devices{
				{
					ID:          deviceID1,
					Name:        "лампочка",
					HouseholdID: "h-id-1",
				},
			},
			userInfo: model.UserInfo{
				Devices: model.Devices{
					{
						ID:          deviceID1,
						Name:        "лампочка",
						HouseholdID: "h-id-1",
					},
					{
						ID:          deviceID2,
						Name:        "торшер",
						HouseholdID: "h-id-2",
					},
				},
				Households: model.Households{
					{
						ID:   "h-id-1",
						Name: "Дача",
					},
					{
						ID:   "h-id-2",
						Name: "Домик",
					},
				},
			},
			expectedNLG: libnlg.FromVariants([]string{
				"Окей, сделала на даче",
				"Сделала на даче",
				"Выполнила на даче",
			}),
		},
		{
			name:          "no_household_specification_1", // user only has one household
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.OnOffCapabilityType),
				CapabilityInstance: string(model.OnOnOffCapabilityInstance),
			},
			frame: frames.ActionFrameV2{
				Devices: frames.DeviceSlots{
					{
						DeviceIDs: []string{deviceID1},
						SlotType:  string(frames.DeviceSlotType),
					},
				},
			},
			devices: model.Devices{
				{
					ID:          deviceID1,
					Name:        "лампочка",
					HouseholdID: "h-id-1",
				},
			},
			userInfo: model.UserInfo{
				Devices: model.Devices{
					{
						ID:          deviceID1,
						Name:        "лампочка",
						HouseholdID: "h-id-1",
					},
					{
						ID:          deviceID2,
						Name:        "торшер",
						HouseholdID: "h-id-1",
					},
				},
				Households: model.Households{
					{
						ID:   "h-id-1",
						Name: "Дача",
					},
				},
			},
			expectedNLG: nlg.OK,
		},
		{
			name:          "no_household_specification_2", // all gathered devices are from the same household
			devicesResult: oneDoneDeviceDevicesResult,
			intentParameters: frames.ActionIntentParametersSlot{
				CapabilityType:     string(model.OnOffCapabilityType),
				CapabilityInstance: string(model.OnOnOffCapabilityInstance),
			},
			frame: frames.ActionFrameV2{
				Devices: frames.DeviceSlots{
					{
						DeviceType: string(model.LightDeviceType),
						SlotType:   string(frames.DeviceTypeSlotType),
					},
				},
				OnOffValue: &frames.OnOffValueSlot{
					Value: true,
				},
			},
			devices: model.Devices{ // suppose the second lamp was filtered out by client info
				{
					ID:          deviceID1,
					Name:        "лампочка",
					HouseholdID: "h-id-1",
				},
			},
			userInfo: model.UserInfo{
				Devices: model.Devices{
					{
						ID:          deviceID1,
						Name:        "лампочка",
						HouseholdID: "h-id-1",
						Type:        model.LightDeviceType,
					},
					{
						ID:          deviceID2,
						Name:        "торшер",
						HouseholdID: "h-id-2",
						Type:        model.LightDeviceType,
					},
				},
				Households: model.Households{
					{
						ID:   "h-id-1",
						Name: "Дача",
					},
					{
						ID:   "h-id-2",
						Name: "Домик",
					},
				},
			},
			expectedNLG: nlg.TurnOn,
		},
	}

	for _, input := range inputs {
		t.Run(input.name, func(t *testing.T) {
			logger := xtestlogs.NopLogger()
			runContext := xtestmegamind.NewRunContext(context.Background(), logger, nil).WithUserInfo(input.userInfo)
			inflectorClient := inflector.Client{Logger: logger}

			var actual libnlg.NLG
			assert.NotPanics(t, func() {
				actual = nlgByIntentParameters(runContext, input.devices, input.intentParameters, input.frame, &inflectorClient)
			})
			assert.Equal(t, input.expectedNLG, actual)
		})
	}
}

func errorDeviceResult(id string, errorCode adapter.ErrorCode) action.ProviderDeviceResult {
	return action.ProviderDeviceResult{
		ID:         id,
		ExternalID: id,
		ActionResults: map[string]adapter.CapabilityActionResultView{
			"bla": {
				Type: "bla",
				State: adapter.CapabilityStateActionResultView{
					Instance: "bla",
					ActionResult: adapter.StateActionResult{
						Status:       adapter.ERROR,
						ErrorCode:    errorCode,
						ErrorMessage: "error",
					},
				},
			},
		},
	}
}

func doneDeviceResult(id string) action.ProviderDeviceResult {
	return action.ProviderDeviceResult{
		ID:         id,
		ExternalID: id,
		ActionResults: map[string]adapter.CapabilityActionResultView{
			"bla": {
				Type: "bla",
				State: adapter.CapabilityStateActionResultView{
					Instance: "bla",
					ActionResult: adapter.StateActionResult{
						Status: adapter.DONE,
					},
				},
			},
		},
	}
}
