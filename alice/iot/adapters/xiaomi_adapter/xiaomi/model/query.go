package model

import (
	"math"
	"strconv"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

func (d *Device) ToCapabilityStateViews() []adapter.CapabilityStateView {
	csv := make([]adapter.CapabilityStateView, 0)
	for _, s := range d.Services {
		switch {
		case s.IsTypeOf(lightService):
			csv = append(csv, d.GetLightCapabilitiesStates()...)
		case s.IsTypeOf(switchService):
			csv = append(csv, d.GetSwitchCapabilitiesStates()...)
		case s.IsTypeOf(acService):
			csv = append(csv, d.GetAcCapabilitiesStates()...)
		case s.IsTypeOf(vacuumService):
			csv = append(csv, d.GetVacuumCapabilitiesStates()...)
		case s.IsTypeOf(airPurifierService):
			csv = append(csv, d.GetPurifierCapabilitiesStates()...)
		case s.IsTypeOf(curtainService):
			csv = append(csv, d.GetCurtainCapabilitiesStates()...)
		case s.IsTypeOf(humidifierService):
			csv = append(csv, d.GetHumidifierCapabilitiesStates()...)
		case s.IsTypeOf(heaterService):
			csv = append(csv, d.GetHeaterCapabilitiesStates()...)
		//case s.IsTypeOf(fanService):
		//	csv = append(csv, d.GetFanCapabilitiesStates()...)
		default:
			continue
		}
	}
	return csv
}
func (d *Device) ToPropertyStateViews(fromCallback bool) []adapter.PropertyStateView {
	psv := make([]adapter.PropertyStateView, 0)
	for _, s := range d.Services {
		switch {
		case s.IsTypeOf(environmentService):
			psv = append(psv, d.GetEnvironmentPropertiesStates()...)
		case s.IsTypeOf(humidifierService):
			psv = append(psv, d.GetHumidifierPropertiesStates()...)
		case s.IsTypeOf(batteryService):
			psv = append(psv, d.GetBatteryPropertiesStates()...)
		case s.IsTypeOf(temperatureHumiditySensorService):
			psv = append(psv, d.GetTemperatureHumiditySensorPropertiesStates()...)
		case s.IsTypeOf(motionSensorService):
			psv = append(psv, d.GetMotionSensorPropertiesStates(fromCallback)...)
		case s.IsTypeOf(illuminationSensorService):
			psv = append(psv, d.GetIlluminationSensorPropertiesStates()...)
		case s.IsTypeOf(magnetSensorService):
			psv = append(psv, d.GetMagnetSensorPropertiesStates(fromCallback)...)
		case s.IsTypeOf(switchSensorService):
			psv = append(psv, d.GetSwitchSensorPropertiesStates()...)
		case s.IsTypeOf(submersionSensorService):
			psv = append(psv, d.GetSubmersionSensorPropertiesStates()...)
		case s.IsTypeOf(gasSensorService):
			psv = append(psv, d.GetGasSensorPropertiesStates()...)
		case s.IsTypeOf(smokeSensorService):
			psv = append(psv, d.GetSmokeSensorPropertiesStates()...)
		case s.IsTypeOf(vibrationSensorService):
			psv = append(psv, d.GetVibrationSensorPropertiesStates()...)
		default:
			continue
		}
	}
	return psv
}

func (d *Device) GetLightCapabilitiesStates() []adapter.CapabilityStateView {
	csv := make([]adapter.CapabilityStateView, 0)

	//on_off
	ps, ok := d.GetPropertyState(onProperty)
	if ok && ps.Property.Status == 0 {
		c := adapter.CapabilityStateView{
			Type: model.OnOffCapabilityType,
			State: model.OnOffCapabilityState{
				Instance: model.OnOnOffCapabilityInstance,
				Value:    boolPropertyValue(ps.Property.Value),
			}}
		csv = append(csv, c)
	}

	//brightness
	ps, ok = d.GetPropertyState(brightnessProperty)
	if ok && ps.Property.Status == 0 {
		c := adapter.CapabilityStateView{
			Type: model.RangeCapabilityType,
			State: model.RangeCapabilityState{
				Instance: model.BrightnessRangeInstance,
				Value:    ps.Property.Value.(float64),
			}}
		csv = append(csv, c)
	}

	//temperature_k or color_setting
	ct, ctOk := d.GetPropertyState(colorTemperatureProperty)
	clr, clrOk := d.GetPropertyState(colorProperty)
	md, mdOk := d.GetPropertyState(modeProperty)

	if ctOk && clrOk && mdOk {
		if ct.Property.Status == 0 && clr.Property.Status == 0 && md.Property.Status == 0 {
			switch md.Property.Value {
			case colorMode:
				c := adapter.CapabilityStateView{
					Type: model.ColorSettingCapabilityType,
					State: model.ColorSettingCapabilityState{
						Instance: model.RgbColorCapabilityInstance,
						Value:    model.RGB(clr.Property.Value.(float64)),
					}}
				csv = append(csv, c)
			case dayMode:
				c := adapter.CapabilityStateView{
					Type: model.ColorSettingCapabilityType,
					State: model.ColorSettingCapabilityState{
						Instance: model.TemperatureKCapabilityInstance,
						Value:    model.TemperatureK(ct.Property.Value.(float64)),
					}}
				csv = append(csv, c)
			}
		}
	} else if ctOk {
		ctProperty, hasServiceProperty := d.GetServiceProperty(lightService, colorTemperatureProperty)
		if ct.Property.Status == 0 && hasServiceProperty {
			switch ctProperty.Unit {
			case percentUnit:
				kelvin := getKelvinFromPercent(ct.Property.Value.(float64))
				c := adapter.CapabilityStateView{
					Type: model.ColorSettingCapabilityType,
					State: model.ColorSettingCapabilityState{
						Instance: model.TemperatureKCapabilityInstance,
						Value:    model.TemperatureK(constraintBetween(kelvin, 3000, 5700)), // philips zhirui
					}}
				csv = append(csv, c)
			case kelvinUnit:
				c := adapter.CapabilityStateView{
					Type: model.ColorSettingCapabilityType,
					State: model.ColorSettingCapabilityState{
						Instance: model.TemperatureKCapabilityInstance,
						Value:    model.TemperatureK(ct.Property.Value.(float64)),
					}}
				csv = append(csv, c)
			}
		}
	} else if clrOk {
		if clr.Property.Status == 0 {
			c := adapter.CapabilityStateView{
				Type: model.ColorSettingCapabilityType,
				State: model.ColorSettingCapabilityState{
					Instance: model.RgbColorCapabilityInstance,
					Value:    model.RGB(clr.Property.Value.(float64)),
				}}
			csv = append(csv, c)
		}
	}
	return csv
}

func (d *Device) GetSwitchCapabilitiesStates() []adapter.CapabilityStateView {
	csv := make([]adapter.CapabilityStateView, 0)
	for _, p := range stateProperties {
		ps, ok := d.GetPropertyState(p)
		if !ok || ps.Property.Status.IsError() {
			continue
		}

		switch p {
		case onProperty:
			c := adapter.CapabilityStateView{
				Type: model.OnOffCapabilityType,
				State: model.OnOffCapabilityState{
					Instance: model.OnOnOffCapabilityInstance,
					Value:    boolPropertyValue(ps.Property.Value),
				}}
			csv = append(csv, c)
		}
	}
	return csv
}

func (d *Device) GetAcCapabilitiesStates() []adapter.CapabilityStateView {
	csv := make([]adapter.CapabilityStateView, 0)
	for _, p := range stateProperties {
		ps, ok := d.GetPropertyState(p)
		if !ok || ps.Property.Status.IsError() {
			continue
		}

		switch p {
		case onProperty:
			c := adapter.CapabilityStateView{
				Type: model.OnOffCapabilityType,
				State: model.OnOffCapabilityState{
					Instance: model.OnOnOffCapabilityInstance,
					Value:    boolPropertyValue(ps.Property.Value),
				}}
			csv = append(csv, c)

		case modeProperty:
			v := d.GetPropertyStateValue(p)
			c := adapter.CapabilityStateView{
				Type: model.ModeCapabilityType,
				State: model.ModeCapabilityState{
					Instance: model.ThermostatModeInstance,
					Value:    yandexAcModesMap[v.(float64)],
				}}
			csv = append(csv, c)

		case targetTemperatureProperty:
			v := d.GetPropertyStateValue(p)
			c := adapter.CapabilityStateView{
				Type: model.RangeCapabilityType,
				State: model.RangeCapabilityState{
					Instance: model.TemperatureRangeInstance,
					Value:    v.(float64),
				}}
			csv = append(csv, c)
		}
	}
	return csv
}

func (d *Device) GetVacuumCapabilitiesStates() []adapter.CapabilityStateView {
	csv := make([]adapter.CapabilityStateView, 0)

	isM1S := d.IsTypeOf("urn:miot-spec-v2:device:vacuum:0000A006:roborock-m1s:2")
	isA08 := d.IsTypeOf("urn:miot-spec-v2:device:vacuum:0000A006:roborock-a08:1")
	isS5E := d.IsTypeOf("urn:miot-spec-v2:device:vacuum:0000A006:roborock-s5e:1")

	for _, p := range stateProperties {
		ps, ok := d.GetPropertyState(p)
		if !ok || ps.Property.Status.IsError() {
			continue
		}

		sweepStatus, pauseStatus := sweepingStatus, pausedStatus
		switch {
		case isM1S || isA08 || isS5E:
			sweepStatus, pauseStatus = float64(5), float64(10)
		case d.IsTypeOf("urn:miot-spec-v2:device:vacuum:0000A006:dreame-p2008:1"):
			sweepStatus, pauseStatus = float64(1), float64(3)
		}
		switch p {
		case statusProperty:
			if _, ok := d.GetServiceAction(vacuumService, startSweepAction); ok {
				c := adapter.CapabilityStateView{
					Type: model.OnOffCapabilityType,
				}
				switch ps.Property.Value.(float64) {
				case sweepStatus:
					c.State = model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    true,
					}
				default:
					c.State = model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    false,
					}
				}
				csv = append(csv, c)
			}
			_, stopSweepOk := d.GetServiceAction(vacuumService, stopSweepAction)
			_, stopSweepingOk := d.GetServiceAction(vacuumService, stopSweepingAction)
			if stopSweepOk || stopSweepingOk {
				c := adapter.CapabilityStateView{
					Type: model.ToggleCapabilityType,
				}
				switch ps.Property.Value.(float64) {
				case pauseStatus:
					c.State = model.ToggleCapabilityState{
						Instance: model.PauseToggleCapabilityInstance,
						Value:    true,
					}
				default:
					c.State = model.ToggleCapabilityState{
						Instance: model.PauseToggleCapabilityInstance,
						Value:    false,
					}
				}
				csv = append(csv, c)
			}

		case speedLevelProperty:
			speedLevel, ok := d.GetServiceProperty(vacuumService, speedLevelProperty)
			if !ok {
				continue
			}
			valueRange, err := speedLevel.GetValueRange()
			if err != nil {
				// valueRange is expected as [begin, end, precision]. we ignore other values for now
				continue
			}
			speedLevelValue := ps.Property.Value.(float64)
			if oneThird, twoThirds := valueRange.GetThirds(); speedLevelValue < oneThird {
				slowSpeedMode := adapter.CapabilityStateView{
					Type: model.ModeCapabilityType,
					State: model.ModeCapabilityState{
						Instance: model.WorkSpeedModeInstance,
						Value:    model.SlowMode,
					},
				}
				csv = append(csv, slowSpeedMode)
			} else if speedLevelValue < twoThirds {
				normalSpeedMode := adapter.CapabilityStateView{
					Type: model.ModeCapabilityType,
					State: model.ModeCapabilityState{
						Instance: model.WorkSpeedModeInstance,
						Value:    model.MediumMode,
					},
				}
				csv = append(csv, normalSpeedMode)
			} else {
				highSpeedMode := adapter.CapabilityStateView{
					Type: model.ModeCapabilityType,
					State: model.ModeCapabilityState{
						Instance: model.WorkSpeedModeInstance,
						Value:    model.FastMode,
					},
				}
				csv = append(csv, highSpeedMode)
			}
		case modeProperty:
			prop, ok := d.GetProperty(p)
			if !ok {
				continue
			}
			modesMap := getYandexVacuumCleanerModeList(prop)
			val := d.GetPropertyStateValue(p)
			modeVal, ok := parseWorkSpeedMode(val)
			if !ok {
				continue
			}
			c := adapter.CapabilityStateView{
				Type: model.ModeCapabilityType,
				State: model.ModeCapabilityState{
					Instance: model.WorkSpeedModeInstance,
					Value:    modesMap[modeVal],
				}}
			csv = append(csv, c)
		}
	}

	return csv
}

