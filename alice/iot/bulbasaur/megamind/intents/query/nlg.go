package query

import (
	"strings"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/common"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/sdk"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/nlg"
	"a.yandex-team.ru/alice/library/go/inflector"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	libnlg "a.yandex-team.ru/alice/library/go/nlg"
	"a.yandex-team.ru/alice/library/go/tools"
)

func queryStateNLG(inf inflector.IInflector, onlineDevices model.Devices) libnlg.NLG {
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

func queryCapabilitiesNLG(inf inflector.IInflector, deviceCapabilityResults DeviceCapabilityQueryResults,
	intentParameters common.QueryIntentParametersSlice, onlineDevices model.Devices) libnlg.NLG {

	// multiple capability intent parameters are not supported, so we use the first instance in intent parameters
	capabilityType := intentParameters[0].CapabilityType
	capabilityInstance := intentParameters[0].CapabilityInstance

	switch {
	case model.CapabilityType(capabilityType) == model.OnOffCapabilityType:
		return queryOnOffNLG(inf, deviceCapabilityResults)
	case model.CapabilityType(capabilityType) == model.ToggleCapabilityType:
		return queryToggleNLG(inf, deviceCapabilityResults)
	case model.CapabilityType(capabilityType) == model.ModeCapabilityType && capabilityInstance == "":
		return queryAllModesNLG(inf, onlineDevices)
	}

	var msg string

	// If all capabilities in deviceCapabilityResults have the same type, instance and value (can be merged),
	// merge them and describe as one capability.
	capability, ok := deviceCapabilityResults.Capabilities().Merge()
	if ok && capability.State() != nil {
		// merge successful, target is Irrelevant in answer

		switch model.CapabilityType(capabilityType) {
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
		default:
			// unknown merged capability
			return nlg.CommonError
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
		switch model.CapabilityType(capabilityType) {
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

func queryOnOffNLG(inf inflector.IInflector, deviceCapabilityResults DeviceCapabilityQueryResults) libnlg.NLG {
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

func queryToggleNLG(inf inflector.IInflector, deviceCapabilityResults DeviceCapabilityQueryResults) libnlg.NLG {
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

func queryAllModesNLG(inf inflector.IInflector, onlineDevices model.Devices) libnlg.NLG {
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

func queryPropertiesNLG(applyContext sdk.ApplyContext, inf inflector.IInflector, devicePropertiesResults DevicePropertyQueryResults, intentParameters common.QueryIntentParametersSlice) (libnlg.NLG, *libmegamind.LEDAnimation) {
	var messages []string // each message is an NLG text about one property instance
	var animation *libmegamind.LEDAnimation

	// Get nlg for each instance of intent parameters
	for _, intentParametersInstance := range intentParameters {
		propertyInstance := model.PropertyInstance(intentParametersInstance.PropertyInstance)
		filteredPropertiesResults := devicePropertiesResults.filterByInstance(propertyInstance)
		filteredPropertiesResults = filteredPropertiesResults.filterByDeviceTypeForClimateInstances(devicePropertiesResults, propertyInstance)
		if len(filteredPropertiesResults) == 0 {
			continue
		}

		instanceMessages, instanceAnimation := singlePropertyNLG(applyContext, filteredPropertiesResults, propertyInstance, inf)
		messages = append(messages, instanceMessages...)
		animation = instanceAnimation
	}

	if len(messages) == 0 {
		if len(devicePropertiesResults.GroupByID()) > 1 {
			return nlg.MultipleQueryUnknownState, nil
		}
		return nlg.QueryUnknownState, nil
	}

	return libnlg.FromVariant(tools.FormatNLGText(strings.Join(messages, ", "))), animation
}

func singlePropertyNLG(applyContext sdk.ApplyContext, propertiesResults DevicePropertyQueryResults, requestedInstance model.PropertyInstance, inf inflector.IInflector) ([]string, *libmegamind.LEDAnimation) {
	messages := make([]string, 0, len(propertiesResults))
	var animation *libmegamind.LEDAnimation

	isMultipleDevices := len(propertiesResults) > 1
	if !isMultipleDevices || (propertiesResults.CanBeAveraged() && requestedInstance.CanBeAveraged()) {
		properties := propertiesResults.Properties()
		var resultValue float64
		var unit model.Unit

		// Calculate average value across properties results of the current propertyInstance.
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

		// Make nlg for averaged value and the current propertyInstance and append it to nlgs.
		state := model.FloatPropertyState{
			Instance: requestedInstance,
			Value:    resultValue,
		}
		parameters := model.FloatPropertyParameters{
			Instance: requestedInstance,
			Unit:     unit,
		}
		msg := model.GetFloatPropertyVoiceStatus(state, parameters, model.TargetNLGOptions{UseDeviceTarget: false})
		messages = append(messages, msg)

		if applyContext.Request().GetBaseRequest().GetInterfaces().GetHasLedDisplay() {
			switch {
			case parameters.Unit == model.UnitPercent:
				if percentAnimation, ok := libmegamind.PercentLEDAnimation(state.Value); ok {
					animation = &percentAnimation
				}
			case requestedInstance == model.TemperaturePropertyInstance && parameters.Unit == model.UnitTemperatureCelsius:
				if temperatureAnimation, ok := libmegamind.TemperatureLEDAnimation(state.Value); ok {
					animation = &temperatureAnimation
				}
			}
		}

		return messages, animation
	}

	// Multiple devices, can't be averaged.
	propertyMessages := make([]string, 0, len(propertiesResults))
	roomsCount := len(propertiesResults.Rooms())
	for _, devicePropertyResult := range propertiesResults {
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

	if len(propertyMessages) > 0 {
		msg := strings.Join(propertyMessages, ", ")
		messages = append(messages, msg)
	}

	return messages, animation
}
