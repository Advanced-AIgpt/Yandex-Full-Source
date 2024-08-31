package mobile

import (
	"fmt"
	"strings"

	"golang.org/x/exp/slices"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/inflector"
	"a.yandex-team.ru/alice/library/go/tools"
)

type SuggestionBlock struct {
	Type     SuggestionBlockType `json:"type"`
	Name     string              `json:"name"`
	Suggests []string            `json:"suggests"`
}

func (sb SuggestionBlock) CapitalizeSuggests() SuggestionBlock {
	capitalized := make([]string, 0, len(sb.Suggests))
	for _, suggest := range sb.Suggests {
		capitalized = append(capitalized, tools.Capitalize(suggest))
	}
	return SuggestionBlock{
		Type:     sb.Type,
		Name:     sb.Name,
		Suggests: capitalized,
	}
}

type SuggestionBlockType string

func (sbt SuggestionBlockType) Name() string {
	name := KnownSuggestionBlockTypeNames[sbt]
	if len(name) == 0 {
		return "Голосовые команды"
	}
	return name
}

var (
	// top commands
	BasicSuggestionBlockType SuggestionBlockType = "basic"

	// capabilities
	ToggleSuggestionBlockType       SuggestionBlockType = "toggles"
	ColorSettingSuggestionBlockType SuggestionBlockType = "color_setting"
	ModesSuggestionBlockType        SuggestionBlockType = "modes"
	CustomButtonSuggestionBlockType SuggestionBlockType = "custom_button"

	// ranges
	BrightnessSuggestionsBlockType  SuggestionBlockType = "brightness"
	TemperatureSuggestionsBlockType SuggestionBlockType = "temperature"
	VolumeSuggestionsBlockType      SuggestionBlockType = "volume"
	ChannelSuggestionsBlockType     SuggestionBlockType = "channel"
	HumiditySuggestionsBlockType    SuggestionBlockType = "humidity"
	OpenSuggestionsBlockType        SuggestionBlockType = "open"

	// one special mode - fan_speed
	FanSpeedSuggestionsBlockType SuggestionBlockType = "fan_speed"

	// other use cases
	QuerySuggestionBlockType SuggestionBlockType = "queries"
	TimerSuggestionBlockType SuggestionBlockType = "timers"

	// video
	VideoSuggestionsBlockType SuggestionBlockType = "video"
)

// https://st.yandex-team.ru/EDIT-36672
var KnownSuggestionBlockTypeNames = map[SuggestionBlockType]string{
	BasicSuggestionBlockType:        "Основные",
	ToggleSuggestionBlockType:       "Включение и выключение",
	ColorSettingSuggestionBlockType: "Цвет",
	ModesSuggestionBlockType:        "Режимы работы",
	CustomButtonSuggestionBlockType: "Настроенные команды",
	QuerySuggestionBlockType:        "Проверка статуса",
	TimerSuggestionBlockType:        "Отложенные команды",
	BrightnessSuggestionsBlockType:  "Яркость",
	TemperatureSuggestionsBlockType: "Температура",
	VolumeSuggestionsBlockType:      "Громкость",
	ChannelSuggestionsBlockType:     "Каналы",
	HumiditySuggestionsBlockType:    "Влажность",
	OpenSuggestionsBlockType:        "Степень открытия",
	FanSpeedSuggestionsBlockType:    "Скорость вентиляции",
	VideoSuggestionsBlockType:       "Видео",
}

func BuildSuggestionBlocks(deviceInflection inflector.Inflection, deviceType model.DeviceType, capabilities model.Capabilities, properties model.Properties, options model.SuggestionsOptions) []SuggestionBlock {
	blocks := make([]SuggestionBlock, 0)

	// basic suggestions are top commands for some device
	basicSuggestions, ok := BuildBasicSuggestions(deviceType, deviceInflection, capabilities, options)
	if ok {
		blocks = append(blocks, basicSuggestions)
	}

	// toggle and onOff suggestions
	toggleSuggestions, ok := BuildToggleActionSuggestions(deviceType, deviceInflection, capabilities, options)
	if ok {
		blocks = append(blocks, toggleSuggestions)
	}

	// color suggestions
	colorSuggestions, ok := BuildColorSettingActionSuggestions(deviceType, deviceInflection, capabilities, options)
	if ok {
		blocks = append(blocks, colorSuggestions)
	}

	// range suggestions have blocks for each instance
	rangeSuggestions, ok := BuildRangeActionSuggestions(deviceType, deviceInflection, capabilities, options)
	if ok {
		blocks = append(blocks, rangeSuggestions...)
	}

	// video suggestions
	videoSuggestions, ok := BuildVideoActionSuggestions(deviceType, deviceInflection, capabilities, options)
	if ok {
		blocks = append(blocks, videoSuggestions)
	}

	fanSpeedSuggestions, ok := BuildFanSpeedActionSuggestions(deviceType, deviceInflection, capabilities, options)
	if ok {
		blocks = append(blocks, fanSpeedSuggestions)
	}

	modeSuggestions, ok := BuildModeActionSuggestions(deviceType, deviceInflection, capabilities, options)
	if ok {
		blocks = append(blocks, modeSuggestions)
	}

	customButtonSuggestions, ok := BuildCustomButtonActionSuggestions(deviceType, deviceInflection, capabilities, options)
	if ok {
		blocks = append(blocks, customButtonSuggestions)
	}

	querySuggestions, ok := BuildQuerySuggestions(deviceInflection, capabilities, properties, options)
	if ok {
		blocks = append(blocks, querySuggestions)
	}
	timerActionSuggestions, ok := BuildTimerActionSuggestions(deviceType, deviceInflection, capabilities, options)
	if ok {
		blocks = append(blocks, timerActionSuggestions)
	}

	return blocks
}

func BuildBasicSuggestions(deviceType model.DeviceType, inflection inflector.Inflection, capabilities model.Capabilities, options model.SuggestionsOptions) (SuggestionBlock, bool) {
	var block SuggestionBlock
	suggestions := deviceType.GenerateBasicSuggestions(inflection, capabilities, options)
	hasSuggestions := len(suggestions) > 0
	if hasSuggestions {
		blockType := BasicSuggestionBlockType
		block = SuggestionBlock{
			Type:     blockType,
			Name:     blockType.Name(),
			Suggests: suggestions,
		}
	}
	return block.CapitalizeSuggests(), hasSuggestions
}

