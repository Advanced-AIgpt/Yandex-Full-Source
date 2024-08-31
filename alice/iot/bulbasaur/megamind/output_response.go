package megamind

import (
	"strings"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/nlg"
	"a.yandex-team.ru/alice/library/go/inflector"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	libnlg "a.yandex-team.ru/alice/library/go/nlg"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
)

func ErrorCodeNLG(errorCode adapter.ErrorCode) libnlg.NLG {
	switch errorCode {
	case adapter.DeviceUnreachable:
		return nlg.DeviceUnreachable
	case adapter.DeviceBusy:
		return nlg.DeviceBusy
	case adapter.DeviceNotFound:
		return nlg.DeviceNotFound
	case adapter.InternalError:
		return nlg.InternalError
	case adapter.InvalidAction:
		return nlg.InvalidAction
	case adapter.InvalidValue:
		return nlg.InvalidValue
	case adapter.NotSupportedInCurrentMode:
		return nlg.NotSupportedInCurrentMode
	case adapter.DoorOpen:
		return nlg.DoorOpenError
	case adapter.LidOpen:
		return nlg.LidOpenError
	case adapter.RemoteControlDisabled:
		return nlg.RemoteControlDisabledError
	case adapter.NotEnoughWater:
		return nlg.NotEnoughWaterError
	case adapter.LowChargeLevel:
		return nlg.LowChargeLevelError
	case adapter.ContainerFull:
		return nlg.ContainerFullError
	case adapter.ContainerEmpty:
		return nlg.ContainerEmptyError
	case adapter.DripTrayFull:
		return nlg.DripTrayFullError
	case adapter.DeviceStuck:
		return nlg.DeviceStuckError
	case adapter.DeviceOff:
		return nlg.DeviceOffError
	case adapter.FirmwareOutOfDate:
		return nlg.FirmwareOutOfDateError
	case adapter.NotEnoughDetergent:
		return nlg.NotEnoughDetergentError
	case adapter.AccountLinkingError:
		return nlg.AccountLinkingError
	case adapter.HumanInvolvementNeeded:
		return nlg.HumanInvolvementNeededError
	default:
		return nlg.CommonError
	}
}

func MultipleDevicesErrorCodeNLG(errorCode adapter.ErrorCode) libnlg.NLG {
	switch errorCode {
	case adapter.DeviceUnreachable:
		return nlg.MultipleDeviceUnreachable
	case adapter.DeviceBusy:
		return nlg.MultipleDeviceBusy
	case adapter.DeviceNotFound:
		return nlg.MultipleDeviceNotFound
	case adapter.InternalError:
		return nlg.MultipleInternalError
	case adapter.InvalidAction:
		return nlg.MultipleInvalidAction
	case adapter.InvalidValue:
		return nlg.MultipleInvalidValue
	case adapter.NotSupportedInCurrentMode:
		return nlg.MultipleNotSupportedInCurrentMode
	case adapter.DoorOpen:
		return nlg.MultipleDoorOpenError
	case adapter.LidOpen:
		return nlg.MultipleLidOpenError
	case adapter.RemoteControlDisabled:
		return nlg.MultipleRemoteControlDisabledError
	case adapter.NotEnoughWater:
		return nlg.MultipleNotEnoughWaterError
	case adapter.LowChargeLevel:
		return nlg.MultipleLowChargeLevelError
	case adapter.ContainerFull:
		return nlg.MultipleContainerFullError
	case adapter.ContainerEmpty:
		return nlg.MultipleContainerEmptyError
	case adapter.DripTrayFull:
		return nlg.MultipleDripTrayFullError
	case adapter.DeviceStuck:
		return nlg.MultipleDeviceStuckError
	case adapter.DeviceOff:
		return nlg.MultipleDeviceOffError
	case adapter.FirmwareOutOfDate:
		return nlg.MultipleFirmwareOutOfDateError
	case adapter.NotEnoughDetergent:
		return nlg.MultipleNotEnoughDetergentError
	case adapter.AccountLinkingError:
		return nlg.MultipleAccountLinkingError
	case adapter.HumanInvolvementNeeded:
		return nlg.MultipleHumanInvolvementNeededError
	default:
		return nlg.CommonError
	}
}

