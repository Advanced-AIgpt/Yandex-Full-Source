package tuya

import (
	"fmt"
	"testing"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/library/go/ptr"
	"github.com/stretchr/testify/assert"
)

func TestContainsAll(t *testing.T) {
	testMap := map[string]string{
		"a": "1",
		"b": "2",
		"c": "3",
	}

	assert.True(t, ContainsAll([]string{"a", "b", "c"}, testMap))
	assert.True(t, ContainsAll([]string{"b", "c"}, testMap))
	assert.False(t, ContainsAll([]string{"a", "b", "c", "d"}, testMap))
	assert.False(t, ContainsAll([]string{"a", "b", "d"}, testMap))
	assert.False(t, ContainsAll(nil, testMap))
}

func TestSplitFloat64ToInts(t *testing.T) {
	testCases := map[float64][]int{
		float64(0):      {0},
		float64(5):      {5},
		float64(10):     {1, 0},
		float64(55):     {5, 5},
		float64(123):    {1, 2, 3},
		float64(100500): {1, 0, 0, 5, 0, 0},
	}

	for testValue, expectedList := range testCases {
		assert.Equal(t, expectedList, SplitFloat64ToInts(testValue))
	}
}

func Test_temperatureKToTempValue(t *testing.T) {
	// old yandex lamps
	assert.Equal(t, 99, temperatureKToTempValue(4200, LampYandexProductID))
	assert.Equal(t, 0, temperatureKToTempValue(2700, LampYandexProductID))
	assert.Equal(t, 255, temperatureKToTempValue(6500, LampYandexProductID))
	// new yandex lamp (yeelight)
	assert.Equal(t, 540, temperatureKToTempValue(4200, LampE27Lemon2YandexProductID))
	assert.Equal(t, 0, temperatureKToTempValue(1500, LampE27Lemon2YandexProductID))
	assert.Equal(t, 1000, temperatureKToTempValue(6500, LampE27Lemon2YandexProductID))
}

func Test_tempValueToTemperatureK(t *testing.T) {
	// old yandex lamps
	assert.Equal(t, 4486, tempValueToTemperatureK(120, LampYandexProductID))
	assert.Equal(t, 2700, tempValueToTemperatureK(0, LampYandexProductID))
	assert.Equal(t, 6500, tempValueToTemperatureK(255, LampYandexProductID))
	// new yandex lamp (yeelight)
	assert.Equal(t, 4500, tempValueToTemperatureK(600, LampE27Lemon2YandexProductID))
	assert.Equal(t, 1500, tempValueToTemperatureK(0, LampE27Lemon2YandexProductID))
	assert.Equal(t, 6500, tempValueToTemperatureK(1000, LampE27Lemon2YandexProductID))
}

func Test_PercentFromRange(t *testing.T) {
	type TestCase struct {
		value           int
		rangeMin        int
		rangeMax        int
		expectedPercent int
	}

	cases := []TestCase{
		{
			1,
			1,
			100,
			0,
		},
		{
			1,
			0,
			100,
			1,
		},
		{
			255,
			1,
			255,
			100,
		},
		{
			75,
			50,
			100,
			50,
		},
	}

	for _, c := range cases {
		assert.Equal(t, c.expectedPercent, percentFromRange(c.value, c.rangeMin, c.rangeMax))
	}
}

func Test_percentage(t *testing.T) {
	assert.Equal(t, 0, rangeValueFromPercent(percentFromRange(0, 0, 360), 0, 255))
	assert.Equal(t, 96, percentFromRange(960, 0, 1000))
	assert.Equal(t, 960, rangeValueFromPercent(96, 0, 1000))
}

func Test_HsvStateToTuyaHsv(t *testing.T) {
	normalizableColor := model.HSV{H: 0, S: 96, V: 55}    // red color
	unnormalizableColor := model.HSV{H: 55, S: 96, V: 55} // non palette color

	// Tuya classic lamps
	assert.Equal(t, model.HSV{H: 0, S: 255, V: 141}, hsvStateToTuyaHsv(normalizableColor, LampYandexProductID), "Yandex old lamp")
	assert.Equal(t, model.HSV{H: 55, S: 245, V: 141}, hsvStateToTuyaHsv(unnormalizableColor, LampYandexProductID), "Yandex old lamp")

	// Yandex yeelight lamps
	assert.Equal(t, model.HSV{H: 0, S: 1000, V: 550}, hsvStateToTuyaHsv(normalizableColor, LampE27Lemon2YandexProductID), "Yandex yeelight lamp")
	assert.Equal(t, model.HSV{H: 55, S: 960, V: 550}, hsvStateToTuyaHsv(unnormalizableColor, LampE27Lemon2YandexProductID), "Yandex yeelight lamp")
}

