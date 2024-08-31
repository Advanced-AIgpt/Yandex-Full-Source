package model

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestColorSettingHSVWCapabilityState_Validate(t *testing.T) {
	capability := MakeCapabilityByType(ColorSettingCapabilityType)
	capability.SetParameters(ColorSettingCapabilityParameters{
		ColorModel: CM(HsvModelType),
		TemperatureK: &TemperatureKParameters{
			Min: 2700,
			Max: 6500,
		},
		ColorSceneParameters: &ColorSceneParameters{
			Scenes: ColorScenes{
				KnownColorScenes[ColorSceneIDFantasy],
				KnownColorScenes[ColorSceneIDCandle],
			},
		},
	})

	validHSV1 := ColorSettingCapabilityState{
		Instance: HsvColorCapabilityInstance,
		Value: HSV{
			H: 5,
			S: 5,
			V: 5,
		},
	}

	validHSV2 := ColorSettingCapabilityState{
		Instance: HsvColorCapabilityInstance,
		Value: HSV{
			H: 0,
			S: 0,
			V: 0,
		},
	}

	validK := ColorSettingCapabilityState{
		Instance: TemperatureKCapabilityInstance,
		Value:    TemperatureK(4700),
	}

	validKMin := ColorSettingCapabilityState{
		Instance: TemperatureKCapabilityInstance,
		Value:    TemperatureK(2700),
	}

	validKMax := ColorSettingCapabilityState{
		Instance: TemperatureKCapabilityInstance,
		Value:    TemperatureK(6500),
	}

	validColorScene := ColorSettingCapabilityState{
		Instance: SceneCapabilityInstance,
		Value:    ColorSceneIDCandle,
	}

	invalidHSVValue := ColorSettingCapabilityState{
		Instance: HsvColorCapabilityInstance,
	}

	invalidHSVValueCases := []HSV{
		{
			H: 370,
			S: 5,
			V: 5,
		},
		{
			H: 5,
			S: 105,
			V: 5,
		},
		{
			H: 5,
			S: 5,
			V: 105,
		},
		{
			H: -5,
			S: 5,
			V: 5,
		},
		{
			H: 5,
			S: -5,
			V: 5,
		},
		{
			H: 5,
			S: 5,
			V: -5,
		},
	}

	invalidTempKValue1 := ColorSettingCapabilityState{
		Instance: TemperatureKCapabilityInstance,
		Value:    TemperatureK(1500),
	}

	invalidTempKValue2 := ColorSettingCapabilityState{
		Instance: TemperatureKCapabilityInstance,
		Value:    TemperatureK(7000),
	}

	invalidInstance := ColorSettingCapabilityState{
		Instance: "rgbw",
		Value:    RGB(35),
	}

	invalidInstanceRGB := ColorSettingCapabilityState{
		Instance: RgbColorCapabilityInstance,
		Value:    RGB(35),
	}

	invalidColorScene := ColorSettingCapabilityState{
		Instance: SceneCapabilityInstance,
		Value:    ColorSceneIDMovie,
	}

	assert.NoError(t, validHSV1.ValidateState(capability))
	assert.NoError(t, validHSV2.ValidateState(capability))
	assert.NoError(t, validK.ValidateState(capability))
	assert.NoError(t, validKMin.ValidateState(capability))
	assert.NoError(t, validKMax.ValidateState(capability))
	assert.NoError(t, validColorScene.ValidateState(capability))

	assert.EqualError(t, invalidTempKValue1.ValidateState(capability), "temperature_k color_setting instance state value is out of supported range: '1500'")
	assert.EqualError(t, invalidTempKValue2.ValidateState(capability), "temperature_k color_setting instance state value is out of supported range: '7000'")

	assert.EqualError(t, invalidInstance.ValidateState(capability), "unknown color_setting state instance: 'rgbw'")
	assert.EqualError(t, invalidInstanceRGB.ValidateState(capability), "unsupported by current device color_setting state instance: 'rgb'")

	assert.EqualError(t, invalidColorScene.ValidateState(capability), "scene value color_setting state value is not supported by current device: 'movie'")

	for _, testCase := range invalidHSVValueCases {
		invalidHSVValue.Value = testCase
		assert.Error(t, invalidHSVValue.ValidateState(capability))
	}

}

