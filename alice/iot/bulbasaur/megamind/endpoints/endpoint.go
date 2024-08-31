package endpoints

import (
	"context"

	"google.golang.org/protobuf/encoding/protojson"
	"google.golang.org/protobuf/proto"
	"google.golang.org/protobuf/types/known/anypb"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/callback"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	yandexiocd "a.yandex-team.ru/alice/iot/bulbasaur/model/yandexio"
	"a.yandex-team.ru/alice/library/go/timestamp"
	endpointpb "a.yandex-team.ru/alice/protos/endpoint"
	batterypb "a.yandex-team.ru/alice/protos/endpoint/capabilities/battery"
	openingpb "a.yandex-team.ru/alice/protos/endpoint/capabilities/opening_sensor"
	vibrationpb "a.yandex-team.ru/alice/protos/endpoint/capabilities/vibration_sensor"
	waterleakpb "a.yandex-team.ru/alice/protos/endpoint/capabilities/water_leak_sensor"
	eventspb "a.yandex-team.ru/alice/protos/endpoint/events"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/ptr"
)

type Endpoint struct {
	*endpointpb.TEndpoint
}

// generic

func (e Endpoint) DeviceType() model.DeviceType {
	if endpointDeviceType, ok := EndpointTypeToDeviceTypeMap[e.GetMeta().GetType()]; ok {
		return endpointDeviceType
	}
	return model.OtherDeviceType
}

func (e Endpoint) generateDeviceName(_ adapter.CapabilityInfoViews, properties adapter.PropertyInfoViews) string {
	switch e.DeviceType() {
	case model.LightDeviceType:
		return "Лампочка"
	case model.SensorDeviceType:
		_, isMotionSensor := properties.GetByTypeAndInstance(model.EventPropertyType, string(model.MotionPropertyInstance))
		_, isOpenSensor := properties.GetByTypeAndInstance(model.EventPropertyType, string(model.OpenPropertyInstance))
		_, isVibrationSensor := properties.GetByTypeAndInstance(model.EventPropertyType, string(model.VibrationPropertyInstance))
		_, isButtonSensor := properties.GetByTypeAndInstance(model.EventPropertyType, string(model.ButtonPropertyInstance))
		_, isSmokeEventSensor := properties.GetByTypeAndInstance(model.EventPropertyType, string(model.SmokePropertyInstance))
		_, isSmokeFloatSensor := properties.GetByTypeAndInstance(model.FloatPropertyType, string(model.SmokeConcentrationPropertyInstance))
		_, isGasEventSensor := properties.GetByTypeAndInstance(model.EventPropertyType, string(model.GasPropertyInstance))
		_, isGasFloatSensor := properties.GetByTypeAndInstance(model.FloatPropertyType, string(model.GasConcentrationPropertyInstance))
		_, isWaterLeakSensor := properties.GetByTypeAndInstance(model.EventPropertyType, string(model.WaterLeakPropertyInstance))
		_, isTemperatureSensor := properties.GetByTypeAndInstance(model.FloatPropertyType, string(model.TemperaturePropertyInstance))
		_, isHumiditySensor := properties.GetByTypeAndInstance(model.FloatPropertyType, string(model.HumidityPropertyInstance))
		_, isIlluminationSensor := properties.GetByTypeAndInstance(model.FloatPropertyType, string(model.IlluminationPropertyInstance))
		_, isTvocSensor := properties.GetByTypeAndInstance(model.FloatPropertyType, string(model.TvocPropertyInstance))
		_, isPM1Sensor := properties.GetByTypeAndInstance(model.FloatPropertyType, string(model.PM1DensityPropertyInstance))
		_, isPM25Sensor := properties.GetByTypeAndInstance(model.FloatPropertyType, string(model.PM2p5DensityPropertyInstance))
		_, isPM10Sensor := properties.GetByTypeAndInstance(model.FloatPropertyType, string(model.PM10DensityPropertyInstance))
		switch {
		case isMotionSensor:
			return "Датчик движения"
		case isOpenSensor:
			return "Датчик двери"
		case isVibrationSensor:
			return "Датчик вибрации"
		case isButtonSensor:
			return "Умная кнопка"
		case isSmokeEventSensor, isSmokeFloatSensor:
			return "Датчик дыма"
		case isGasEventSensor, isGasFloatSensor:
			return "Датчик газа"
		case isWaterLeakSensor:
			return "Датчик протечки"
		case isTemperatureSensor, isHumiditySensor:
			return "Датчик климата"
		case isIlluminationSensor:
			return "Датчик освещенности"
		case isTvocSensor, isPM1Sensor, isPM25Sensor, isPM10Sensor:
			return "Датчик качества воздуха"
		default:
			return "Датчик"
		}
	default:
		return e.DeviceType().GenerateDeviceName()
	}
}

