package philips

import (
	"fmt"
	"math"
	"strings"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/tools"
)

const ColorModeColorTemperature = "ct"
const ColorModeHsv = "hs"
const ColorModeXY = "xy"

type LightCapabilities struct {
	Certified bool `json:"certified"`
	Control   *struct {
		MinDimLevel    *int32      `json:"mindimlevel"`
		MaxLumen       *int32      `json:"maxlumen"`
		ColorgamutType string      `json:"colorgamuttype"`
		Colorgamut     [][]float32 `json:"colorgamut"`
		Ct             *struct {
			Min uint16 `json:"min"`
			Max uint16 `json:"max"`
		} `json:"ct"`
	} `json:"control"`
	Streaming struct {
		Renderer bool `json:"render"`
		Proxy    bool `json:"proxy"`
	} `json:"streaming"`
}

type LightState struct {
	On        *bool     `json:"on,omitempty"`
	Bri       *uint8    `json:"bri,omitempty"`
	Hue       *uint16   `json:"hue,omitempty"`
	Sat       *uint8    `json:"sat,omitempty"`
	Effect    *string   `json:"effect,omitempty"`
	Xy        []float32 `json:"xy,omitempty"`
	Ct        *uint16   `json:"ct,omitempty"`
	Alert     *string   `json:"alert,omitempty"`
	Colormode *string   `json:"colormode,omitempty"`
	Mode      *string   `json:"mode,omitempty"`
	Reachable *bool     `json:"reachable,omitempty"`
}

type LightInfo struct {
	State    LightState `json:"state"`
	Swupdate struct {
		State       string `json:"state"`
		LastInstall string `json:"lastinstall"`
	} `json:"swupdate"`
	Type             string             `json:"type"`
	Name             string             `json:"name"`
	ModelID          string             `json:"modelid"`
	ManufacturerName string             `json:"manufacturername"`
	ProductName      string             `json:"productname"`
	Capabilities     *LightCapabilities `json:"capabilities"`
	Config           struct {
		Archetype string `json:"archetype"`
		Function  string `json:"function"`
		Direction string `json:"direction"`
	} `json:"config"`
	UniqueID  string `json:"uniqueid"`
	Swversion string `json:"swversion"`
}

