package data

import (
	"fmt"
	"strings"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/testing"
	"a.yandex-team.ru/alice/library/go/random"
	"a.yandex-team.ru/alice/library/go/tools"
)

func GenerateDevice() (device model.Device) {
	for strings.TrimSpace(device.ExternalName) == "" {
		device.ExternalName = testing.RandString(random.RandRange(5, 100))
	}
	device.Aliases = make([]string, 0)
	device.ExternalID = strings.ReplaceAll(testing.RandString(random.RandRange(5, 100)), " ", "-")
	device.OriginalType = model.DeviceType(random.Choice(model.KnownDeviceTypes))

	// hack to prevent errors with multiroom
	nonSpeakerTypes := make([]string, 0, len(model.KnownDeviceTypes))
	for _, dt := range model.KnownDeviceTypes {
		if model.DeviceType(dt).IsSmartSpeaker() {
			continue
		}
		nonSpeakerTypes = append(nonSpeakerTypes, dt)
	}
	device.OriginalType = model.DeviceType(random.Choice(nonSpeakerTypes))

	device.Type = device.OriginalType
	device.SkillID = model.VIRTUAL
	device.Capabilities = generateCapabilities(random.RandRange(0, len(model.KnownCapabilityTypes)+1))
	device.Properties = generateProperties(random.RandRange(0, len(model.KnownPropertyTypes)+1))

	device.Status = model.UnknownDeviceStatus

	return device
}

func generateCapabilities(n int) (capabilities []model.ICapability) {
	if n == 0 {
		return make([]model.ICapability, 0)
	}

	skipType := make(map[model.CapabilityType]bool)

	capability := generateCapability(model.OnOffCapabilityType)
	capabilities = append(capabilities, capability)
	skipType[model.OnOffCapabilityType] = true

	for len(capabilities) < n && len(model.KnownCapabilityTypes) > len(skipType) {
		unusedCapabilityTypes := make([]string, 0, len(model.KnownCapabilityTypes)-len(skipType))
		for _, cType := range model.KnownCapabilityTypes {
			if !skipType[cType] {
				unusedCapabilityTypes = append(unusedCapabilityTypes, string(cType))
			}
		}

		cType := model.CapabilityType(random.Choice(unusedCapabilityTypes))
		c := generateCapability(cType)

		skipType[cType] = true
		capabilities = append(capabilities, c)
	}

	return
}

