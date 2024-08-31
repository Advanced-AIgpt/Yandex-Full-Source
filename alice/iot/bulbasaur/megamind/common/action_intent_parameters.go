package common

import (
	"fmt"
	"math"
	"sort"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/tools"
	megamindcommonpb "a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// ActionIntentParameters reflects values stored in the intent_parameters slot of action frames.
// Inside the slot it's stored as a string with lots of spaces and new lines, so the parsing is tricky.
type ActionIntentParameters struct {
	CapabilityType     model.CapabilityType `json:"capability_type,omitempty"`
	CapabilityInstance string               `json:"capability_instance,omitempty"`
	CapabilityValue    interface{}          `json:"capability_value,omitempty"`
	CapabilityUnit     model.Unit           `json:"capability_unit,omitempty"`
	RelativityType     RelativityType       `json:"relativity_type,omitempty"`
}

func (aip *ActionIntentParameters) FromProto(p *megamindcommonpb.TIoTActionIntentParameters) error {
	aip.CapabilityType = model.CapabilityType(p.GetCapabilityType())
	aip.CapabilityInstance = p.GetCapabilityInstance()

	capabilityValue := p.GetCapabilityValue()
	if capabilityValue == nil {
		return xerrors.New("capability value is empty")
	}

	aip.CapabilityUnit = model.Unit(capabilityValue.GetUnit())
	aip.RelativityType = RelativityType(capabilityValue.GetRelativityType())

	if capabilityValue.GetValue() == nil {
		switch capabilityValue.GetRelativityType() {
		case string(Invert):
			return nil
		default:
			return xerrors.New("underlying capability value is empty and relativity is not invert")
		}
	}

	switch value := capabilityValue.GetValue().(type) {
	case *megamindcommonpb.TIoTActionIntentParameters_TCapabilityValue_BoolValue:
		aip.CapabilityValue = value.BoolValue
	case *megamindcommonpb.TIoTActionIntentParameters_TCapabilityValue_NumValue:
		aip.CapabilityValue = value.NumValue
	case *megamindcommonpb.TIoTActionIntentParameters_TCapabilityValue_ModeValue:
		aip.CapabilityValue = value.ModeValue
	default:
		return xerrors.New("unknown capability value type")
	}

	return nil
}

type ActionIntentParametersSlice []ActionIntentParameters

func CapabilityFromIntentParameters(device model.Device, intentParameters ActionIntentParameters) (model.ICapability, error) {
	capability := model.MakeCapabilityByType(intentParameters.CapabilityType)

	switch c := capability.(type) {
	case *model.ColorSettingCapability:
		capability = fillColorSettingCapability(c, device, intentParameters)
	case *model.CustomButtonCapability:
		capability = fillCustomButtonCapability(c, intentParameters)
	case *model.ModeCapability:
		capability = fillModeCapability(c, device, intentParameters)
	case *model.OnOffCapability:
		capability = fillOnOffCapability(c, device, intentParameters)
	case *model.QuasarCapability:
		capability = fillQuasarCapability(c, intentParameters)
	case *model.QuasarServerActionCapability:
		capability = fillQuasarServerActionCapability(c, intentParameters)
	case *model.RangeCapability:
		capability = fillRangeCapability(c, device, intentParameters)
	case *model.ToggleCapability:
		capability = fillToggleCapability(c, device, intentParameters)
	case *model.VideoStreamCapability:
		capability = fillVideoStreamCapability(c, device, intentParameters)
	default:
		return nil, xerrors.New(fmt.Sprintf("unsupported capability: %T", c))
	}

	return capability, nil
}

func fillColorSettingCapability(c *model.ColorSettingCapability, device model.Device, intentParameters ActionIntentParameters) *model.ColorSettingCapability {
	//TemperatureK is the only parameter within ColorSettingCapability which can have `relative` parameter
	switch intentParameters.CapabilityInstance {
	case string(model.TemperatureKCapabilityInstance):
		// assuming TemperatureK has raw kelvin value, in this case `relative` parameter should be nil
		if intentParameters.RelativityType == "" {
			c.SetState(model.ColorSettingCapabilityState{
				Instance: model.TemperatureKCapabilityInstance,
				Value:    model.TemperatureK(intentParameters.CapabilityValue.(int)),
			})

			// `relative` is not nil, so we need to get Next or previous TemperatureK value from ColorPalette
		} else {
			capability, _ := device.GetCapabilityByTypeAndInstance(model.ColorSettingCapabilityType, string(model.TemperatureKCapabilityInstance))
			var newColor model.Color

			//if state is not nil, get current color from state
			if capability.State() != nil && capability.State().(model.ColorSettingCapabilityState).Instance == model.TemperatureKCapabilityInstance {
				currentColor, _ := capability.State().(model.ColorSettingCapabilityState).ToColor()

				switch intentParameters.RelativityType {
				case Increase:
					newColor = model.ColorPalette.FilterType(model.WhiteColor).GetNext(currentColor)
				case Decrease:
					newColor = model.ColorPalette.FilterType(model.WhiteColor).GetPrevious(currentColor)
				}

				maxValue := capability.Parameters().(model.ColorSettingCapabilityParameters).TemperatureK.Max
				minValue := capability.Parameters().(model.ColorSettingCapabilityParameters).TemperatureK.Min

				if newColor.Temperature > maxValue || newColor.Temperature < minValue {
					newColor = currentColor
				}

				//otherwise, set color to default
			} else {
				newColor = model.ColorPalette.GetDefaultWhiteColor()
			}
			c.SetState(newColor.ToColorSettingCapabilityState(model.TemperatureKCapabilityInstance))
		}
	case model.HypothesisColorSceneCapabilityInstance:
		colorScene := model.KnownColorScenes[(model.ColorSceneID((intentParameters.CapabilityValue).(string)))] // you cannot set unknown color scene via mm
		c.SetState(colorScene.ToColorSettingCapabilityState())
	default:
		color, _ := model.ColorPalette.GetColorByID(model.ColorID((intentParameters.CapabilityValue).(string))) // you cannot set unknown color via mm
		capability := device.GetCapabilitiesByType(model.ColorSettingCapabilityType)[0]
		c.SetState(color.ToColorSettingCapabilityState(
			capability.Parameters().(model.ColorSettingCapabilityParameters).GetColorSettingCapabilityInstance()))
	}

	return c
}

func fillCustomButtonCapability(c *model.CustomButtonCapability, intentParameters ActionIntentParameters) *model.CustomButtonCapability {
	c.SetState(model.CustomButtonCapabilityState{
		Instance: model.CustomButtonCapabilityInstance(intentParameters.CapabilityInstance),
		Value:    intentParameters.CapabilityValue.(bool),
	})
	return c
}

func fillModeCapability(c *model.ModeCapability, device model.Device, intentParameters ActionIntentParameters) *model.ModeCapability {
	if intentParameters.RelativityType == "" {
		c.SetState(model.ModeCapabilityState{
			Instance: model.ModeCapabilityInstance(intentParameters.CapabilityInstance),
			Value:    model.ModeValue(intentParameters.CapabilityValue.(string)),
		})
	} else {
		capability, _ := device.GetCapabilityByTypeAndInstance(model.ModeCapabilityType, intentParameters.CapabilityInstance)
		if parameters, ok := capability.Parameters().(model.ModeCapabilityParameters); ok {
			knownCurModes := make([]model.Mode, 0, len(parameters.Modes))
			for _, mode := range parameters.Modes {
				knownCurModes = append(knownCurModes, model.KnownModes[mode.Value])
			}
			sort.Sort(model.ModesSorting(knownCurModes))
			var curMode string
			// need to get current state, if state is nil, its first state of array
			if state, ok := capability.State().(model.ModeCapabilityState); ok {
				curMode = string(state.Value)
			} else {
				curMode = string(knownCurModes[0].Value)
			}
			var modeIndex = 0
			for ind, value := range knownCurModes {
				if string(value.Value) == curMode {
					modeIndex = ind
					break
				}
			}
			switch intentParameters.RelativityType {
			case Increase:
				if modeIndex+1 < len(knownCurModes) {
					modeIndex++
				} else {
					modeIndex = 0
				}
			case Decrease:
				if modeIndex-1 >= 0 {
					modeIndex--
				} else {
					modeIndex = len(knownCurModes) - 1
				}
			}
			c.SetState(model.ModeCapabilityState{
				Instance: model.ModeCapabilityInstance(intentParameters.CapabilityInstance),
				Value:    knownCurModes[modeIndex].Value,
			})
		}
	}

	return c
}

func fillOnOffCapability(c *model.OnOffCapability, device model.Device, intentParameters ActionIntentParameters) *model.OnOffCapability {
	if intentParameters.RelativityType != "" {
		capability, _ := device.GetCapabilityByTypeAndInstance(model.OnOffCapabilityType, intentParameters.CapabilityInstance)
		if capability.State() == nil {
			capability.SetState(c.DefaultState())
		}
		newValue := capability.State().(model.OnOffCapabilityState).Value
		switch intentParameters.RelativityType {
		case Invert:
			newValue = !capability.State().(model.OnOffCapabilityState).Value
		}
		c.SetState(model.OnOffCapabilityState{
			Instance: model.OnOffCapabilityInstance(intentParameters.CapabilityInstance),
			Value:    newValue,
		})
	} else {
		c.SetState(model.OnOffCapabilityState{
			Instance: model.OnOffCapabilityInstance(intentParameters.CapabilityInstance),
			Value:    intentParameters.CapabilityValue.(bool),
		})
	}

	return c
}

func fillQuasarCapability(c *model.QuasarCapability, intentParameters ActionIntentParameters) *model.QuasarCapability {
	c.SetState(model.QuasarCapabilityState{
		Instance: model.QuasarCapabilityInstance(intentParameters.CapabilityInstance),
		Value:    model.MakeQuasarCapabilityValueByInstance(model.QuasarCapabilityInstance(intentParameters.CapabilityInstance), intentParameters.CapabilityValue),
	})

	return c
}

func fillQuasarServerActionCapability(c *model.QuasarServerActionCapability, intentParameters ActionIntentParameters) *model.QuasarServerActionCapability {
	c.SetState(model.QuasarServerActionCapabilityState{
		Instance: model.QuasarServerActionCapabilityInstance(intentParameters.CapabilityInstance),
		Value:    intentParameters.CapabilityValue.(string),
	})

	return c
}

func fillRangeCapability(c *model.RangeCapability, device model.Device, intentParameters ActionIntentParameters) *model.RangeCapability {
	capability, _ := device.GetCapabilityByTypeAndInstance(model.RangeCapabilityType, intentParameters.CapabilityInstance)
	switch {
	case intentParameters.RelativityType == "":
		c = fillRangeFromAbsoluteValue(c, device, intentParameters.CapabilityInstance, intentParameters.CapabilityValue)
	case capability.Retrievable() && capability.Parameters().(model.RangeCapabilityParameters).Range != nil:
		c = fillRangeFromRetrievableRelativeValue(c, device, capability, intentParameters.CapabilityInstance,
			intentParameters.RelativityType, intentParameters.CapabilityValue)
	case !capability.Retrievable():
		c = fillRangeFromNonRetrievableRelativeValue(c, intentParameters.CapabilityInstance, intentParameters.RelativityType,
			intentParameters.CapabilityValue)
	default:
		panic(fmt.Sprintf("unexpected device capability: %v", capability))
	}

	return c
}

func fillRangeFromAbsoluteValue(c *model.RangeCapability, device model.Device, instance string, value interface{}) *model.RangeCapability {
	// if the value is 'max' or 'min', get absolute value from the device's range
	v, ok := value.(string)
	if ok {
		capability, _ := device.GetCapabilityByTypeAndInstance(model.RangeCapabilityType, instance)
		switch v {
		case model.Max:
			c.SetState(model.RangeCapabilityState{
				Instance: model.RangeCapabilityInstance(instance),
				Value:    capability.Parameters().(model.RangeCapabilityParameters).Range.Max,
			})
		case model.Min:
			c.SetState(model.RangeCapabilityState{
				Instance: model.RangeCapabilityInstance(instance),
				Value:    capability.Parameters().(model.RangeCapabilityParameters).Range.Min,
			})
		default:
			panic(fmt.Sprintf("unknown range capability string value: %q", v))
		}
	} else {
		// otherwise, set value to float64
		c.SetState(model.RangeCapabilityState{
			Instance: model.RangeCapabilityInstance(instance),
			Value:    value.(float64),
		})
	}

	return c
}

func fillRangeFromRetrievableRelativeValue(c *model.RangeCapability, device model.Device, capability model.ICapability, instance string,
	relativityType RelativityType, value interface{}) *model.RangeCapability {
	maxValue := capability.Parameters().(model.RangeCapabilityParameters).Range.Max
	minValue := capability.Parameters().(model.RangeCapabilityParameters).Range.Min
	precision := capability.Parameters().(model.RangeCapabilityParameters).Range.Precision

	// get currentValue from the state, if the state is nil, use the default one
	var currentValue float64
	if capability.State() == nil {
		currentValue = capability.DefaultState().(model.RangeCapabilityState).Value
	} else {
		currentValue = capability.State().(model.RangeCapabilityState).Value
	}

	// get delta, this is the number of steps divided by the BinNum, so full range can be achieved by BinNum number of deltas
	var delta float64
	if value == nil {
		if model.MultiDeltaRangeInstances.Contains(instance) {
			steps := (maxValue - minValue) / precision
			binNum := model.MultiDeltaRangeInstances.LookupBinNum(instance)
			delta = math.Round(steps/binNum) * precision
			if delta == 0 {
				delta = precision
			}
		} else {
			delta = precision
		}
	} else {
		delta = value.(float64)
	}
	if relativityType == Decrease {
		delta = -delta
	}

	newState := model.RangeCapabilityState{
		Instance: model.RangeCapabilityInstance(instance),
	}
	// IOT-303: old logic, relative flag is banned for retrievable:true devices
	if _, found := RelativeFlagNonSupportingSkills[device.SkillID]; found {
		newValue := currentValue + delta
		if newValue > maxValue {
			newValue = maxValue
		}
		if newValue < minValue {
			newValue = minValue
		}
		newState.Value = newValue
	} else {
		// IOT-303: new logic with relative flag in documentation
		newState.Value = delta
		newState.Relative = tools.AOB(true)
	}

	c.SetState(newState)
	return c
}

func fillRangeFromNonRetrievableRelativeValue(c *model.RangeCapability, instance string, relativityType RelativityType, value interface{}) *model.RangeCapability {
	var newValue float64
	switch {
	case value != nil:
		newValue = value.(float64)
	case model.RangeCapabilityInstance(instance) == model.VolumeRangeInstance:
		// https://st.yandex-team.ru/QUASAR-4167
		newValue = float64(3)
	default:
		newValue = float64(1)
	}

	if relativityType == Decrease {
		newValue = -newValue
	}

	c.SetState(model.RangeCapabilityState{
		Instance: model.RangeCapabilityInstance(instance),
		Relative: tools.AOB(true),
		Value:    newValue,
	})

	return c
}

func fillToggleCapability(c *model.ToggleCapability, device model.Device, intentParameters ActionIntentParameters) *model.ToggleCapability {
	if intentParameters.RelativityType != "" {
		capability, _ := device.GetCapabilityByTypeAndInstance(model.ToggleCapabilityType, intentParameters.CapabilityInstance)
		if capability.State() == nil {
			capability.SetState(capability.DefaultState())
		}
		newValue := capability.State().(model.ToggleCapabilityState).Value
		switch intentParameters.RelativityType {
		case Invert:
			newValue = !capability.State().(model.ToggleCapabilityState).Value
		}
		c.SetState(model.ToggleCapabilityState{
			Instance: model.ToggleCapabilityInstance(intentParameters.CapabilityInstance),
			Value:    newValue,
		})
	} else {
		c.SetState(model.ToggleCapabilityState{
			Instance: model.ToggleCapabilityInstance(intentParameters.CapabilityInstance),
			Value:    intentParameters.CapabilityValue.(bool),
		})
	}

	return c
}

func fillVideoStreamCapability(c *model.VideoStreamCapability, device model.Device, intentParameters ActionIntentParameters) *model.VideoStreamCapability {
	c.SetState(model.VideoStreamCapabilityState{
		Instance: model.GetStreamCapabilityInstance,
		Value:    intentParameters.CapabilityValue.(model.VideoStreamCapabilityValue),
	})

	return c
}