func Test_TuyaHsvToHsvState(t *testing.T) {
	type testCase struct {
		name      string
		color     model.HSV
		expected  model.HSV
		productID TuyaDeviceProductID
	}
	testCases := []testCase{
		{
			name:      "old lamp normalize color",
			color:     model.HSV{H: 0, S: 255, V: 140},
			expected:  model.HSV{H: 0, S: 96, V: 55},
			productID: LampYandexProductID,
		},
		{
			name:      "old lamp unnormalized color",
			color:     model.HSV{H: 55, S: 245, V: 140},
			expected:  model.HSV{H: 55, S: 96, V: 55},
			productID: LampYandexProductID,
		},
		{
			name:      "new lamp normalized color",
			color:     model.HSV{H: 0, S: 1000, V: 140},
			expected:  model.HSV{H: 0, S: 96, V: 14},
			productID: LampE27Lemon2YandexProductID,
		},
		{
			name:      "new lamp unnormalized color",
			color:     model.HSV{H: 55, S: 245, V: 140},
			expected:  model.HSV{H: 55, S: 25, V: 14},
			productID: LampE27Lemon2YandexProductID,
		},
	}
	for _, tc := range testCases {
		assert.Equal(t, tc.expected, tuyaHsvToHsvState(tc.color, tc.productID), tc.name)
	}
}

func Test_PercentAndRangeConvertEqual(t *testing.T) {
	rangeMin := 1
	rangeMax := 255
	iterateRange := make([]int, 100)
	for percent := range iterateRange {
		valueFromPercent := rangeValueFromPercent(percent, rangeMin, rangeMax)
		assert.Equal(t, percent, percentFromRange(valueFromPercent, rangeMin, rangeMax), fmt.Sprintf("Testing percent value: %d", percent))
	}
}

func Test_HSV_ToTuyaAndBack(t *testing.T) {
	for _, color := range model.ColorPalette {
		colorHSV := color.ValueHSV
		for v := 1; v < 100; v++ {
			// we adjust brightness in color mode as V in HSV
			adjustedHSV := colorHSV
			adjustedHSV.V = v
			for _, pid := range KnownYandexLampProductID {
				converted := hsvStateToTuyaHsv(adjustedHSV, TuyaDeviceProductID(pid))
				unconverted := tuyaHsvToHsvState(converted, TuyaDeviceProductID(pid))
				assert.Equal(t, adjustedHSV, unconverted, "color convertation for product id %s is failed: %v is not equal %v, hsv -> tuya = %v", pid, colorHSV, unconverted, converted)
			}
		}

	}
}

func TestFilterTvKnownKeys(t *testing.T) {
	keysMap := map[string]string{
		"1":            "1",
		"2":            "2",
		"3":            "3",
		"channel_down": "channel_down",
		"channel_up":   "channel_up",
	}
	allowedKeys := FilterTvKnownKeys(keysMap)
	expectedAllowedKeys := map[string]string{
		"channel_down": "channel_down",
		"channel_up":   "channel_up",
	}
	assert.Equal(t, expectedAllowedKeys, allowedKeys)
}

func TestReplaceInputSourceIRKeys(t *testing.T) {
	keysMap := map[string]string{
		"1":                             "1",
		"2":                             "2",
		"3":                             "3",
		"channel_down":                  "channel_down",
		"channel_up":                    "channel_up",
		string(InputSourceHDMI2KeyName): "hdmi_2_key",
		string(InputSourceKeyName):      "input_key",
	}
	replacedKeys := ReplaceInputSourceIRKeys(keysMap)
	expectedReplacedKeys := map[string]string{
		"1":                           "1",
		"2":                           "2",
		"3":                           "3",
		"channel_down":                "channel_down",
		"channel_up":                  "channel_up",
		string(InputSourceOneKeyName): "input_key",
		string(InputSourceTwoKeyName): "hdmi_2_key",
	}
	assert.Equal(t, expectedReplacedKeys, replacedKeys)
}

func TestLampFirmwareSupportScenes(t *testing.T) {
	oldVersion := "1.0.3"
	newVersion := "1.0.19"
	newVersion2 := "1.0.15"

	lamp4Version := "1.0.2"
	assert.False(t, lampFirmwareSupportScenes(LampE27Lemon2YandexProductID, ptr.String(oldVersion)))
	assert.True(t, lampFirmwareSupportScenes(LampE27Lemon2YandexProductID, ptr.String(newVersion)))
	assert.True(t, lampFirmwareSupportScenes(LampE27Lemon2YandexProductID, ptr.String(newVersion2)))
	assert.False(t, lampFirmwareSupportScenes(LampE27Lemon2YandexProductID, ptr.String(lamp4Version)))
	assert.True(t, lampFirmwareSupportScenes(LampE27Lemon3YandexProductID, ptr.String(lamp4Version)))
}