func BuildToggleActionSuggestions(deviceType model.DeviceType, inflection inflector.Inflection, capabilities model.Capabilities, options model.SuggestionsOptions) (SuggestionBlock, bool) {
	suggestions := make([]string, 0)
	for _, capability := range capabilities {
		switch capability.Type() {
		case model.OnOffCapabilityType:
			suggestions = append(suggestions, deviceType.GenerateOnOffSuggestions(inflection, options)...)
		case model.ToggleCapabilityType:
			instance := capability.Parameters().(model.ToggleCapabilityParameters).Instance
			suggestions = append(suggestions, getBasicToggleSuggests(instance, inflection, options)...)
			suggestions = append(suggestions, getExtraToggleSuggests(instance, inflection, options)...)
		}
	}

	var block SuggestionBlock
	hasSuggestions := len(suggestions) > 0
	if hasSuggestions {
		blockType := ToggleSuggestionBlockType
		block = SuggestionBlock{
			Type:     blockType,
			Name:     blockType.Name(),
			Suggests: suggestions,
		}
	}
	return block.CapitalizeSuggests(), hasSuggestions
}

func BuildColorSettingActionSuggestions(deviceType model.DeviceType, inflection inflector.Inflection, capabilities model.Capabilities, options model.SuggestionsOptions) (SuggestionBlock, bool) {
	suggestions := make([]string, 0)
	for _, capability := range capabilities {
		switch capability.Type() {
		case model.ColorSettingCapabilityType:
			if capability.Parameters().(model.ColorSettingCapabilityParameters).TemperatureK != nil {
				// TODO(aaulayev): add compatibility for new light device types here and below (IOT-879)
				// It's required by Begemot that this kind of suggestions must work for LightDeviceType only
				if deviceType == model.LightDeviceType {
					suggestions = append(suggestions,
						"сделай свет потеплее", "сделай свет похолоднее",
					)
				}
				suggestions = append(suggestions,
					"сделай свет "+inflection.Rod+" потеплее",
					"включи свет потеплее для "+inflection.Rod,
					"сделай свет "+inflection.Rod+" похолоднее",
					"включи свет похолоднее для "+inflection.Rod)
			}
			if parameters := capability.Parameters().(model.ColorSettingCapabilityParameters); parameters.ColorModel != nil {
				if defaultColor, ok := parameters.GetDefaultColor(); ok {
					colorName := strings.ToLower(defaultColor.Name)
					if deviceType == model.LightDeviceType {
						suggestions = append(suggestions,
							"включи "+colorName+" свет")
					}
					suggestions = append(suggestions,
						"измени цвет "+inflection.Rod+" на "+colorName,
						"включи "+colorName+" цвет для "+inflection.Rod,
						"измени цвет "+inflection.Rod+" на "+colorName)
				}
			}
			if parameters := capability.Parameters().(model.ColorSettingCapabilityParameters); parameters.ColorSceneParameters != nil {
				if scenes := parameters.GetAvailableScenes(); len(scenes) > 0 {
					// due to discovery validation it always safe to call
					// we need to get suggestions without color scene with name Alice because suggestions would look ugly otherwise
					defaultSceneID := scenes[0].ID
					if defaultSceneID == model.ColorSceneIDAlice && len(scenes) > 1 {
						defaultSceneID = scenes[1].ID
					}
					defaultSceneName := model.KnownColorScenes[defaultSceneID].Name
					if deviceType == model.LightDeviceType {
						suggestions = append(suggestions,
							"включи режим "+defaultSceneName)
					}
					suggestions = append(suggestions,
						"поставь режим "+defaultSceneName+" на "+inflection.Pr,
						"включи режим освещения "+defaultSceneName+" на "+inflection.Pr,
					)
				}
			}
		}
	}
	for i := range suggestions {
		suggestions[i] = options.AddHouseholdToSuggest(suggestions[i])
	}

	var block SuggestionBlock
	hasSuggestions := len(suggestions) > 0
	if hasSuggestions {
		blockType := ColorSettingSuggestionBlockType
		block = SuggestionBlock{
			Type:     blockType,
			Name:     blockType.Name(),
			Suggests: suggestions,
		}
	}
	return block.CapitalizeSuggests(), hasSuggestions
}

func BuildModeActionSuggestions(deviceType model.DeviceType, inflection inflector.Inflection, capabilities model.Capabilities, options model.SuggestionsOptions) (SuggestionBlock, bool) {
	suggestions := make([]string, 0)
	for _, capability := range capabilities {
		switch capability.Type() {
		case model.ModeCapabilityType:
			parameters := capability.Parameters().(model.ModeCapabilityParameters)
			if parameters.Instance == model.FanSpeedModeInstance {
				continue // fan speed is special and has its own block
			}
			suggestions = append(suggestions, getBasicModeSuggests(parameters.Instance, parameters.Modes, inflection, options)...)
			suggestions = append(suggestions, getSuggestsForOldModes(deviceType, parameters.Instance, parameters.Modes, inflection, options)...)
		}
	}

	var block SuggestionBlock
	hasSuggestions := len(suggestions) > 0
	if hasSuggestions {
		blockType := ModesSuggestionBlockType
		block = SuggestionBlock{
			Type:     blockType,
			Name:     blockType.Name(),
			Suggests: suggestions,
		}
	}
	return block.CapitalizeSuggests(), hasSuggestions
}

func BuildFanSpeedActionSuggestions(deviceType model.DeviceType, inflection inflector.Inflection, capabilities model.Capabilities, options model.SuggestionsOptions) (SuggestionBlock, bool) {
	suggestions := make([]string, 0)
	for _, capability := range capabilities {
		switch capability.Type() {
		case model.ModeCapabilityType:
			parameters := capability.Parameters().(model.ModeCapabilityParameters)
			if parameters.Instance != model.FanSpeedModeInstance {
				continue
			}
			suggestions = append(suggestions, getSuggestsForFanSpeedMode(deviceType, parameters.Instance, parameters.Modes, inflection, options)...)
			suggestions = append(suggestions, getRelativeModeSuggests(parameters.Instance, inflection, options)...)
		}
	}

	var block SuggestionBlock
	hasSuggestions := len(suggestions) > 0
	if hasSuggestions {
		blockType := FanSpeedSuggestionsBlockType
		block = SuggestionBlock{
			Type:     blockType,
			Name:     blockType.Name(),
			Suggests: suggestions,
		}
	}
	return block.CapitalizeSuggests(), hasSuggestions
}