func parseWorkSpeedMode(val interface{}) (float64, bool) {
	result, ok := val.(float64)
	if ok {
		return result, true
	}

	if stringVal, ok := val.(string); ok {
		// bug in xiaomi api, value can be string int
		// device: urn:miot-spec-v2:device:vacuum:0000A006:viomi-v7:2
		intVal, err := strconv.Atoi(stringVal)
		if err != nil {
			return 0, false
		}
		return float64(intVal), true
	}
	return 0, false
}

func (d *Device) GetPurifierCapabilitiesStates() []adapter.CapabilityStateView {
	csv := make([]adapter.CapabilityStateView, 0)
	for _, p := range stateProperties {
		ps, ok := d.GetPropertyState(p)
		if !ok || ps.Property.Status.IsError() {
			continue
		}

		switch p {
		case onProperty:
			c := adapter.CapabilityStateView{
				Type: model.OnOffCapabilityType,
				State: model.OnOffCapabilityState{
					Instance: model.OnOnOffCapabilityInstance,
					Value:    boolPropertyValue(ps.Property.Value),
				}}
			csv = append(csv, c)
		}
	}
	return csv
}

func (d *Device) GetHumidifierCapabilitiesStates() []adapter.CapabilityStateView {
	csv := make([]adapter.CapabilityStateView, 0)
	for _, p := range stateProperties {
		ps, ok := d.GetPropertyState(p)
		if !ok || ps.Property.Status.IsError() {
			continue
		}

		switch p {
		case onProperty:
			c := adapter.CapabilityStateView{
				Type: model.OnOffCapabilityType,
				State: model.OnOffCapabilityState{
					Instance: model.OnOnOffCapabilityInstance,
					Value:    boolPropertyValue(ps.Property.Value),
				}}
			csv = append(csv, c)

		case fanLevelProperty:
			v := d.GetPropertyStateValue(p)
			c := adapter.CapabilityStateView{
				Type: model.ModeCapabilityType,
				State: model.ModeCapabilityState{
					Instance: model.FanSpeedModeInstance,
					Value:    yandexFanModesMap[v.(float64)],
				}}
			csv = append(csv, c)
		}
	}
	return csv
}

