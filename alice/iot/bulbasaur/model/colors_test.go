package model

import (
	"fmt"
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestColorPaletteGetDefaultWhiteColor(t *testing.T) {
	result := ColorPalette.GetDefaultWhiteColor()
	color, exists := ColorPalette["white"]
	if !exists {
		t.Error(fmt.Errorf("failed to get `white` color"))
	}
	assert.Equal(t, color, result)
}

func TestColorPaletteGetColorByTemperatureK(t *testing.T) {
	t.Run("check that palette mapping is correct", func(t *testing.T) {
		for temperature, colorID := range ColorIDColorTemperatureMap {
			expectedColor := ColorPalette[colorID]
			color := ColorPalette.GetColorByTemperatureK(temperature)

			assert.Equal(t, expectedColor, color)
		}
	})
	t.Run("check that rounding works ok", func(t *testing.T) {
		fieryWhite := ColorPalette[ColorIDFieryWhite]
		assert.Equal(t, fieryWhite, ColorPalette.GetColorByTemperatureK(1450))
		assert.Equal(t, fieryWhite, ColorPalette.GetColorByTemperatureK(1549))
	})
	t.Run("check that other values are converted to non-palette white", func(t *testing.T) {
		color := ColorPalette.GetColorByTemperatureK(9500)
		assert.Equal(t, Color{Type: "white", ValueHSV: HSV{H: 222, S: 20, V: 100}, Temperature: TemperatureK(9500)}, color)
	})
}

func TestColorPaletteGetNearestColorByColorTemperature(t *testing.T) {
	t.Run("out of left bound", func(t *testing.T) {
		fieryWhite := ColorPalette[ColorIDFieryWhite]
		assert.Equal(t, fieryWhite, ColorPalette.GetNearestColorByColorTemperature(1000))
	})
	t.Run("out of right bound", func(t *testing.T) {
		heavenlyWhite := ColorPalette[ColorIDHeavenlyWhite]
		assert.Equal(t, heavenlyWhite, ColorPalette.GetNearestColorByColorTemperature(10000))
	})
	t.Run("check values between bounds", func(t *testing.T) {
		fieryWhite := ColorPalette[ColorIDFieryWhite] // 1500
		softWhite := ColorPalette[ColorIDSoftWhite]   // 2700
		assert.Equal(t, fieryWhite, ColorPalette.GetNearestColorByColorTemperature(2099))
		assert.Equal(t, softWhite, ColorPalette.GetNearestColorByColorTemperature(2100)) // 2100 is breakpoint
		assert.Equal(t, softWhite, ColorPalette.GetNearestColorByColorTemperature(2999))
	})
}

func TestColorPaletteGetColorByHS(t *testing.T) {
	for _, color := range ColorPalette {
		colorT, exists := ColorPalette.GetColorByHS(HSV{color.ValueHSV.H, color.ValueHSV.S, 0})
		assert.True(t, exists)
		assert.Equal(t, color, colorT)
	}
	color, exists := ColorPalette.GetColorByHS(HSV{500, 500, 0})
	assert.False(t, exists)
	assert.Equal(t, Color{}, color)
}

func TestGetPaletteByColorType(t *testing.T) {
	hsvPaletteSlice := ColorPalette.FilterType(Multicolor).ToSortedSlice()
	whitePaletteSlice := ColorPalette.FilterType(WhiteColor).ToSortedSlice()

	for _, color := range hsvPaletteSlice {
		if color.Type != Multicolor {
			t.Error("GetPaletteByColorType test failed - Hsv palette contains not Hsv color")
		}
	}
	for _, color := range whitePaletteSlice {
		if color.Type != WhiteColor {
			t.Error("GetPaletteByColorType test failed - White palette contains not white color")
		}
	}

	totalSelectedColors := hsvPaletteSlice
	totalSelectedColors = append(totalSelectedColors, whitePaletteSlice...)
	assert.Equal(t, len(ColorPalette), len(totalSelectedColors))
}

func TestGetColorPaletteByCapability(t *testing.T) {
	testCapability := MakeCapabilityByType(ColorSettingCapabilityType)
	temperatureFullRangeParams := &TemperatureKParameters{Min: 0, Max: 10000}
	temperatureSingleValueParams := &TemperatureKParameters{Min: 1, Max: 1}
	hsvModel := HsvModelType

	// 1 case: Full white range device without HSV
	testCapabilityParameters := ColorSettingCapabilityParameters{
		TemperatureK: temperatureFullRangeParams,
	}
	testCapability.SetParameters(testCapabilityParameters)
	palette := testCapabilityParameters.GetAvailableColors()
	assert.Equal(t, ColorPalette.FilterType(WhiteColor).ToSortedSlice(), palette)

	// 2 case: HSV device without white mode
	testCapabilityParameters = ColorSettingCapabilityParameters{
		ColorModel: &hsvModel,
	}
	testCapability.SetParameters(testCapabilityParameters)
	palette = testCapabilityParameters.GetAvailableColors()
	assert.Equal(t, ColorPalette.FilterType(Multicolor).ToSortedSlice(), palette)

	// 3 case: Full white range device with HSV
	testCapabilityParameters = ColorSettingCapabilityParameters{
		ColorModel:   &hsvModel,
		TemperatureK: temperatureFullRangeParams,
	}
	testCapability.SetParameters(testCapabilityParameters)
	palette = testCapabilityParameters.GetAvailableColors()
	assert.Equal(t, ColorPalette.ToSortedSlice(), palette)

	// 4 case: Single white color device
	testCapabilityParameters = ColorSettingCapabilityParameters{
		TemperatureK: temperatureSingleValueParams,
	}
	testCapability.SetParameters(testCapabilityParameters)
	palette = testCapabilityParameters.GetAvailableColors()
	assert.Equal(t, []Color{ColorPalette.GetDefaultWhiteColor()}, palette)

	// 5 case: Single white color device + HSV
	testCapabilityParameters = ColorSettingCapabilityParameters{
		TemperatureK: temperatureSingleValueParams,
		ColorModel:   &hsvModel,
	}
	testCapability.SetParameters(testCapabilityParameters)
	palette = testCapabilityParameters.GetAvailableColors()
	expectedPalette := []Color{ColorPalette.GetDefaultWhiteColor()}
	expectedPalette = append(expectedPalette, ColorPalette.FilterType(Multicolor).ToSortedSlice()...)
	assert.Equal(t, expectedPalette, palette)
}

func TestGetColorByCapabilityState(t *testing.T) {
	// 1 case: capability with red (HSV type) color from palette
	expectedColor, exists := ColorPalette["red"]
	if !exists {
		t.Error("Failed to get red color by id")
	}
	testColorCapability := MakeCapabilityByType(ColorSettingCapabilityType)
	testColorCapability.SetState(ColorSettingCapabilityState{
		Instance: HsvColorCapabilityInstance,
		Value:    expectedColor.ValueHSV,
	})

	color, exists := testColorCapability.State().(ColorSettingCapabilityState).ToColor()
	assert.Equal(t, expectedColor, color)
	assert.True(t, exists)

	// 2 case: capability with cold_white (white type) color from palette
	expectedColor, exists = ColorPalette["cold_white"]
	if !exists {
		t.Error("Failed to get cold_white color by id")
	}

	var temperature TemperatureK
	for temperatureFromMap, colorID := range ColorIDColorTemperatureMap {
		if colorID == "cold_white" {
			temperature = temperatureFromMap
			break
		}
	}

	testColorCapability = MakeCapabilityByType(ColorSettingCapabilityType)
	testColorCapability.SetState(ColorSettingCapabilityState{
		Instance: TemperatureKCapabilityInstance,
		Value:    temperature,
	})

	color, exists = testColorCapability.State().(ColorSettingCapabilityState).ToColor()
	assert.Equal(t, expectedColor, color)
	assert.True(t, exists)

	// 3 case: capability with HSV color not from palette
	expectedColor = Color{
		Type: Multicolor,
		ValueHSV: HSV{
			H: 255,
			S: 1,
			V: 1,
		},
		ValueRGB: 131586,
	}
	testColorCapability = MakeCapabilityByType(ColorSettingCapabilityType)
	testColorCapability.SetState(ColorSettingCapabilityState{
		Instance: HsvColorCapabilityInstance,
		Value:    expectedColor.ValueHSV,
	})

	color, exists = testColorCapability.State().(ColorSettingCapabilityState).ToColor()
	assert.Equal(t, expectedColor, color)
	assert.True(t, exists)

	// 4 case: capability with white color not from palette
	testColorCapability = MakeCapabilityByType(ColorSettingCapabilityType)
	testColorCapability.SetState(ColorSettingCapabilityState{
		Instance: TemperatureKCapabilityInstance,
		Value:    TemperatureK(9999),
	})

	expectedColor = ColorPalette[ColorIDHeavenlyWhite]
	color, exists = testColorCapability.State().(ColorSettingCapabilityState).ToColor()
	assert.Equal(t, expectedColor, color)
	assert.True(t, exists)
}

// Test GetColorSettingCapabilityStateByColorID & getColorTemperatureByID at the same time
func TestGetColorSettingCapabilityStateByColorID(t *testing.T) {
	// test hsv color
	redColor := ColorPalette["red"]
	redColorHsv := HSV{
		H: redColor.ValueHSV.H,
		S: redColor.ValueHSV.S,
		V: redColor.ValueHSV.V,
	}
	redColorCapabilityState := ColorSettingCapabilityState{
		Instance: HsvColorCapabilityInstance,
		Value:    redColorHsv,
	}

	color := ColorPalette["red"]
	capabilityState := color.ToColorSettingCapabilityState(HsvColorCapabilityInstance)
	assert.Equal(t, redColorCapabilityState, capabilityState)

	// Test white color
	whiteColorCapabilityState := ColorSettingCapabilityState{
		Instance: TemperatureKCapabilityInstance,
		Value:    TemperatureK(4500),
	}

	color = ColorPalette["white"]
	capabilityState = color.ToColorSettingCapabilityState(TemperatureKCapabilityInstance)
	assert.Equal(t, whiteColorCapabilityState, capabilityState)

	// Fake color
	color, exists := ColorPalette["mishareds"]
	assert.False(t, exists)
	assert.Equal(t, ColorSettingCapabilityState{}, color.ToColorSettingCapabilityState(""))
}

func TestRgbToHsvConverter(t *testing.T) {
	for _, c := range ColorPalette {
		assert.InDelta(t, c.ValueHSV.H, rgbToHsv(c.ValueRGB).H, 5.0)
		assert.InDelta(t, c.ValueHSV.S, rgbToHsv(c.ValueRGB).S, 5.0)
		assert.InDelta(t, c.ValueHSV.V, rgbToHsv(c.ValueRGB).V, 5.0)
	}
}

func TestHsvToRgbConverter(t *testing.T) {
	for _, c := range ColorPalette {
		//FIXME: there should be a better way to check conversion
		assert.InDelta(t, int(c.ValueRGB), int(hsvToRgb(c.ValueHSV)), (2<<15)*3)
	}
}

func TestColorPaletteReverse(t *testing.T) {
	c1 := Color{ID: "c1", Type: WhiteColor, Temperature: TemperatureK(1)}
	c2 := Color{ID: "c2", Type: WhiteColor, Temperature: TemperatureK(2)}

	assert.Equal(t, 2, len(ColorPaletteType{"c1": c1, "c2": c2}.toSlice()))
	assert.Equal(t, 2, len(ColorPaletteType{"c1": c1, "c2": c2}.ToSortedSlice()))
	assert.Equal(t, []Color{c1, c2}, ColorPaletteType{"1": c1, "2": c2}.ToSortedSlice())
	assert.Equal(t, []Color{c1, c2}, reverse([]Color{c2, c1}))
}

func TestColorPaletteIteration(t *testing.T) {
	//simple
	c1 := Color{ID: "c1", Type: WhiteColor, Temperature: TemperatureK(1)}
	c2 := Color{ID: "c2", Type: WhiteColor, Temperature: TemperatureK(2)}

	cp := ColorPaletteType{
		"c1": c1,
		"c2": c2}

	assert.Equal(t, c2, cp.GetNext(c1))
	assert.Equal(t, c1, cp.GetPrevious(c2))

	//last
	assert.Equal(t, c2, cp.GetNext(c2))

	//first
	assert.Equal(t, c1, cp.GetPrevious(c1))

	//out of range, right side
	c3 := Color{ID: "c3", Type: WhiteColor, Temperature: TemperatureK(3)}
	assert.Equal(t, c3, cp.GetNext(c3))
	assert.Equal(t, c2, cp.GetPrevious(c3))

	//out of range, left side
	c4 := Color{ID: "c4", Type: WhiteColor, Temperature: TemperatureK(0)}
	assert.Equal(t, c4, cp.GetPrevious(c4))
	assert.Equal(t, c1, cp.GetNext(c4))
}

func TestTemperatureToRgbConversionValid(t *testing.T) {
	for i := -10000; i < 10000; i++ {
		r, g, b := temperatureToRgb(TemperatureK(i))
		assert.True(t, 0 <= r && r <= 255)
		assert.True(t, 0 <= g && g <= 255)
		assert.True(t, 0 <= b && b <= 255)
	}
}