func BuildRangeActionSuggestions(deviceType model.DeviceType, inflection inflector.Inflection, capabilities model.Capabilities, options model.SuggestionsOptions) ([]SuggestionBlock, bool) {
	blocks := make([]SuggestionBlock, 0, len(capabilities))
	for _, capability := range capabilities {
		block, hasBlock := BuildRangeActionSuggestion(deviceType, inflection, capability, options)
		if hasBlock {
			blocks = append(blocks, block)
		}
	}

	hasBlocks := len(blocks) > 0
	return blocks, hasBlocks
}

func BuildRangeActionSuggestion(deviceType model.DeviceType, inflection inflector.Inflection, capability model.ICapability, options model.SuggestionsOptions) (SuggestionBlock, bool) {
	suggestions := make([]string, 0)
	switch capability.Type() {
	case model.RangeCapabilityType:
		switch capability.Parameters().(model.RangeCapabilityParameters).Instance {
		case model.BrightnessRangeInstance:
			if deviceType == model.LightDeviceType {
				suggestions = append(suggestions,
					"прибавь яркость света")
			}
			suggestions = append(suggestions,
				"прибавь яркость для "+inflection.Rod,
				"увеличь яркость на "+inflection.Pr)
			if deviceType == model.LightDeviceType {
				suggestions = append(suggestions,
					"убавь яркость света")
			}
			suggestions = append(suggestions,
				"уменьши яркость для "+inflection.Rod,
				"убавь яркость на "+inflection.Pr)

			if params := capability.Parameters().(model.RangeCapabilityParameters); params.Unit == model.UnitPercent {
				if params.Range != nil && params.RandomAccess {
					suggestions = append(suggestions,
						"включи яркость "+inflection.Rod+" 20 процентов",
						"установи яркость "+inflection.Rod+" в 40 процентов",
					)
					if deviceType == model.LightDeviceType && inflection.Rod != "света" {
						suggestions = append(suggestions,
							"включи яркость света 20 процентов",
							"установи яркость света в 40 процентов",
						)
					}
				}
			}
			if deviceType == model.LightDeviceType && inflection.Rod != "света" {
				suggestions = append(suggestions,
					"включи яркость света на максимум",
					"включи максимальную яркость света")
			}
			suggestions = append(suggestions,
				"включи яркость "+inflection.Rod+" на максимум",
				"включи максимальную яркость "+inflection.Rod)

			if deviceType == model.LightDeviceType && inflection.Rod != "света" {
				suggestions = append(suggestions,
					"включи яркость света на минимум",
					"включи минимальную яркость света")
			}
			suggestions = append(suggestions,
				"включи яркость "+inflection.Rod+" на минимум",
				"включи минимальную яркость для "+inflection.Rod)
		case model.VolumeRangeInstance:
			suggestions = append(suggestions,
				"сделай "+inflection.Vin+" погромче",
				"прибавь звук на "+inflection.Pr,
				"сделай "+inflection.Vin+" потише",
				"убавь звук на "+inflection.Pr)
		case model.ChannelRangeInstance:
			suggestions = append(suggestions,
				"включи следующий канал на "+inflection.Pr,
				"переключи на следующий канал "+inflection.Rod,
				"включи предыдущий канал на "+inflection.Pr,
				"переключи на предыдущий канал "+inflection.Rod)
			if capability.Parameters().(model.RangeCapabilityParameters).RandomAccess {
				suggestions = append(suggestions,
					"включи 5 канал на "+inflection.Pr,
					"переключи на 2 канал на "+inflection.Pr,
					"включи на "+inflection.Pr+" 17 канал")
			}
		case model.TemperatureRangeInstance:
			suggestions = append(suggestions,
				"прибавь температуру на "+inflection.Pr,
				"увеличь температуру для "+inflection.Rod,
				"убавь температуру для "+inflection.Rod,
				"уменьши температуру "+inflection.Rod,
				"включи максимальную температуру на "+inflection.Pr,
				"включи температуру "+inflection.Rod+" на максимум",
				"включи минимальную температуру на "+inflection.Pr,
				"включи температуру "+inflection.Rod+" на минимум",
			)

			params := capability.Parameters().(model.RangeCapabilityParameters)
			if params.Range != nil && params.RandomAccess {
				// nearest int divisible on 10 to max and to min
				adjustedMx := int(params.Range.Max) - (int(params.Range.Max) % 10)
				adjustedMn := int(params.Range.Min) + (10 - int(params.Range.Min)%10)
				switch params.Unit {
				case model.UnitTemperatureCelsius:
					if params.Range.Contains(float64(adjustedMx)) {
						suggestions = append(suggestions,
							fmt.Sprintf("включи температуру %s %d градусов", inflection.Rod, adjustedMx),
						)
					}
					if params.Range.Contains(float64(adjustedMn)) {
						suggestions = append(suggestions,
							fmt.Sprintf("установи температуру %s на %d градусов", inflection.Rod, adjustedMn),
						)
					}
				default:
					if params.Range.Contains(float64(adjustedMx)) {
						suggestions = append(suggestions,
							fmt.Sprintf("включи температуру %s %d", inflection.Rod, adjustedMx),
						)
					}
					if params.Range.Contains(float64(adjustedMn)) {
						suggestions = append(suggestions,
							fmt.Sprintf("установи температуру %s на %d", inflection.Rod, adjustedMn),
						)
					}
				}
			}
			if deviceType == model.ThermostatDeviceType {
				suggestions = append(suggestions,
					"прибавь температуру",
					"увеличь температуру",
					"убавь температуру",
					"уменьши температуру",
					"включи максимальную температуру",
					"включи температуру на максимум",
					"включи минимальную температуру",
					"включи температуру на минимум",
					"сделай теплее",
					"сделай холоднее",
					"поставь температуру потеплее",
					"поставь температуру похолоднее",
					"сделай температуру похолоднее",
					"сделай температуру прохладнее")
				params := capability.Parameters().(model.RangeCapabilityParameters)
				if params.Range != nil && params.RandomAccess {
					// nearest int divisible on 10 to max and to min
					adjustedMx := int(params.Range.Max) - (int(params.Range.Max) % 10)
					adjustedMn := int(params.Range.Min) + (10 - int(params.Range.Min)%10)
					switch params.Unit {
					case model.UnitTemperatureCelsius:
						if params.Range.Contains(float64(adjustedMx)) {
							suggestions = append(suggestions,
								fmt.Sprintf("включи температуру %d градусов", adjustedMx),
							)
						}
						if params.Range.Contains(float64(adjustedMn)) {
							suggestions = append(suggestions,
								fmt.Sprintf("установи температуру на %d градусов", adjustedMn),
							)
						}
					default:
						if params.Range.Contains(float64(adjustedMx)) {
							suggestions = append(suggestions,
								fmt.Sprintf("включи температуру %d", adjustedMx),
							)
						}
						if params.Range.Contains(float64(adjustedMn)) {
							suggestions = append(suggestions,
								fmt.Sprintf("установи температуру на %d", adjustedMn),
							)
						}
					}
				}
			}
		case model.HumidityRangeInstance:
			suggestions = append(suggestions,
				"прибавь влажность на "+inflection.Pr,
				"увеличь влажность на "+inflection.Pr,
				"убавь влажность на "+inflection.Pr,
				"уменьши влажность "+inflection.Rod,
				"поставь максимальную влажность на "+inflection.Pr,
				"установи влажность "+inflection.Rod+" на минимум",
			)
			params := capability.Parameters().(model.RangeCapabilityParameters)
			if params.Unit == model.UnitPercent && params.Range != nil && params.RandomAccess {
				suggestions = append(suggestions,
					fmt.Sprintf("установи влажность %s на %.0f процентов", inflection.Rod, params.Range.Max))
			}
		case model.OpenRangeInstance:
			suggestions = append(suggestions,
				"открой "+inflection.Vin+" на треть",
				"приоткрой "+inflection.Vin,
				"прикрой "+inflection.Vin,
				"приоткрой "+inflection.Vin+" на половину")
			params := capability.Parameters().(model.RangeCapabilityParameters)
			if params.Unit == model.UnitPercent {
				suggestions = append(suggestions, "приоткрой "+inflection.Vin+" на 10 процентов")
			}
			if params.Range != nil && params.RandomAccess {
				suggestions = append(suggestions,
					"открой "+inflection.Vin+" на 50",
					"установи открытие "+inflection.Rod+" на 10",
				)
				if params.Unit == model.UnitPercent {
					suggestions = append(suggestions,
						"открой "+inflection.Vin+" на 50 процентов",
						"поставь открытие "+inflection.Rod+" на 10 процентов",
					)
				}
			}
		}
	}
	for i := range suggestions {
		suggestions[i] = options.AddHouseholdToSuggest(suggestions[i])
	}

	var block SuggestionBlock
	hasSuggestions := len(suggestions) > 0
	if hasSuggestions {
		blockType := SuggestionBlockType(capability.Instance())
		block = SuggestionBlock{
			Type:     blockType,
			Name:     blockType.Name(),
			Suggests: suggestions,
		}
	}
	return block.CapitalizeSuggests(), hasSuggestions
}