func (d *Device) GetCurtainCapabilitiesStates() []adapter.CapabilityStateView {
	csv := make([]adapter.CapabilityStateView, 0)
	for _, p := range stateProperties {
		ps, ok := d.GetPropertyState(p)
		if !ok || ps.Property.Status.IsError() {
			continue
		}
		switch p {
		case statusProperty:
			v := d.GetPropertyStateValue(p)
			c := adapter.CapabilityStateView{
				Type: model.OnOffCapabilityType,
				State: model.OnOffCapabilityState{
					Instance: model.OnOnOffCapabilityInstance,
					Value:    v.(float64) == openCurtainMode,
				},
			}
			csv = append(csv, c)
		case currentPositionProperty:
			v := d.GetPropertyStateValue(p)
			c := adapter.CapabilityStateView{
				Type: model.RangeCapabilityType,
				State: model.RangeCapabilityState{
					Instance: model.OpenRangeInstance,
					Value:    v.(float64),
				},
			}
			csv = append(csv, c)
		}
	}
	return csv
}

func (d *Device) GetHumidifierPropertiesStates() []adapter.PropertyStateView {
	psv := make([]adapter.PropertyStateView, 0)
	for _, p := range stateProperties {
		ps, ok := d.GetPropertyState(p)
		if !ok || ps.Property.Status.IsError() {
			continue
		}

		switch p {
		case waterLevelProperty:
			waterLevel, ok := d.GetServiceProperty(humidifierService, waterLevelProperty)
			if !ok {
				continue
			}
			valueRange, err := waterLevel.GetValueRange()
			if err != nil {
				// valueRange is expected as [begin, end, precision]. we ignore other values for now
				continue
			}
			waterLevelState := adapter.PropertyStateView{
				Type: model.FloatPropertyType,
				State: model.FloatPropertyState{
					Instance: model.WaterLevelPropertyInstance,
					Value:    math.Round((ps.Property.Value.(float64) - valueRange.Begin) / (valueRange.End - valueRange.Begin) * 100),
				},
			}
			psv = append(psv, waterLevelState)
		}
	}
	return psv
}