func (e Endpoint) MarshalJSON() ([]byte, error) {
	return protojson.Marshal(e.TEndpoint)
}

func (e *Endpoint) UnmarshalJSON(b []byte) error {
	innerEndpoint := &endpointpb.TEndpoint{}
	if err := protojson.Unmarshal(b, innerEndpoint); err != nil {
		return err
	}
	e.TEndpoint = innerEndpoint
	return nil
}

// discovery

func (e Endpoint) ToDeviceInfoView(parentEndpointID string, parentLocation EndpointLocation) adapter.DeviceInfoView {
	capabilities, properties := e.getDiscoveryInfoViews()
	return adapter.DeviceInfoView{
		ID:           e.GetId(),
		Name:         e.generateDeviceName(capabilities, properties),
		Type:         e.DeviceType(),
		HouseholdID:  parentLocation.HouseholdID,
		Room:         parentLocation.RoomName,
		Capabilities: capabilities,
		Properties:   properties,
		CustomData:   yandexiocd.CustomData{ParentEndpointID: parentEndpointID},
		DeviceInfo:   e.getDeviceInfo(),
	}
}

func (e Endpoint) getDeviceInfo() *model.DeviceInfo {
	if deviceInfo := e.GetMeta().GetDeviceInfo(); deviceInfo != nil {
		return &model.DeviceInfo{
			Manufacturer: ptr.String(deviceInfo.GetManufacturer()),
			Model:        ptr.String(deviceInfo.GetModel()),
			SwVersion:    ptr.String(deviceInfo.GetSwVersion()),
			HwVersion:    ptr.String(deviceInfo.GetHwVersion()),
		}
	}
	return nil
}