func (light LightInfo) ProcessActionRequests(actions []adapter.CapabilityActionView) (processed []adapter.CapabilityActionResultView, pending []adapter.CapabilityActionView, newState LightStateChangeRequest) {
	skip := func(action adapter.CapabilityActionView, code adapter.ErrorCode, message string) {
		processed = append(processed, adapter.CapabilityActionResultView{
			Type: action.Type,
			State: adapter.CapabilityStateActionResultView{
				Instance: action.State.GetInstance(),
				ActionResult: adapter.StateActionResult{
					Status:       adapter.ERROR,
					ErrorCode:    code,
					ErrorMessage: message,
				},
			},
		})
	}

	for _, action := range actions {
		switch action.Type {
		case model.OnOffCapabilityType:
			if controllable, _ := light.onSupport(); controllable {
				state := action.State.(model.OnOffCapabilityState)

				newState.On = &state.Value
				pending = append(pending, action)
			} else {
				skip(action, adapter.InvalidAction, fmt.Sprintf(ErrorMessageInstanceNotSupported, action.State.GetInstance(), action.Type))
			}

		case model.RangeCapabilityType:
			if controllable, _ := light.briSupport(); (action.State.GetInstance() == string(model.BrightnessRangeInstance)) && controllable {
				state := action.State.(model.RangeCapabilityState)

				if state.Relative != nil && *state.Relative {
					value := state.Value
					if value >= -100 && value <= 100 {
						BriInc := BrightnessPercentToUint8(math.Abs(value))
						if math.Signbit(value) {
							BriInc = -BriInc
						}

						newState.BriInc = tools.AOI16(int16(BriInc))
						pending = append(pending, action)
					} else {
						skip(action, adapter.InvalidValue, fmt.Sprintf(ErrorMessageValueOutOfRange, value, -100., 100.))
					}
				} else {
					value := state.Value
					if value >= 1 && value <= 100 {
						newState.Bri = tools.AOUI8(BrightnessPercentToUint8(value))
						pending = append(pending, action)
					} else {
						skip(action, adapter.InvalidValue, fmt.Sprintf(ErrorMessageValueOutOfRange, value, 1., 100.))
					}
				}
			} else {
				skip(action, adapter.InvalidAction, fmt.Sprintf(ErrorMessageInstanceNotSupported, action.State.GetInstance(), action.Type))
			}

		case model.ColorSettingCapabilityType:
			switch action.State.GetInstance() {
			case string(model.TemperatureKCapabilityInstance):
				if controllable, _ := light.ctSupport(); controllable {
					state := action.State.(model.ColorSettingCapabilityState)
					value := state.Value.(model.TemperatureK)

					max := model.TemperatureK(toKelvin(light.Capabilities.Control.Ct.Min))
					min := model.TemperatureK(toKelvin(light.Capabilities.Control.Ct.Max))
					if value >= min && value <= max {
						newState.Ct = tools.AOUI16(toMerid(int(value)))
						pending = append(pending, action)
					} else {
						skip(action, adapter.InvalidValue, fmt.Sprintf(ErrorMessageValueOutOfRange, float64(value), float64(min), float64(max)))
					}
				} else {
					skip(action, adapter.InvalidAction, fmt.Sprintf(ErrorMessageInstanceNotSupported, action.State.GetInstance(), action.Type))
				}
			case string(model.HsvColorCapabilityInstance):
				if controllable, _ := light.hsSupport(); controllable {
					state := action.State.(model.ColorSettingCapabilityState)
					value := state.Value.(model.HSV)

					newState.Hue = tools.AOUI16(uint16(float32(value.H) / 360 * 65535))
					// change brightness only on direct request
					//newState.Bri = tools.AOUI8(uint8(float32(value.V-1)/99*253 + 1))
					newState.Sat = tools.AOUI8(uint8(float32(value.S) / 100 * 254))
					pending = append(pending, action)
				} else {
					skip(action, adapter.InvalidAction, fmt.Sprintf(ErrorMessageInstanceNotSupported, action.State.GetInstance(), action.Type))
				}
			default:
				skip(action, adapter.InvalidAction, fmt.Sprintf(ErrorMessageInstanceNotSupported, action.State.GetInstance(), action.Type))
			}
		default:
			skip(action, adapter.InvalidAction, fmt.Sprintf(ErrorMessageCapabilityNotSupported, action.Type))
		}
	}

	// Turn on device if OnOffCapability was not present
	if newState.On == nil && len(pending) > 0 {
		newState.On = tools.AOB(true)
	}

	// Drop actions that conflict with "turn off the light"
	if newState.On != nil && !*newState.On {
		var filteredActions []adapter.CapabilityActionView

		for _, action := range pending {
			switch action.Type {
			case model.RangeCapabilityType:
				switch action.State.GetInstance() {
				case string(model.BrightnessRangeInstance):
					skip(action, adapter.NotSupportedInCurrentMode, ErrorMessageNotSupportedOnTurnOff)
					continue
				}
			case model.ColorSettingCapabilityType:
				switch action.State.GetInstance() {
				case string(model.TemperatureKCapabilityInstance), string(model.HsvColorCapabilityInstance):
					skip(action, adapter.NotSupportedInCurrentMode, ErrorMessageNotSupportedOnTurnOff)
					continue
				}
			}

			filteredActions = append(filteredActions, action)
		}

		pending = filteredActions
	}

	return processed, pending, newState
}