func (d *Device) GetBatteryPropertiesStates() []adapter.PropertyStateView {
	psv := make([]adapter.PropertyStateView, 0)
	for _, p := range stateProperties {
		ps, ok := d.GetPropertyState(p)
		if !ok || ps.Property.Status.IsError() {
			continue
		}

		prop, ok := d.GetProperty(p)
		if !ok {
			continue
		}

		switch p {
		case batteryLevelProperty:
			if prop.Unit == percentUnit {
				psv = append(psv, adapter.PropertyStateView{
					Type: model.FloatPropertyType,
					State: model.FloatPropertyState{
						Instance: model.BatteryLevelPropertyInstance,
						Value:    ps.Property.Value.(float64),
					},
				})
			} else {
				value, ok := xiaomiBatteryLevelEvents[ps.Property.Value.(float64)]
				if !ok {
					continue
				}
				psv = append(psv, adapter.PropertyStateView{
					Type: model.EventPropertyType,
					State: model.EventPropertyState{
						Instance: model.BatteryLevelPropertyInstance,
						Value:    value,
					},
				})
			}
		}
	}
	return psv
}

func (d *Device) GetEnvironmentPropertiesStates() []adapter.PropertyStateView {
	psv := make([]adapter.PropertyStateView, 0)
	for _, p := range stateProperties {
		ps, ok := d.GetPropertyState(p)
		if !ok || ps.Property.Status.IsError() {
			continue
		}

		switch p {
		case temperatureProperty:
			temperatureState := adapter.PropertyStateView{
				Type: model.FloatPropertyType,
				State: model.FloatPropertyState{
					Instance: model.TemperaturePropertyInstance,
					Value:    math.Round(ps.Property.Value.(float64)*10) / 10,
				},
			}
			psv = append(psv, temperatureState)
		case relativeHumidityProperty:
			humidityState := adapter.PropertyStateView{
				Type: model.FloatPropertyType,
				State: model.FloatPropertyState{
					Instance: model.HumidityPropertyInstance,
					Value:    math.Round(ps.Property.Value.(float64)),
				},
			}
			psv = append(psv, humidityState)
		case pm1DensityProperty:
			psv = append(psv, toPMDensityPropertyStateView(ps, model.PM1DensityPropertyInstance))
		case pm2p5DensityProperty:
			psv = append(psv, toPMDensityPropertyStateView(ps, model.PM2p5DensityPropertyInstance))
		case pm10DensityProperty:
			psv = append(psv, toPMDensityPropertyStateView(ps, model.PM10DensityPropertyInstance))
		}
	}
	return psv
}