func BuildCustomButtonActionSuggestions(deviceType model.DeviceType, inflection inflector.Inflection, capabilities model.Capabilities, options model.SuggestionsOptions) (SuggestionBlock, bool) {
	suggestions := make([]string, 0)
	for _, capability := range capabilities {
		switch capability.Type() {
		case model.CustomButtonCapabilityType:
			suggestions = append(suggestions, capability.BasicSuggestions(deviceType, inflection, options)...)
		}
	}

	var block SuggestionBlock
	hasSuggestions := len(suggestions) > 0
	if hasSuggestions {
		blockType := CustomButtonSuggestionBlockType
		block = SuggestionBlock{
			Type:     blockType,
			Name:     blockType.Name(),
			Suggests: suggestions,
		}
	}
	return block.CapitalizeSuggests(), hasSuggestions
}

func BuildVideoActionSuggestions(deviceType model.DeviceType, inflection inflector.Inflection, capabilities model.Capabilities, options model.SuggestionsOptions) (SuggestionBlock, bool) {
	suggestions := make([]string, 0)
	for _, capability := range capabilities {
		switch capability.Type() {
		case model.VideoStreamCapabilityType:
			suggestions = append(suggestions, capability.BasicSuggestions(deviceType, inflection, options)...)
		}
	}

	var block SuggestionBlock
	hasSuggestions := len(suggestions) > 0
	if hasSuggestions {
		blockType := VideoSuggestionsBlockType
		block = SuggestionBlock{
			Type:     blockType,
			Name:     blockType.Name(),
			Suggests: suggestions,
		}
	}
	return block.CapitalizeSuggests(), hasSuggestions
}

func BuildQuerySuggestions(inflection inflector.Inflection, capabilities model.Capabilities, properties model.Properties, options model.SuggestionsOptions) (SuggestionBlock, bool) {
	suggestions := make([]string, 0)
	if capabilities.HaveRetrievableState() || properties.HaveRetrievableState() || properties.HaveReportableState() {
		suggestions = append(suggestions, options.AddHouseholdToSuggest(fmt.Sprintf("Что с %s?", inflection.Tvor)))
	}
	generalModeSuggestAdded := false
	for _, capability := range capabilities {
		if !capability.Retrievable() {
			continue
		}
		if capability.Type() == model.ModeCapabilityType && !generalModeSuggestAdded {
			suggestions = append(suggestions, options.AddHouseholdToSuggest(fmt.Sprintf("Какой режим стоит на %s?", inflection.Pr)))
			generalModeSuggestAdded = true
		}
		suggestions = append(suggestions, capability.QuerySuggestions(inflection, options)...)
	}

	for _, property := range properties {
		if property.Retrievable() || property.Reportable() {
			suggestions = append(suggestions, property.QuerySuggestions(inflection, options)...)
		}
	}

	var block SuggestionBlock
	hasSuggestions := len(suggestions) > 0
	if hasSuggestions {
		blockType := QuerySuggestionBlockType
		block = SuggestionBlock{
			Type:     blockType,
			Name:     blockType.Name(),
			Suggests: suggestions,
		}
	}
	return block.CapitalizeSuggests(), hasSuggestions
}