func (light LightInfo) ToCapabilityInfoViews() []adapter.CapabilityInfoView {
	capabilities := make([]adapter.CapabilityInfoView, 0, 3)

	// https://developers.meethue.com/develop/hue-api/supported-devices/
	// 1.1 On/Off light
	controllable, retrievable := light.onSupport()
	if controllable {
		capabilities = append(capabilities, adapter.CapabilityInfoView{
			Type:        model.OnOffCapabilityType,
			Retrievable: retrievable,
		})
	}

	// 1.2 Dimmable light
	controllable, retrievable = light.briSupport()
	if controllable {
		capabilities = append(capabilities, adapter.CapabilityInfoView{
			Type:        model.RangeCapabilityType,
			Retrievable: retrievable,
			Parameters: model.RangeCapabilityParameters{
				Instance:     model.BrightnessRangeInstance,
				Unit:         model.UnitPercent,
				RandomAccess: true,
				Range: &model.Range{
					Min:       1,
					Max:       100,
					Precision: 1,
				},
			},
		})
	}

	colorCapabilityParameters := model.ColorSettingCapabilityParameters{}
	colorCapabilityRetrievable := false

	// 1.3 Color temperature light
	controllable, retrievable = light.ctSupport()
	if controllable {
		colorCapabilityParameters.TemperatureK = &model.TemperatureKParameters{
			Max: model.TemperatureK(toKelvin(light.Capabilities.Control.Ct.Min)),
			Min: model.TemperatureK(toKelvin(light.Capabilities.Control.Ct.Max)),
		}

		colorCapabilityRetrievable = colorCapabilityRetrievable || retrievable
	}

	// 1.4 Color light
	controllable, retrievable = light.hsSupport()
	if controllable {
		colorCapabilityParameters.ColorModel = AOColorModelType(model.HsvModelType)

		colorCapabilityRetrievable = colorCapabilityRetrievable || retrievable
	}

	// 1.5 Extended color light = (1.3 Color temperature light) + (1.4 Color light)
	// pass

	if colorCapabilityParameters.TemperatureK != nil || colorCapabilityParameters.ColorModel != nil {
		capabilities = append(capabilities, adapter.CapabilityInfoView{
			Type:        model.ColorSettingCapabilityType,
			Retrievable: colorCapabilityRetrievable,
			Parameters:  colorCapabilityParameters,
		})
	}

	return capabilities
}

func (light *LightInfo) ToCapabilityStateViews() []adapter.CapabilityStateView {
	state := light.State
	capabilityStates := make([]adapter.CapabilityStateView, 0, 3)

	// https://developers.meethue.com/develop/hue-api/supported-devices/
	// 1.1 On/Off light
	controllable, retrievable := light.onSupport()
	if controllable && retrievable {
		capabilityStates = append(capabilityStates, adapter.CapabilityStateView{
			Type: model.OnOffCapabilityType,
			State: model.OnOffCapabilityState{
				Instance: model.OnOnOffCapabilityInstance,
				Value:    *state.On,
			},
		})
	}

	// 1.2 Dimmable light
	controllable, retrievable = light.briSupport()
	if controllable && retrievable {
		capabilityStates = append(capabilityStates, adapter.CapabilityStateView{
			Type: model.RangeCapabilityType,
			State: model.RangeCapabilityState{
				Instance: model.BrightnessRangeInstance,
				Value:    BrightnessUint8ToPercent(*state.Bri),
			},
		})
	}

	if state.Colormode != nil {
		switch *state.Colormode {
		case ColorModeColorTemperature:
			// 1.3 Color temperature light
			controllable, retrievable = light.ctSupport()
			if controllable && retrievable {
				capabilityStates = append(capabilityStates, adapter.CapabilityStateView{
					Type: model.ColorSettingCapabilityType,
					State: model.ColorSettingCapabilityState{
						Instance: model.TemperatureKCapabilityInstance,
						Value:    model.TemperatureK(toKelvin(*light.State.Ct)),
					},
				})
			}
		case ColorModeHsv:
			// 1.4 Color light
			controllable, retrievable = light.hsSupport()
			if controllable && retrievable {
				var bri uint8 = 100
				if light.State.Bri != nil {
					bri = *light.State.Bri
				}

				capabilityStates = append(capabilityStates, adapter.CapabilityStateView{
					Type: model.ColorSettingCapabilityType,
					State: model.ColorSettingCapabilityState{
						Instance: model.HsvColorCapabilityInstance,
						Value: model.HSV{
							H: int(float32(*light.State.Hue) / 65535 * 360),
							S: int(float32(*light.State.Sat) / 254 * 100),
							V: int(BrightnessUint8ToPercent(bri)),
						},
					},
				})
			}
		case ColorModeXY:
			// TODO: https://developers.meethue.com/develop/application-design-guidance/color-conversion-formulas-rgb-to-xy-and-back/#xy-to-rgb-color
		default:
			// 1.5 Extended color light = (1.3 Color temperature light) + (1.4 Color light)
			// pass
		}
	}

	return capabilityStates
}

