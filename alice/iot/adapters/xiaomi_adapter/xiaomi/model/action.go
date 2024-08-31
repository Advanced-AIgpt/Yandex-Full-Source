package model

import (
	"math"

	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/iotapi"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

func (d *Device) GetPropertyStates(actionRequestView adapter.DeviceActionRequestView) []PropertyState {
	propertyStates := make([]PropertyState, 0)
	for _, capability := range actionRequestView.Capabilities {
		switch capability.Type {
		case model.OnOffCapabilityType:
			propertyState := PropertyState{
				Type:     model.OnOffCapabilityType,
				Instance: string(model.OnOnOffCapabilityInstance),
				Property: iotapi.Property{
					IsSplit: d.IsSplit,
				},
			}
			switch {
			case d.IsTypeOf(curtainType):
				if propID, ok := d.GetPropertyID(motorControlProperty); ok {
					propertyState.Property.Pid = propID
					if capability.State.(model.OnOffCapabilityState).Value {
						propertyState.Property.Value = openCurtainMode
						propertyStates = append(propertyStates, propertyState)
					} else { //if we want to switch off curtain, set correct mode and stop calculating other properties
						propertyState.Property.Value = closeCurtainMode
						propertyStates = []PropertyState{propertyState}
						return propertyStates
					}
				}
			default:
				if propID, ok := d.GetPropertyID(onProperty); ok {
					propertyState.Property.Pid = propID
					propertyState.Property.Value = capability.State.(model.OnOffCapabilityState).Value
					propertyStates = append(propertyStates, propertyState)

					//if we want to switch off device, there is no reason to calculate other properties
					if !capability.State.(model.OnOffCapabilityState).Value {
						propertyStates = []PropertyState{propertyState}
						return propertyStates
					}
				}
			}
		case model.ColorSettingCapabilityType:
			propertyState := PropertyState{
				Type:     model.ColorSettingCapabilityType,
				Instance: string(capability.State.(model.ColorSettingCapabilityState).Instance),
				Property: iotapi.Property{
					Value:   capability.State.(model.ColorSettingCapabilityState).Value,
					IsSplit: d.IsSplit,
				},
			}
			switch instance := propertyState.Instance; instance {
			case string(model.TemperatureKCapabilityInstance):
				colorTemperature, hasServiceProperty := d.GetServiceProperty(lightService, colorTemperatureProperty)
				if propID, ok := d.GetPropertyID(colorTemperatureProperty); ok && hasServiceProperty {
					if colorTemperature.Unit == percentUnit { // philips zhirui
						kelvinValue := float64(propertyState.Property.Value.(model.TemperatureK))
						percentValue := getPercentFromKelvin(kelvinValue)
						propertyState.Property.Value = int(constraintBetween(percentValue, 1, 100))
					}
					propertyState.Property.Pid = propID
					propertyStates = append(propertyStates, propertyState)
				}
			case string(model.RgbColorCapabilityInstance):
				if propID, ok := d.GetPropertyID(colorProperty); ok {
					propertyState.Property.Pid = propID
					propertyStates = append(propertyStates, propertyState)
				}
			}
		case model.RangeCapabilityType:
			propertyState := PropertyState{
				Type:     model.RangeCapabilityType,
				Instance: string(capability.State.(model.RangeCapabilityState).Instance),
				Property: iotapi.Property{
					Value:   capability.State.(model.RangeCapabilityState).Value,
					IsSplit: d.IsSplit,
				},
			}
			switch instance := propertyState.Instance; model.RangeCapabilityInstance(instance) {
			case model.BrightnessRangeInstance:
				if propID, ok := d.GetPropertyID(brightnessProperty); ok {
					propertyState.Property.Pid = propID
					propertyStates = append(propertyStates, propertyState)
				}
			case model.TemperatureRangeInstance:
				propertyState.Instance = string(model.TemperatureRangeInstance)
				if propID, ok := d.GetPropertyID(targetTemperatureProperty); ok {
					propertyState.Property.Pid = propID
					propertyStates = append(propertyStates, propertyState)
				}
			case model.OpenRangeInstance:
				propertyState.Instance = string(model.OpenRangeInstance)
				if propID, ok := d.GetPropertyID(targetPositionProperty); ok {
					propertyState.Property.Pid = propID
					propertyStates = append(propertyStates, propertyState)
				}
			}
		case model.ModeCapabilityType:
			propertyState := PropertyState{
				Type:     model.ModeCapabilityType,
				Instance: string(capability.State.(model.ModeCapabilityState).Instance),
				Property: iotapi.Property{
					IsSplit: d.IsSplit,
				},
			}
			switch instance := propertyState.Instance; model.ModeCapabilityInstance(instance) {
			case model.ThermostatModeInstance:
				if propID, ok := d.GetPropertyID(modeProperty); ok {
					propertyState.Property.Pid = propID
					propertyState.Property.Value = xiaomiAcModesMap[capability.State.(model.ModeCapabilityState).Value]
					propertyStates = append(propertyStates, propertyState)
				} else if propID, ok := d.GetPropertyID(irModeProperty); ok && d.IsTypeOf(acType) {
					propertyState.Property.Pid = propID
					propertyState.Property.Value = xiaomiAcModesMap[capability.State.(model.ModeCapabilityState).Value]
					propertyStates = append(propertyStates, propertyState)
				}
			case model.FanSpeedModeInstance:
				if propID, ok := d.GetPropertyID(fanLevelProperty); ok {
					propertyState.Property.Pid = propID
					propertyState.Property.Value = xiaomiFanModesMap[capability.State.(model.ModeCapabilityState).Value]
					propertyStates = append(propertyStates, propertyState)
				}
			case model.HeatModeInstance:
				if propID, ok := d.GetPropertyID(modeProperty); ok {
					propertyState.Property.Pid = propID
					propertyState.Property.Value = xiaomiHeaterModesMap[capability.State.(model.ModeCapabilityState).Value]
					propertyStates = append(propertyStates, propertyState)
				}
			case model.WorkSpeedModeInstance:
				if propID, ok := d.GetPropertyID(speedLevelProperty); ok {
					speedLevel, ok := d.GetServiceProperty(vacuumService, speedLevelProperty)
					if !ok {
						continue
					}
					begin, end, precision := speedLevel.ValueRange[0], speedLevel.ValueRange[1], speedLevel.ValueRange[2]
					var resultValue float64
					switch capability.State.(model.ModeCapabilityState).Value {
					case model.SlowMode:
						resultValue = begin
					case model.MediumMode:
						resultValue = begin + math.Floor((end-begin)/2/precision)*precision
					case model.FastMode:
						resultValue = end
					}

					propertyState.Property.Pid = propID
					propertyState.Property.Value = resultValue
					propertyStates = append(propertyStates, propertyState)
				}

				if prop, ok := d.GetProperty(modeProperty); ok {
					modesMap := getXiaomiVacuumCleanerModeList(prop)
					propertyState.Property.Pid, _ = d.GetPropertyID(modeProperty)
					propertyState.Property.Value = modesMap[capability.State.(model.ModeCapabilityState).Value]
					propertyStates = append(propertyStates, propertyState)
				}
			}
		}
	}

	return propertyStates
}

func (d *Device) GetActionStates(actionRequestView adapter.DeviceActionRequestView) []ActionState {
	actions := make([]ActionState, 0)

	switch {
	case d.IsTypeOf(vacuumType):
		for _, c := range actionRequestView.Capabilities {
			actionState := ActionState{
				Type:     c.Type,
				Instance: c.State.GetInstance(),
				Action: iotapi.Action{
					In: make([]interface{}, 0),
				},
			}
			switch c.Type {
			case model.OnOffCapabilityType:
				if c.State.(model.OnOffCapabilityState).Value {
					if actionID, ok := d.GetActionID(startSweepAction); ok {
						actionState.Action.Aid = actionID
						actions = append(actions, actionState)
					}
				} else {
					if actionID, ok := d.GetActionID(startChargeAction); ok {
						actionState.Action.Aid = actionID
						actions = append(actions, actionState)
					}
				}
			case model.ToggleCapabilityType:
				switch c.State.GetInstance() {
				case string(model.PauseToggleCapabilityInstance):
					if c.State.(model.ToggleCapabilityState).Value {
						if actionID, ok := d.GetActionID(stopSweepAction); ok {
							actionState.Action.Aid = actionID
							actions = append(actions, actionState)
						} else if actionID, ok := d.GetActionID(stopSweepingAction); ok {
							actionState.Action.Aid = actionID
							actions = append(actions, actionState)
						}
					} else {
						if actionID, ok := d.GetActionID(startSweepAction); ok {
							actionState.Action.Aid = actionID
							actions = append(actions, actionState)
						}
					}
				}
			}
		}
	//case d.IsTypeOf(tvType) || d.IsTypeOf(tvBoxType):
	//	for _, c := range actionRequestView.Capabilities {
	//		actionState := ActionState{
	//			Type:     c.Type,
	//			Instance: c.State.GetInstance(),
	//			Action:   iotapi.Action{},
	//		}
	//		switch c.Type {
	//		case model.OnOffCapabilityType:
	//			if c.State.(model.OnOffCapabilityState).Value {
	//				if actionID, ok := d.GetActionID(turnOnAction); ok {
	//					actionState.Action.Aid = actionID
	//					actionState.Action.In = make([]interface{}, 0)
	//					actions = append(actions, actionState)
	//				}
	//			} else {
	//				if actionID, ok := d.GetActionID(turnOffAction); ok {
	//					actionState.Action.Aid = actionID
	//					actionState.Action.In = make([]interface{}, 0)
	//					actions = append(actions, actionState)
	//				}
	//			}
	//		case model.RangeCapabilityType:
	//			rangeState := c.State.(model.RangeCapabilityState)
	//			isRelative := rangeState.Relative != nil && *rangeState.Relative
	//			switch c.State.GetInstance() {
	//			case string(model.VolumeRangeInstance):
	//				if isRelative {
	//					if rangeState.Value > 0 {
	//						if actionID, ok := d.GetActionID(volumeUpAction); ok {
	//							actionState.Action.Aid = actionID
	//							actionState.Action.In = make([]interface{}, 0)
	//							actions = append(actions, actionState)
	//						}
	//					} else {
	//						if actionID, ok := d.GetActionID(volumeDownAction); ok {
	//							actionState.Action.Aid = actionID
	//							actionState.Action.In = make([]interface{}, 0)
	//							actions = append(actions, actionState)
	//						}
	//					}
	//				}
	//			case string(model.ChannelRangeInstance):
	//				if isRelative {
	//					if rangeState.Value > 0 {
	//						if actionID, ok := d.GetActionID(channelUpAction); ok {
	//							actionState.Action.Aid = actionID
	//							actionState.Action.In = make([]interface{}, 0)
	//							actions = append(actions, actionState)
	//						}
	//					} else {
	//						if actionID, ok := d.GetActionID(channelDownAction); ok {
	//							actionState.Action.Aid = actionID
	//							actionState.Action.In = make([]interface{}, 0)
	//							actions = append(actions, actionState)
	//						}
	//					}
	//				}
	//			}
	//		case model.ToggleCapabilityType:
	//			switch c.State.GetInstance() {
	//			case string(model.MuteToggleCapabilityInstance):
	//				if c.State.(model.ToggleCapabilityState).Value {
	//					if actionID, ok := d.GetActionID(muteOnAction); ok {
	//						actionState.Action.Aid = actionID
	//						actionState.Action.In = make([]interface{}, 0)
	//						actions = append(actions, actionState)
	//					}
	//				} else {
	//					if actionID, ok := d.GetActionID(muteOffAction); ok {
	//						actionState.Action.Aid = actionID
	//						actionState.Action.In = make([]interface{}, 0)
	//						actions = append(actions, actionState)
	//					}
	//				}
	//			}
	//		}
	//	}
	case d.IsTypeOf(acType):
		for _, c := range actionRequestView.Capabilities {
			actionState := ActionState{
				Type:     c.Type,
				Instance: c.State.GetInstance(),
				Action: iotapi.Action{
					In: make([]interface{}, 0),
				},
			}
			switch c.Type {
			case model.OnOffCapabilityType:
				if c.State.(model.OnOffCapabilityState).Value {
					if actionID, ok := d.GetActionID(turnOnAction); ok {
						actionState.Action.Aid = actionID
						actions = append(actions, actionState)
					}
				} else {
					if actionID, ok := d.GetActionID(turnOffAction); ok {
						actionState.Action.Aid = actionID
						actions = append(actions, actionState)
					}
				}
			case model.RangeCapabilityType:
				rangeState := c.State.(model.RangeCapabilityState)
				isRelative := rangeState.Relative != nil && *rangeState.Relative
				switch c.State.GetInstance() {
				case string(model.TemperatureRangeInstance):
					if isRelative {
						if rangeState.Value > 0 {
							if actionID, ok := d.GetActionID(temperatureUpAction); ok {
								actionState.Action.Aid = actionID
								actions = append(actions, actionState)
							}
						} else {
							if actionID, ok := d.GetActionID(temperatureDownAction); ok {
								actionState.Action.Aid = actionID
								actions = append(actions, actionState)
							}
						}
					}
				}
			}
		}
	}
	return actions
}

type ActionState struct {
	Action   iotapi.Action
	Type     model.CapabilityType
	Instance string
}

func (as ActionState) ToCapabilityActionResultView() adapter.CapabilityActionResultView {
	result := adapter.CapabilityActionResultView{
		Type: as.Type,
		State: adapter.CapabilityStateActionResultView{
			Instance:     as.Instance,
			ActionResult: as.getStateActionResult(),
		},
	}
	return result
}

func (as ActionState) getStateActionResult() adapter.StateActionResult {
	return getStateActionResult(as.Action.Status)
}

type PropertyState struct {
	Property iotapi.Property
	Type     model.CapabilityType
	Instance string
}

func (ps PropertyState) ToCapabilityActionResultView() adapter.CapabilityActionResultView {
	result := adapter.CapabilityActionResultView{
		Type: ps.Type,
		State: adapter.CapabilityStateActionResultView{
			Instance:     ps.Instance,
			ActionResult: ps.getStateActionResult(),
		},
	}
	return result
}

func (ps PropertyState) getStateActionResult() adapter.StateActionResult {
	return getStateActionResult(ps.Property.Status)
}

type EventOccurred struct {
	Event iotapi.Event
}

func getStateActionResult(status iotapi.Status) adapter.StateActionResult {
	if !status.IsError() {
		return adapter.StateActionResult{Status: adapter.DONE}
	}

	err := status.XiaomiError()
	var errorCode adapter.ErrorCode
	switch err.ErrorCode() {
	case iotapi.DeviceOfflineErrorCode, iotapi.OperationTimeoutErrorCode:
		errorCode = adapter.DeviceUnreachable
	case iotapi.DeviceDoesNotExistErrorCode:
		errorCode = adapter.DeviceNotFound
	case iotapi.NotSupportedInCurrentStateErrorCode:
		errorCode = adapter.NotSupportedInCurrentMode
	case iotapi.TokenIllegalErrorCode, iotapi.TokenExpiredErrorCode, iotapi.AuthorizationExpiredErrorCode, iotapi.DeviceNotBoundErrorCode:
		errorCode = adapter.AccountLinkingError
	default:
		errorCode = adapter.InternalError
	}
	if err.HTTPCode == 401 {
		errorCode = adapter.AccountLinkingError
	}
	return adapter.StateActionResult{
		Status:       adapter.ERROR,
		ErrorCode:    errorCode,
		ErrorMessage: err.Error(),
	}
}