func (e Endpoint) getDiscoveryInfoViews() ([]adapter.CapabilityInfoView, []adapter.PropertyInfoView) {
	capabilities := make([]adapter.CapabilityInfoView, 0, len(e.GetCapabilities()))
	properties := make([]adapter.PropertyInfoView, 0, len(e.GetCapabilities()))
	for _, c := range e.GetCapabilities() {
		var (
			onOffCapabilityMessage     = new(endpointpb.TOnOffCapability)
			levelCapabilityMessage     = new(endpointpb.TLevelCapability)
			colorCapabilityMessage     = new(endpointpb.TColorCapability)
			buttonCapabilityMessage    = new(endpointpb.TButtonCapability)
			motionCapabilityMessage    = new(endpointpb.TMotionCapability)
			openingCapabilityMessage   = new(openingpb.TOpeningSensorCapability)
			vibrationCapabilityMessage = new(vibrationpb.TVibrationSensorCapability)
			waterLeakCapabilityMessage = new(waterleakpb.TWaterLeakSensorCapability)
			batteryCapabilityMessage   = new(batterypb.TBatteryCapability)
		)
		switch {
		case c.MessageIs(onOffCapabilityMessage):
			if err := c.UnmarshalTo(onOffCapabilityMessage); err != nil {
				continue
			}
			eMeta := onOffCapabilityMessage.GetMeta()
			eParameters := onOffCapabilityMessage.GetParameters()
			eState := onOffCapabilityMessage.GetState()

			onOffCapability := adapter.CapabilityInfoView{
				Reportable:  eMeta.GetReportable(),
				Retrievable: eMeta.GetRetrievable(),
				Type:        model.OnOffCapabilityType,
				Parameters: model.OnOffCapabilityParameters{
					Split: eParameters.GetSplit(),
				},
			}
			if eState != nil {
				onOffCapability.State = model.OnOffCapabilityState{
					Instance: model.OnOnOffCapabilityInstance,
					Value:    eState.GetOn(),
				}
			}
			capabilities = append(capabilities, onOffCapability)
		case c.MessageIs(levelCapabilityMessage):
			if err := c.UnmarshalTo(levelCapabilityMessage); err != nil {
				continue
			}
			eMeta := levelCapabilityMessage.GetMeta()
			eParameters := levelCapabilityMessage.GetParameters()
			eState := levelCapabilityMessage.GetState()

			if isProperty(eMeta) {
				unit, value := EndpointUnitMap[eParameters.GetUnit()], eState.GetLevel()
				if unit == model.UnitPressurePascal {
					unit, value = model.UnitPressureMmHg, model.GetMMHGFromPascal(value)
				} else if eParameters.GetUnit() == endpointpb.EUnit_PPBUnit {
					unit, value = model.UnitPPM, value/1000 // we don't have PPB unit, so we use PPM instead
				}
				propertyInstance := EndpointLevelInstanceToPropertyInstanceMap[eParameters.GetInstance()]
				floatProperty := adapter.PropertyInfoView{
					Reportable:  eMeta.GetReportable(),
					Retrievable: eMeta.GetRetrievable(),
					Type:        model.FloatPropertyType,
					Parameters: model.FloatPropertyParameters{
						Instance: propertyInstance,
						Unit:     unit,
					},
				}
				if eState != nil {
					floatProperty.State = model.FloatPropertyState{
						Instance: propertyInstance,
						Value:    value,
					}
				}
				properties = append(properties, floatProperty)
			} else {
				capabilityInstance := EndpointLevelInstanceToCapabilityInstanceMap[eParameters.GetInstance()]
				rangeCapability := adapter.CapabilityInfoView{
					Reportable:  eMeta.GetReportable(),
					Retrievable: eMeta.GetRetrievable(),
					Type:        model.RangeCapabilityType,
					Parameters: model.RangeCapabilityParameters{
						Instance:     capabilityInstance,
						Unit:         EndpointUnitMap[eParameters.GetUnit()],
						RandomAccess: supportsRangeRandomAccess(eMeta.GetSupportedDirectives()),
						Range:        getRange(eParameters.GetRange()),
					},
				}
				if eState != nil {
					rangeCapability.State = model.RangeCapabilityState{
						Instance: capabilityInstance,
						Value:    eState.GetLevel(),
					}
				}
				capabilities = append(capabilities, rangeCapability)
			}
		case c.MessageIs(colorCapabilityMessage):
			if err := c.UnmarshalTo(colorCapabilityMessage); err != nil {
				continue
			}
			eMeta := colorCapabilityMessage.GetMeta()
			eParameters := colorCapabilityMessage.GetParameters()
			parameters := model.ColorSettingCapabilityParameters{}

			if temperatureKParameters := eParameters.GetTemperatureKParameters(); temperatureKParameters != nil {
				parameters.TemperatureK = &model.TemperatureKParameters{
					Min: model.TemperatureK(temperatureKParameters.GetRange().GetMin()),
					Max: model.TemperatureK(temperatureKParameters.GetRange().GetMax()),
				}
			}
			if colorSceneParameters := eParameters.GetColorSceneParameters(); colorSceneParameters != nil {
				parameters.ColorSceneParameters = &model.ColorSceneParameters{
					Scenes: make(model.ColorScenes, 0, len(colorSceneParameters.GetSupportedScenes())),
				}
				for _, supportedScene := range colorSceneParameters.GetSupportedScenes() {
					scene, ok := model.KnownEndpointSceneToModelScene[supportedScene]
					if ok {
						parameters.ColorSceneParameters.Scenes = append(parameters.ColorSceneParameters.Scenes, model.KnownColorScenes[scene])
					}
				}
			}
			colorSettingCapability := adapter.CapabilityInfoView{
				Reportable:  eMeta.GetReportable(),
				Retrievable: eMeta.GetRetrievable(),
				Type:        model.ColorSettingCapabilityType,
				Parameters:  parameters,
			}
			if eState := colorCapabilityMessage.GetState(); eState != nil {
				switch state := eState.GetValue().(type) {
				case *endpointpb.TColorCapability_TState_ColorScene:
					scene, ok := model.KnownEndpointSceneToModelScene[state.ColorScene]
					if ok {
						colorSettingCapability.State = model.ColorSettingCapabilityState{
							Instance: model.SceneCapabilityInstance,
							Value:    scene,
						}
					}
				case *endpointpb.TColorCapability_TState_TemperatureK:
					colorSettingCapability.State = model.ColorSettingCapabilityState{
						Instance: model.TemperatureKCapabilityInstance,
						Value:    model.TemperatureK(eState.GetTemperatureK()),
					}
				}
			}
			capabilities = append(capabilities, colorSettingCapability)
		case c.MessageIs(buttonCapabilityMessage):
			if err := c.UnmarshalTo(buttonCapabilityMessage); err != nil {
				continue
			}
			eMeta := buttonCapabilityMessage.GetMeta()

			buttonCapability := adapter.PropertyInfoView{
				Reportable:  eMeta.GetReportable(),
				Retrievable: eMeta.GetRetrievable(),
				Type:        model.EventPropertyType,
				Parameters: model.EventPropertyParameters{
					Instance: model.ButtonPropertyInstance,
					Events:   getEvents(eMeta.GetSupportedEvents()),
				},
			}
			properties = append(properties, buttonCapability)
		case c.MessageIs(motionCapabilityMessage):
			if err := c.UnmarshalTo(motionCapabilityMessage); err != nil {
				continue
			}
			eMeta := motionCapabilityMessage.GetMeta()

			motionCapability := adapter.PropertyInfoView{
				Reportable:  eMeta.GetReportable(),
				Retrievable: eMeta.GetRetrievable(),
				Type:        model.EventPropertyType,
				Parameters: model.EventPropertyParameters{
					Instance: model.MotionPropertyInstance,
					Events:   getEvents(eMeta.GetSupportedEvents()),
				},
			}
			properties = append(properties, motionCapability)
		case c.MessageIs(openingCapabilityMessage):
			if err := c.UnmarshalTo(openingCapabilityMessage); err != nil {
				continue
			}
			eMeta := openingCapabilityMessage.GetMeta()

			openingCapability := adapter.PropertyInfoView{
				Reportable:  eMeta.GetReportable(),
				Retrievable: eMeta.GetRetrievable(),
				Type:        model.EventPropertyType,
				Parameters: model.EventPropertyParameters{
					Instance: model.OpenPropertyInstance,
					Events:   getEvents(eMeta.GetSupportedEvents()),
				},
			}
			properties = append(properties, openingCapability)
		case c.MessageIs(vibrationCapabilityMessage):
			if err := c.UnmarshalTo(vibrationCapabilityMessage); err != nil {
				continue
			}
			eMeta := vibrationCapabilityMessage.GetMeta()

			vibrationCapability := adapter.PropertyInfoView{
				Reportable:  eMeta.GetReportable(),
				Retrievable: eMeta.GetRetrievable(),
				Type:        model.EventPropertyType,
				Parameters: model.EventPropertyParameters{
					Instance: model.VibrationPropertyInstance,
					Events:   getEvents(eMeta.GetSupportedEvents()),
				},
			}
			properties = append(properties, vibrationCapability)
		case c.MessageIs(waterLeakCapabilityMessage):
			if err := c.UnmarshalTo(waterLeakCapabilityMessage); err != nil {
				continue
			}
			eMeta := waterLeakCapabilityMessage.GetMeta()

			waterLeakCapability := adapter.PropertyInfoView{
				Reportable:  eMeta.GetReportable(),
				Retrievable: eMeta.GetRetrievable(),
				Type:        model.EventPropertyType,
				Parameters: model.EventPropertyParameters{
					Instance: model.WaterLeakPropertyInstance,
					Events:   getEvents(eMeta.GetSupportedEvents()),
				},
			}
			properties = append(properties, waterLeakCapability)
		case c.MessageIs(batteryCapabilityMessage):
			if err := c.UnmarshalTo(batteryCapabilityMessage); err != nil {
				continue
			}
			eMeta := batteryCapabilityMessage.GetMeta()
			eState := batteryCapabilityMessage.GetState()

			batteryProperty := adapter.PropertyInfoView{
				Reportable:  eMeta.GetReportable(),
				Retrievable: eMeta.GetRetrievable(),
				Type:        model.FloatPropertyType,
				Parameters: model.FloatPropertyParameters{
					Instance: model.BatteryLevelPropertyInstance,
					Unit:     model.UnitPercent,
				},
			}
			if eState != nil {
				batteryProperty.State = model.FloatPropertyState{
					Instance: model.BatteryLevelPropertyInstance,
					Value:    eState.GetPercentage(),
				}
			}
			properties = append(properties, batteryProperty)
		default:
			continue
		}
	}
	return capabilities, properties
}