func BrightnessPercentToUint8(m float64) uint8 {
	return uint8(math.Round(m / 100 * 255))
}

func BrightnessUint8ToPercent(m uint8) float64 {
	result := math.Round(float64(m) / 255 * 100)
	if result == 0 {
		result = 1
	}
	return result
}

func (light *LightInfo) ToDeviceStateView() adapter.DeviceStateView {
	return adapter.DeviceStateView{
		ID:           light.UniqueID,
		Capabilities: light.ToCapabilityStateViews(),
	}
}

func (light *LightInfo) onSupport() (controllable, retrievable bool) {
	controllable = true
	retrievable = light.State.On != nil
	return controllable, retrievable
}

func (light *LightInfo) briSupport() (controllable, retrievable bool) {
	retrievable = light.State.Bri != nil
	// TODO: light.State.Bri != nil is hack. Check at Philips
	controllable = (light.Capabilities != nil && light.Capabilities.Control != nil && light.Capabilities.Control.MinDimLevel != nil) || light.State.Bri != nil
	return controllable, retrievable
}

func (light *LightInfo) ctSupport() (controllable, retrievable bool) {
	controllable = light.Capabilities != nil && light.Capabilities.Control != nil && light.Capabilities.Control.Ct != nil
	retrievable = light.State.Ct != nil
	return controllable, retrievable
}

func (light *LightInfo) hsSupport() (controllable, retrievable bool) {
	controllable = light.Capabilities != nil && light.Capabilities.Control != nil && light.Capabilities.Control.ColorgamutType != ""
	retrievable = light.State.Hue != nil && light.State.Sat != nil
	return controllable, retrievable
}

type LightStateChangeRequest struct {
	On             *bool     `json:"on,omitempty"`
	Bri            *uint8    `json:"bri,omitempty"`
	Hue            *uint16   `json:"hue,omitempty"`
	Sat            *uint8    `json:"sat,omitempty"`
	Xy             []float32 `json:"xy,omitempty"`
	Ct             *uint16   `json:"ct,omitempty"`
	Alert          *string   `json:"alert,omitempty"`
	Effect         *string   `json:"effect,omitempty"`
	TransitionTime *uint16   `json:"transitiontime,omitempty"`
	BriInc         *int16    `json:"bri_inc,omitempty"`
	SatInc         *int16    `json:"sat_inc,omitempty"`
	HueInc         *int32    `json:"hue_inc,omitempty"`
	CtInc          *int32    `json:"ct_inc,omitempty"`
	XyInc          []float32 `json:"xy_inc,omitempty"`
}

type LightStateChangeResult []SingleStateChangeResult