func (d *Device) GetHeaterCapabilitiesStates() []adapter.CapabilityStateView {
	csv := make([]adapter.CapabilityStateView, 0)
	for _, p := range stateProperties {
		ps, ok := d.GetPropertyState(p)
		if !ok || ps.Property.Status.IsError() {
			continue
		}
		switch p {
		case onProperty:
			c := adapter.CapabilityStateView{
				Type: model.OnOffCapabilityType,
				State: model.OnOffCapabilityState{
					Instance: model.OnOnOffCapabilityInstance,
					Value:    boolPropertyValue(ps.Property.Value),
				}}
			csv = append(csv, c)

		case targetTemperatureProperty:
			v := d.GetPropertyStateValue(p)
			c := adapter.CapabilityStateView{
				Type: model.RangeCapabilityType,
				State: model.RangeCapabilityState{
					Instance: model.TemperatureRangeInstance,
					Value:    v.(float64),
				}}
			csv = append(csv, c)

		case modeProperty:
			v := d.GetPropertyStateValue(p).(float64)
			c := adapter.CapabilityStateView{
				Type: model.ModeCapabilityType,
				State: model.ModeCapabilityState{
					Instance: model.HeatModeInstance,
					Value:    yandexHeaterModesMap[v],
				}}
			csv = append(csv, c)
		}
	}
	return csv
}

func toPMDensityPropertyStateView(propertyState PropertyState, instance model.PropertyInstance) adapter.PropertyStateView {
	return adapter.PropertyStateView{
		Type: model.FloatPropertyType,
		State: model.FloatPropertyState{
			Instance: instance,
			Value:    propertyState.Property.Value.(float64),
		},
	}
}