func BuildTimerActionSuggestions(deviceType model.DeviceType, inflection inflector.Inflection, capabilities model.Capabilities, options model.SuggestionsOptions) (SuggestionBlock, bool) {
	suggestions := make([]string, 0)

	// onOffs have a special command - switch state for x minutes
	onOffTimerCommands := []string{"на 15 минут", "через 2 часа", "завтра в час дня"}
	otherTimerCommands := []string{"через 20 минут", "через 4 часа", "завтра в 11 утра"}

	otherSuggestions := make([]string, 0)
	for _, capability := range capabilities {
		switch {
		case slices.Contains(model.NonSupportingIntervalActionsCapabilities, string(capability.Type())):
			continue
		case capability.Type() == model.OnOffCapabilityType && !slices.Contains(model.NonSupportingIntervalActionsDeviceTypes, string(deviceType)):
			onOffSuggestions := capability.BasicSuggestions(deviceType, inflection, options)
			for i, suggest := range onOffSuggestions {
				command := onOffTimerCommands[i%len(onOffTimerCommands)]
				suggestions = append(suggestions, fmt.Sprintf("%s %s", suggest, command))
			}
		default:
			otherSuggestions = append(otherSuggestions, capability.BasicSuggestions(deviceType, inflection, options)...)
		}
	}
	// we want to showcase cancelScenario and cancelAllScenarios right after onOffs
	if len(suggestions) > 0 {
		suggestions = append(suggestions, "Отмени все команды")
	}
	for i, suggest := range otherSuggestions {
		command := otherTimerCommands[i%len(otherTimerCommands)]
		suggestions = append(suggestions, fmt.Sprintf("%s %s", suggest, command))
	}
	var block SuggestionBlock
	hasSuggestions := len(suggestions) > 0
	if hasSuggestions {
		blockType := TimerSuggestionBlockType
		block = SuggestionBlock{
			Type:     blockType,
			Name:     blockType.Name(),
			Suggests: suggestions,
		}
	}
	return block.CapitalizeSuggests(), hasSuggestions
}

func getBasicToggleSuggests(instance model.ToggleCapabilityInstance, inflection inflector.Inflection, options model.SuggestionsOptions) []string {
	var toggleNames []string
	switch instance {
	case model.MuteToggleCapabilityInstance:
		toggleNames = []string{"звук"}
	case model.BacklightToggleCapabilityInstance:
		toggleNames = []string{"подсветку"}
	case model.ControlsLockedToggleCapabilityInstance:
		toggleNames = []string{"детский режим"}
	case model.IonizationToggleCapabilityInstance:
		toggleNames = []string{"ионизатор", "ионизацию", "ионизирование воздуха"}
	case model.OscillationToggleCapabilityInstance:
		toggleNames = []string{"вращение"}
	case model.KeepWarmToggleCapabilityInstance:
		toggleNames = []string{"поддержку температуры", "поддержание тепла"}
	case model.PauseToggleCapabilityInstance:
		toggleNames = []string{"паузу"}
	default:
		return nil
	}

	var suggests []string
	for _, toggleName := range toggleNames {
		if instance != model.PauseToggleCapabilityInstance {
			suggests = append(suggests,
				fmt.Sprintf("выключи %s %s", toggleName, inflection.Rod),
				fmt.Sprintf("включи %s %s", toggleName, inflection.Rod),
			)
		}
		suggests = append(suggests,
			fmt.Sprintf("выключи %s на %s", toggleName, inflection.Pr),
			fmt.Sprintf("включи %s на %s", toggleName, inflection.Pr),
		)
	}
	for i := range suggests {
		suggests[i] = options.AddHouseholdToSuggest(suggests[i])
	}
	return suggests
}

func getExtraToggleSuggests(instance model.ToggleCapabilityInstance, inflection inflector.Inflection, options model.SuggestionsOptions) []string {
	suggestions := make([]string, 0)
	switch instance {
	case model.ControlsLockedToggleCapabilityInstance:
		suggestions = append(suggestions,
			fmt.Sprintf("заблокируй управление %s", inflection.Rod),
			fmt.Sprintf("разблокируй управление %s", inflection.Rod),
			fmt.Sprintf("разблокируй управление на %s", inflection.Pr),
			fmt.Sprintf("заблокируй управление на %s", inflection.Pr),
		)
	case model.KeepWarmToggleCapabilityInstance:
		suggestions = append(suggestions,
			fmt.Sprintf("поддерживай тепло на %s", inflection.Pr),
			fmt.Sprintf("не поддерживай тепло на %s", inflection.Pr),
			fmt.Sprintf("поддерживай температуру %s", inflection.Rod),
			fmt.Sprintf("не поддерживай температуру %s", inflection.Rod),
		)
	case model.PauseToggleCapabilityInstance:
		suggestions = append(suggestions,
			fmt.Sprintf("поставь паузу на %s", inflection.Pr),
			fmt.Sprintf("поставь %s на паузу", inflection.Vin),
			fmt.Sprintf("приостанови %s", inflection.Vin),
			fmt.Sprintf("сними %s с паузы", inflection.Vin),
			fmt.Sprintf("поставь паузу на %s", inflection.Pr),
		)
	case model.TrunkToggleCapabilityInstance:
		suggestions = append(suggestions,
			fmt.Sprintf("отопри багажник %s", inflection.Rod),
			fmt.Sprintf("открой багажник %s", inflection.Rod),
		)
	case model.CentralLockCapabilityInstance:
		suggestions = append(suggestions,
			fmt.Sprintf("открой %s", inflection.Vin),
			fmt.Sprintf("открой замок %s", inflection.Rod),
			fmt.Sprintf("разблокируй %s", inflection.Vin),
			fmt.Sprintf("закрой двери в %s", inflection.Pr),
			fmt.Sprintf("запри %s", inflection.Vin),
			fmt.Sprintf("заблокируй %s", inflection.Vin))
	}
	for i := range suggestions {
		suggestions[i] = options.AddHouseholdToSuggest(suggestions[i])
	}
	return suggestions
}

