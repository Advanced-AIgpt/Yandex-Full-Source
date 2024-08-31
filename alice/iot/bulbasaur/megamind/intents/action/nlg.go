package action

import (
	"fmt"
	"strings"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/action"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/common"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/frames"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/sdk"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/nlg"
	"a.yandex-team.ru/alice/library/go/inflector"
	libnlg "a.yandex-team.ru/alice/library/go/nlg"
	"a.yandex-team.ru/library/go/slices"
)

func nlgFromDevicesResult(actionIntentNLG libnlg.NLG, devicesResult action.DevicesResult) libnlg.NLG {
	deviceResults := devicesResult.Flatten()
	if len(deviceResults) == 0 {
		return actionIntentNLG
	}

	deviceErrors := devicesResult.ErrorCodeCountMap()
	if len(deviceErrors) > 0 {
		return deviceErrorNLG(deviceErrors, deviceResults, actionIntentNLG)
	}

	return actionIntentNLG.UseTextOnly(devicesResult.ContainsTextOnlyNLG())
}

func deviceErrorNLG(
	errorCodeCountMap adapter.ErrorCodeCountMap,
	deviceResults []action.ProviderDeviceResult,
	defaultNLG libnlg.NLG,
) libnlg.NLG {
	if len(deviceResults) == 1 {
		for errorCode := range errorCodeCountMap { // there's only one key-value pair in errorCodeCountMap
			return megamind.ErrorCodeNLG(errorCode)
		}
	}

	totalErrs := errorCodeCountMap.Total()

	// all devices returned an error
	if totalErrs == len(deviceResults) {
		// same error code from all devices
		if len(errorCodeCountMap) == 1 {
			for errorCode := range errorCodeCountMap {
				return megamind.MultipleDevicesErrorCodeNLG(errorCode)
			}
		}

		// different error codes
		return nlg.AllActionsFailedError
	}

	// some devices returned an error
	return defaultNLG
}

func nlgByIntentParameters(
	runContext sdk.RunContext,
	devices model.Devices,
	winnerIntentParameters frames.ActionIntentParametersSlot,
	frame frames.ActionFrameV2,
	inflectorClient inflector.IInflector,
) libnlg.NLG {
	// Specify household if user has multiple households,
	// but all devices from the frame are from one household: https://st.yandex-team.ru/IOT-962.
	userInfo, _ := runContext.UserInfo()
	if len(userInfo.Households) > 1 && len(frame.Households.HouseholdIDs()) == 0 {
		gatheredDevices, _ := frame.GatherDevices(runContext)
		if len(gatheredDevices) == 0 {
			return nlg.CommonError
		}
		if len(gatheredDevices.GroupByHousehold()) == 1 {
			return householdSpecifiedNLG(userInfo, devices, inflectorClient)
		}
	}

	switch model.CapabilityType(winnerIntentParameters.CapabilityType) {
	case model.OnOffCapabilityType:
		return onOffCapabilityNLG(frame, winnerIntentParameters)
	case model.RangeCapabilityType:
		return rangeCapabilityNLG(frame, winnerIntentParameters)
	case model.ToggleCapabilityType:
		return toggleCapabilityNLG(winnerIntentParameters)
	case model.ColorSettingCapabilityType:
		return colorSettingCapabilityNLG(winnerIntentParameters)
	case model.ModeCapabilityType:
		return modeCapabilityNLG(frame, winnerIntentParameters)
	default:
		return nlg.OK
	}
}

func householdSpecifiedNLG(userInfo model.UserInfo, devices model.Devices, inflectorClient inflector.IInflector) libnlg.NLG {
	if len(devices) == 0 {
		return nlg.OK
	}

	// At this moment all devices must be in one household, so it's safe to use household from any of them.
	userDevice, ok := userInfo.Devices.GetDeviceByID(devices[0].ID)
	if !ok {
		return nlg.OK
	}

	household, ok := userInfo.Households.GetByID(userDevice.HouseholdID)
	if !ok {
		return nlg.OK
	}

	householdInflection := inflector.TryInflect(inflectorClient, household.Name, inflector.GrammaticalCases)
	householdNLG := model.GetHouseholdSpecifiedNLG(householdInflection)

	return libnlg.FromVariants(householdNLG.Variants)
}