func QueryPropertiesOutputResponse(request *scenarios.TScenarioApplyRequest, inf inflector.IInflector, devicePropertiesResults DevicePropertyQueryResults, extractedQuery model.ExtractedQuery) libmegamind.OutputResponse {
	propertyInstance := model.PropertyInstance(extractedQuery.SearchFor.Instance)
	devicePropertiesResults = filterDevicePropertyResultsForInstance(devicePropertiesResults, propertyInstance)

	isMultipleDevices := len(devicePropertiesResults) > 1
	if !isMultipleDevices || (devicePropertiesResults.CanBeAveraged() && propertyInstance.CanBeAveraged()) {
		properties := devicePropertiesResults.Properties()
		var resultValue float64
		var unit model.Unit
		for _, p := range properties {
			if state := p.State(); state != nil {
				resultValue += state.(model.FloatPropertyState).Value
			}
			if parameters := p.Parameters(); parameters != nil {
				if pUnit := parameters.(model.FloatPropertyParameters).Unit; pUnit != "" {
					unit = pUnit
				}
			}
		}
		resultValue = resultValue / float64(len(properties))

		state := model.FloatPropertyState{
			Instance: propertyInstance,
			Value:    resultValue,
		}
		parameters := model.FloatPropertyParameters{
			Instance: propertyInstance,
			Unit:     unit,
		}
		msg := model.GetFloatPropertyVoiceStatus(state, parameters, model.TargetNLGOptions{UseDeviceTarget: false})
		var animation *libmegamind.LEDAnimation
		if request.GetBaseRequest().GetInterfaces().GetHasLedDisplay() {
			switch {
			case parameters.Unit == model.UnitPercent:
				if percentAnimation, ok := libmegamind.PercentLEDAnimation(state.Value); ok {
					animation = &percentAnimation
				}
			case propertyInstance == model.TemperaturePropertyInstance && parameters.Unit == model.UnitTemperatureCelsius:
				if temperatureAnimation, ok := libmegamind.TemperatureLEDAnimation(state.Value); ok {
					animation = &temperatureAnimation
				}
			}
		}
		return libmegamind.OutputResponse{
			NLG:       libnlg.FromVariant(tools.FormatNLGText(msg)),
			Animation: animation,
		}
	}

	propertyMessages := make([]string, 0, len(devicePropertiesResults))
	roomsCount := len(devicePropertiesResults.Rooms())
	for _, devicePropertyResult := range devicePropertiesResults {
		property := devicePropertyResult.Property
		if property.State() == nil || property.Type() != model.FloatPropertyType {
			continue
		}

		nlgOptions := model.TargetNLGOptions{
			UseDeviceTarget:  isMultipleDevices,
			DeviceName:       devicePropertyResult.Name,
			DeviceInflection: inflector.TryInflect(inf, devicePropertyResult.Name, inflector.GrammaticalCases),
		}
		if room := devicePropertyResult.Room; room != nil && roomsCount > 1 {
			nlgOptions.UseRoomTarget = true
			nlgOptions.RoomName = room.Name
			nlgOptions.RoomInflection = inflector.TryInflect(inf, room.Name, inflector.GrammaticalCases)
		}

		state := property.State().(model.FloatPropertyState)
		parameters := property.Parameters().(model.FloatPropertyParameters)
		propertyMsg := model.GetFloatPropertyVoiceStatus(state, parameters, nlgOptions)
		propertyMessages = append(propertyMessages, propertyMsg)
	}

	if len(propertyMessages) == 0 {
		if isMultipleDevices {
			return libmegamind.OutputResponse{NLG: nlg.MultipleQueryUnknownState}
		}
		return libmegamind.OutputResponse{NLG: nlg.QueryUnknownState}

	}
	msg := strings.Join(propertyMessages, ", ")
	return libmegamind.OutputResponse{NLG: libnlg.FromVariant(tools.FormatNLGText(msg))}
}

func filterDevicePropertyResultsForInstance(devicePropertiesResults DevicePropertyQueryResults, propertyInstance model.PropertyInstance) DevicePropertyQueryResults {
	switch propertyInstance {
	case model.TemperaturePropertyInstance, model.HumidityPropertyInstance, model.CO2LevelPropertyInstance, model.PressurePropertyInstance:
		deviceTypeResultsMap := devicePropertiesResults.GroupByDeviceType()
		// https://st.yandex-team.ru/IOT-720#5fc806033ca599731bf6465b
		if results, ok := deviceTypeResultsMap[model.SensorDeviceType]; ok {
			return results
		}
		if results, ok := deviceTypeResultsMap[model.AcDeviceType]; ok {
			return results
		}
		if results, ok := deviceTypeResultsMap[model.HumidifierDeviceType]; ok {
			return results
		}
		if results, ok := deviceTypeResultsMap[model.PurifierDeviceType]; ok {
			return results
		}
		if results, ok := deviceTypeResultsMap[model.OtherDeviceType]; ok {
			return results
		}
	}
	return devicePropertiesResults
}