var (
	modeAdjectivesMaleVin = map[model.ModeValue]string{
		model.OneMode:   "первый",
		model.TwoMode:   "второй",
		model.ThreeMode: "третий",
		model.FourMode:  "четвертый",
		model.FiveMode:  "пятый",
		model.SixMode:   "шестой",
		model.SevenMode: "седьмой",
		model.EightMode: "восьмой",
		model.NineMode:  "девятый",
		model.TenMode:   "десятый",
	}
	modeAdjectivesFemaleVin = map[model.ModeValue]string{
		model.QuietMode:      "тихую",
		model.NormalMode:     "нормальную",
		model.MaxMode:        "максимальную",
		model.MinMode:        "минимальную",
		model.StationaryMode: "статичную",
		model.FastMode:       "быструю",
		model.SlowMode:       "медленную",
		model.OneMode:        "первую",
		model.TwoMode:        "вторую",
		model.ThreeMode:      "третью",
		model.FourMode:       "четвертую",
		model.FiveMode:       "пятую",
		model.SixMode:        "шестую",
		model.SevenMode:      "седьмую",
		model.EightMode:      "восьмую",
		model.NineMode:       "девятую",
		model.TenMode:        "десятую",
	}
	modeAdjectivesNeutralVin = map[model.ModeValue]string{
		model.QuietMode:      "тихое",
		model.NormalMode:     "нормальное",
		model.MaxMode:        "максимальное",
		model.MinMode:        "минимальное",
		model.StationaryMode: "статичное",
		model.FastMode:       "быстрое",
		model.SlowMode:       "медленное",
		model.OneMode:        "первое",
		model.TwoMode:        "второе",
		model.ThreeMode:      "третье",
		model.FourMode:       "четвертое",
		model.FiveMode:       "пятое",
		model.SixMode:        "шестое",
		model.SevenMode:      "седьмое",
		model.EightMode:      "восьмое",
		model.NineMode:       "девятое",
		model.TenMode:        "десятое",
	}
	modeValueNounInflections = map[model.ModeValue]inflector.Inflection{
		model.FanOnlyMode: {
			Im:   "вентиляция",
			Rod:  "вентиляции",
			Dat:  "вентиляции",
			Vin:  "вентиляцию",
			Tvor: "вентиляцией",
			Pr:   "вентиляции",
		},
		model.CoolMode: {
			Im:   "охлаждение",
			Rod:  "охлаждения",
			Dat:  "охлаждению",
			Vin:  "охлаждение",
			Tvor: "охлаждением",
			Pr:   "охлаждении",
		},
		model.HeatMode: {
			Im:   "обогрев",
			Rod:  "обогрева",
			Dat:  "обогреву",
			Vin:  "обогрев",
			Tvor: "обогревом",
			Pr:   "обогреве",
		},
		model.DryMode: {
			Im:   "осушение",
			Rod:  "осушения",
			Dat:  "осушению",
			Vin:  "осушение",
			Tvor: "осушением",
			Pr:   "осушении",
		},
	}
	modeInstanceFemaleGender = []string{
		string(model.FanSpeedModeInstance),
		string(model.WorkSpeedModeInstance),
		string(model.CleanUpModeInstance),
		string(model.ProgramModeInstance),
		string(model.DishwashingModeInstance),
	}
	modeInstanceNeutralGender = []string{
		string(model.SwingModeInstance),
	}
)

func getBasicModeSuggests(instance model.ModeCapabilityInstance, modes []model.Mode, inflection inflector.Inflection, options model.SuggestionsOptions) []string {
	suggestFormat := make([]string, 0)
	for _, mode := range modes {
		switch mode.Value {
		case model.TurboMode, model.ExpressMode:
			if instance != model.FanSpeedModeInstance && instance != model.WorkSpeedModeInstance && instance != model.ProgramModeInstance && instance != model.CoffeeModeInstance {
				suggestFormat = append(suggestFormat, fmt.Sprintf("%s режим %s", *model.KnownModes[mode.Value].Name, modeInstanceInflections[instance].Rod))
			}
			suggestFormat = append(suggestFormat, fmt.Sprintf("%s %s", *model.KnownModes[mode.Value].Name, modeInstanceInflections[instance].Vin))
		case model.AutoMode, model.EcoMode:
			switch instance {
			case model.ThermostatModeInstance:
				suggestFormat = append(suggestFormat, fmt.Sprintf("%s режим", *model.KnownModes[mode.Value].Name))
			case model.ProgramModeInstance:
				suggestFormat = append(suggestFormat, fmt.Sprintf("%s %s", *model.KnownModes[mode.Value].Name, modeInstanceInflections[instance].Vin))
			default:
				suggestFormat = append(suggestFormat, fmt.Sprintf("%s режим %s", *model.KnownModes[mode.Value].Name, modeInstanceInflections[instance].Rod))
			}
		case model.FanOnlyMode, model.CoolMode, model.HeatMode, model.DryMode:
			if instance == model.ThermostatModeInstance {
				suggestFormat = append(suggestFormat, fmt.Sprintf("режим %s", modeValueNounInflections[mode.Value].Rod))
			} else {
				suggestFormat = append(suggestFormat, fmt.Sprintf("%s в режим %s", modeInstanceInflections[instance].Vin, modeValueNounInflections[mode.Value].Rod))
			}
		case model.HorizontalMode:
			// sounds really bad with other instances so no suggests for them
			if instance == model.SwingModeInstance {
				suggestFormat = append(suggestFormat, "горизонтальное направление воздуха")
			}
		case model.VerticalMode:
			// same here
			if instance == model.SwingModeInstance {
				suggestFormat = append(suggestFormat, "вертикальное направление воздуха")
			}
		case model.QuietMode, model.NormalMode, model.MaxMode, model.MinMode, model.StationaryMode, model.FastMode, model.SlowMode:
			// adjectives
			if tools.Contains(string(instance), modeInstanceFemaleGender) {
				suggestFormat = append(suggestFormat, fmt.Sprintf("%s %s", modeAdjectivesFemaleVin[mode.Value], modeInstanceInflections[instance].Vin))
			} else if tools.Contains(string(instance), modeInstanceNeutralGender) {
				suggestFormat = append(suggestFormat, fmt.Sprintf("%s %s", modeAdjectivesNeutralVin[mode.Value], modeInstanceInflections[instance].Vin))
			} else {
				suggestFormat = append(suggestFormat, fmt.Sprintf("%s %s", *model.KnownModes[mode.Value].Name, modeInstanceInflections[instance].Vin))
			}
		case model.OneMode, model.TwoMode, model.ThreeMode, model.FourMode, model.FiveMode,
			model.SixMode, model.SevenMode, model.EightMode, model.NineMode, model.TenMode:
			if instance == model.InputSourceModeInstance {
				suggestFormat = append(suggestFormat, fmt.Sprintf("%s %s", modeAdjectivesMaleVin[mode.Value], modeInstanceInflections[instance].Vin))
			}
		case model.WoolMode, model.PreHeatMode,
			model.LatteMode, model.CappuccinoMode, model.EspressoMode, model.DoubleEspressoMode, model.AmericanoMode,
			model.VacuumMode, model.BoilingMode, model.BakingMode, model.DessertMode, model.BabyFoodMode,
			model.FowlMode, model.FryingMode, model.YogurtMode, model.CerealsMode, model.MacaroniMode,
			model.MilkPorridgeMode, model.MulticookerMode, model.SteamMode, model.PastaMode, model.PizzaMode,
			model.PilafMode, model.SauceMode, model.SoupMode, model.StewingMode, model.SlowCookMode,
			model.DeepFryerMode, model.BreadMode, model.AspicMode, model.CheesecakeMode, model.WindFreeMode:
			suggestFormat = append(suggestFormat, fmt.Sprintf("режим %s", *model.KnownModes[mode.Value].Name))
		}
	}

	setVerbs := []string{
		"поставь",
		"включи",
	}
	targetEndings := []string{
		"на " + inflection.Pr,
	}

	suggests := make([]string, 0, len(setVerbs)*len(targetEndings)*len(suggestFormat))
	for _, verb := range setVerbs {
		for _, ending := range targetEndings {
			for _, suggest := range suggestFormat {
				suggests = append(suggests, strings.ToLower(verb+" "+suggest+" "+ending))
			}
		}
	}
	for i := range suggests {
		suggests[i] = options.AddHouseholdToSuggest(suggests[i])
	}
	// relative mode suggests
	suggests = append(suggests, getRelativeModeSuggests(instance, inflection, options)...)
	return suggests
}