func onOffCapabilityNLG(frame frames.ActionFrameV2, intentParameters frames.ActionIntentParametersSlot) libnlg.NLG {
	switch {
	case intentParameters.RelativityType == string(common.Invert):
		return nlg.OK
	case frame.OnOffValue != nil && frame.OnOffValue.Value:
		switch {
		case slices.ContainsAny(frame.RequiredDeviceType.DeviceTypes, []string{string(model.OpenableDeviceType), string(model.CurtainDeviceType)}):
			return nlg.Open
		case slices.ContainsAny(frame.RequiredDeviceType.DeviceTypes, []string{string(model.KettleDeviceType)}):
			return nlg.Boil
		case slices.ContainsAny(frame.RequiredDeviceType.DeviceTypes, []string{string(model.VacuumCleanerDeviceType)}):
			return nlg.OK
		default:
			return nlg.TurnOn
		}
	case frame.OnOffValue != nil && !frame.OnOffValue.Value:
		switch {
		case slices.ContainsAny(frame.RequiredDeviceType.DeviceTypes, []string{string(model.OpenableDeviceType), string(model.CurtainDeviceType)}):
			return nlg.Close
		case slices.ContainsAny(frame.RequiredDeviceType.DeviceTypes, []string{string(model.VacuumCleanerDeviceType)}):
			return nlg.OK
		default:
			return nlg.TurnOff
		}
	default:
		return nlg.OK
	}
}

func rangeCapabilityNLG(frame frames.ActionFrameV2, intentParameters frames.ActionIntentParametersSlot) libnlg.NLG {
	switch {
	case frame.RangeValue != nil && frame.RangeValue.StringValue == frames.MaxRangeValue:
		switch intentParameters.CapabilityInstance {
		case string(model.BrightnessRangeInstance):
			return nlg.MaxBrightness
		case string(model.TemperatureRangeInstance):
			return nlg.MaxTemperature
		case string(model.HumidityRangeInstance):
			return nlg.MaxHumidity
		case string(model.VolumeRangeInstance):
			return nlg.MaxVolume
		default:
			return nlg.OK
		}
	case frame.RangeValue != nil && frame.RangeValue.StringValue == frames.MinRangeValue:
		switch intentParameters.CapabilityInstance {
		case string(model.BrightnessRangeInstance):
			return nlg.MaxDim
		case string(model.TemperatureRangeInstance):
			return nlg.MinTemperature
		case string(model.HumidityRangeInstance):
			return nlg.MinHumidity
		case string(model.VolumeRangeInstance):
			return nlg.MinVolume
		default:
			return nlg.OK
		}
	case intentParameters.RelativityType == string(common.Increase):
		switch intentParameters.CapabilityInstance {
		case string(model.BrightnessRangeInstance):
			return nlg.Brighten
		case string(model.TemperatureRangeInstance):
			return nlg.IncreaseTemperature
		case string(model.HumidityRangeInstance):
			return nlg.IncreaseHumidity
		case string(model.VolumeRangeInstance):
			return nlg.IncreaseVolume
		case string(model.ChannelRangeInstance):
			return nlg.IncreaseChannel
		default:
			return nlg.OK
		}
	case intentParameters.RelativityType == string(common.Decrease):
		switch intentParameters.CapabilityInstance {
		case string(model.BrightnessRangeInstance):
			return nlg.Dim
		case string(model.TemperatureRangeInstance):
			return nlg.DecreaseTemperature
		case string(model.HumidityRangeInstance):
			return nlg.DecreaseHumidity
		case string(model.VolumeRangeInstance):
			return nlg.DecreaseVolume
		case string(model.ChannelRangeInstance):
			return nlg.DecreaseChannel
		default:
			return nlg.OK
		}
	default:
		switch intentParameters.CapabilityInstance {
		case string(model.BrightnessRangeInstance):
			return nlg.ChangeBrightness
		case string(model.TemperatureRangeInstance):
			return nlg.ChangeTemperature
		case string(model.HumidityRangeInstance):
			return nlg.ChangeHumidity
		case string(model.VolumeRangeInstance):
			return nlg.ChangeVolume
		case string(model.ChannelRangeInstance):
			return nlg.ChangeChannel
		default:
			return nlg.OK
		}
	}
}

func toggleCapabilityNLG(intentParameters frames.ActionIntentParametersSlot) libnlg.NLG {
	switch intentParameters.CapabilityInstance {
	case string(model.MuteToggleCapabilityInstance):
		return nlg.PressMuteButton
	default:
		return nlg.OK
	}
}

func colorSettingCapabilityNLG(intentParameters frames.ActionIntentParametersSlot) libnlg.NLG {
	switch intentParameters.CapabilityInstance {
	case model.HypothesisColorCapabilityInstance:
		return nlg.ChangeColor
	case string(model.TemperatureKCapabilityInstance):
		switch intentParameters.RelativityType {
		case string(common.Increase):
			return nlg.IncreaseTemperatureK
		case string(common.Decrease):
			return nlg.DecreaseTemperatureK
		default:
			return nlg.OK
		}
	default:
		return nlg.OK
	}
}

