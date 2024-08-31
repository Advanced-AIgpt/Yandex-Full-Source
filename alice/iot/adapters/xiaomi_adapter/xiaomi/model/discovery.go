package model

import (
	"fmt"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

// forms fully qualified ids for all propertyIDs as "deviceID.serviceID.propertyID"
func (d *Device) getFQPropertyIDs(serviceID int, propertyIDs []int) []string {
	fqPropertyIDs := make([]string, 0, len(propertyIDs))
	for _, propertyID := range propertyIDs {
		if d.IsSplit {
			fqPropertyIDs = append(fqPropertyIDs, fmt.Sprintf("%s.%d", d.DID, propertyID))
		} else {
			fqPropertyIDs = append(fqPropertyIDs, fmt.Sprintf("%s.%d.%d", d.DID, serviceID, propertyID))
		}
	}
	return fqPropertyIDs
}

func (d *Device) getFQEventIDs(serviceID int, eventIDs []int) []string {
	fqEventIDs := make([]string, 0, len(eventIDs))
	for _, eventID := range eventIDs {
		if d.IsSplit {
			fqEventIDs = append(fqEventIDs, fmt.Sprintf("%s.%d", d.DID, eventID))
		} else {
			fqEventIDs = append(fqEventIDs, fmt.Sprintf("%s.%d.%d", d.DID, serviceID, eventID))
		}
	}
	return fqEventIDs
}

func (d *Device) ToDeviceInfoViewWithSubscriptions() (propertyIDs []string, eventIDs []string, div adapter.DeviceInfoView, err error) {
	div.ID = d.DID
	div.Name = d.Name
	div.Room = d.Room.Name
	div.Type = KnownDeviceCategories.GetDeviceType(d.Category)
	div.DeviceInfo = d.GetDeviceInfo()
	div.CustomData = d.GetCustomData()

	cs := make([]adapter.CapabilityInfoView, 0)
	ps := make([]adapter.PropertyInfoView, 0)
	propertyIDs = make([]string, 0)
	eventIDs = make([]string, 0)

	for _, s := range d.Services {
		switch {
		case s.IsTypeOf(lightService):
			lightCapabilities, lightCapabilitiesPropertyIDs := s.ToLightCapabilities()
			cs = append(cs, lightCapabilities...)
			propertyIDs = append(propertyIDs, d.getFQPropertyIDs(s.Iid, lightCapabilitiesPropertyIDs)...)
		case s.IsTypeOf(switchService):
			switchCapabilities, switchCapabilitiesPropertyIDs := s.ToSwitchCapabilities()
			cs = append(cs, switchCapabilities...)
			propertyIDs = append(propertyIDs, d.getFQPropertyIDs(s.Iid, switchCapabilitiesPropertyIDs)...)
		case s.IsTypeOf(acService):
			acCapabilities, acCapabilitiesPropertyIDs := s.ToACCapabilities()
			cs = append(cs, acCapabilities...)
			propertyIDs = append(propertyIDs, d.getFQPropertyIDs(s.Iid, acCapabilitiesPropertyIDs)...)
		case s.IsTypeOf(fanService):
			fanCapabilities, fanCapabilitiesPropertyIDs := s.ToFanCapabilities()
			cs = append(cs, fanCapabilities...)
			propertyIDs = append(propertyIDs, d.getFQPropertyIDs(s.Iid, fanCapabilitiesPropertyIDs)...)
		case s.IsTypeOf(airPurifierService):
			airPurifierCapabilities, airPurifierCapabilitiesPropertyIDs := s.ToAirPurifierCapabilities()
			cs = append(cs, airPurifierCapabilities...)
			propertyIDs = append(propertyIDs, d.getFQPropertyIDs(s.Iid, airPurifierCapabilitiesPropertyIDs)...)
		case s.IsTypeOf(curtainService):
			curtainCapabilities, curtainCapabilitiesPropertyIDs := s.ToCurtainCapabilities()
			cs = append(cs, curtainCapabilities...)
			propertyIDs = append(propertyIDs, d.getFQPropertyIDs(s.Iid, curtainCapabilitiesPropertyIDs)...)
		case s.IsTypeOf(humidifierService):
			humidifierCapabilities, humidifierCapabilitiesPropertyIDs := s.ToHumidifierCapabilities()
			cs = append(cs, humidifierCapabilities...)
			propertyIDs = append(propertyIDs, d.getFQPropertyIDs(s.Iid, humidifierCapabilitiesPropertyIDs)...)

			humidifierProperties, humidifierPropertiesIDs := s.ToHumidifierProperties()
			ps = append(ps, humidifierProperties...)
			propertyIDs = append(propertyIDs, d.getFQPropertyIDs(s.Iid, humidifierPropertiesIDs)...)
		case s.IsTypeOf(environmentService):
			environmentProperties, environmentPropertiesIDs := s.ToEnvironmentProperties()
			ps = append(ps, environmentProperties...)
			propertyIDs = append(propertyIDs, d.getFQPropertyIDs(s.Iid, environmentPropertiesIDs)...)
		case s.IsTypeOf(batteryService):
			batteryProperties, batteryPropertiesIDs := s.ToBatteryProperties()
			ps = append(ps, batteryProperties...)
			propertyIDs = append(propertyIDs, d.getFQPropertyIDs(s.Iid, batteryPropertiesIDs)...)
		case s.IsTypeOf(vacuumService):
			vacuumCapabilities, vacuumCapabilitiesPropertyIDs := s.ToVacuumCapabilities()
			cs = append(cs, vacuumCapabilities...)
			propertyIDs = append(propertyIDs, d.getFQPropertyIDs(s.Iid, vacuumCapabilitiesPropertyIDs)...)
		case s.IsTypeOf(temperatureHumiditySensorService):
			sensorProperties, sensorPropertiesPropertyIDs := s.ToTemperatureHumiditySensorProperties()
			ps = append(ps, sensorProperties...)
			propertyIDs = append(propertyIDs, d.getFQPropertyIDs(s.Iid, sensorPropertiesPropertyIDs)...)
		case s.IsTypeOf(motionSensorService):
			motionStateProperties, motionStatePropertiesPropertyIDs := s.ToMotionSensorProperties()
			ps = append(ps, motionStateProperties...)
			propertyIDs = append(propertyIDs, d.getFQPropertyIDs(s.Iid, motionStatePropertiesPropertyIDs)...)
		case s.IsTypeOf(illuminationSensorService):
			illuminationProperties, illuminationPropertiesPropertyIDs := s.ToIlluminationSensorProperties()
			ps = append(ps, illuminationProperties...)
			propertyIDs = append(propertyIDs, d.getFQPropertyIDs(s.Iid, illuminationPropertiesPropertyIDs)...)
		case s.IsTypeOf(magnetSensorService):
			property, subscriptionPropertyIDs, subscriptionEventIDs := s.ToMagnetSensorProperty()
			ps = append(ps, property)
			propertyIDs = append(propertyIDs, d.getFQPropertyIDs(s.Iid, subscriptionPropertyIDs)...)
			eventIDs = append(eventIDs, d.getFQEventIDs(s.Iid, subscriptionEventIDs)...)
		case s.IsTypeOf(switchSensorService):
			switchSensorProperty, switchSensorPropertiesEventIDs := s.ToSwitchSensorProperty()
			ps = append(ps, switchSensorProperty)
			eventIDs = append(eventIDs, d.getFQEventIDs(s.Iid, switchSensorPropertiesEventIDs)...)
		case s.IsTypeOf(submersionSensorService):
			submersionSensorProperty, submersionSensorPropertiesEventIDs := s.ToSubmersionSensorProperty()
			ps = append(ps, submersionSensorProperty)
			eventIDs = append(eventIDs, d.getFQEventIDs(s.Iid, submersionSensorPropertiesEventIDs)...)
		case s.IsTypeOf(gasSensorService):
			gasSensorProperties, gasSensorPropertiesIDs := s.ToGasSensorProperties()
			ps = append(ps, gasSensorProperties...)
			propertyIDs = append(propertyIDs, d.getFQPropertyIDs(s.Iid, gasSensorPropertiesIDs)...)
		case s.IsTypeOf(smokeSensorService):
			gasSensorProperties, gasSensorPropertiesIDs := s.ToSmokeSensorProperties()
			ps = append(ps, gasSensorProperties...)
			propertyIDs = append(propertyIDs, d.getFQPropertyIDs(s.Iid, gasSensorPropertiesIDs)...)
		case s.IsTypeOf(smokeSensorService):
			gasSensorProperties, gasSensorPropertiesIDs := s.ToSmokeSensorProperties()
			ps = append(ps, gasSensorProperties...)
			propertyIDs = append(propertyIDs, d.getFQPropertyIDs(s.Iid, gasSensorPropertiesIDs)...)
		case s.IsTypeOf(vibrationSensorService):
			vibrationSensorProperty, vibrationSensorPropertiesEventIDs := s.ToVibrationSensorProperty()
			ps = append(ps, vibrationSensorProperty)
			eventIDs = append(eventIDs, d.getFQEventIDs(s.Iid, vibrationSensorPropertiesEventIDs)...)
		case s.IsTypeOf(heaterService):
			heaterCapabilities, heaterCapabilitiesEventIDs := s.ToHeaterCapabilities()
			cs = append(cs, heaterCapabilities...)
			eventIDs = append(eventIDs, d.getFQEventIDs(s.Iid, heaterCapabilitiesEventIDs)...)
		//case s.IsTypeOf(irTvService) || s.IsTypeOf(irTvBoxService):
		//	cs = append(cs, s.ToIRTVCapabilities()...)
		//case s.IsTypeOf(irAcService):
		//	cs = append(cs, s.ToIRACCapabilities()...)
		default:
			continue
		}
	}

	div.Capabilities = cs
	div.Properties = ps
	return propertyIDs, eventIDs, div, nil
}

func (s *Service) ToLightCapabilities() ([]adapter.CapabilityInfoView, []int) {
	cs := make([]adapter.CapabilityInfoView, 0)
	propertyIDs := make([]int, 0)
	for _, p := range s.Properties {
		switch {
		case p.IsTypeOf(onProperty):
			cs = append(cs, p.ToOnOffCapability())
			if p.HasAccess(NotifyAccess) {
				propertyIDs = append(propertyIDs, p.Iid)
			}
		case p.IsTypeOf(brightnessProperty):
			cs = append(cs, p.ToBrightnessCapability())
			if p.HasAccess(NotifyAccess) {
				propertyIDs = append(propertyIDs, p.Iid)
			}
		default:
			continue
		}
	}
	if c, colorPropertyIDs, ok := s.ToColorSettingCapability(); ok {
		cs = append(cs, c)
		propertyIDs = append(propertyIDs, colorPropertyIDs...)
	}
	return cs, propertyIDs
}

func (s *Service) ToColorSettingCapability() (adapter.CapabilityInfoView, []int, bool) {
	var found bool
	c := adapter.CapabilityInfoView{Type: model.ColorSettingCapabilityType}
	params := model.ColorSettingCapabilityParameters{}
	propertyIDs := make([]int, 0)

	for _, p := range s.Properties {
		if p.IsTypeOf(colorProperty) {
			if p.Unit == "hsv" || p.Unit == "rgb" {
				found = true
				cmt := model.ColorModelType(p.Unit)
				params.ColorModel = &cmt
			}

			if p.HasAccess(ReadAccess) {
				c.Retrievable = true
			}
			if p.HasAccess(NotifyAccess) {
				c.Reportable = true
				propertyIDs = append(propertyIDs, p.Iid)
			}
		}
		if p.IsTypeOf(colorTemperatureProperty) {
			switch p.Unit {
			case kelvinUnit:
				found = true
				tkp := model.TemperatureKParameters{
					Min: model.TemperatureK(p.ValueRange[0]),
					Max: model.TemperatureK(p.ValueRange[1]),
				}
				params.TemperatureK = &tkp

				if p.HasAccess(ReadAccess) {
					c.Retrievable = true
				}
				if p.HasAccess(NotifyAccess) {
					c.Reportable = true
					propertyIDs = append(propertyIDs, p.Iid)
				}
			case percentUnit:
				found = true
				tkp := model.TemperatureKParameters{
					Min: 3000,
					Max: 5700,
				}
				params.TemperatureK = &tkp

				if p.HasAccess(ReadAccess) {
					c.Retrievable = true
				}
				if p.HasAccess(NotifyAccess) {
					c.Reportable = true
					propertyIDs = append(propertyIDs, p.Iid)
				}
			}

		}
	}

	c.Parameters = params
	return c, propertyIDs, found
}

func (s *Service) ToSwitchCapabilities() ([]adapter.CapabilityInfoView, []int) {
	cs := make([]adapter.CapabilityInfoView, 0)
	propertyIDs := make([]int, 0)
	for _, p := range s.Properties {
		switch {
		case p.IsTypeOf(onProperty):
			cs = append(cs, p.ToOnOffCapability())
			if p.HasAccess(NotifyAccess) {
				propertyIDs = append(propertyIDs, p.Iid)
			}
		default:
			continue
		}
	}
	return cs, propertyIDs
}

func (s *Service) ToACCapabilities() ([]adapter.CapabilityInfoView, []int) {
	cs := make([]adapter.CapabilityInfoView, 0)
	propertyIDs := make([]int, 0)
	for _, p := range s.Properties {
		switch {
		case p.IsTypeOf(onProperty):
			cs = append(cs, p.ToOnOffCapability())
			if p.HasAccess(NotifyAccess) {
				propertyIDs = append(propertyIDs, p.Iid)
			}
		case p.IsTypeOf(modeProperty):
			cs = append(cs, p.ToAcModeCapability())
			if p.HasAccess(NotifyAccess) {
				propertyIDs = append(propertyIDs, p.Iid)
			}
		case p.IsTypeOf(targetTemperatureProperty):
			cs = append(cs, p.ToTemperatureCapability())
			if p.HasAccess(NotifyAccess) {
				propertyIDs = append(propertyIDs, p.Iid)
			}
		default:
			continue
		}
	}
	return cs, propertyIDs
}

func (s *Service) ToFanCapabilities() ([]adapter.CapabilityInfoView, []int) {
	cs := make([]adapter.CapabilityInfoView, 0)
	propertyIDs := make([]int, 0)

	for _, p := range s.Properties {
		switch {
		case p.IsTypeOf(onProperty):
			cs = append(cs, p.ToOnOffCapability())
			if p.HasAccess(NotifyAccess) {
				propertyIDs = append(propertyIDs, p.Iid)
			}
		//case p.IsTypeOf(fanLevelProperty):
		//	cs = append(cs, p.ToFanLevelCapability())
		//case p.IsTypeOf(verticalSwingProperty):
		//	cs = append(cs, p.ToToggleSwingCapability())
		default:
			continue
		}
	}
	return cs, propertyIDs
}

func (s *Service) ToAirPurifierCapabilities() ([]adapter.CapabilityInfoView, []int) {
	cs := make([]adapter.CapabilityInfoView, 0)
	propertyIDs := make([]int, 0)
	for _, p := range s.Properties {
		switch {
		case p.IsTypeOf(onProperty):
			cs = append(cs, p.ToOnOffCapability())
			if p.HasAccess(NotifyAccess) {
				propertyIDs = append(propertyIDs, p.Iid)
			}
		default:
			continue
		}
	}
	return cs, propertyIDs
}

// todo: think about curtains - target and current position bonanza
func (s *Service) ToCurtainCapabilities() ([]adapter.CapabilityInfoView, []int) {
	cs := make([]adapter.CapabilityInfoView, 0)
	propertyIDs := make([]int, 0)

	var hasCurrentPositionProperties bool
	for _, p := range s.Properties {
		switch {
		case p.IsTypeOf(currentPositionProperty):
			hasCurrentPositionProperties = p.HasAccess(ReadAccess)
			if p.HasAccess(NotifyAccess) {
				propertyIDs = append(propertyIDs, p.Iid)
			}
		}
	}

	for _, p := range s.Properties {
		switch {
		case p.IsTypeOf(motorControlProperty):
			cs = append(cs, p.ToMotorControlCapability())
		case p.IsTypeOf(targetPositionProperty):
			cs = append(cs, p.ToTargetPositionCapability(hasCurrentPositionProperties))
		default:
			continue
		}
	}
	return cs, propertyIDs
}

func (s *Service) ToHumidifierCapabilities() ([]adapter.CapabilityInfoView, []int) {
	cs := make([]adapter.CapabilityInfoView, 0)
	propertyIDs := make([]int, 0)
	for _, p := range s.Properties {
		switch {
		case p.IsTypeOf(onProperty):
			cs = append(cs, p.ToOnOffCapability())
			if p.HasAccess(NotifyAccess) {
				propertyIDs = append(propertyIDs, p.Iid)
			}
		case p.IsTypeOf(fanLevelProperty):
			cs = append(cs, p.ToFanLevelCapability())
			if p.HasAccess(NotifyAccess) {
				propertyIDs = append(propertyIDs, p.Iid)
			}
		default:
			continue
		}
	}
	return cs, propertyIDs
}

// ToVacuumCapabilities return vacuum capabilities and property ids for subscription
// todo: look into vacuum capabilities closer - propertyIDs
func (s *Service) ToVacuumCapabilities() ([]adapter.CapabilityInfoView, []int) {
	cs := make([]adapter.CapabilityInfoView, 0)
	propertyIDs := make([]int, 0)

	var hasWorkSpeedMode bool
	for _, p := range s.Properties {
		if p.IsTypeOf(statusProperty) && p.HasAccess(NotifyAccess) {
			propertyIDs = append(propertyIDs, p.Iid)
		}
		switch {
		case !hasWorkSpeedMode && (p.IsTypeOf(speedLevelProperty) || p.IsTypeOf(modeProperty)):
			cs = append(cs, p.ToWorkSpeedCapability())
			hasWorkSpeedMode = true
		}
	}
	isA01 := s.IsTypeOf("urn:miot-spec-v2:service:vacuum:00007810:roborock-a01:1")
	for _, a := range s.Actions {
		switch {
		case a.IsTypeOf(startSweepAction):
			cs = append(cs, a.ToOnOffCapability(!isA01, isA01))
		case a.IsTypeOf(stopSweepAction) || a.IsTypeOf(stopSweepingAction):
			cs = append(cs, a.ToPauseCapability(!isA01))
		}
	}
	return cs, propertyIDs
}

func (s *Service) ToIRTVCapabilities() []adapter.CapabilityInfoView {
	cs := make([]adapter.CapabilityInfoView, 0)

	for _, a := range s.Actions {
		switch {
		case a.IsTypeOf(turnOnAction):
			cs = append(cs, a.ToOnOffCapability(false, false))
		case a.IsTypeOf(channelUpAction):
			cs = append(cs, a.ToChannelCapability())
		case a.IsTypeOf(volumeUpAction):
			cs = append(cs, a.ToVolumeCapability())
		case a.IsTypeOf(muteOnAction):
			cs = append(cs, a.ToMuteCapability(false))
		default:
			continue
		}
	}
	return cs
}

func (s *Service) ToIRACCapabilities() []adapter.CapabilityInfoView {
	cs := make([]adapter.CapabilityInfoView, 0)

	for _, p := range s.Properties {
		switch {
		case p.IsTypeOf(irModeProperty):
			cs = append(cs, p.ToIrAcModeCapability())
		}
	}

	for _, a := range s.Actions {
		switch {
		case a.IsTypeOf(turnOnAction):
			cs = append(cs, a.ToOnOffCapability(false, false))
		case a.IsTypeOf(temperatureUpAction):
			cs = append(cs, a.ToTemperatureCapability())
		//case a.IsTypeOf(fanSpeedUpAction):
		//	cs = append(cs, a.ToFanSpeedCapability()) // TODO: This needs unretrievable range IOT-267
		default:
			continue
		}
	}
	return cs
}

func (s *Service) ToBatteryProperties() ([]adapter.PropertyInfoView, []int) {
	ps := make([]adapter.PropertyInfoView, 0)
	propertyIDs := make([]int, 0)

	for _, p := range s.Properties {
		if !p.HasAccess(ReadAccess) {
			// as unretrievable properties are not supported yet, this is a hack to prevent such properties in devices
			continue
		}
		switch {
		case p.IsTypeOf(batteryLevelProperty):
			ps = append(ps, p.ToBatteryLevelProperty())
			if p.HasAccess(NotifyAccess) {
				propertyIDs = append(propertyIDs, p.Iid)
			}
		default:
			continue
		}
	}
	return ps, propertyIDs
}

func (s *Service) ToHumidifierProperties() ([]adapter.PropertyInfoView, []int) {
	ps := make([]adapter.PropertyInfoView, 0)
	propertyIDs := make([]int, 0)

	for _, p := range s.Properties {
		if !p.HasAccess(ReadAccess) {
			// as unretrievable properties are not supported yet, this is a hack to prevent such properties in devices
			continue
		}
		switch {
		case p.IsTypeOf(waterLevelProperty):
			ps = append(ps, p.ToWaterLevelProperty())
			if p.HasAccess(NotifyAccess) {
				propertyIDs = append(propertyIDs, p.Iid)
			}
		default:
			continue
		}
	}
	return ps, propertyIDs
}

func (s *Service) ToGasSensorProperties() ([]adapter.PropertyInfoView, []int) {
	ps := make([]adapter.PropertyInfoView, 0)
	propertyIDs := make([]int, 0)

	for _, p := range s.Properties {
		switch {
		case p.IsTypeOf(gasConcentrationProperty):
			ps = append(ps, p.ToGasConcentrationProperty())
			if p.HasAccess(NotifyAccess) {
				propertyIDs = append(propertyIDs, p.Iid)
			}
		default:
			continue
		}
	}
	return ps, propertyIDs
}

func (s *Service) ToSmokeSensorProperties() ([]adapter.PropertyInfoView, []int) {
	ps := make([]adapter.PropertyInfoView, 0)
	propertyIDs := make([]int, 0)

	for _, p := range s.Properties {
		switch {
		case p.IsTypeOf(smokeConcentrationProperty):
			ps = append(ps, p.ToSmokeConcentrationProperty())
			if p.HasAccess(NotifyAccess) {
				propertyIDs = append(propertyIDs, p.Iid)
			}
		default:
			continue
		}
	}
	return ps, propertyIDs
}

func (s *Service) ToEnvironmentProperties() ([]adapter.PropertyInfoView, []int) {
	ps := make([]adapter.PropertyInfoView, 0)
	propertyIDs := make([]int, 0)

	for _, p := range s.Properties {
		if !p.HasAccess(ReadAccess) {
			// as unretrievable properties are not supported yet, this is a hack to prevent such properties in devices
			continue
		}
		switch {
		case p.IsTypeOf(relativeHumidityProperty):
			ps = append(ps, p.ToHumidityProperty())
			if p.HasAccess(NotifyAccess) {
				propertyIDs = append(propertyIDs, p.Iid)
			}
		case p.IsTypeOf(temperatureProperty):
			ps = append(ps, p.ToTemperatureProperty())
			if p.HasAccess(NotifyAccess) {
				propertyIDs = append(propertyIDs, p.Iid)
			}
		case p.IsTypeOf(pm1DensityProperty):
			ps = append(ps, p.ToPMDensityProperty(model.PM1DensityPropertyInstance))
		case p.IsTypeOf(pm2p5DensityProperty):
			ps = append(ps, p.ToPMDensityProperty(model.PM2p5DensityPropertyInstance))
		case p.IsTypeOf(pm10DensityProperty):
			ps = append(ps, p.ToPMDensityProperty(model.PM10DensityPropertyInstance))
		default:
			continue
		}
	}
	return ps, propertyIDs
}

func (s *Service) ToTemperatureHumiditySensorProperties() ([]adapter.PropertyInfoView, []int) {
	ps := make([]adapter.PropertyInfoView, 0)
	propertyIDs := make([]int, 0)
	for _, p := range s.Properties {
		switch {
		case p.IsTypeOf(temperatureProperty):
			ps = append(ps, p.ToTemperatureProperty())
			if p.HasAccess(NotifyAccess) {
				propertyIDs = append(propertyIDs, p.Iid)
			}
		case p.IsTypeOf(relativeHumidityProperty):
			ps = append(ps, p.ToHumidityProperty())
			if p.HasAccess(NotifyAccess) {
				propertyIDs = append(propertyIDs, p.Iid)
			}
		case p.IsTypeOf(atmosphericPressureProperty):
			if p.Unit != pascalUnit {
				continue
			}
			ps = append(ps, p.ToPressureProperty(model.UnitPressureMmHg))
			if p.HasAccess(NotifyAccess) {
				propertyIDs = append(propertyIDs, p.Iid)
			}
		default:
			continue
		}
	}
	return ps, propertyIDs
}

func (s *Service) ToMotionSensorProperties() ([]adapter.PropertyInfoView, []int) {
	ps := make([]adapter.PropertyInfoView, 0)
	propertyIDs := make([]int, 0)
	for _, p := range s.Properties {
		switch {
		case p.IsTypeOf(motionStateProperty):
			ps = append(ps, p.ToMotionStateProperty())
			if p.HasAccess(NotifyAccess) {
				propertyIDs = append(propertyIDs, p.Iid)
			}
		case p.IsFloatIlluminationProperty():
			ps = append(ps, p.ToIlluminationProperty())
			if p.HasAccess(NotifyAccess) {
				propertyIDs = append(propertyIDs, p.Iid)
			}
		default:
			continue
		}
	}
	return ps, propertyIDs
}

func (s *Service) ToIlluminationSensorProperties() ([]adapter.PropertyInfoView, []int) {
	ps := make([]adapter.PropertyInfoView, 0)
	propertyIDs := make([]int, 0)
	for _, p := range s.Properties {
		switch {
		case p.IsFloatIlluminationProperty():
			ps = append(ps, p.ToIlluminationProperty())
			if p.HasAccess(NotifyAccess) {
				propertyIDs = append(propertyIDs, p.Iid)
			}
		default:
			continue
		}
	}
	return ps, propertyIDs
}

func (s *Service) ToMagnetSensorProperty() (adapter.PropertyInfoView, []int, []int) {
	propertyIDs := make([]int, 0)
	eventIDs := make([]int, 0)

	property := adapter.PropertyInfoView{
		Type:        model.EventPropertyType,
		Retrievable: false,
		Reportable:  false,
		Parameters: model.EventPropertyParameters{
			Instance: model.OpenPropertyInstance,
			Events:   make([]model.Event, 0),
		},
	}
	isPropertyFound := false

	for _, p := range s.Properties {
		switch {
		case p.IsTypeOf(contactStateProperty):
			isPropertyFound = true

			property.Retrievable = p.HasAccess(ReadAccess)
			property.Reportable = p.HasAccess(NotifyAccess)
			property.Parameters = model.EventPropertyParameters{
				Instance: model.OpenPropertyInstance,
				Events: []model.Event{
					model.KnownEvents[model.EventKey{Instance: model.OpenPropertyInstance, Value: model.OpenedEvent}],
					model.KnownEvents[model.EventKey{Instance: model.OpenPropertyInstance, Value: model.ClosedEvent}],
				},
			}

			if property.Reportable {
				propertyIDs = append(propertyIDs, p.Iid)
			}
		default:
			continue
		}
	}

	if isPropertyFound && property.Reportable {
		return property, propertyIDs, eventIDs
	}

	events := make([]model.Event, 0)
	for _, e := range s.Events {
		switch {
		case e.IsTypeOf(openEvent):
			events = append(events, model.KnownEvents[model.EventKey{Instance: model.OpenPropertyInstance, Value: model.OpenedEvent}])
			eventIDs = append(eventIDs, e.Iid)
		case e.IsTypeOf(closeEvent):
			events = append(events, model.KnownEvents[model.EventKey{Instance: model.OpenPropertyInstance, Value: model.ClosedEvent}])
			eventIDs = append(eventIDs, e.Iid)
		default:
			continue
		}
	}

	if len(events) > 0 {
		property.Reportable = true
		property.Parameters = model.EventPropertyParameters{
			Instance: model.OpenPropertyInstance,
			Events:   events,
		}
	}

	return property, propertyIDs, eventIDs
}

func (s *Service) ToSwitchSensorProperty() (adapter.PropertyInfoView, []int) {
	eventIDs := make([]int, 0)
	events := make([]model.Event, 0)

	for _, e := range s.Events {
		switch {
		case e.IsTypeOf(clickEvent):
			events = append(events, model.KnownEvents[model.EventKey{Instance: model.ButtonPropertyInstance, Value: model.ClickEvent}])
			eventIDs = append(eventIDs, e.Iid)
		case e.IsTypeOf(doubleClickEvent):
			events = append(events, model.KnownEvents[model.EventKey{Instance: model.ButtonPropertyInstance, Value: model.DoubleClickEvent}])
			eventIDs = append(eventIDs, e.Iid)
		case e.IsTypeOf(longPressEvent):
			events = append(events, model.KnownEvents[model.EventKey{Instance: model.ButtonPropertyInstance, Value: model.LongPressEvent}])
			eventIDs = append(eventIDs, e.Iid)
		default:
			continue
		}
	}

	return adapter.PropertyInfoView{
		Type:        model.EventPropertyType,
		Retrievable: false,
		Reportable:  true,
		Parameters: model.EventPropertyParameters{
			Instance: model.ButtonPropertyInstance,
			Events:   events,
		},
	}, eventIDs
}

func (s *Service) ToSubmersionSensorProperty() (adapter.PropertyInfoView, []int) {
	eventIDs := make([]int, 0)
	events := make([]model.Event, 0)

	for _, e := range s.Events {
		switch {
		case e.IsTypeOf(submersionDetectedEvent):
			events = append(events, model.KnownEvents[model.EventKey{Instance: model.WaterLeakPropertyInstance, Value: model.LeakEvent}])
			eventIDs = append(eventIDs, e.Iid)
		case e.IsTypeOf(noSubmersionDetectedEvent):
			events = append(events, model.KnownEvents[model.EventKey{Instance: model.WaterLeakPropertyInstance, Value: model.DryEvent}])
			eventIDs = append(eventIDs, e.Iid)
		default:
			continue
		}
	}

	return adapter.PropertyInfoView{
		Type:        model.EventPropertyType,
		Retrievable: false,
		Reportable:  true,
		Parameters: model.EventPropertyParameters{
			Instance: model.WaterLeakPropertyInstance,
			Events:   events,
		},
	}, eventIDs
}

func (s *Service) ToVibrationSensorProperty() (adapter.PropertyInfoView, []int) {
	eventIDs := make([]int, 0)
	events := make([]model.Event, 0)

	for _, e := range s.Events {
		switch {
		case e.IsTypeOf(tiltEvent):
			events = append(events, model.KnownEvents[model.EventKey{Instance: model.VibrationPropertyInstance, Value: model.TiltEvent}])
			eventIDs = append(eventIDs, e.Iid)
		case e.IsTypeOf(fallEvent):
			events = append(events, model.KnownEvents[model.EventKey{Instance: model.VibrationPropertyInstance, Value: model.FallEvent}])
			eventIDs = append(eventIDs, e.Iid)
		case e.IsTypeOf(vibrationEvent):
			events = append(events, model.KnownEvents[model.EventKey{Instance: model.VibrationPropertyInstance, Value: model.VibrationEvent}])
			eventIDs = append(eventIDs, e.Iid)
		default:
			continue
		}
	}

	return adapter.PropertyInfoView{
		Type:        model.EventPropertyType,
		Retrievable: false,
		Reportable:  true,
		Parameters: model.EventPropertyParameters{
			Instance: model.VibrationPropertyInstance,
			Events:   events,
		},
	}, eventIDs
}

func (s *Service) ToHeaterCapabilities() ([]adapter.CapabilityInfoView, []int) {
	cs := make([]adapter.CapabilityInfoView, 0)
	propertyIDs := make([]int, 0)
	for _, p := range s.Properties {
		switch {
		case p.IsTypeOf(onProperty):
			cs = append(cs, p.ToOnOffCapability())
			if p.HasAccess(NotifyAccess) {
				propertyIDs = append(propertyIDs, p.Iid)
			}
		case p.IsTypeOf(targetTemperatureProperty):
			cs = append(cs, p.ToTemperatureCapability())
			if p.HasAccess(NotifyAccess) {
				propertyIDs = append(propertyIDs, p.Iid)
			}
		case p.IsTypeOf(modeProperty):
			cs = append(cs, p.ToHeaterModeCapability())
			if p.HasAccess(NotifyAccess) {
				propertyIDs = append(propertyIDs, p.Iid)
			}
		default:
			continue
		}
	}
	return cs, propertyIDs
}

func (p *Property) ToOnOffCapability() adapter.CapabilityInfoView {
	c := adapter.CapabilityInfoView{
		Type:        model.OnOffCapabilityType,
		Retrievable: p.HasAccess(ReadAccess),
		Reportable:  p.HasAccess(NotifyAccess),
		Parameters:  model.OnOffCapabilityParameters{},
	}
	return c
}

func (p *Property) ToBrightnessCapability() adapter.CapabilityInfoView {
	c := adapter.CapabilityInfoView{Type: model.RangeCapabilityType, Retrievable: p.HasAccess(ReadAccess), Reportable: p.HasAccess(NotifyAccess)}
	c.Parameters = model.RangeCapabilityParameters{
		Instance:     model.BrightnessRangeInstance,
		Unit:         model.UnitPercent,
		RandomAccess: true,
		Range: &model.Range{
			Min:       p.ValueRange[0],
			Max:       p.ValueRange[1],
			Precision: p.ValueRange[2],
		},
	}
	return c
}

func (p *Property) ToTemperatureCapability() adapter.CapabilityInfoView {
	c := adapter.CapabilityInfoView{Type: model.RangeCapabilityType, Retrievable: p.HasAccess(ReadAccess), Reportable: p.HasAccess(NotifyAccess)}
	c.Parameters = model.RangeCapabilityParameters{
		Instance:     model.TemperatureRangeInstance,
		Unit:         model.UnitTemperatureCelsius,
		RandomAccess: true,
		Range: &model.Range{
			Min:       p.ValueRange[0],
			Max:       p.ValueRange[1],
			Precision: p.ValueRange[2],
		},
	}
	return c
}

func (p *Property) ToHeaterModeCapability() adapter.CapabilityInfoView {
	c := adapter.CapabilityInfoView{
		Type:        model.ModeCapabilityType,
		Retrievable: p.HasAccess(ReadAccess),
		Reportable:  p.HasAccess(NotifyAccess),
	}

	switch {
	case p.IsTypeOf(modeProperty):
		modes := make([]model.Mode, 0)
		for _, v := range p.ValueList {
			switch v.Value {
			case int(autoHeaterMode):
				modes = append(modes, model.KnownModes[yandexHeaterModesMap[autoHeaterMode]])
			case int(maxHeaterMode):
				modes = append(modes, model.KnownModes[yandexHeaterModesMap[maxHeaterMode]])
			case int(normalHeaterMode):
				modes = append(modes, model.KnownModes[yandexHeaterModesMap[normalHeaterMode]])
			case int(minHeaterMode):
				modes = append(modes, model.KnownModes[yandexHeaterModesMap[minHeaterMode]])
			}
		}

		c.Parameters = model.ModeCapabilityParameters{
			Instance: model.HeatModeInstance,
			Modes:    modes,
		}
	}
	return c
}

func (p *Property) ToWorkSpeedCapability() adapter.CapabilityInfoView {
	c := adapter.CapabilityInfoView{
		Type:        model.ModeCapabilityType,
		Retrievable: p.HasAccess(ReadAccess),
		Reportable:  p.HasAccess(NotifyAccess),
	}

	switch {
	case p.IsTypeOf(speedLevelProperty):
		c.Parameters = model.ModeCapabilityParameters{
			Instance: model.WorkSpeedModeInstance,
			Modes: []model.Mode{
				model.KnownModes[model.SlowMode],
				model.KnownModes[model.MediumMode],
				model.KnownModes[model.FastMode],
			},
		}
	case p.IsTypeOf(modeProperty):
		modes := make([]model.Mode, 0)
		for _, v := range p.ValueList {
			switch v.Value {
			case int(silentMode), 101:
				modes = append(modes, model.KnownModes[yandexVacuumModesMap[silentMode]])
			case int(basicMode), 102:
				modes = append(modes, model.KnownModes[yandexVacuumModesMap[basicMode]])
			case int(strongMode), 103:
				modes = append(modes, model.KnownModes[yandexVacuumModesMap[strongMode]])
			case int(fullSpeedMode), 104:
				modes = append(modes, model.KnownModes[yandexVacuumModesMap[fullSpeedMode]])
			}
		}

		c.Parameters = model.ModeCapabilityParameters{
			Instance: model.WorkSpeedModeInstance,
			Modes:    modes,
		}
	}
	return c
}

func (p *Property) ToAcModeCapability() adapter.CapabilityInfoView {
	c := adapter.CapabilityInfoView{Type: model.ModeCapabilityType, Retrievable: p.HasAccess(ReadAccess), Reportable: p.HasAccess(NotifyAccess)}

	modes := make([]model.Mode, 0)
	for _, v := range p.ValueList {
		switch v.Value {
		case int(autoMode):
			modes = append(modes, model.KnownModes[model.AutoMode])
		case int(coolMode):
			modes = append(modes, model.KnownModes[model.CoolMode])
		case int(dryMode):
			modes = append(modes, model.KnownModes[model.DryMode])
		case int(heatMode):
			modes = append(modes, model.KnownModes[model.HeatMode])
		case int(fanMode):
			modes = append(modes, model.KnownModes[model.FanOnlyMode])
		}
	}

	c.Parameters = model.ModeCapabilityParameters{
		Instance: model.ThermostatModeInstance,
		Modes:    modes,
	}
	return c
}

func (p *Property) ToMotorControlCapability() adapter.CapabilityInfoView {
	c := adapter.CapabilityInfoView{
		Type:        model.OnOffCapabilityType,
		Retrievable: false,
		Reportable:  false,
		Parameters:  model.OnOffCapabilityParameters{Split: true},
	}
	return c
}

func (p *Property) ToTargetPositionCapability(isRetrievable bool) adapter.CapabilityInfoView {
	c := adapter.CapabilityInfoView{Type: model.RangeCapabilityType, Retrievable: isRetrievable, Reportable: p.HasAccess(NotifyAccess)}
	c.Parameters = model.RangeCapabilityParameters{
		Instance:     model.OpenRangeInstance,
		Unit:         model.UnitPercent,
		RandomAccess: true,
		Range: &model.Range{
			Min:       p.ValueRange[0],
			Max:       p.ValueRange[1],
			Precision: p.ValueRange[2],
		},
	}
	return c
}

func (p *Property) ToFanLevelCapability() adapter.CapabilityInfoView {
	c := adapter.CapabilityInfoView{Type: model.ModeCapabilityType, Retrievable: p.HasAccess(ReadAccess), Reportable: p.HasAccess(NotifyAccess)}

	modes := make([]model.Mode, 0)
	for _, v := range p.ValueList {
		switch v.Value {
		case int(autoFanMode):
			modes = append(modes, model.KnownModes[model.AutoMode])
		case int(lowFanMode):
			modes = append(modes, model.KnownModes[model.LowMode])
		case int(mediumFanMode):
			modes = append(modes, model.KnownModes[model.MediumMode])
		case int(highFanMode):
			modes = append(modes, model.KnownModes[model.HighMode])
		}
	}

	c.Parameters = model.ModeCapabilityParameters{
		Instance: model.FanSpeedModeInstance,
		Modes:    modes,
	}
	return c
}

func (p *Property) ToIrAcModeCapability() adapter.CapabilityInfoView {
	c := adapter.CapabilityInfoView{Type: model.ModeCapabilityType, Retrievable: false, Reportable: false}

	modes := make([]model.Mode, 0)
	for _, v := range p.ValueList {
		switch v.Value {
		case int(autoMode):
			modes = append(modes, model.KnownModes[model.AutoMode])
		case int(coolMode):
			modes = append(modes, model.KnownModes[model.CoolMode])
		case int(dryMode):
			modes = append(modes, model.KnownModes[model.DryMode])
		case int(heatMode):
			modes = append(modes, model.KnownModes[model.HeatMode])
		case int(fanMode):
			modes = append(modes, model.KnownModes[model.FanOnlyMode])
		}
	}

	c.Parameters = model.ModeCapabilityParameters{
		Instance: model.ThermostatModeInstance,
		Modes:    modes,
	}
	return c
}

func (p *Property) ToTemperatureProperty() adapter.PropertyInfoView {
	return adapter.PropertyInfoView{
		Type:        model.FloatPropertyType,
		Retrievable: p.HasAccess(ReadAccess),
		Reportable:  p.HasAccess(NotifyAccess),
		Parameters: model.FloatPropertyParameters{
			Instance: model.TemperaturePropertyInstance,
			Unit:     model.UnitTemperatureCelsius,
		},
	}
}

func (p *Property) ToPMDensityProperty(instance model.PropertyInstance) adapter.PropertyInfoView {
	return adapter.PropertyInfoView{
		Type:        model.FloatPropertyType,
		Retrievable: p.HasAccess(ReadAccess),
		Reportable:  p.HasAccess(NotifyAccess),
		Parameters: model.FloatPropertyParameters{
			Instance: instance,
			Unit:     model.UnitDensityMcgM3,
		},
	}
}

func (p *Property) ToHumidityProperty() adapter.PropertyInfoView {
	return adapter.PropertyInfoView{
		Type:        model.FloatPropertyType,
		Retrievable: p.HasAccess(ReadAccess),
		Reportable:  p.HasAccess(NotifyAccess),
		Parameters: model.FloatPropertyParameters{
			Instance: model.HumidityPropertyInstance,
			Unit:     model.UnitPercent,
		},
	}
}

func (p *Property) ToPressureProperty(targetUnit model.Unit) adapter.PropertyInfoView {
	return adapter.PropertyInfoView{
		Type:        model.FloatPropertyType,
		Retrievable: p.HasAccess(ReadAccess),
		Reportable:  p.HasAccess(NotifyAccess),
		Parameters: model.FloatPropertyParameters{
			Instance: model.PressurePropertyInstance,
			Unit:     targetUnit,
		},
	}
}

func (p *Property) ToWaterLevelProperty() adapter.PropertyInfoView {
	return adapter.PropertyInfoView{
		Type:        model.FloatPropertyType,
		Retrievable: p.HasAccess(ReadAccess),
		Reportable:  p.HasAccess(NotifyAccess),
		Parameters: model.FloatPropertyParameters{
			Instance: model.WaterLevelPropertyInstance,
			Unit:     model.UnitPercent,
		},
	}
}

func (p *Property) ToGasConcentrationProperty() adapter.PropertyInfoView {
	return adapter.PropertyInfoView{
		Type:        model.FloatPropertyType,
		Retrievable: p.HasAccess(ReadAccess),
		Reportable:  p.HasAccess(NotifyAccess),
		Parameters: model.FloatPropertyParameters{
			Instance: model.GasConcentrationPropertyInstance,
			Unit:     model.UnitPercent,
		},
	}
}

func (p *Property) ToSmokeConcentrationProperty() adapter.PropertyInfoView {
	return adapter.PropertyInfoView{
		Type:        model.FloatPropertyType,
		Retrievable: p.HasAccess(ReadAccess),
		Reportable:  p.HasAccess(NotifyAccess),
		Parameters: model.FloatPropertyParameters{
			Instance: model.SmokeConcentrationPropertyInstance,
			Unit:     model.UnitPercent,
		},
	}
}

func (p *Property) ToBatteryLevelProperty() adapter.PropertyInfoView {
	if p.Unit == percentUnit { // use float property only for percentage properties
		return adapter.PropertyInfoView{
			Type:        model.FloatPropertyType,
			Retrievable: p.HasAccess(ReadAccess),
			Reportable:  p.HasAccess(NotifyAccess),
			Parameters: model.FloatPropertyParameters{
				Instance: model.BatteryLevelPropertyInstance,
				Unit:     model.UnitPercent,
			},
		}
	}

	return adapter.PropertyInfoView{
		Type:        model.EventPropertyType,
		Retrievable: p.HasAccess(ReadAccess),
		Reportable:  p.HasAccess(NotifyAccess),
		Parameters: model.EventPropertyParameters{
			Instance: model.BatteryLevelPropertyInstance,
			Events: []model.Event{
				model.KnownEvents[model.EventKey{Instance: model.BatteryLevelPropertyInstance, Value: model.NormalEvent}],
				model.KnownEvents[model.EventKey{Instance: model.BatteryLevelPropertyInstance, Value: model.LowEvent}],
			},
		},
	}
}

func (p *Property) ToMotionStateProperty() adapter.PropertyInfoView {
	return adapter.PropertyInfoView{
		Type:        model.EventPropertyType,
		Retrievable: false, // always not retrievable, see https://st.yandex-team.ru/IOT-998
		Reportable:  p.HasAccess(NotifyAccess),
		Parameters: model.EventPropertyParameters{
			Instance: model.MotionPropertyInstance,
			Events: []model.Event{
				model.KnownEvents[model.EventKey{Instance: model.MotionPropertyInstance, Value: model.DetectedEvent}],
				model.KnownEvents[model.EventKey{Instance: model.MotionPropertyInstance, Value: model.NotDetectedEvent}],
			},
		},
	}
}

func (p *Property) IsFloatIlluminationProperty() bool {
	// support only float illumination properties. skip enums (value lst) properties
	// example of skip: urn:miot-spec-v2:device:motion-sensor:0000A014:lumi-bmgl01:1
	// https://st.yandex-team.ru/IOT-1535
	return p.IsTypeOf(illuminationProperty) && len(p.ValueList) == 0
}

func (p *Property) ToIlluminationProperty() adapter.PropertyInfoView {
	return adapter.PropertyInfoView{
		Type:        model.FloatPropertyType,
		Retrievable: p.HasAccess(ReadAccess),
		Reportable:  p.HasAccess(NotifyAccess),
		Parameters: model.FloatPropertyParameters{
			Instance: model.IlluminationPropertyInstance,
			Unit:     model.UnitIlluminationLux,
		},
	}
}

func (p *Property) ToContactStateProperty() adapter.PropertyInfoView {
	return adapter.PropertyInfoView{
		Type:        model.EventPropertyType,
		Retrievable: p.HasAccess(ReadAccess),
		Reportable:  p.HasAccess(NotifyAccess),
		Parameters: model.EventPropertyParameters{
			Instance: model.OpenPropertyInstance,
			Events: []model.Event{
				model.KnownEvents[model.EventKey{Instance: model.OpenPropertyInstance, Value: model.OpenedEvent}],
				model.KnownEvents[model.EventKey{Instance: model.OpenPropertyInstance, Value: model.ClosedEvent}],
			},
		},
	}
}

func (a *Action) ToOnOffCapability(isRetrievable, isSplit bool) adapter.CapabilityInfoView {
	c := adapter.CapabilityInfoView{
		Type:        model.OnOffCapabilityType,
		Retrievable: isRetrievable,
		Reportable:  false,
		Parameters:  model.OnOffCapabilityParameters{Split: isSplit},
	}
	return c
}

func (a *Action) ToChannelCapability() adapter.CapabilityInfoView {
	c := adapter.CapabilityInfoView{
		Type:        model.RangeCapabilityType,
		Retrievable: false,
		Reportable:  false,
		Parameters: model.RangeCapabilityParameters{
			Instance:     model.ChannelRangeInstance,
			RandomAccess: false,
			Looped:       false,
		},
	}
	return c
}

func (a *Action) ToVolumeCapability() adapter.CapabilityInfoView {
	c := adapter.CapabilityInfoView{
		Type:        model.RangeCapabilityType,
		Retrievable: false,
		Reportable:  false,
		Parameters: model.RangeCapabilityParameters{
			Instance:     model.VolumeRangeInstance,
			RandomAccess: false,
			Looped:       false,
		},
	}
	return c
}

func (a *Action) ToTemperatureCapability() adapter.CapabilityInfoView {
	c := adapter.CapabilityInfoView{
		Type:        model.RangeCapabilityType,
		Retrievable: false,
		Reportable:  false,
		Parameters: model.RangeCapabilityParameters{
			Instance:     model.TemperatureRangeInstance,
			Unit:         model.UnitTemperatureCelsius,
			RandomAccess: false,
			Looped:       false,
		},
	}
	return c
}

func (a *Action) ToMuteCapability(isRetrievable bool) adapter.CapabilityInfoView {
	c := adapter.CapabilityInfoView{
		Type:        model.ToggleCapabilityType,
		Retrievable: isRetrievable,
		Reportable:  false,
		Parameters: model.ToggleCapabilityParameters{
			Instance: model.MuteToggleCapabilityInstance,
		},
	}
	return c
}

func (a *Action) ToPauseCapability(isRetrievable bool) adapter.CapabilityInfoView {
	c := adapter.CapabilityInfoView{
		Type:        model.ToggleCapabilityType,
		Retrievable: isRetrievable,
		Reportable:  false,
		Parameters: model.ToggleCapabilityParameters{
			Instance: model.PauseToggleCapabilityInstance,
		},
	}
	return c
}