func generateCapability(cType model.CapabilityType) model.ICapability {
	c := model.MakeCapabilityByType(cType)
	c.SetRetrievable(random.FlipCoin())

	switch cType {
	case model.OnOffCapabilityType:
		c.SetParameters(model.OnOffCapabilityParameters{})
		if c.Retrievable() && random.FlipCoin() {
			c.SetState(generateCapabilityState(c))
		}

	case model.ColorSettingCapabilityType:
		params := model.ColorSettingCapabilityParameters{}
		switch random.FlipCube(3) {
		case 0:
			minTemperatureK := random.RandRange(2000, 8001)
			params.TemperatureK = &model.TemperatureKParameters{
				Min: model.TemperatureK(minTemperatureK),
				Max: model.TemperatureK(random.RandRange(minTemperatureK, 8001)),
			}
		case 1:
			if random.FlipCoin() {
				params.ColorModel = model.CM(model.HsvModelType)
			} else {
				params.ColorModel = model.CM(model.RgbModelType)
			}
		case 2:
			if random.FlipCoin() {
				params.ColorModel = model.CM(model.HsvModelType)
			} else {
				params.ColorModel = model.CM(model.RgbModelType)
			}

			minTemperatureK := random.RandRange(2000, 8000)
			params.TemperatureK = &model.TemperatureKParameters{
				Min: model.TemperatureK(minTemperatureK),
				Max: model.TemperatureK(random.RandRange(minTemperatureK, 8000)),
			}
		}
		c.SetParameters(params)

		if c.Retrievable() && random.FlipCoin() {
			c.SetState(generateCapabilityState(c))
		}

	case model.RangeCapabilityType:
		var rng *model.Range
		if random.FlipCoin() {
			minRange := random.RandRange(0, 100)
			rng = &model.Range{
				Max:       float64(random.RandRange(minRange, 100)),
				Min:       float64(minRange),
				Precision: 1,
			}
		}
		var instances []string
		for instance := range model.KnownRangeInstanceNames {
			instances = append(instances, string(instance))
		}
		params := model.RangeCapabilityParameters{
			Instance:     model.RangeCapabilityInstance(random.Choice(instances)),
			RandomAccess: random.FlipCoin(),
			Looped:       random.FlipCoin(),
			Range:        rng,
		}
		c.SetParameters(params)

		if c.Retrievable() && params.RandomAccess && random.FlipCoin() {
			c.SetState(generateCapabilityState(c))
		}

	case model.ModeCapabilityType:
		modeInstances := make([]string, 0, len(model.KnownModeInstancesNames))
		for _, modeInstanceName := range model.KnownModeInstancesNames {
			modeInstances = append(modeInstances, modeInstanceName)
		}
		instance := random.Choice(modeInstances)

		var knownModes []model.Mode
		for _, mode := range model.KnownModes {
			knownModes = append(knownModes, mode)
		}

		randomRange := random.RandRange(1, len(model.KnownModes))
		modes := make([]model.Mode, 0, randomRange)
		for _, mode := range knownModes {
			modes = append(modes, mode)
			randomRange--
			if randomRange == 0 {
				break
			}
		}

		params := model.ModeCapabilityParameters{
			Instance: model.ModeCapabilityInstance(instance),
			Modes:    modes,
		}
		c.SetParameters(params)

	case model.ToggleCapabilityType:
		var instances []string
		for instance := range model.KnownToggleInstanceNames {
			instances = append(instances, string(instance))
		}
		instance := random.Choice(instances)
		params := model.ToggleCapabilityParameters{
			Instance: model.ToggleCapabilityInstance(instance),
		}
		c.SetParameters(params)

	case model.CustomButtonCapabilityType:
		params := model.CustomButtonCapabilityParameters{
			Instance:      model.CustomButtonCapabilityInstance(testing.RandLatinString(20)),
			InstanceNames: []string{testing.RandCyrillicWithNumbersString(20)},
		}
		c.SetParameters(params)

		if c.Retrievable() && random.FlipCoin() {
			c.SetState(generateCapabilityState(c))
		}

	case model.QuasarServerActionCapabilityType:
		params := model.QuasarServerActionCapabilityParameters{
			Instance: model.QuasarServerActionCapabilityInstance(random.Choice(model.KnownQuasarServerActionInstances)),
		}
		c.SetParameters(params)

		if c.Retrievable() && random.FlipCoin() {
			c.SetState(generateCapabilityState(c))
		}

	case model.QuasarCapabilityType:
		params := model.QuasarCapabilityParameters{
			Instance: model.QuasarCapabilityInstance(random.Choice(model.KnownQuasarCapabilityInstances)),
		}
		c.SetParameters(params)

		if c.Retrievable() && random.FlipCoin() {
			c.SetState(generateCapabilityState(c))
		}

	case model.VideoStreamCapabilityType:
		params := model.VideoStreamCapabilityParameters{
			Protocols: []model.VideoStreamProtocol{model.HLSStreamingProtocol},
		}
		c.SetParameters(params)
		c.SetRetrievable(false)
	}

	return c
}