func TestColorSettingRGBCapabilityState_Validate(t *testing.T) {
	capability := MakeCapabilityByType(ColorSettingCapabilityType)
	capability.SetParameters(ColorSettingCapabilityParameters{
		ColorModel: CM(RgbModelType),
	})

	validRGBValue := ColorSettingCapabilityState{
		Instance: RgbColorCapabilityInstance,
		Value:    RGB(1000),
	}

	validRGBValueMin := ColorSettingCapabilityState{
		Instance: RgbColorCapabilityInstance,
		Value:    RGB(0),
	}

	validRGBValueMax := ColorSettingCapabilityState{
		Instance: RgbColorCapabilityInstance,
		Value:    RGB(16777215),
	}

	invalidInstanceHSV := ColorSettingCapabilityState{
		Instance: HsvColorCapabilityInstance,
		Value: HSV{
			H: 0,
			S: 0,
			V: 0,
		},
	}

	invalidInstanceTempK := ColorSettingCapabilityState{
		Instance: TemperatureKCapabilityInstance,
		Value:    TemperatureK(4500),
	}

	invalidRGBValue1 := ColorSettingCapabilityState{
		Instance: RgbColorCapabilityInstance,
		Value:    RGB(-1),
	}

	invalidRGBValue2 := ColorSettingCapabilityState{
		Instance: RgbColorCapabilityInstance,
		Value:    RGB(96777215),
	}

	assert.NoError(t, validRGBValue.ValidateState(capability))
	assert.NoError(t, validRGBValueMin.ValidateState(capability))
	assert.NoError(t, validRGBValueMax.ValidateState(capability))

	assert.EqualError(t, invalidInstanceHSV.ValidateState(capability), "unsupported by current device color_setting state instance: 'hsv'")
	assert.EqualError(t, invalidInstanceTempK.ValidateState(capability), "unsupported by current device state color_setting instance: 'temperature_k'")

	assert.EqualError(t, invalidRGBValue1.ValidateState(capability), "rgb color_setting instance state value is out of supported range: '-1'")
	assert.EqualError(t, invalidRGBValue2.ValidateState(capability), "rgb color_setting instance state value is out of supported range: '96777215'")
}

func TestColorSettingCapabilityParameters_GetDefaultColor(t *testing.T) {
	// Case 1: Full color
	parameters := ColorSettingCapabilityParameters{
		ColorModel: CM(HsvModelType),
		TemperatureK: &TemperatureKParameters{
			Min: 2500,
			Max: 6500,
		},
	}

	expectedColor, _ := ColorPalette.GetColorByID("soft_white")
	defaultColor, okDefaultColor := parameters.GetDefaultColor()
	assert.True(t, okDefaultColor)
	assert.Equalf(t, expectedColor, defaultColor, "full color")

	// Case 2: hsv only
	parameters = ColorSettingCapabilityParameters{
		ColorModel: CM(HsvModelType),
	}

	expectedColor, _ = ColorPalette.GetColorByID("red")
	defaultColor, okDefaultColor = parameters.GetDefaultColor()
	assert.True(t, okDefaultColor)
	assert.Equalf(t, expectedColor, defaultColor, "hsv only")

	// Case 3: full white only
	parameters = ColorSettingCapabilityParameters{
		TemperatureK: &TemperatureKParameters{
			Min: 2500,
			Max: 6500,
		},
	}

	expectedColor, _ = ColorPalette.GetColorByID("soft_white")
	defaultColor, okDefaultColor = parameters.GetDefaultColor()
	assert.True(t, okDefaultColor)
	assert.Equalf(t, expectedColor, defaultColor, "full while color")

	// Case 4: single white color
	parameters = ColorSettingCapabilityParameters{
		TemperatureK: &TemperatureKParameters{
			Min: 6500,
			Max: 6500,
		},
	}

	expectedColor, _ = ColorPalette.GetColorByID("cold_white")
	defaultColor, okDefaultColor = parameters.GetDefaultColor()
	assert.True(t, okDefaultColor)
	assert.Equalf(t, expectedColor, defaultColor, "single white color")

	// Case 4: white color outside known range
	parameters = ColorSettingCapabilityParameters{
		TemperatureK: &TemperatureKParameters{
			Min: 7000,
			Max: 7000,
		},
	}

	expectedColor, _ = ColorPalette.GetColorByID("white")
	defaultColor, okDefaultColor = parameters.GetDefaultColor()
	assert.True(t, okDefaultColor)
	assert.Equalf(t, expectedColor, defaultColor, "white color outside known range")
}