func QueryOnOffNLG(inf inflector.IInflector, deviceCapabilityResults DeviceCapabilityQueryResults) libnlg.NLG {
	// need to gather state info
	onOffTargetStates := make([]bool, 0)
	for _, capability := range deviceCapabilityResults.Capabilities() {
		if capability.Type() != model.OnOffCapabilityType || capability.State() == nil {
			continue
		}
		stateValue := capability.State().(model.OnOffCapabilityState).Value
		onOffTargetStates = append(onOffTargetStates, stateValue)
	}

	if len(onOffTargetStates) == 0 {
		if len(deviceCapabilityResults) > 1 {
			return nlg.MultipleQueryUnknownState
		}
		return nlg.QueryUnknownState
	}

	hasSameOnOffTargetState := true
	for i := 1; i < len(onOffTargetStates); i++ {
		if onOffTargetStates[i] != onOffTargetStates[i-1] {
			hasSameOnOffTargetState = false
			break
		}
	}

	// need to gather device type info
	deviceTypeResults := deviceCapabilityResults.GroupByDeviceType()
	if len(deviceTypeResults) > 1 {
		return nlg.MultipleQueryUnknownState
	}
	// map always has one entry, this code picks first
	var dt model.DeviceType
	for dtValue := range deviceTypeResults {
		dt = dtValue
	}

	isMultipleDevices := len(deviceCapabilityResults) > 1
	if hasSameOnOffTargetState && isMultipleDevices {
		var msg string
		isOn := onOffTargetStates[0]
		switch dt {
		case model.OpenableDeviceType, model.CurtainDeviceType:
			if isOn {
				msg = "Устройства открыты"
			} else {
				msg = "Устройства закрыты"
			}
		default:
			if isOn {
				msg = "Устройства включены"
			} else {
				msg = "Устройства выключены"
			}
		}
		return libnlg.FromVariant(tools.FormatNLGText(msg))
	}

	roomsCount := len(deviceCapabilityResults.Rooms())
	results := make([]string, 0, len(deviceCapabilityResults))
	for _, deviceCapabilityResult := range deviceCapabilityResults {
		capability := deviceCapabilityResult.Capability
		if capability.State() == nil {
			continue
		}

		nlgOptions := model.TargetNLGOptions{
			UseDeviceTarget:  true,
			DeviceName:       deviceCapabilityResult.Name,
			DeviceInflection: inflector.TryInflect(inf, deviceCapabilityResult.Name, inflector.GrammaticalCases),
		}
		if room := deviceCapabilityResult.Room; room != nil && roomsCount > 1 {
			nlgOptions.UseRoomTarget = true
			nlgOptions.RoomName = room.Name
			nlgOptions.RoomInflection = inflector.TryInflect(inf, room.Name, inflector.GrammaticalCases)
		}
		state := capability.State().(model.OnOffCapabilityState)
		result := model.GetDeviceOnOffCapabilityVoiceStatus(state, deviceCapabilityResult.DeviceType, nlgOptions)
		results = append(results, result)
	}
	if len(results) == 0 {
		if len(deviceCapabilityResults) > 1 {
			return nlg.MultipleQueryUnknownState
		}
		return nlg.QueryUnknownState
	}
	msg := strings.Join(results, ", ")
	return libnlg.FromVariant(tools.FormatNLGText(msg))
}