func (d *Device) GetTemperatureHumiditySensorPropertiesStates() []adapter.PropertyStateView {
	psv := make([]adapter.PropertyStateView, 0)
	for _, p := range stateProperties {
		ps, ok := d.GetPropertyState(p)
		if !ok || ps.Property.Status.IsError() {
			continue
		}

		switch p {
		case temperatureProperty:
			temperatureState := adapter.PropertyStateView{
				Type: model.FloatPropertyType,
				State: model.FloatPropertyState{
					Instance: model.TemperaturePropertyInstance,
					Value:    math.Round(ps.Property.Value.(float64)*10) / 10,
				},
			}
			psv = append(psv, temperatureState)
		case relativeHumidityProperty:
			humidityState := adapter.PropertyStateView{
				Type: model.FloatPropertyType,
				State: model.FloatPropertyState{
					Instance: model.HumidityPropertyInstance,
					Value:    math.Round(ps.Property.Value.(float64)),
				},
			}
			psv = append(psv, humidityState)
		case atmosphericPressureProperty:
			pressureProperty, ok := d.GetServiceProperty(temperatureHumiditySensorService, atmosphericPressureProperty)
			if !ok {
				continue
			}
			var value float64
			switch pressureProperty.Unit {
			case pascalUnit:
				value = getMMHGFromPascal(ps.Property.Value.(float64))
			default:
				continue
			}
			pressureState := adapter.PropertyStateView{
				Type: model.FloatPropertyType,
				State: model.FloatPropertyState{
					Instance: model.PressurePropertyInstance,
					Value:    math.Round(value),
				},
			}
			psv = append(psv, pressureState)
		}
	}
	return psv
}

func (d *Device) GetMotionSensorPropertiesStates(fromCallback bool) []adapter.PropertyStateView {
	psv := make([]adapter.PropertyStateView, 0)
	for _, p := range stateProperties {
		ps, ok := d.GetPropertyState(p)
		if !ok || ps.Property.Status.IsError() {
			continue
		}

		switch p {
		case motionStateProperty:
			if !fromCallback {
				// motion property is always not retrievable, update it only from callbacks (https://st.yandex-team.ru/IOT-998)
				continue
			}

			value := model.NotDetectedEvent
			if ps.Property.Value.(bool) {
				value = model.DetectedEvent
			}

			motionState := adapter.PropertyStateView{
				Type: model.EventPropertyType,
				State: model.EventPropertyState{
					Instance: model.MotionPropertyInstance,
					Value:    value,
				},
			}
			psv = append(psv, motionState)

		case illuminationProperty:
			illuminationState := adapter.PropertyStateView{
				Type: model.FloatPropertyType,
				State: model.FloatPropertyState{
					Instance: model.IlluminationPropertyInstance,
					Value:    ps.Property.Value.(float64),
				},
			}
			psv = append(psv, illuminationState)
		}
	}
	return psv
}

func (d *Device) GetIlluminationSensorPropertiesStates() []adapter.PropertyStateView {
	psv := make([]adapter.PropertyStateView, 0)
	for _, p := range stateProperties {
		ps, ok := d.GetPropertyState(p)
		if !ok || ps.Property.Status.IsError() {
			continue
		}

		switch p {
		case illuminationProperty:
			illuminationState := adapter.PropertyStateView{
				Type: model.FloatPropertyType,
				State: model.FloatPropertyState{
					Instance: model.IlluminationPropertyInstance,
					Value:    ps.Property.Value.(float64),
				},
			}
			psv = append(psv, illuminationState)
		}
	}
	return psv
}

func (d *Device) GetMagnetSensorPropertiesStates(fromCallback bool) []adapter.PropertyStateView {
	psv := make([]adapter.PropertyStateView, 0)

	for _, p := range stateProperties {
		ps, ok := d.GetPropertyState(p)
		if !ok || ps.Property.Status.IsError() {
			continue
		}
		switch p {
		case contactStateProperty:
			value := model.OpenedEvent
			if ps.Property.Value.(bool) {
				value = model.ClosedEvent
			}

			contactState := adapter.PropertyStateView{
				Type: model.EventPropertyType,
				State: model.EventPropertyState{
					Instance: model.OpenPropertyInstance,
					Value:    value,
				},
			}
			psv = append(psv, contactState)
		}
	}

	for _, event := range contactEvents {
		e, ok := d.GetEventOccurred(event)
		if !ok || e.Event.Status.IsError() {
			continue
		}

		var value model.EventValue
		switch event {
		case openEvent:
			value = model.OpenedEvent
		case closeEvent:
			value = model.ClosedEvent
		default:
			continue
		}

		contactState := adapter.PropertyStateView{
			Type: model.EventPropertyType,
			State: model.EventPropertyState{
				Instance: model.OpenPropertyInstance,
				Value:    value,
			},
		}
		psv = append(psv, contactState)
	}

	return psv
}

