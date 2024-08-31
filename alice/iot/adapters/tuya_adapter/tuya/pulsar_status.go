package tuya

import (
	"encoding/json"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type pulsarStatus struct {
	Code      string
	Value     interface{}
	Timestamp uint64
}

func (ps *pulsarStatus) UnmarshalJSON(data []byte) error {
	status := struct {
		Code      string          `json:"code"`
		Timestamp uint64          `json:"t"`
		Value     json.RawMessage `json:"value"`
	}{}
	if err := json.Unmarshal(data, &status); err != nil {
		return err
	}

	switch status.Code {
	case string(WorkModeCommand):
		var value TuyaWorkMode
		if err := json.Unmarshal(status.Value, &value); err != nil {
			return err
		}
		ps.Value = value

	case string(ColorDataCommand):
		var rawValue string
		if err := json.Unmarshal(status.Value, &rawValue); err != nil {
			return err
		}

		var hsv struct {
			H, S, V float64
		}
		if err := json.Unmarshal([]byte(rawValue), &hsv); err != nil {
			return err
		}

		ps.Value = model.HSV{H: int(hsv.H), S: int(hsv.S), V: int(hsv.V)}

	case string(BrightnessCommand), string(TempValueCommand), string(AmperageCommand), string(VoltageCommand), string(PowerCommand):
		var value float64
		if err := json.Unmarshal(status.Value, &value); err != nil {
			return err
		}
		ps.Value = int(value)

	case string(SwitchCommand), string(Switch1Command), string(LedSwitchCommand), string(SwitchLedCommand):
		var value bool
		if err := json.Unmarshal(status.Value, &value); err != nil {
			return err
		}
		ps.Value = value
	case string(SceneData2Command):
		var rawValue string
		if err := json.Unmarshal(status.Value, &rawValue); err != nil {
			return err
		}
		var value SceneDataState
		if err := json.Unmarshal([]byte(rawValue), &value); err != nil {
			return err
		}
		ps.Value = value
	default:
		ps.Value = status.Value
	}

	ps.Code = status.Code
	ps.Timestamp = status.Timestamp

	return nil
}

type PulsarStatuses []pulsarStatus

func (s PulsarStatuses) getStatusMap() map[TuyaCommandName]pulsarStatus {
	statusMap := make(map[TuyaCommandName]pulsarStatus, len(s))
	for _, status := range s {
		if fromMap, ok := statusMap[TuyaCommandName(status.Code)]; ok && fromMap.Timestamp > status.Timestamp {
			continue
		}
		statusMap[TuyaCommandName(status.Code)] = status
	}
	return statusMap
}

func (s PulsarStatuses) ToCapabilityStateView(getDevice func() (UserDevice, error)) ([]adapter.CapabilityStateView, error) {
	result := make([]adapter.CapabilityStateView, 0, len(s))
	statusMap := s.getStatusMap()

	// WorkModeCommand
	if status, ok := statusMap[WorkModeCommand]; ok {
		// Switch to color, temp_k or to scene.
		// This is just a change notification - no new values are sent in message with this command.
		// So we need to get old status and send brightness and color_setting

		// Color is stored in H and S components of HSV, which is kept in ColorDataCommand
		// Brightness for color is stored in V component of HSV color, which is kept in ColorDataCommand

		// Temperature is stored in TempValueCommand
		// Brightness for temperature is stored in BrightnessCommand

		// Scene is stored in SceneData2Command

		// These values conflict with each other, so in this block we ensure
		// that statusMap contains ColorDataCommand or a pair of {TempValueCommand, BrightnessCommand} or SceneData2Command
		switch status.Value.(TuyaWorkMode) {
		case ColorWorkMode:
			// we switched to color mode
			if _, ok := statusMap[ColorDataCommand]; !ok {
				// if color data is not present - we need to add it to statusMap to later make color and brightness
				device, err := getDevice()
				if err != nil {
					return nil, err
				}
				if oldStatus, ok := device.GetColorDataItem(); ok {
					statusMap[ColorDataCommand] = pulsarStatus{
						Code:  oldStatus.Code,
						Value: oldStatus.Value,
					}
				}
			}
			// we need to also dump brightness, temperature and scene_data_v2 - they conflict with color
			delete(statusMap, BrightnessCommand)
			delete(statusMap, TempValueCommand)
			delete(statusMap, SceneData2Command)
		case WhiteWorkMode:
			// we switched to white mode
			if _, ok := statusMap[TempValueCommand]; !ok {
				// if temperature is not present - we need to add it to statusMap to later make temperature
				device, err := getDevice()
				if err != nil {
					return nil, err
				}
				if oldStatus, ok := device.GetTempValueItem(); ok {
					statusMap[TempValueCommand] = pulsarStatus{
						Code:  oldStatus.Code,
						Value: oldStatus.Value,
					}
				}
			}
			if _, ok := statusMap[BrightnessCommand]; !ok {
				// if brightness is not present - we need to add it to statusMap to later make brightness
				device, err := getDevice()
				if err != nil {
					return nil, err
				}
				if oldStatus, ok := device.GetBrightValueItem(); ok {
					statusMap[BrightnessCommand] = pulsarStatus{
						Code:  oldStatus.Code,
						Value: oldStatus.Value,
					}
				}
			}
			// we also need to dump color_data and scene_data_v2 command, as it conflicts with white mode
			delete(statusMap, ColorDataCommand)
			delete(statusMap, SceneData2Command)
		case SceneWorkMode:
			if _, ok := statusMap[SceneData2Command]; !ok {
				// if scene_data_v2 is not present - we need to add it to statusMap to later make scene number
				device, err := getDevice()
				if err != nil {
					return nil, err
				}
				if oldStatus, ok := device.GetSceneDataItem(); ok {
					statusMap[SceneData2Command] = pulsarStatus{
						Code:  oldStatus.Code,
						Value: oldStatus.Value,
					}
				}
			}
			// we also need to dump color_data, brightness and temperature command, as it conflicts with scene mode
			delete(statusMap, ColorDataCommand)
			delete(statusMap, BrightnessCommand)
			delete(statusMap, TempValueCommand)
		}
	}

	// ColorDataCommand
	if status, ok := statusMap[ColorDataCommand]; ok {
		// in this block we form actual color data for pulsar

		// adjust hsv values first
		device, err := getDevice()
		if err != nil {
			return nil, err
		}
		hsvData := tuyaHsvToHsvState(status.Value.(model.HSV), device.ProductID)

		// then send brightness from V
		result = append(result, adapter.CapabilityStateView{
			Type: model.RangeCapabilityType,
			State: model.RangeCapabilityState{
				Instance: model.BrightnessRangeInstance,
				Value:    float64(hsvData.V),
			},
		})

		// and send adjusted hsv color
		result = append(result, adapter.CapabilityStateView{
			Type: model.ColorSettingCapabilityType,
			State: model.ColorSettingCapabilityState{
				Instance: model.HsvColorCapabilityInstance,
				Value:    hsvData,
			},
		})

		// we need to also dump brightness, temperature and scene_data - they conflict with color
		delete(statusMap, BrightnessCommand)
		delete(statusMap, TempValueCommand)
		delete(statusMap, SceneData2Command)
	}

	// BrightnessCommand
	if status, ok := statusMap[BrightnessCommand]; ok {
		// in this block we form brightness data for white mode

		// adjust brightness first: decrease max brightness (really 255) to 80% (204) to prevent bad lamp damage
		device, err := getDevice()
		if err != nil {
			return nil, err
		}
		lampSpec := LampSpecByPID(device.ProductID)
		brightValue := float64(percentFromRange(status.Value.(int), lampSpec.BrightValueLocalSpec.Min, lampSpec.BrightValueLocalSpec.Max))
		if brightValue == 0 {
			brightValue = 1
		}
		// send brightness for temperature_k
		result = append(result, adapter.CapabilityStateView{
			Type: model.RangeCapabilityType,
			State: model.RangeCapabilityState{
				Instance: model.BrightnessRangeInstance,
				Value:    brightValue,
			},
		})

		// we also need to dump color_data and scene_data command, as they conflicts with white mode
		delete(statusMap, ColorDataCommand)
		delete(statusMap, SceneData2Command)
	}

	// TempValueCommand
	if status, ok := statusMap[TempValueCommand]; ok {
		// in this block we form temperature_k data for white mode

		// adjust temperature - either get rounded values or concrete known tempK if we are in exact tempValue
		device, err := getDevice()
		if err != nil {
			return nil, err
		}
		lampSpec := LampSpecByPID(device.ProductID)
		value := tempValueToTemperatureK(status.Value.(int), device.ProductID)
		if temperatureK, ok := lampSpec.tempValueToTemperatureKMap[status.Value.(int)]; ok {
			value = int(temperatureK)
		}

		// send temperature_k
		result = append(result, adapter.CapabilityStateView{
			Type: model.ColorSettingCapabilityType,
			State: model.ColorSettingCapabilityState{
				Instance: model.TemperatureKCapabilityInstance,
				Value:    model.TemperatureK(value),
			},
		})

		// we also need to dump color_data and scene_data command, as they conflict with white mode
		delete(statusMap, ColorDataCommand)
		delete(statusMap, SceneData2Command)
	}

	if status, ok := statusMap[SceneData2Command]; ok {
		if sceneValue, ok := status.Value.(SceneDataState); ok {
			if colorSceneID, ok := sceneValue.ColorSceneID(); ok {
				result = append(result, adapter.CapabilityStateView{
					Type: model.ColorSettingCapabilityType,
					State: model.ColorSettingCapabilityState{
						Instance: model.SceneCapabilityInstance,
						Value:    colorSceneID,
					},
				})
				// we also need to dump color or white command, as it conflicts with scene mode
				delete(statusMap, BrightnessCommand)
				delete(statusMap, TempValueCommand)
				delete(statusMap, ColorDataCommand)
			} else {
				return nil, xerrors.New("failed to get color scene id from scene data state: unknown scene num")
			}
		} else {
			return nil, xerrors.New("failed to cast status value to scene data state: invalid cast")
		}
	}

	// OnOffCapability
	for _, code := range []SwitchCommandName{LedSwitchCommand, SwitchLedCommand, SwitchCommand, Switch1Command} {
		if status, ok := statusMap[TuyaCommandName(code)]; ok {
			result = append(result, adapter.CapabilityStateView{
				Type: model.OnOffCapabilityType,
				State: model.OnOffCapabilityState{
					Instance: model.OnOnOffCapabilityInstance,
					Value:    status.Value.(bool),
				},
			})
			break
		}
	}

	return result, nil
}

func (s PulsarStatuses) ToPropertyStateView() []adapter.PropertyStateView {
	properties := make([]adapter.PropertyStateView, 0, len(s))
	statusMap := s.getStatusMap()

	// VoltageCommand
	if status, ok := statusMap[VoltageCommand]; ok {
		propertyVoltage := adapter.PropertyStateView{
			Type: model.FloatPropertyType,
			State: model.FloatPropertyState{
				Instance: model.VoltagePropertyInstance,
				Value:    deciVoltToVolt(float64(status.Value.(int))), // voltage in dV, 2200 dV = 220 V
			},
		}
		properties = append(properties, propertyVoltage)
	}

	// PowerCommand
	if status, ok := statusMap[PowerCommand]; ok {
		propertyPower := adapter.PropertyStateView{
			Type: model.FloatPropertyType,
			State: model.FloatPropertyState{
				Instance: model.PowerPropertyInstance,
				Value:    deciWattToWatt(float64(status.Value.(int))), // active power in dW, 66 dW = 6.6 W
			},
		}
		properties = append(properties, propertyPower)
	}

	// AmperageCommand
	if status, ok := statusMap[AmperageCommand]; ok {
		propertyAmperage := adapter.PropertyStateView{
			Type: model.FloatPropertyType,
			State: model.FloatPropertyState{
				Instance: model.AmperagePropertyInstance,
				Value:    milliAmpereToAmpere(float64(status.Value.(int))), // amperage in mA, 35 mA = 0.035 A
			},
		}
		properties = append(properties, propertyAmperage)
	}

	return properties
}