// callback

func (e Endpoint) ToDeviceStateView() callback.DeviceStateView {
	capabilities := make([]adapter.CapabilityStateView, 0, len(e.GetCapabilities()))
	properties := make([]adapter.PropertyStateView, 0, len(e.GetCapabilities()))

	for _, c := range e.GetCapabilities() {
		var (
			onOffCapabilityMessage = new(endpointpb.TOnOffCapability)
			levelCapabilityMessage = new(endpointpb.TLevelCapability)
		)
		switch {
		case c.MessageIs(onOffCapabilityMessage):
			if err := c.UnmarshalTo(onOffCapabilityMessage); err != nil {
				continue
			}
			eState := onOffCapabilityMessage.GetState()
			if eState == nil {
				continue
			}
			onOffCapability := adapter.CapabilityStateView{
				Type: model.OnOffCapabilityType,
				State: model.OnOffCapabilityState{
					Instance: model.OnOnOffCapabilityInstance,
					Value:    eState.GetOn(),
				},
			}
			capabilities = append(capabilities, onOffCapability)
		case c.MessageIs(levelCapabilityMessage):
			if err := c.UnmarshalTo(levelCapabilityMessage); err != nil {
				continue
			}
			eMeta := levelCapabilityMessage.GetMeta()
			eParameters := levelCapabilityMessage.GetParameters()
			eState := levelCapabilityMessage.GetState()
			if eState == nil {
				continue
			}
			if isProperty(eMeta) {
				value := eState.GetLevel()
				if eParameters.GetUnit() == endpointpb.EUnit_PressurePascalUnit {
					value = model.GetMMHGFromPascal(value)
				} else if eParameters.GetUnit() == endpointpb.EUnit_PPBUnit {
					value = value / 1000 // we don't have PPB unit, so we use PPM instead
				}

				floatProperty := adapter.PropertyStateView{
					Type: model.FloatPropertyType,
					State: model.FloatPropertyState{
						Instance: EndpointLevelInstanceToPropertyInstanceMap[eParameters.GetInstance()],
						Value:    value,
					},
				}
				properties = append(properties, floatProperty)
			} else {
				rangeCapability := adapter.CapabilityStateView{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: EndpointLevelInstanceToCapabilityInstanceMap[eParameters.GetInstance()],
						Value:    eState.GetLevel(),
					},
				}
				capabilities = append(capabilities, rangeCapability)
			}
		default:
			continue
		}
	}
	return callback.DeviceStateView{
		ID:           e.GetId(),
		Capabilities: capabilities,
		Properties:   properties,
	}
}