func getRelativeModeSuggests(instance model.ModeCapabilityInstance, inflection inflector.Inflection, options model.SuggestionsOptions) []string {
	suggests := make([]string, 0, 2)
	switch {
	case instance == model.ThermostatModeInstance:
		suggests = append(suggests,
			fmt.Sprintf("поставь предыдущий режим работы %s", inflection.Rod),
			fmt.Sprintf("включи следующий режим работы %s", inflection.Rod))
	case slices.Contains(modeInstanceFemaleGender, string(instance)):
		suggests = append(suggests,
			fmt.Sprintf("включи предыдущую %s %s", modeInstanceInflections[instance].Vin, inflection.Rod),
			fmt.Sprintf("поставь следующую %s на %s", modeInstanceInflections[instance].Vin, inflection.Pr))
	case slices.Contains(modeInstanceNeutralGender, string(instance)):
		suggests = append(suggests,
			fmt.Sprintf("включи предыдущее %s %s", modeInstanceInflections[instance].Vin, inflection.Rod),
			fmt.Sprintf("поставь следующее %s на %s", modeInstanceInflections[instance].Vin, inflection.Pr))
	default:
		suggests = append(suggests,
			fmt.Sprintf("включи предыдущий %s %s", modeInstanceInflections[instance].Vin, inflection.Rod),
			fmt.Sprintf("поставь следующий %s на %s", modeInstanceInflections[instance].Vin, inflection.Pr))
	}
	for i := range suggests {
		suggests[i] = options.AddHouseholdToSuggest(suggests[i])
	}
	return suggests
}

func getSuggestsForFanSpeedMode(deviceType model.DeviceType, instance model.ModeCapabilityInstance, modes []model.Mode, inflection inflector.Inflection, options model.SuggestionsOptions) []string {
	suggestions := make([]string, 0)
	switch instance {
	case model.FanSpeedModeInstance:
		for _, mode := range modes {
			switch mode.Value {
			case model.AutoMode:
				if deviceType == model.ThermostatDeviceType {
					suggestions = append(suggestions,
						"включи скорость вентиляции в авто",
						"включи скорость вентиляции на авто")
				}
			case model.LowMode:
				suggestions = append(suggestions,
					"включи низкую скорость вентиляции на "+inflection.Pr,
					"включи скорость вентиляции "+inflection.Rod+" на минимум")
				if deviceType == model.ThermostatDeviceType {
					suggestions = append(suggestions,
						"включи низкую скорость вентиляции",
						"включи скорость вентиляции на минимум")
				}
			case model.MediumMode:
				suggestions = append(suggestions,
					"включи среднюю скорость вентиляции на "+inflection.Pr,
					"включи скорость вентиляции "+inflection.Rod+" на среднюю")
				if deviceType == model.ThermostatDeviceType {
					suggestions = append(suggestions,
						"включи среднюю скорость вентиляции",
						"включи скорость вентиляции на среднюю")
				}
			case model.HighMode:
				suggestions = append(suggestions,
					"включи высокую скорость вентиляции на "+inflection.Pr,
					"включи скорость вентиляции "+inflection.Rod+" на максимум")
				if deviceType == model.ThermostatDeviceType {
					suggestions = append(suggestions,
						"включи высокую скорость вентиляции",
						"включи скорость вентиляции на максимум")
				}
			}
		}
	}
	for i := range suggestions {
		suggestions[i] = options.AddHouseholdToSuggest(suggestions[i])
	}
	return suggestions
}