func generateCapabilityState(c model.ICapability) model.ICapabilityState {
	switch c.Type() {
	case model.OnOffCapabilityType:
		return model.OnOffCapabilityState{
			Instance: model.OnOnOffCapabilityInstance,
			Value:    random.FlipCoin(),
		}

	case model.ColorSettingCapabilityType:
		if params, ok := c.Parameters().(model.ColorSettingCapabilityParameters); ok {
			state := model.ColorSettingCapabilityState{}

			availableInstances := make([]string, 0, 2)
			if params.TemperatureK != nil {
				availableInstances = append(availableInstances, string(model.TemperatureKCapabilityInstance))
			}
			if params.ColorModel != nil {
				availableInstances = append(availableInstances, params.GetInstance())
			}

			state.Instance = model.ColorSettingCapabilityInstance(random.Choice(availableInstances))

			switch state.Instance {
			case model.TemperatureKCapabilityInstance:
				state.Value = model.TemperatureK(random.RandRange(int(params.TemperatureK.Min), int(params.TemperatureK.Max+1)))
			case model.HsvColorCapabilityInstance:
				state.Value = model.HSV{
					H: random.RandRange(0, 360),
					S: random.RandRange(0, 101),
					V: random.RandRange(0, 101),
				}
			case model.RgbColorCapabilityInstance:
				state.Value = model.RGB(random.RandRange(0, 16777216))
			}

			return state
		}

	case model.RangeCapabilityType:
		if params, ok := c.Parameters().(model.RangeCapabilityParameters); ok {
			state := model.RangeCapabilityState{
				Instance: params.Instance,
			}

			min := 0.0
			max := 100.0
			precision := 0.5
			if params.Range != nil {
				min = params.Range.Min
				max = params.Range.Max
				precision = params.Range.Precision
			}
			state.Value = min + precision*float64(random.RandRange(0, int((max-min)/precision)))

			return state
		}

	case model.ModeCapabilityType:
		if params, ok := c.Parameters().(model.ModeCapabilityParameters); ok {
			state := model.ModeCapabilityState{}
			state.Instance = params.Instance

			values := make([]string, len(params.Modes))
			for i, mode := range params.Modes {
				values[i] = string(mode.Value)
			}

			state.Value = model.ModeValue(random.Choice(values))
			return state
		}

	case model.ToggleCapabilityType:
		if params, ok := c.Parameters().(model.ToggleCapabilityParameters); ok {
			return model.ToggleCapabilityState{
				Instance: params.Instance,
				Value:    random.FlipCoin(),
			}
		}

	case model.CustomButtonCapabilityType:
		if params, ok := c.Parameters().(model.CustomButtonCapabilityParameters); ok {
			return model.CustomButtonCapabilityState{
				Instance: params.Instance,
				Value:    random.FlipCoin(),
			}
		}
	case model.QuasarServerActionCapabilityType:
		if params, ok := c.Parameters().(model.QuasarServerActionCapabilityParameters); ok {
			return model.QuasarServerActionCapabilityState{
				Instance: params.Instance,
				Value:    testing.RandCyrillicWithNumbersString(20),
			}
		}
	case model.QuasarCapabilityType:
		if params, ok := c.Parameters().(model.QuasarCapabilityParameters); ok {
			var value model.QuasarCapabilityValue
			switch params.Instance {
			case model.WeatherCapabilityInstance:
				value = model.WeatherQuasarCapabilityValue{}
			case model.VolumeCapabilityInstance:
				value = model.VolumeQuasarCapabilityValue{Value: random.RandRange(1, 10)}
			case model.MusicPlayCapabilityInstance:
				value = model.MusicPlayQuasarCapabilityValue{
					SearchText: testing.RandCyrillicWithNumbersString(20),
				}
			case model.NewsCapabilityInstance:
				value = model.NewsQuasarCapabilityValue{
					Topic: model.SpeakerNewsTopic(random.Choice(model.KnownSpeakerNewsTopics)),
				}
			case model.SoundPlayCapabilityInstance:
				value = model.SoundPlayQuasarCapabilityValue{Sound: "chainsaw-1"}
			case model.StopEverythingCapabilityInstance:
				value = model.StopEverythingQuasarCapabilityValue{}
			case model.TTSCapabilityInstance:
				value = model.TTSQuasarCapabilityValue{Text: testing.RandCyrillicWithNumbersString(20)}
			case model.AliceShowCapabilityInstance:
				value = model.AliceShowQuasarCapabilityValue{}
			}
			return model.QuasarCapabilityState{
				Instance: params.Instance,
				Value:    value,
			}
		}
	case model.VideoStreamCapabilityType:
		if params, ok := c.Parameters().(model.VideoStreamCapabilityParameters); ok {
			return model.VideoStreamCapabilityState{
				Instance: model.GetStreamCapabilityInstance,
				Value: model.VideoStreamCapabilityValue{
					Protocols: []model.VideoStreamProtocol{params.Protocols[random.FlipCube(len(params.Protocols))]},
				},
			}
		}
	}

	return nil
}