/**
Sample response of Hue API:
[
    {
        "error": {
            "type": 6,
            "address": "/lights/1/state/brit",
            "description": "parameter, brit, not available"
        }
    },
    {
        "success": {
            "/lights/1/state/on": true
        }
    },
    {
        "success": {
            "/lights/1/state/hue": 128
        }
    }
]
*/
func (lscr LightStateChangeResult) ToCapabilityActionResultViews(actions []adapter.CapabilityActionView) []adapter.CapabilityActionResultView {
	lscrMap := make(map[string]adapter.StateActionResult)

	for _, line := range lscr {
		for address := range line.Success {
			parts := strings.Split(address, "/")
			key := parts[len(parts)-1]

			lscrMap[key] = adapter.StateActionResult{Status: adapter.DONE}
		}
		if line.Error.Address != "" {
			parts := strings.Split(line.Error.Address, "/")
			key := parts[len(parts)-1]

			sar := adapter.StateActionResult{
				Status:       adapter.ERROR,
				ErrorCode:    adapter.InternalError,
				ErrorMessage: line.Error.Description,
			}

			switch line.Error.Type {
			case 6, 8:
				sar.ErrorCode = adapter.InvalidAction
			case 7:
				sar.ErrorCode = adapter.InvalidValue
			case 201:
				sar.ErrorCode = adapter.NotSupportedInCurrentMode
			}

			lscrMap[key] = sar
		}
	}

	result := make([]adapter.CapabilityActionResultView, 0, len(actions))
	for _, action := range actions {
		sar := adapter.StateActionResult{
			Status:       adapter.ERROR,
			ErrorCode:    adapter.InternalError,
			ErrorMessage: ErrorMessageNoResultForArgument,
		}

		switch action.Type {
		case model.OnOffCapabilityType:
			if changeResult, ok := lscrMap["on"]; ok {
				sar = changeResult
			}

		case model.RangeCapabilityType:
			if action.State.GetInstance() == string(model.BrightnessRangeInstance) {
				state := action.State.(model.RangeCapabilityState)
				if state.Relative != nil && *state.Relative {
					if changeResult, ok := lscrMap["bri_inc"]; ok {
						sar = changeResult
					}
				} else {
					if changeResult, ok := lscrMap["bri"]; ok {
						sar = changeResult
					}
				}
			}

		case model.ColorSettingCapabilityType:
			switch action.State.GetInstance() {
			case string(model.TemperatureKCapabilityInstance):
				if changeResult, ok := lscrMap["ct"]; ok {
					sar = changeResult
				}

			case string(model.HsvModelType):
				hueResult, ok := lscrMap["hue"]
				if !ok {
					hueResult = sar
				}

				satResult, ok := lscrMap["sat"]
				if !ok {
					hueResult = sar
				}

				if satResult.Status == adapter.ERROR {
					sar = satResult
				} else {
					sar = hueResult
				}
			}
		}

		result = append(result, adapter.CapabilityActionResultView{
			Type: action.Type,
			State: adapter.CapabilityStateActionResultView{
				Instance:     action.State.GetInstance(),
				ActionResult: sar,
			},
		})
	}

	return result
}

type SingleStateChangeResult struct {
	Success map[string]interface{} `json:"success"`
	Error   struct {
		Type        int    `json:"type"`
		Address     string `json:"address"`
		Description string `json:"description"`
	} `json:"error"`
}

type CustomData struct {
	Username string `json:"username"`
}

func AOColorModelType(v model.ColorModelType) *model.ColorModelType {
	return &v
}

func toKelvin(m uint16) int {
	if m == 0 {
		m = 1
	}
	return int(float64(1000000) / float64(m))
}

const (
	ErrorMessageBridgeOffline          = "Bridge is offline"
	ErrorMessageDeviceUnreachable      = "Device unreachable"
	ErrorMessageDeviceNotFound         = "Device not found"
	ErrorMessageCapabilityNotSupported = "Capability %q is not supported"
	ErrorMessageInstanceNotSupported   = "Instance %q is not supported for capability %q"
	ErrorMessageValueOutOfRange        = "Value %f is out of range [%f; %f]"
	ErrorMessageNotSupportedOnTurnOff  = "Action is not supported when device is turning off"
	ErrorMessageNoResultForArgument    = "No response for %q argument"
)