// todo: merge this into getBasicModeSuggests
func getSuggestsForOldModes(deviceType model.DeviceType, instance model.ModeCapabilityInstance, modes []model.Mode, inflection inflector.Inflection, options model.SuggestionsOptions) []string {
	suggestions := make([]string, 0)
	switch instance {
	case model.ThermostatModeInstance:
		for _, mode := range modes {
			switch mode.Value {
			case model.EcoMode:
				if deviceType == model.ThermostatDeviceType {
					suggestions = append(suggestions,
						"включи эко режим",
						"переключи в эко режим")
				}
			case model.AutoMode:
				if deviceType == model.ThermostatDeviceType {
					suggestions = append(suggestions,
						"включи авто режим",
						"переключи в авто режим")
				}
			case model.FanOnlyMode:
				if deviceType == model.ThermostatDeviceType {
					suggestions = append(suggestions,
						"включи режим вентиляции",
						"переключи в режим вентиляции")
				}
			case model.CoolMode:
				if deviceType == model.ThermostatDeviceType {
					suggestions = append(suggestions,
						"включи режим охлаждения",
						"переключи в режим охлаждения")
				}
			case model.HeatMode:
				if deviceType == model.ThermostatDeviceType {
					suggestions = append(suggestions,
						"включи режим обогрева",
						"переключи в режим обогрева")
				}
			case model.DryMode:
				if deviceType == model.ThermostatDeviceType {
					suggestions = append(suggestions,
						"включи режим осушения",
						"переключи в режим осушения")
				}
			case model.WindFreeMode:
				if deviceType == model.ThermostatDeviceType || deviceType == model.AcDeviceType {
					suggestions = append(suggestions,
						"включи режим WindFree",
						"переключи в режим WindFree")
				}
			default:
			}
		}
	case model.DishwashingModeInstance:
		for _, mode := range modes {
			switch mode.Value {
			case model.AutoMode:
				suggestions = append(suggestions,
					"помой посуду в режиме авто",
					"включи мойку посуды в авто режим")
			case model.Auto45Mode:
				suggestions = append(suggestions,
					"запусти мойку посуды в режиме авто 45 на "+inflection.Pr,
					"включи мойку посуды в режиме авто 45 градусов на "+inflection.Pr)
			case model.Auto60Mode:
				suggestions = append(suggestions,
					"поставь мойку посуды в режиме авто 60 на "+inflection.Pr,
					"помой посуду в режиме авто 60",
					"запусти мойку посуды на 60 градусов")
			case model.Auto75Mode:
				suggestions = append(suggestions,
					"включи мойку посуды в режиме авто 75 на "+inflection.Pr,
					"помой посуду в режиме авто 75",
					"запусти мойку посуды на 75 градусов")
			case model.Fast45Mode:
				suggestions = append(suggestions,
					"поставь быструю мойку посуды на 45 на "+inflection.Pr)
			case model.Fast60Mode:
				suggestions = append(suggestions,
					"включи быструю мойку посуды на 60",
					"запусти быструю мойку посуды на 60 на "+inflection.Pr)
			case model.Fast75Mode:
				suggestions = append(suggestions,
					"поставь быструю мойку посуды на 75 на "+inflection.Pr)
			case model.PreRinseMode:
				suggestions = append(suggestions,
					"запусти программу ополаскивания на "+inflection.Pr,
					"прополощи мою посуду",
					"включи мойку посуды в режим ополаскивания")
			case model.IntensiveMode:
				suggestions = append(suggestions,
					"запусти "+inflection.Vin+" в интенсивном режиме",
					"включи интенсивную программу мойки",
					"поставь мойку посуды в интенсивный режим")
			case model.GlassMode:
				suggestions = append(suggestions,
					"запусти "+inflection.Vin+" в режиме мойки стекла",
					"включи программу мойки стекла",
					"помой стеклянную посуду")
			}
		}
	case model.TeaModeInstance:
		for _, mode := range modes {
			switch mode.Value {
			case model.BlackTeaMode:
				if deviceType != model.KettleDeviceType {
					suggestions = append(suggestions,
						fmt.Sprintf("сделай черный чай на %s", inflection.Pr),
						fmt.Sprintf("приготовь черный чай на %s", inflection.Pr),
					)
				} else {
					suggestions = append(suggestions,
						"сделай черный чай",
						"приготовь черный чай",
					)
				}
			case model.GreenTeaMode:
				if deviceType != model.KettleDeviceType {
					suggestions = append(suggestions,
						fmt.Sprintf("сделай зеленый чай на %s", inflection.Pr),
						fmt.Sprintf("приготовь зеленый чай на %s", inflection.Pr),
					)
				} else {
					suggestions = append(suggestions,
						"сделай зеленый чай",
						"приготовь зеленый чай",
					)
				}
			case model.PuerhTeaMode:
				if deviceType != model.KettleDeviceType {
					suggestions = append(suggestions,
						fmt.Sprintf("сделай пуэр на %s", inflection.Pr),
						fmt.Sprintf("приготовь пуэр на %s", inflection.Pr),
					)
				} else {
					suggestions = append(suggestions,
						"сделай пуэр",
						"приготовь пуэр",
					)
				}
			case model.WhiteTeaMode:
				if deviceType != model.KettleDeviceType {
					suggestions = append(suggestions,
						fmt.Sprintf("сделай белый чай на %s", inflection.Pr),
						fmt.Sprintf("приготовь белый чай на %s", inflection.Pr),
					)
				} else {
					suggestions = append(suggestions,
						"сделай белый чай",
						"приготовь белый чай",
					)
				}
			case model.OolongTeaMode:
				if deviceType != model.KettleDeviceType {
					suggestions = append(suggestions,
						fmt.Sprintf("сделай улун на %s", inflection.Pr),
						fmt.Sprintf("приготовь улун на %s", inflection.Pr),
					)
				} else {
					suggestions = append(suggestions,
						"сделай улун",
						"приготовь улун",
					)
				}
			case model.RedTeaMode:
				if deviceType != model.KettleDeviceType {
					suggestions = append(suggestions,
						fmt.Sprintf("сделай красный чай на %s", inflection.Pr),
						fmt.Sprintf("приготовь красный чай на %s", inflection.Pr),
					)
				} else {
					suggestions = append(suggestions,
						"сделай красный чай",
						"приготовь красный чай",
					)
				}
			case model.HerbalTeaMode:
				if deviceType != model.KettleDeviceType {
					suggestions = append(suggestions,
						fmt.Sprintf("сделай травяной чай на %s", inflection.Pr),
						fmt.Sprintf("приготовь травяной чай на %s", inflection.Pr),
					)
				} else {
					suggestions = append(suggestions,
						"сделай травяной чай",
						"приготовь травяной чай",
					)
				}
			case model.FlowerTeaMode:
				if deviceType != model.KettleDeviceType {
					suggestions = append(suggestions,
						fmt.Sprintf("сделай цветочный чай на %s", inflection.Pr),
						fmt.Sprintf("приготовь цветочный чай на %s", inflection.Pr),
					)
				} else {
					suggestions = append(suggestions,
						"сделай цветочный чай",
						"приготовь цветочный чай",
					)
				}
			}
		}
	}
	for i := range suggestions {
		suggestions[i] = options.AddHouseholdToSuggest(suggestions[i])
	}
	return suggestions
}