func (d *Device) GetSwitchSensorPropertiesStates() []adapter.PropertyStateView {
	psv := make([]adapter.PropertyStateView, 0)
	for _, event := range buttonEvents {
		e, ok := d.GetEventOccurred(event)
		if !ok || e.Event.Status.IsError() {
			continue
		}

		var value model.EventValue
		switch event {
		case clickEvent:
			value = model.ClickEvent
		case doubleClickEvent:
			value = model.DoubleClickEvent
		case longPressEvent:
			value = model.LongPressEvent
		default:
			continue
		}

		switchState := adapter.PropertyStateView{
			Type: model.EventPropertyType,
			State: model.EventPropertyState{
				Instance: model.ButtonPropertyInstance,
				Value:    value,
			},
		}
		psv = append(psv, switchState)
	}
	return psv
}

func (d *Device) GetSubmersionSensorPropertiesStates() []adapter.PropertyStateView {
	var value model.EventValue
	for _, event := range waterLeakEvents {
		e, ok := d.GetEventOccurred(event)
		if !ok || e.Event.Status.IsError() {
			continue
		}
		switch event {
		case submersionDetectedEvent:
			value = model.LeakEvent
		case noSubmersionDetectedEvent:
			value = model.DryEvent
		}
	}

	// no event happened, check available properties for submersion state
	if value == "" {
		if propertyState, ok := d.GetPropertyState(submersionStateProperty); ok {
			if propertyVal, ok := propertyState.Property.Value.(bool); ok {
				if propertyVal {
					value = model.LeakEvent
				} else {
					value = model.DryEvent
				}
			}
		}
	}

	if value == "" {
		return nil
	}

	return []adapter.PropertyStateView{
		{
			Type: model.EventPropertyType,
			State: model.EventPropertyState{
				Instance: model.WaterLeakPropertyInstance,
				Value:    value,
			},
		},
	}
}

func (d *Device) GetVibrationSensorPropertiesStates() []adapter.PropertyStateView {
	psv := make([]adapter.PropertyStateView, 0)
	for _, event := range vibrationEvents {
		e, ok := d.GetEventOccurred(event)
		if !ok || e.Event.Status.IsError() {
			continue
		}

		var value model.EventValue
		switch event {
		case tiltEvent:
			value = model.TiltEvent
		case fallEvent:
			value = model.FallEvent
		case vibrationEvent:
			value = model.VibrationEvent
		default:
			continue
		}

		sensorState := adapter.PropertyStateView{
			Type: model.EventPropertyType,
			State: model.EventPropertyState{
				Instance: model.VibrationPropertyInstance,
				Value:    value,
			},
		}
		psv = append(psv, sensorState)
	}
	return psv
}

func (d *Device) GetGasSensorPropertiesStates() []adapter.PropertyStateView {
	psv := make([]adapter.PropertyStateView, 0)
	for _, p := range stateProperties {
		ps, ok := d.GetPropertyState(p)
		if !ok || ps.Property.Status.IsError() {
			continue
		}

		switch p {
		case gasConcentrationProperty:
			gasConcentrationState := adapter.PropertyStateView{
				Type: model.FloatPropertyType,
				State: model.FloatPropertyState{
					Instance: model.GasConcentrationPropertyInstance,
					Value:    math.Round(ps.Property.Value.(float64)*10) / 10,
				},
			}
			psv = append(psv, gasConcentrationState)
		}
	}
	return psv
}

func (d *Device) GetSmokeSensorPropertiesStates() []adapter.PropertyStateView {
	psv := make([]adapter.PropertyStateView, 0)
	for _, p := range stateProperties {
		ps, ok := d.GetPropertyState(p)
		if !ok || ps.Property.Status.IsError() {
			continue
		}

		switch p {
		case smokeConcentrationProperty:
			smokeConcentrationState := adapter.PropertyStateView{
				Type: model.FloatPropertyType,
				State: model.FloatPropertyState{
					Instance: model.SmokeConcentrationPropertyInstance,
					Value:    math.Round(ps.Property.Value.(float64)*10) / 10,
				},
			}
			psv = append(psv, smokeConcentrationState)
		}
	}
	return psv
}