func generateProperties(n int) (properties model.Properties) {
	if n == 0 {
		return make(model.Properties, 0)
	}

	skipInstance := make(map[model.PropertyInstance]bool)
	for len(properties) < n && len(model.KnownPropertyInstances) > len(skipInstance) {
		unusedPropertyInstance := make([]string, 0, len(model.KnownPropertyInstances)-len(skipInstance))
		for _, pType := range model.KnownPropertyInstances {
			if !skipInstance[model.PropertyInstance(pType)] {
				unusedPropertyInstance = append(unusedPropertyInstance, pType)
			}
		}

		pInstance := model.PropertyInstance(random.Choice(unusedPropertyInstance))
		pType := model.PropertyType(random.Choice(pInstance.GetPropertyTypes()))
		p := generateProperty(pType, pInstance)

		skipInstance[pInstance] = true
		properties = append(properties, p)
	}

	return
}

func generateProperty(pType model.PropertyType, pInstance model.PropertyInstance) model.IProperty {
	p := model.MakePropertyByType(pType)
	p.SetRetrievable(random.FlipCoin())

	switch pType {
	case model.EventPropertyType:
		eventValues := model.EventPropertyInstanceToEventValues(pInstance)
		events := make([]model.Event, 0, len(eventValues))
		for _, e := range eventValues {
			events = append(events, model.Event{Value: model.EventValue(e)})
		}
		p.SetParameters(model.EventPropertyParameters{
			Instance: pInstance,
			Events:   events,
		})
		p.SetState(model.EventPropertyState{
			Instance: pInstance,
			Value:    model.EventValue(random.Choice(eventValues)),
		})
	case model.FloatPropertyType:
		p.SetParameters(model.FloatPropertyParameters{Instance: pInstance})
		p.SetState(model.FloatPropertyState{
			Instance: pInstance,
			Value:    float64(random.RandRange(0, 100)),
		})
	default:
		panic(fmt.Sprintf("unknown property type %q", pType))
	}
	return p
}

func GenerateDeviceInfo() (deviceInfo *model.DeviceInfo) {
	deviceInfo = &model.DeviceInfo{}
	if random.FlipCoin() {
		var Manufacturer string
		for strings.TrimSpace(Manufacturer) == "" {
			Manufacturer = testing.RandLatinString(random.RandRange(5, 100))
		}
		deviceInfo.Manufacturer = tools.AOS(Manufacturer)
	}
	if random.FlipCoin() {
		var SwVersion string
		for strings.TrimSpace(SwVersion) == "" {
			SwVersion = testing.RandAlphabetString(random.RandRange(5, 100), testing.VersionLetterBytes)
		}
		deviceInfo.SwVersion = tools.AOS(SwVersion)
	}
	if random.FlipCoin() {
		var HwVersion string
		for strings.TrimSpace(HwVersion) == "" {
			HwVersion = testing.RandAlphabetString(random.RandRange(5, 100), testing.VersionLetterBytes)
		}
		deviceInfo.HwVersion = tools.AOS(HwVersion)
	}
	if random.FlipCoin() {
		var Model string
		for strings.TrimSpace(Model) == "" {
			Model = testing.RandString(random.RandRange(5, 100))
		}
		deviceInfo.Model = tools.AOS(Model)
	}
	return
}