func WrapEndpoints(protoEndpoints []*endpointpb.TEndpoint) []Endpoint {
	if protoEndpoints == nil {
		return nil
	}
	result := make([]Endpoint, 0, len(protoEndpoints))
	for _, protoEndpoint := range protoEndpoints {
		result = append(result, Endpoint{TEndpoint: protoEndpoint})
	}
	return result
}

func getEvents(events []endpointpb.TCapability_EEventType) model.Events {
	result := make(model.Events, 0, len(events))
	for _, event := range events {
		eventValue, seen := model.EndpointEventToEventValue[event]
		if !seen {
			continue
		}
		result = append(result, model.Event{Value: eventValue})
	}
	return result
}

func bound(value, left, right uint64) uint64 {
	if value > right {
		return right
	}
	if value < left {
		return left
	}
	return value
}

func ConvertEndpointEventsToDeviceStateView(ctx context.Context, logger log.Logger, endpointEvents *eventspb.TEndpointEvents) callback.DeviceStateView {
	deviceStateView := callback.DeviceStateView{ID: endpointEvents.GetEndpointId(), Status: EndpointStatusMap[endpointEvents.GetEndpointStatus().GetStatus()]}
	unmarshalOptions := proto.UnmarshalOptions{Merge: false, AllowPartial: true, DiscardUnknown: true}
	for _, event := range endpointEvents.GetCapabilityEvents() {
		var (
			updateOnOffStateEvent   = new(endpointpb.TOnOffCapability_TUpdateStateEvent)
			updateLevelStateEvent   = new(endpointpb.TLevelCapability_TUpdateStateEvent)
			updateColorStateEvent   = new(endpointpb.TColorCapability_TUpdateStateEvent)
			updateBatteryStateEvent = new(batterypb.TBatteryCapability_TUpdateStateEvent)

			buttonClickEvent              = new(endpointpb.TButtonCapability_TButtonClickEvent)
			buttonDoubleClickEvent        = new(endpointpb.TButtonCapability_TButtonDoubleClickEvent)
			buttonLongPressEvent          = new(endpointpb.TButtonCapability_TButtonLongPressEvent)
			buttonLongReleaseEvent        = new(endpointpb.TButtonCapability_TButtonLongReleaseEvent)
			motionDetectedEvent           = new(endpointpb.TMotionCapability_TMotionDetectedEvent)
			waterLeakSensorLeakEvent      = new(waterleakpb.TWaterLeakSensorCapability_TWaterLeakSensorLeakEvent)
			waterLeakSensorDryEvent       = new(waterleakpb.TWaterLeakSensorCapability_TWaterLeakSensorDryEvent)
			vibrationSensorVibrationEvent = new(vibrationpb.TVibrationSensorCapability_TVibrationSensorVibrationEvent)
			vibrationSensorTiltEvent      = new(vibrationpb.TVibrationSensorCapability_TVibrationSensorTiltEvent)
			vibrationSensorFallEvent      = new(vibrationpb.TVibrationSensorCapability_TVibrationSensorFallEvent)
			openingSensorOpenedEvent      = new(openingpb.TOpeningSensorCapability_TOpeningSensorOpenedEvent)
			openingSensorClosedEvent      = new(openingpb.TOpeningSensorCapability_TOpeningSensorClosedEvent)
		)
		switch {
		case event.GetEvent().MessageIs(updateOnOffStateEvent):
			if err := anypb.UnmarshalTo(event.GetEvent(), updateOnOffStateEvent, unmarshalOptions); err != nil {
				ctxlog.Errorf(ctx, logger, "failed to unmarshal event %s: %v", event.GetEvent().GetTypeUrl(), err)
				continue
			}
			if updateOnOffStateEvent.GetCapability().GetState() == nil {
				continue
			}
			deviceStateView.Capabilities = append(deviceStateView.Capabilities, adapter.CapabilityStateView{
				Type: model.OnOffCapabilityType,
				State: model.OnOffCapabilityState{
					Instance: model.OnOnOffCapabilityInstance,
					Value:    updateOnOffStateEvent.GetCapability().GetState().GetOn(),
				},
				Timestamp: timestamp.FromProto(event.GetTimestamp()),
			})
		case event.GetEvent().MessageIs(updateLevelStateEvent):
			if err := anypb.UnmarshalTo(event.GetEvent(), updateLevelStateEvent, unmarshalOptions); err != nil {
				ctxlog.Errorf(ctx, logger, "failed to unmarshal event %s: %v", event.GetEvent().GetTypeUrl(), err)
				continue
			}
			if updateLevelStateEvent.GetCapability().GetState() == nil {
				continue
			}
			if isProperty(updateLevelStateEvent.GetCapability().GetMeta()) {
				// property
				unit := updateLevelStateEvent.GetCapability().GetParameters().GetUnit()
				value := updateLevelStateEvent.GetCapability().GetState().GetLevel()

				if unit == endpointpb.EUnit_PressurePascalUnit {
					value = model.GetMMHGFromPascal(value)
				} else if unit == endpointpb.EUnit_PPBUnit {
					value = value / 1000 // we don't have PPB unit, so we use PPM instead
				}

				propertyInstance := EndpointLevelInstanceToPropertyInstanceMap[updateLevelStateEvent.GetCapability().GetParameters().GetInstance()]
				deviceStateView.Properties = append(deviceStateView.Properties, adapter.PropertyStateView{
					Type: model.FloatPropertyType,
					State: model.FloatPropertyState{
						Instance: propertyInstance,
						Value:    value,
					},
					Timestamp: timestamp.FromProto(event.GetTimestamp()),
				})
			} else {
				// capability
				capabilityInstance := EndpointLevelInstanceToCapabilityInstanceMap[updateLevelStateEvent.GetCapability().GetParameters().GetInstance()]
				deviceStateView.Capabilities = append(deviceStateView.Capabilities, adapter.CapabilityStateView{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: capabilityInstance,
						Value:    updateLevelStateEvent.GetCapability().GetState().GetLevel(),
					},
					Timestamp: timestamp.FromProto(event.GetTimestamp()),
				})
			}
		case event.GetEvent().MessageIs(updateColorStateEvent):
			if err := anypb.UnmarshalTo(event.GetEvent(), updateColorStateEvent, unmarshalOptions); err != nil {
				ctxlog.Errorf(ctx, logger, "failed to unmarshal event %s: %v", event.GetEvent().GetTypeUrl(), err)
				continue
			}
			if updateColorStateEvent.GetCapability().GetState() == nil {
				continue
			}
			capabilityStateView := adapter.CapabilityStateView{
				Type:      model.ColorSettingCapabilityType,
				Timestamp: timestamp.FromProto(event.GetTimestamp()),
			}
			leftBound, rightBound := model.DefaultLeftBoundTemperatureK, model.DefaultRightBoundTemperatureK // default bounds in case parameters are empty
			if eParameters := updateColorStateEvent.GetCapability().GetParameters(); eParameters != nil {
				temperatureRange := eParameters.GetTemperatureKParameters().GetRange()
				if temperatureRange != nil {
					leftBound, rightBound = temperatureRange.GetMin(), temperatureRange.GetMax()
				}
			}
			if eState := updateColorStateEvent.GetCapability().GetState(); eState != nil {
				switch state := eState.GetValue().(type) {
				case *endpointpb.TColorCapability_TState_ColorScene:
					scene, ok := model.KnownEndpointSceneToModelScene[state.ColorScene]
					if ok {
						capabilityStateView.State = model.ColorSettingCapabilityState{
							Instance: model.SceneCapabilityInstance,
							Value:    scene,
						}
					}
				case *endpointpb.TColorCapability_TState_TemperatureK:
					capabilityStateView.State = model.ColorSettingCapabilityState{
						Instance: model.TemperatureKCapabilityInstance,
						Value:    model.TemperatureK(bound(eState.GetTemperatureK(), leftBound, rightBound)),
					}
				default:
					ctxlog.Errorf(ctx, logger, "unknown color state value: %v", eState.GetValue())
				}
			}
			deviceStateView.Capabilities = append(deviceStateView.Capabilities, capabilityStateView)
		case event.GetEvent().MessageIs(updateBatteryStateEvent):
			if err := anypb.UnmarshalTo(event.GetEvent(), updateBatteryStateEvent, unmarshalOptions); err != nil {
				ctxlog.Errorf(ctx, logger, "failed to unmarshal event %s: %v", event.GetEvent().GetTypeUrl(), err)
				continue
			}
			if updateBatteryStateEvent.GetCapability().GetState() == nil {
				continue
			}
			deviceStateView.Properties = append(deviceStateView.Properties, adapter.PropertyStateView{
				Type: model.FloatPropertyType,
				State: model.FloatPropertyState{
					Instance: model.BatteryLevelPropertyInstance,
					Value:    updateBatteryStateEvent.GetCapability().GetState().GetPercentage(),
				},
				Timestamp: timestamp.FromProto(event.GetTimestamp()),
			})
		case event.GetEvent().MessageIs(buttonClickEvent):
			if err := anypb.UnmarshalTo(event.GetEvent(), buttonClickEvent, unmarshalOptions); err != nil {
				ctxlog.Errorf(ctx, logger, "failed to unmarshal event %s: %v", event.GetEvent().GetTypeUrl(), err)
				continue
			}
			deviceStateView.Properties = append(deviceStateView.Properties, adapter.PropertyStateView{
				Type: model.EventPropertyType,
				State: model.EventPropertyState{
					Instance: model.ButtonPropertyInstance,
					Value:    model.ClickEvent,
				},
				Timestamp: timestamp.FromProto(event.GetTimestamp()),
			})
		case event.GetEvent().MessageIs(buttonDoubleClickEvent):
			if err := anypb.UnmarshalTo(event.GetEvent(), buttonDoubleClickEvent, unmarshalOptions); err != nil {
				ctxlog.Errorf(ctx, logger, "failed to unmarshal event %s: %v", event.GetEvent().GetTypeUrl(), err)
				continue
			}
			deviceStateView.Properties = append(deviceStateView.Properties, adapter.PropertyStateView{
				Type: model.EventPropertyType,
				State: model.EventPropertyState{
					Instance: model.ButtonPropertyInstance,
					Value:    model.DoubleClickEvent,
				},
				Timestamp: timestamp.FromProto(event.GetTimestamp()),
			})
		case event.GetEvent().MessageIs(buttonLongPressEvent):
			if err := anypb.UnmarshalTo(event.GetEvent(), buttonLongPressEvent, unmarshalOptions); err != nil {
				ctxlog.Errorf(ctx, logger, "failed to unmarshal event %s: %v", event.GetEvent().GetTypeUrl(), err)
				continue
			}
			deviceStateView.Properties = append(deviceStateView.Properties, adapter.PropertyStateView{
				Type: model.EventPropertyType,
				State: model.EventPropertyState{
					Instance: model.ButtonPropertyInstance,
					Value:    model.LongPressEvent,
				},
				Timestamp: timestamp.FromProto(event.GetTimestamp()),
			})
		case event.GetEvent().MessageIs(buttonLongReleaseEvent):
			ctxlog.Info(ctx, logger, "button long release is not supported, skip it")
			continue
		case event.GetEvent().MessageIs(motionDetectedEvent):
			if err := anypb.UnmarshalTo(event.GetEvent(), motionDetectedEvent, unmarshalOptions); err != nil {
				ctxlog.Errorf(ctx, logger, "failed to unmarshal event %s: %v", event.GetEvent().GetTypeUrl(), err)
				continue
			}
			deviceStateView.Properties = append(deviceStateView.Properties, adapter.PropertyStateView{
				Type: model.EventPropertyType,
				State: model.EventPropertyState{
					Instance: model.MotionPropertyInstance,
					Value:    model.DetectedEvent,
				},
				Timestamp: timestamp.FromProto(event.GetTimestamp()),
			})
		case event.GetEvent().MessageIs(waterLeakSensorLeakEvent):
			if err := anypb.UnmarshalTo(event.GetEvent(), waterLeakSensorLeakEvent, unmarshalOptions); err != nil {
				ctxlog.Errorf(ctx, logger, "failed to unmarshal event %s: %v", event.GetEvent().GetTypeUrl(), err)
				continue
			}
			deviceStateView.Properties = append(deviceStateView.Properties, adapter.PropertyStateView{
				Type: model.EventPropertyType,
				State: model.EventPropertyState{
					Instance: model.WaterLeakPropertyInstance,
					Value:    model.LeakEvent,
				},
				Timestamp: timestamp.FromProto(event.GetTimestamp()),
			})
		case event.GetEvent().MessageIs(waterLeakSensorDryEvent):
			if err := anypb.UnmarshalTo(event.GetEvent(), waterLeakSensorDryEvent, unmarshalOptions); err != nil {
				ctxlog.Errorf(ctx, logger, "failed to unmarshal event %s: %v", event.GetEvent().GetTypeUrl(), err)
				continue
			}
			deviceStateView.Properties = append(deviceStateView.Properties, adapter.PropertyStateView{
				Type: model.EventPropertyType,
				State: model.EventPropertyState{
					Instance: model.WaterLeakPropertyInstance,
					Value:    model.DryEvent,
				},
				Timestamp: timestamp.FromProto(event.GetTimestamp()),
			})
		case event.GetEvent().MessageIs(vibrationSensorVibrationEvent):
			if err := anypb.UnmarshalTo(event.GetEvent(), vibrationSensorVibrationEvent, unmarshalOptions); err != nil {
				ctxlog.Errorf(ctx, logger, "failed to unmarshal event %s: %v", event.GetEvent().GetTypeUrl(), err)
				continue
			}
			deviceStateView.Properties = append(deviceStateView.Properties, adapter.PropertyStateView{
				Type: model.EventPropertyType,
				State: model.EventPropertyState{
					Instance: model.VibrationPropertyInstance,
					Value:    model.VibrationEvent,
				},
				Timestamp: timestamp.FromProto(event.GetTimestamp()),
			})
		case event.GetEvent().MessageIs(vibrationSensorTiltEvent):
			if err := anypb.UnmarshalTo(event.GetEvent(), vibrationSensorTiltEvent, unmarshalOptions); err != nil {
				ctxlog.Errorf(ctx, logger, "failed to unmarshal event %s: %v", event.GetEvent().GetTypeUrl(), err)
				continue
			}
			deviceStateView.Properties = append(deviceStateView.Properties, adapter.PropertyStateView{
				Type: model.EventPropertyType,
				State: model.EventPropertyState{
					Instance: model.VibrationPropertyInstance,
					Value:    model.TiltEvent,
				},
				Timestamp: timestamp.FromProto(event.GetTimestamp()),
			})
		case event.GetEvent().MessageIs(vibrationSensorFallEvent):
			if err := anypb.UnmarshalTo(event.GetEvent(), vibrationSensorFallEvent, unmarshalOptions); err != nil {
				ctxlog.Errorf(ctx, logger, "failed to unmarshal event %s: %v", event.GetEvent().GetTypeUrl(), err)
				continue
			}
			deviceStateView.Properties = append(deviceStateView.Properties, adapter.PropertyStateView{
				Type: model.EventPropertyType,
				State: model.EventPropertyState{
					Instance: model.VibrationPropertyInstance,
					Value:    model.FallEvent,
				},
				Timestamp: timestamp.FromProto(event.GetTimestamp()),
			})
		case event.GetEvent().MessageIs(openingSensorOpenedEvent):
			if err := anypb.UnmarshalTo(event.GetEvent(), openingSensorOpenedEvent, unmarshalOptions); err != nil {
				ctxlog.Errorf(ctx, logger, "failed to unmarshal event %s: %v", event.GetEvent().GetTypeUrl(), err)
				continue
			}
			deviceStateView.Properties = append(deviceStateView.Properties, adapter.PropertyStateView{
				Type: model.EventPropertyType,
				State: model.EventPropertyState{
					Instance: model.OpenPropertyInstance,
					Value:    model.OpenedEvent,
				},
				Timestamp: timestamp.FromProto(event.GetTimestamp()),
			})
		case event.GetEvent().MessageIs(openingSensorClosedEvent):
			if err := anypb.UnmarshalTo(event.GetEvent(), openingSensorClosedEvent, unmarshalOptions); err != nil {
				ctxlog.Errorf(ctx, logger, "failed to unmarshal event %s: %v", event.GetEvent().GetTypeUrl(), err)
				continue
			}
			deviceStateView.Properties = append(deviceStateView.Properties, adapter.PropertyStateView{
				Type: model.EventPropertyType,
				State: model.EventPropertyState{
					Instance: model.OpenPropertyInstance,
					Value:    model.ClosedEvent,
				},
				Timestamp: timestamp.FromProto(event.GetTimestamp()),
			})
		default:
			ctxlog.Infof(ctx, logger, "unknown event %s", event.GetEvent().GetTypeUrl())
		}
	}
	return deviceStateView
}