func QueryToggleNLG(inf inflector.IInflector, deviceCapabilityResults DeviceCapabilityQueryResults) libnlg.NLG {
	toggleTargetStates := make([]bool, 0)
	for _, capability := range deviceCapabilityResults.Capabilities() {
		if capability.Type() != model.ToggleCapabilityType || capability.State() == nil {
			continue
		}
		stateValue := capability.State().(model.ToggleCapabilityState).Value
		toggleTargetStates = append(toggleTargetStates, stateValue)
	}

	if len(toggleTargetStates) == 0 {
		if len(deviceCapabilityResults) > 1 {
			return nlg.MultipleQueryUnknownState
		}
		return nlg.QueryUnknownState
	}

	hasSameTargetState := true
	for i := 1; i < len(toggleTargetStates); i++ {
		if toggleTargetStates[i] != toggleTargetStates[i-1] {
			hasSameTargetState = false
			break
		}
	}

	if hasSameTargetState {
		for _, deviceCapabilityResult := range deviceCapabilityResults {
			capability := deviceCapabilityResult.Capability
			if capability.State() == nil {
				continue
			}
			state := capability.State().(model.ToggleCapabilityState)
			msg := model.GetToggleCapabilityVoiceStatus(state, model.TargetNLGOptions{UseDeviceTarget: false})
			return libnlg.FromVariant(tools.FormatNLGText(msg))
		}
	}

	roomsCount := len(deviceCapabilityResults.Rooms())
	results := make([]string, 0, len(deviceCapabilityResults))
	for _, deviceCapabilityResult := range deviceCapabilityResults {
		capability := deviceCapabilityResult.Capability
		if capability.State() == nil {
			continue
		}

		nlgOptions := model.TargetNLGOptions{
			UseDeviceTarget:  true,
			DeviceName:       deviceCapabilityResult.Name,
			DeviceInflection: inflector.TryInflect(inf, deviceCapabilityResult.Name, inflector.GrammaticalCases),
		}
		if room := deviceCapabilityResult.Room; room != nil && roomsCount > 1 {
			nlgOptions.UseRoomTarget = true
			nlgOptions.RoomName = room.Name
			nlgOptions.RoomInflection = inflector.TryInflect(inf, room.Name, inflector.GrammaticalCases)
		}
		state := capability.State().(model.ToggleCapabilityState)
		result := model.GetToggleCapabilityVoiceStatus(state, nlgOptions)
		results = append(results, result)
	}
	if len(results) == 0 {
		if len(deviceCapabilityResults) > 1 {
			return nlg.MultipleQueryUnknownState
		}
		return nlg.QueryUnknownState
	}
	msg := strings.Join(results, ", ")
	return libnlg.FromVariant(tools.FormatNLGText(msg))
}

func QueryCapabilitiesNLG(inf inflector.IInflector, deviceCapabilityResults DeviceCapabilityQueryResults, extractedQuery model.ExtractedQuery) libnlg.NLG {
	// onOffs and toggles have special rules for state merging
	switch model.CapabilityType(extractedQuery.SearchFor.Type) {
	case model.OnOffCapabilityType:
		return QueryOnOffNLG(inf, deviceCapabilityResults)
	case model.ToggleCapabilityType:
		return QueryToggleNLG(inf, deviceCapabilityResults)
	}

	var msg string
	capability, ok := deviceCapabilityResults.Capabilities().Merge()
	if ok && capability.State() != nil {
		// merge successful, target is Irrelevant in answer

		switch model.CapabilityType(extractedQuery.SearchFor.Type) {
		case model.ColorSettingCapabilityType:
			state := capability.State().(model.ColorSettingCapabilityState)
			// we have to return status no matter what even if color is not in our palette
			msg, _ = model.GetColorCapabilityVoiceStatus(state, model.TargetNLGOptions{UseDeviceTarget: false})
		case model.ModeCapabilityType:
			state := capability.State().(model.ModeCapabilityState)
			msg = model.GetModeCapabilityVoiceStatus(state, model.TargetNLGOptions{UseDeviceTarget: false})
		case model.RangeCapabilityType:
			state := capability.State().(model.RangeCapabilityState)
			parameters := capability.Parameters().(model.RangeCapabilityParameters)
			msg = model.GetRangeCapabilityVoiceStatus(state, parameters, model.TargetNLGOptions{UseDeviceTarget: false})
		}
		return libnlg.FromVariant(tools.FormatNLGText(msg))
	}

	roomsCount := len(deviceCapabilityResults.Rooms())
	results := make([]string, 0, len(deviceCapabilityResults))
	for _, deviceCapabilityResult := range deviceCapabilityResults {
		capability = deviceCapabilityResult.Capability
		if capability.State() == nil {
			continue
		}

		deviceName := deviceCapabilityResult.Name
		nlgOptions := model.TargetNLGOptions{
			UseDeviceTarget:  true,
			DeviceName:       deviceName,
			DeviceInflection: inflector.TryInflect(inf, deviceName, inflector.GrammaticalCases),
		}
		if room := deviceCapabilityResult.Room; room != nil && roomsCount > 1 {
			nlgOptions.UseRoomTarget = true
			nlgOptions.RoomName = room.Name
			nlgOptions.RoomInflection = inflector.TryInflect(inf, room.Name, inflector.GrammaticalCases)
		}
		switch model.CapabilityType(extractedQuery.SearchFor.Type) {
		case model.ColorSettingCapabilityType:
			state := capability.State().(model.ColorSettingCapabilityState)
			result, hasColorStatus := model.GetColorCapabilityVoiceStatus(state, nlgOptions)
			if hasColorStatus {
				results = append(results, result)
			}
		case model.ModeCapabilityType:
			state := capability.State().(model.ModeCapabilityState)
			result := model.GetModeCapabilityVoiceStatus(state, nlgOptions)
			results = append(results, result)
		case model.RangeCapabilityType:
			state := capability.State().(model.RangeCapabilityState)
			parameters := capability.Parameters().(model.RangeCapabilityParameters)
			result := model.GetRangeCapabilityVoiceStatus(state, parameters, nlgOptions)
			results = append(results, result)
		}
	}
	if len(results) == 0 {
		if len(deviceCapabilityResults) > 1 {
			return nlg.MultipleQueryUnknownState
		}
		return nlg.QueryUnknownState
	}
	msg = strings.Join(results, ", ")
	return libnlg.FromVariant(tools.FormatNLGText(msg))
}