func modeCapabilityNLG(frame frames.ActionFrameV2, intentParameters frames.ActionIntentParametersSlot) libnlg.NLG {
	switch {
	case intentParameters.RelativityType == string(common.Increase):
		switch intentParameters.CapabilityInstance {
		case string(model.ThermostatModeInstance):
			return nlg.NextWorkingMode
		default:
			return nlg.NextMode
		}
	case intentParameters.RelativityType == string(common.Decrease):
		switch intentParameters.CapabilityInstance {
		case string(model.ThermostatModeInstance):
			return nlg.PreviousWorkingMode
		default:
			return nlg.PreviousMode
		}
	default:
		switch {
		case frame.ModeValue != nil && CommonSwitchModeNames[frame.ModeValue.ModeValue] != "":
			return SwitchToMode(frame.ModeValue.ModeValue)
		case frame.ModeValue != nil && frame.ModeValue.ModeValue == model.HeatMode:
			return nlg.SwitchToHeatMode
		case frame.ModeValue != nil && frame.ModeValue.ModeValue == model.CoolMode:
			return nlg.SwitchToCoolMode
		default:
			return nlg.OK
		}
	}
}

func SwitchToMode(modeValue model.ModeValue) libnlg.NLG {
	modeName, ok := CommonSwitchModeNames[modeValue]
	if !ok {
		return nlg.OK
	}

	return libnlg.NLG{
		libnlg.NewAssetWithText(fmt.Sprintf("Включаю %s.", strings.ToLower(modeName))),
		libnlg.NewAssetWithText(fmt.Sprintf("%s активирован.", modeName)),
		libnlg.NewAssetWithText(fmt.Sprintf("Окей. %s.", modeName)),
		libnlg.NewAssetWithText(fmt.Sprintf("Как скажете. %s.", modeName)),
		libnlg.NewAssetWithText(fmt.Sprintf("%s, поехали.", modeName)),
	}
}

// CommonSwitchModeNames contains mode names that can be used in SwitchToMode function
var CommonSwitchModeNames = map[model.ModeValue]string{
	model.EcoMode:          "Эко-режим",
	model.AutoMode:         "Автоматический режим",
	model.DryMode:          "Режим осушения",
	model.FanOnlyMode:      "Режим вентиляции",
	model.GlassMode:        "Режим мойки стекла",
	model.PreRinseMode:     "Режим ополаскивания",
	model.PreHeatMode:      "Режим подогрева",
	model.TurboMode:        "Турбо режим",
	model.FastMode:         "Быстрый режим",
	model.SlowMode:         "Медленный режим",
	model.ExpressMode:      "Экспресс-режим",
	model.QuietMode:        "Тихий режим",
	model.NormalMode:       "Нормальный режим",
	model.HorizontalMode:   "Горизонтальный режим",
	model.VerticalMode:     "Вертикальный режим",
	model.StationaryMode:   "Неподвижный режим",
	model.IntensiveMode:    "Интенсивный режим",
	model.VacuumMode:       "Режим вакуум",
	model.BoilingMode:      "Режим варки",
	model.BakingMode:       "Режим выпечки",
	model.DessertMode:      "Режим десерта",
	model.FowlMode:         "Режим дичь",
	model.BabyFoodMode:     "Режим детского питания",
	model.FryingMode:       "Режим жарки",
	model.YogurtMode:       "Режим йогурт",
	model.CerealsMode:      "Режим крупы",
	model.MacaroniMode:     "Режим макароны",
	model.MilkPorridgeMode: "Режим молочная каша",
	model.MulticookerMode:  "Режим мультиповар",
	model.SteamMode:        "Режим пар",
	model.PastaMode:        "Режим паста",
	model.PizzaMode:        "Режим пицца",
	model.PilafMode:        "Режим плов",
	model.SauceMode:        "Режим соус",
	model.StewingMode:      "Режим тушение",
	model.SlowCookMode:     "Режим томление",
	model.DeepFryerMode:    "Режим фритюр",
	model.BreadMode:        "Режим хлеб",
	model.AspicMode:        "Режим холодец",
	model.CheesecakeMode:   "Режим чизкейк",
}

func customIntervalActionNLG(devices model.Devices, createdTime time.Time, intervalEndTime time.Time, location *time.Location) libnlg.NLG {
	if len(devices) == 0 {
		return nlg.CommonIntervalAction(createdTime, intervalEndTime, location)
	}

	hasOnlyOnOffCapabilities := true
	onOffTargetStates := make([]bool, 0)
	for _, device := range devices {
		for _, capability := range device.Capabilities {
			if capability.Type() != model.OnOffCapabilityType {
				hasOnlyOnOffCapabilities = false
				break
			}
			stateValue := capability.State().(model.OnOffCapabilityState).Value
			onOffTargetStates = append(onOffTargetStates, stateValue)
		}
	}

	hasSameOnOffTargetState := true
	for i := 1; i < len(onOffTargetStates); i++ {
		if onOffTargetStates[i] != onOffTargetStates[i-1] {
			hasSameOnOffTargetState = false
			break
		}
	}

	if hasOnlyOnOffCapabilities && hasSameOnOffTargetState {
		return nlg.OnOffStateIntervalAction(onOffTargetStates[0], createdTime, intervalEndTime, location)
	}
	return nlg.CommonIntervalAction(createdTime, intervalEndTime, location)
}
