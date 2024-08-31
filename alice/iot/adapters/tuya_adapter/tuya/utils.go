package tuya

import (
	"fmt"
	"math"
	"strconv"
	"strings"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func percentage(part int, total int) int {
	p := int(math.Round(float64(100) * float64(part) / float64(total)))
	if p > 100 {
		p = 100
	} else if p < 0 {
		p = 0
	}
	return p
}

func percentFromRange(value int, minValue int, maxValue int) int {
	return percentage(value-minValue, maxValue-minValue)
}

func strToBool(value string) bool {
	return strings.ToLower(value) == "true"
}

//Get a value from a range using its percentage within the specified range of all values
//(the inverse of the function percent_from_range)
//Usage example:
//Recalculate the brightness value from relative percentage of the lamp to current value according to Tuya specification
func rangeValueFromPercent(percent int, minValue int, maxValue int) int {
	return int(math.Round(float64(minValue) + float64(maxValue-minValue)*float64(percent)*float64(0.01)))
}

func hsvStateToTuyaHsv(hsvState model.HSV, pid TuyaDeviceProductID) model.HSV {
	lampSpec := LampSpecByPID(pid)
	color := model.HSV{
		H: hsvState.H,
		S: rangeValueFromPercent(hsvState.S, lampSpec.ColorSaturation.Min, lampSpec.ColorSaturation.Max),
		V: rangeValueFromPercent(hsvState.V, lampSpec.ColorBrightness.Min, lampSpec.ColorBrightness.Max),
	}

	//normalize color to provide users more natural colors with poor Tuya lamps
	if paletteColor, inPalette := model.ColorPalette.GetColorByHS(hsvState); inPalette {
		if paletteColor, exist := getColorPalette(pid).GetColorByID(paletteColor.ID); exist {
			color.H = paletteColor.ValueHSV.H
			color.S = rangeValueFromPercent(percentFromRange(paletteColor.ValueHSV.S, 0, 1000), lampSpec.ColorSaturation.Min, lampSpec.ColorSaturation.Max)
		}
	}

	return color
}

func tuyaHsvToHsvState(hsvState model.HSV, pid TuyaDeviceProductID) model.HSV {
	lampSpec := LampSpecByPID(pid)
	brightnessPercent := percentFromRange(hsvState.V, lampSpec.ColorBrightness.Min, lampSpec.ColorBrightness.Max)
	if brightnessPercent == 0 {
		brightnessPercent = 1
	}

	saturationPercent := percentFromRange(hsvState.S, lampSpec.ColorSaturation.Min, lampSpec.ColorSaturation.Max)
	hue := hsvState.H

	unnormalizedColor := model.HSV{
		H: hue,
		S: saturationPercent,
		V: brightnessPercent,
	}

	// Unnormalize color
	color := model.HSV{
		H: hue,
		S: rangeValueFromPercent(saturationPercent, 0, 1000),
		V: brightnessPercent,
	}
	if normalizedColor, normalized := getColorByHS(color, pid); normalized {
		if paletteColor, inPalette := model.ColorPalette.GetColorByID(normalizedColor.ID); inPalette {
			unnormalizedColor.H = paletteColor.ValueHSV.H
			unnormalizedColor.S = paletteColor.ValueHSV.S
		}
	}

	return unnormalizedColor
}

func ContainsAll(main []string, toBeContained map[string]string) bool {
	if main == nil || toBeContained == nil {
		return false
	}

	for _, elem := range main {
		if _, found := toBeContained[elem]; !found {
			return false
		}
	}
	return true
}

func ContainsAny(main []string, toBeContained map[string]string) bool {
	if main == nil || toBeContained == nil {
		return false
	}

	for _, elem := range main {
		if _, found := toBeContained[elem]; found {
			return true
		}
	}
	return false
}

func SplitFloat64ToInts(value float64) []int {
	stringvalue := strconv.Itoa(int(value))
	digits := make([]int, 0, len(stringvalue))

	for _, charDigit := range stringvalue {
		intDigit, _ := strconv.Atoi(fmt.Sprintf("%c", charDigit))
		digits = append(digits, intDigit)
	}

	return digits
}

func temperatureKToTempValue(temperatureK model.TemperatureK, pid TuyaDeviceProductID) int {
	lampSpec := LampSpecByPID(pid)
	colorTemperaturePercent := percentFromRange(int(temperatureK), lampSpec.TempKSpec.Min, lampSpec.TempKSpec.Max)

	return rangeValueFromPercent(colorTemperaturePercent, lampSpec.TempValueSpec.Min, lampSpec.TempValueSpec.Max)
}

func tempValueToTemperatureK(tempValue int, pid TuyaDeviceProductID) int {
	lampSpec := LampSpecByPID(pid)
	tempValuePercent := percentFromRange(tempValue, lampSpec.TempValueSpec.Min, lampSpec.TempValueSpec.Max)

	return rangeValueFromPercent(tempValuePercent, lampSpec.TempKSpec.Min, lampSpec.TempKSpec.Max)
}

func FilterTvKnownKeys(keysMap map[string]string) map[string]string {
	filteredMap := make(map[string]string)

	// Filter out digits if not all of them presents in current keysMap
	filterOutDigits := !ContainsAll(ChannelDigitsIRKeys, keysMap)

	for k, v := range keysMap {
		if tools.Contains(k, KnownTVIRKeys) && !(filterOutDigits && tools.Contains(k, ChannelDigitsIRKeys)) {
			filteredMap[k] = v
		}
	}

	return filteredMap
}

func ReplaceInputSourceIRKeys(keysMap map[string]string) map[string]string {
	replacedMap := make(map[string]string)

	// Filter out real input sources ir key names
	for k, v := range keysMap {
		if !tools.Contains(k, InputSourceIRKeys) {
			replacedMap[k] = v
		}
	}

	inputSourcesKeysMap, _ := MaskInputSourceKeysUnderFictionalKeyNames(keysMap)
	for k, v := range inputSourcesKeysMap {
		replacedMap[string(k)] = v
	}
	return replacedMap
}

func GetLastNumberOfFirmwareVersion(version string) (int64, error) {
	versionSplitted := strings.Split(version, ".")
	if len(versionSplitted) != 3 {
		return 0, xerrors.New("failed to split version by 3 parts: version parts count is not equal to 3")
	}
	lastBitofVersion := versionSplitted[2]
	intVersion, err := strconv.ParseInt(lastBitofVersion, 10, 32)
	if err != nil {
		return 0, xerrors.New("failed to get last number of firmware version: last part is not a number")
	}
	return intVersion, nil
}

func lampFirmwareSupportScenes(productID TuyaDeviceProductID, version *string) bool {
	if version == nil {
		return false
	}
	intVersion, err := GetLastNumberOfFirmwareVersion(*version)
	if err != nil {
		return false
	}
	switch productID {
	case LampE27Lemon2YandexProductID:
		return intVersion >= 15
	case LampE27Lemon3YandexProductID:
		return intVersion >= 2
	case LampE14Test2YandexProductID:
		return intVersion >= 16
	case LampE14MPYandexProductID:
		return intVersion >= 0
	case LampGU10TestYandexProductID:
		return intVersion >= 16
	case LampGU10MPYandexProductID:
		return intVersion >= 0
	default:
		return false
	}
}