func QueryStateNLG(inf inflector.IInflector, onlineDevices model.Devices) libnlg.NLG {
	deviceMessages := make([]string, 0, len(onlineDevices))
	roomsCount := len(onlineDevices.GetRooms())
	for _, device := range onlineDevices {
		deviceName := device.Name
		nlgOptions := model.TargetNLGOptions{
			UseDeviceTarget:  true,
			DeviceName:       deviceName,
			DeviceInflection: inflector.TryInflect(inf, deviceName, inflector.GrammaticalCases),
		}
		if room := device.Room; room != nil && roomsCount > 1 {
			nlgOptions.UseRoomTarget = true
			nlgOptions.RoomName = room.Name
			nlgOptions.RoomInflection = inflector.TryInflect(inf, room.Name, inflector.GrammaticalCases)
		}
		deviceMessage := model.GetDeviceStateVoiceStatus(device, nlgOptions)
		deviceMessages = append(deviceMessages, strings.TrimSpace(deviceMessage))
	}
	msg := strings.Join(deviceMessages, "; ")
	return libnlg.FromVariant(tools.FormatNLGText(msg))
}

func QueryAllModesNLG(inf inflector.IInflector, onlineDevices model.Devices) libnlg.NLG {
	modeQueryResults := make(map[string]DeviceCapabilityQueryResults)
	for _, d := range onlineDevices {
		for _, c := range d.Capabilities {
			if c.Type() == model.ModeCapabilityType {
				modeQueryResult := DeviceCapabilityQueryResult{
					Name:       d.Name,
					DeviceType: d.Type,
					Capability: c,
				}
				modeQueryResults[c.Key()] = append(modeQueryResults[c.Key()], modeQueryResult)
			}
		}
	}

	modeMessages := make([]string, 0, len(modeQueryResults))
	for _, queryResults := range modeQueryResults {
		capability, ok := queryResults.Capabilities().Merge()
		if !ok {
			continue
		}
		if capability.State() != nil {
			// merge successful, target is Irrelevant in answer
			state := capability.State().(model.ModeCapabilityState)
			msg := model.GetModeCapabilityVoiceStatus(state, model.TargetNLGOptions{UseDeviceTarget: false})
			modeMessages = append(modeMessages, msg)
			continue
		}
		// states can't be merged, iterate over device states
		roomsCount := len(queryResults.Rooms())
		for _, deviceQueryResult := range queryResults {
			capability = deviceQueryResult.Capability
			if capability.State() == nil {
				continue
			}
			deviceName := deviceQueryResult.Name

			inflection := inflector.TryInflect(inf, deviceName, inflector.GrammaticalCases)
			nlgOptions := model.TargetNLGOptions{
				UseDeviceTarget:  true,
				DeviceName:       deviceName,
				DeviceInflection: inflection,
			}
			if room := deviceQueryResult.Room; room != nil && roomsCount > 1 {
				nlgOptions.UseRoomTarget = true
				nlgOptions.RoomName = room.Name
				nlgOptions.RoomInflection = inflector.TryInflect(inf, room.Name, inflector.GrammaticalCases)
			}
			state := capability.State().(model.ModeCapabilityState)
			msg := model.GetModeCapabilityVoiceStatus(state, nlgOptions)
			modeMessages = append(modeMessages, strings.TrimSpace(msg))
		}
	}

	if len(modeMessages) == 0 {
		if len(onlineDevices) > 1 {
			return nlg.MultipleQueryUnknownState
		}
		return nlg.QueryUnknownState
	}
	msg := strings.Join(modeMessages, ", ")
	return libnlg.FromVariant(tools.FormatNLGText(msg))
}
