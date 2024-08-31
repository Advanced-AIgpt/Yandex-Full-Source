package endpoints

import (
	"context"
	"fmt"
	"testing"

	"github.com/stretchr/testify/assert"
	"google.golang.org/protobuf/proto"
	"google.golang.org/protobuf/types/known/anypb"
	"google.golang.org/protobuf/types/known/timestamppb"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/callback"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	xtestlogs "a.yandex-team.ru/alice/iot/bulbasaur/xtest/logs"
	endpointpb "a.yandex-team.ru/alice/protos/endpoint"
	batterypb "a.yandex-team.ru/alice/protos/endpoint/capabilities/battery"
	openingpb "a.yandex-team.ru/alice/protos/endpoint/capabilities/opening_sensor"
	vibrationpb "a.yandex-team.ru/alice/protos/endpoint/capabilities/vibration_sensor"
	waterleakpb "a.yandex-team.ru/alice/protos/endpoint/capabilities/water_leak_sensor"
	eventspb "a.yandex-team.ru/alice/protos/endpoint/events"
)

func Test_getEvents(t *testing.T) {
	protoEvents := []endpointpb.TCapability_EEventType{
		endpointpb.TCapability_ButtonClickEventType,
		endpointpb.TCapability_ButtonDoubleClickEventType,
		endpointpb.TCapability_ButtonLongPressEventType,
		endpointpb.TCapability_MotionDetectedEventType,
		endpointpb.TCapability_WaterLeakSensorLeakEventType,
		endpointpb.TCapability_WaterLeakSensorDryEventType,
		endpointpb.TCapability_VibrationSensorVibrationEventType,
		endpointpb.TCapability_VibrationSensorTiltEventType,
		endpointpb.TCapability_VibrationSensorFallEventType,
		endpointpb.TCapability_OpeningSensorOpenedEventType,
		endpointpb.TCapability_OpeningSensorClosedEventType,
	}
	expectedEvents := model.Events{
		model.Event{Value: model.ClickEvent},
		model.Event{Value: model.DoubleClickEvent},
		model.Event{Value: model.LongPressEvent},
		model.Event{Value: model.DetectedEvent},
		model.Event{Value: model.LeakEvent},
		model.Event{Value: model.DryEvent},
		model.Event{Value: model.VibrationEvent},
		model.Event{Value: model.TiltEvent},
		model.Event{Value: model.FallEvent},
		model.Event{Value: model.OpenedEvent},
		model.Event{Value: model.ClosedEvent},
	}
	actualEvents := getEvents(protoEvents)
	assert.Equal(t, expectedEvents, actualEvents)
}

func TestEndpoint_DeviceInfoView(t *testing.T) {
	type fields struct {
		TEndpoint *endpointpb.TEndpoint
	}
	tests := []struct {
		testName     string
		deviceName   string
		fields       fields
		capabilities []adapter.CapabilityInfoView
		properties   []adapter.PropertyInfoView
	}{
		{
			testName:   "lamp",
			deviceName: "Лампочка",
			fields: fields{
				TEndpoint: &endpointpb.TEndpoint{
					Id:   "lamp",
					Meta: &endpointpb.TEndpoint_TMeta{Type: endpointpb.TEndpoint_LightEndpointType},
					Capabilities: []*anypb.Any{
						mustNewAny(&endpointpb.TOnOffCapability{
							Meta: &endpointpb.TCapability_TMeta{
								SupportedEvents: []endpointpb.TCapability_EEventType{},
								SupportedDirectives: []endpointpb.TCapability_EDirectiveType{
									endpointpb.TCapability_OnOffDirectiveType,
								},
								Retrievable: true,
								Reportable:  true,
							},
							Parameters: &endpointpb.TOnOffCapability_TParameters{
								Split: false,
							},
							State: &endpointpb.TOnOffCapability_TState{
								On: true,
							},
						}),
					},
				},
			},
			capabilities: []adapter.CapabilityInfoView{
				{
					Reportable:  true,
					Retrievable: true,
					Type:        model.OnOffCapabilityType,
					Parameters:  model.OnOffCapabilityParameters{Split: false},
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    true,
					},
				},
			},
			properties: []adapter.PropertyInfoView{},
		},
		{
			testName:   "tvoc",
			deviceName: "Датчик качества воздуха",
			fields: fields{
				TEndpoint: &endpointpb.TEndpoint{
					Meta: &endpointpb.TEndpoint_TMeta{Type: endpointpb.TEndpoint_SensorEndpointType},
					Capabilities: []*anypb.Any{
						mustNewAny(&endpointpb.TLevelCapability{
							Meta: &endpointpb.TCapability_TMeta{
								SupportedEvents:     []endpointpb.TCapability_EEventType{},
								SupportedDirectives: []endpointpb.TCapability_EDirectiveType{},
								Retrievable:         false,
								Reportable:          true,
							},
							Parameters: &endpointpb.TLevelCapability_TParameters{
								Instance: endpointpb.TLevelCapability_TVOCInstance,
								Range: &endpointpb.TRange{
									Min:       0,
									Max:       100,
									Precision: 1,
								},
								Unit: endpointpb.EUnit_PPBUnit,
							},
							State: &endpointpb.TLevelCapability_TState{
								Level: 10000,
							},
						}),
						mustNewAny(&batterypb.TBatteryCapability{
							Meta: &endpointpb.TCapability_TMeta{
								SupportedEvents:     []endpointpb.TCapability_EEventType{},
								SupportedDirectives: []endpointpb.TCapability_EDirectiveType{},
								Retrievable:         false,
								Reportable:          true,
							},
							Parameters: &batterypb.TBatteryCapability_TParameters{},
							State: &batterypb.TBatteryCapability_TState{
								Percentage: 99,
							},
						}),
					},
				},
			},
			capabilities: []adapter.CapabilityInfoView{},
			properties: []adapter.PropertyInfoView{
				{
					Type:        model.FloatPropertyType,
					Reportable:  true,
					Retrievable: false,
					Parameters: model.FloatPropertyParameters{
						Instance: model.TvocPropertyInstance,
						Unit:     model.UnitPPM,
					},
					State: model.FloatPropertyState{
						Instance: model.TvocPropertyInstance,
						Value:    10,
					},
				},
				{
					Type:        model.FloatPropertyType,
					Reportable:  true,
					Retrievable: false,
					Parameters: model.FloatPropertyParameters{
						Instance: model.BatteryLevelPropertyInstance,
						Unit:     model.UnitPercent,
					},
					State: model.FloatPropertyState{
						Instance: model.BatteryLevelPropertyInstance,
						Value:    99,
					},
				},
			},
		},
		{
			testName:   "curtain",
			deviceName: "Шторы",
			fields: fields{
				TEndpoint: &endpointpb.TEndpoint{
					Id:   "curtain",
					Meta: &endpointpb.TEndpoint_TMeta{Type: endpointpb.TEndpoint_WindowCoveringEndpointType},
					Capabilities: []*anypb.Any{
						mustNewAny(&endpointpb.TLevelCapability{
							Meta: &endpointpb.TCapability_TMeta{
								SupportedEvents: []endpointpb.TCapability_EEventType{},
								SupportedDirectives: []endpointpb.TCapability_EDirectiveType{
									endpointpb.TCapability_SetAbsoluteLevelDirectiveType,
									endpointpb.TCapability_SetRelativeLevelDirectiveType,
								},
								Retrievable: true,
								Reportable:  true,
							},
							Parameters: &endpointpb.TLevelCapability_TParameters{
								Instance: endpointpb.TLevelCapability_CoverInstance,
								Range: &endpointpb.TRange{
									Min:       0,
									Max:       100,
									Precision: 1,
								},
								Unit: endpointpb.EUnit_PercentUnit,
							},
							State: &endpointpb.TLevelCapability_TState{
								Level: 75,
							},
						}),
					},
				},
			},
			capabilities: []adapter.CapabilityInfoView{
				{
					Reportable:  true,
					Retrievable: true,
					Type:        model.RangeCapabilityType,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.OpenRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Looped:       false,
						Range:        &model.Range{Min: 0, Max: 100, Precision: 1},
					},
					State: model.RangeCapabilityState{
						Instance: model.OpenRangeInstance,
						Value:    75,
					},
				},
			},
			properties: []adapter.PropertyInfoView{},
		},
		{
			testName:   "socket",
			deviceName: "Розетка",
			fields: fields{
				TEndpoint: &endpointpb.TEndpoint{
					Id:   "socket",
					Meta: &endpointpb.TEndpoint_TMeta{Type: endpointpb.TEndpoint_SocketEndpointType},
					Capabilities: []*anypb.Any{
						mustNewAny(&endpointpb.TLevelCapability{
							Meta: &endpointpb.TCapability_TMeta{
								SupportedEvents:     []endpointpb.TCapability_EEventType{},
								SupportedDirectives: []endpointpb.TCapability_EDirectiveType{},
								Retrievable:         true,
								Reportable:          true,
							},
							Parameters: &endpointpb.TLevelCapability_TParameters{
								Instance: endpointpb.TLevelCapability_AmperageInstance,
								Range: &endpointpb.TRange{
									Min:       0,
									Max:       5,
									Precision: 1,
								},
								Unit: endpointpb.EUnit_AmpereUnit,
							},
							State: &endpointpb.TLevelCapability_TState{
								Level: 1,
							},
						}),
						mustNewAny(&endpointpb.TLevelCapability{
							Meta: &endpointpb.TCapability_TMeta{
								SupportedEvents:     []endpointpb.TCapability_EEventType{},
								SupportedDirectives: []endpointpb.TCapability_EDirectiveType{},
								Retrievable:         true,
								Reportable:          true,
							},
							Parameters: &endpointpb.TLevelCapability_TParameters{
								Instance: endpointpb.TLevelCapability_VoltageInstance,
								Range: &endpointpb.TRange{
									Min:       0,
									Max:       220,
									Precision: 1,
								},
								Unit: endpointpb.EUnit_VoltUnit,
							},
							State: &endpointpb.TLevelCapability_TState{
								Level: 220,
							},
						}),
						mustNewAny(&endpointpb.TLevelCapability{
							Meta: &endpointpb.TCapability_TMeta{
								SupportedEvents:     []endpointpb.TCapability_EEventType{},
								SupportedDirectives: []endpointpb.TCapability_EDirectiveType{},
								Retrievable:         true,
								Reportable:          true,
							},
							Parameters: &endpointpb.TLevelCapability_TParameters{
								Instance: endpointpb.TLevelCapability_PowerInstance,
								Range: &endpointpb.TRange{
									Min:       0,
									Max:       100,
									Precision: 1,
								},
								Unit: endpointpb.EUnit_WattUnit,
							},
							State: &endpointpb.TLevelCapability_TState{
								Level: 220,
							},
						}),
					},
				},
			},
			capabilities: []adapter.CapabilityInfoView{},
			properties: []adapter.PropertyInfoView{
				{
					Type:        model.FloatPropertyType,
					Reportable:  true,
					Retrievable: true,
					Parameters:  model.FloatPropertyParameters{Instance: model.AmperagePropertyInstance, Unit: model.UnitAmpere},
					State:       model.FloatPropertyState{Instance: model.AmperagePropertyInstance, Value: 1},
				},
				{
					Type:        model.FloatPropertyType,
					Reportable:  true,
					Retrievable: true,
					Parameters:  model.FloatPropertyParameters{Instance: model.VoltagePropertyInstance, Unit: model.UnitVolt},
					State:       model.FloatPropertyState{Instance: model.VoltagePropertyInstance, Value: 220},
				},
				{
					Type:        model.FloatPropertyType,
					Reportable:  true,
					Retrievable: true,
					Parameters:  model.FloatPropertyParameters{Instance: model.PowerPropertyInstance, Unit: model.UnitWatt},
					State:       model.FloatPropertyState{Instance: model.PowerPropertyInstance, Value: 220},
				},
			},
		},
		{
			testName:   "opening_sensor",
			deviceName: "Датчик двери",
			fields: fields{
				TEndpoint: &endpointpb.TEndpoint{
					Meta: &endpointpb.TEndpoint_TMeta{Type: endpointpb.TEndpoint_SensorEndpointType},
					Capabilities: []*anypb.Any{
						mustNewAny(&openingpb.TOpeningSensorCapability{
							Meta: &endpointpb.TCapability_TMeta{
								SupportedDirectives: []endpointpb.TCapability_EDirectiveType{},
								SupportedEvents: []endpointpb.TCapability_EEventType{
									endpointpb.TCapability_OpeningSensorOpenedEventType,
									endpointpb.TCapability_OpeningSensorClosedEventType,
								},
								Retrievable: false,
								Reportable:  true,
							},
							Parameters: &openingpb.TOpeningSensorCapability_TParameters{},
						}),
					},
				},
			},
			capabilities: []adapter.CapabilityInfoView{},
			properties: []adapter.PropertyInfoView{
				{
					Type:        model.EventPropertyType,
					Reportable:  true,
					Retrievable: false,
					Parameters: model.EventPropertyParameters{
						Instance: model.OpenPropertyInstance,
						Events: []model.Event{
							{Value: model.OpenedEvent},
							{Value: model.ClosedEvent},
						},
					},
				},
			},
		},
		{
			testName:   "water_leak_sensor",
			deviceName: "Датчик протечки",
			fields: fields{
				TEndpoint: &endpointpb.TEndpoint{
					Meta: &endpointpb.TEndpoint_TMeta{Type: endpointpb.TEndpoint_SensorEndpointType},
					Capabilities: []*anypb.Any{
						mustNewAny(&waterleakpb.TWaterLeakSensorCapability{
							Meta: &endpointpb.TCapability_TMeta{
								SupportedDirectives: []endpointpb.TCapability_EDirectiveType{},
								SupportedEvents: []endpointpb.TCapability_EEventType{
									endpointpb.TCapability_WaterLeakSensorLeakEventType,
									endpointpb.TCapability_WaterLeakSensorDryEventType,
								},
								Retrievable: false,
								Reportable:  true,
							},
							Parameters: &waterleakpb.TWaterLeakSensorCapability_TParameters{},
						}),
					},
				},
			},
			capabilities: []adapter.CapabilityInfoView{},
			properties: []adapter.PropertyInfoView{
				{
					Type:        model.EventPropertyType,
					Reportable:  true,
					Retrievable: false,
					Parameters: model.EventPropertyParameters{
						Instance: model.WaterLeakPropertyInstance,
						Events: []model.Event{
							{Value: model.LeakEvent},
							{Value: model.DryEvent},
						},
					},
				},
			},
		},
		{
			testName:   "vibration_sensor",
			deviceName: "Датчик вибрации",
			fields: fields{
				TEndpoint: &endpointpb.TEndpoint{
					Meta: &endpointpb.TEndpoint_TMeta{Type: endpointpb.TEndpoint_SensorEndpointType},
					Capabilities: []*anypb.Any{
						mustNewAny(&vibrationpb.TVibrationSensorCapability{
							Meta: &endpointpb.TCapability_TMeta{
								SupportedEvents: []endpointpb.TCapability_EEventType{
									endpointpb.TCapability_VibrationSensorVibrationEventType,
									endpointpb.TCapability_VibrationSensorTiltEventType,
									endpointpb.TCapability_VibrationSensorFallEventType,
								},
								SupportedDirectives: []endpointpb.TCapability_EDirectiveType{},
								Retrievable:         false,
								Reportable:          true,
							},
							Parameters: &vibrationpb.TVibrationSensorCapability_TParameters{},
							State:      &vibrationpb.TVibrationSensorCapability_TState{},
						}),
					},
				},
			},
			capabilities: []adapter.CapabilityInfoView{},
			properties: []adapter.PropertyInfoView{
				{
					Type:        model.EventPropertyType,
					Reportable:  true,
					Retrievable: false,
					Parameters: model.EventPropertyParameters{
						Instance: model.VibrationPropertyInstance,
						Events: []model.Event{
							{Value: model.VibrationEvent},
							{Value: model.TiltEvent},
							{Value: model.FallEvent},
						},
					},
				},
			},
		},
	}
	for _, tt := range tests {
		t.Run(tt.testName, func(t *testing.T) {
			e := Endpoint{
				TEndpoint: tt.fields.TEndpoint,
			}
			capabilities, properties := e.getDiscoveryInfoViews()
			assert.Equalf(t, tt.capabilities, capabilities, "getDiscoveryInfoViews()")
			assert.Equalf(t, tt.properties, properties, "getDiscoveryInfoViews()")
			deviceName := e.generateDeviceName(capabilities, properties)
			assert.Equalf(t, tt.deviceName, deviceName, "generateDeviceName()")
		})
	}
}

func TestConvertEndpointEventsToDeviceStateView(t *testing.T) {
	ctx := context.Background()
	logger := xtestlogs.NopLogger()
	deviceEvents := &eventspb.TEndpointEvents{
		EndpointId: "midi",
		CapabilityEvents: []*eventspb.TCapabilityEvent{
			{
				Timestamp: &timestamppb.Timestamp{Seconds: 1905},
				Event: mustNewAny(&endpointpb.TOnOffCapability_TUpdateStateEvent{
					Capability: &endpointpb.TOnOffCapability{
						State: &endpointpb.TOnOffCapability_TState{On: true},
					},
				}),
			},
			{
				Timestamp: &timestamppb.Timestamp{Seconds: 1917},
				Event: mustNewAny(&endpointpb.TOnOffCapability_TUpdateStateEvent{
					Capability: &endpointpb.TOnOffCapability{
						State: &endpointpb.TOnOffCapability_TState{On: false},
					},
				}),
			},
			{
				Timestamp: &timestamppb.Timestamp{Seconds: 1999},
				Event: mustNewAny(&endpointpb.TColorCapability_TUpdateStateEvent{
					Capability: &endpointpb.TColorCapability{
						State: &endpointpb.TColorCapability_TState{
							Value: &endpointpb.TColorCapability_TState_TemperatureK{
								TemperatureK: 2222,
							},
						},
					},
				}),
			},
			{
				Timestamp: &timestamppb.Timestamp{Seconds: 2086},
				Event: mustNewAny(&endpointpb.TColorCapability_TUpdateStateEvent{
					Capability: &endpointpb.TColorCapability{
						State: &endpointpb.TColorCapability_TState{
							Value: &endpointpb.TColorCapability_TState_ColorScene{
								ColorScene: endpointpb.TColorCapability_NightScene,
							},
						},
					},
				}),
			},
			{
				Timestamp: &timestamppb.Timestamp{Seconds: 5},
				Event: mustNewAny(&endpointpb.TLevelCapability_TUpdateStateEvent{
					Capability: &endpointpb.TLevelCapability{
						Meta:       &endpointpb.TCapability_TMeta{SupportedDirectives: []endpointpb.TCapability_EDirectiveType{endpointpb.TCapability_SetAbsoluteLevelDirectiveType}},
						Parameters: &endpointpb.TLevelCapability_TParameters{Instance: endpointpb.TLevelCapability_BrightnessInstance},
						State:      &endpointpb.TLevelCapability_TState{Level: 25},
					},
				}),
			},
			{
				Timestamp: &timestamppb.Timestamp{Seconds: 20},
				Event: mustNewAny(&batterypb.TBatteryCapability_TUpdateStateEvent{
					Capability: &batterypb.TBatteryCapability{
						State: &batterypb.TBatteryCapability_TState{Percentage: 99},
					},
				}),
			},
			{
				Timestamp: &timestamppb.Timestamp{Seconds: 20},
				Event: mustNewAny(&endpointpb.TLevelCapability_TUpdateStateEvent{
					Capability: &endpointpb.TLevelCapability{
						Meta:       &endpointpb.TCapability_TMeta{SupportedDirectives: []endpointpb.TCapability_EDirectiveType{}},
						Parameters: &endpointpb.TLevelCapability_TParameters{Instance: endpointpb.TLevelCapability_AmperageInstance},
						State:      &endpointpb.TLevelCapability_TState{Level: 5},
					},
				}),
			},
			{
				Timestamp: &timestamppb.Timestamp{Seconds: 20},
				Event: mustNewAny(&endpointpb.TLevelCapability_TUpdateStateEvent{
					Capability: &endpointpb.TLevelCapability{
						Meta:       &endpointpb.TCapability_TMeta{SupportedDirectives: []endpointpb.TCapability_EDirectiveType{}},
						Parameters: &endpointpb.TLevelCapability_TParameters{Instance: endpointpb.TLevelCapability_VoltageInstance},
						State:      &endpointpb.TLevelCapability_TState{Level: 220},
					},
				}),
			},
			{
				Timestamp: &timestamppb.Timestamp{Seconds: 20},
				Event:     mustNewAny(&waterleakpb.TWaterLeakSensorCapability_TWaterLeakSensorLeakEvent{}),
			},
			{
				Timestamp: &timestamppb.Timestamp{Seconds: 20},
				Event:     mustNewAny(&waterleakpb.TWaterLeakSensorCapability_TWaterLeakSensorDryEvent{}),
			},
			{
				Timestamp: &timestamppb.Timestamp{Seconds: 20},
				Event:     mustNewAny(&openingpb.TOpeningSensorCapability_TOpeningSensorOpenedEvent{}),
			},
			{
				Timestamp: &timestamppb.Timestamp{Seconds: 20},
				Event:     mustNewAny(&openingpb.TOpeningSensorCapability_TOpeningSensorClosedEvent{}),
			},
			{
				Timestamp: &timestamppb.Timestamp{Seconds: 20},
				Event:     mustNewAny(&vibrationpb.TVibrationSensorCapability_TVibrationSensorVibrationEvent{}),
			},
			{
				Timestamp: &timestamppb.Timestamp{Seconds: 20},
				Event:     mustNewAny(&vibrationpb.TVibrationSensorCapability_TVibrationSensorTiltEvent{}),
			},
			{
				Timestamp: &timestamppb.Timestamp{Seconds: 20},
				Event:     mustNewAny(&vibrationpb.TVibrationSensorCapability_TVibrationSensorFallEvent{}),
			},
		},
	}
	expectedDeviceStateView := callback.DeviceStateView{
		ID:     "midi",
		Status: model.UnknownDeviceStatus,
		Capabilities: []adapter.CapabilityStateView{
			{
				Type: model.OnOffCapabilityType,
				State: model.OnOffCapabilityState{
					Instance: model.OnOnOffCapabilityInstance,
					Value:    true,
				},
				Timestamp: 1905,
			},
			{
				Type: model.OnOffCapabilityType,
				State: model.OnOffCapabilityState{
					Instance: model.OnOnOffCapabilityInstance,
					Value:    false,
				},
				Timestamp: 1917,
			},
			{
				Type: model.ColorSettingCapabilityType,
				State: model.ColorSettingCapabilityState{
					Instance: model.TemperatureKCapabilityInstance,
					Value:    model.TemperatureK(2700),
				},
				Timestamp: 1999,
			},
			{
				Type: model.ColorSettingCapabilityType,
				State: model.ColorSettingCapabilityState{
					Instance: model.SceneCapabilityInstance,
					Value:    model.ColorSceneIDNight,
				},
				Timestamp: 2086,
			},
			{
				Type: model.RangeCapabilityType,
				State: model.RangeCapabilityState{
					Instance: model.BrightnessRangeInstance,
					Value:    25,
				},
				Timestamp: 5,
			},
		},
		Properties: []adapter.PropertyStateView{
			{
				Type: model.FloatPropertyType,
				State: model.FloatPropertyState{
					Instance: model.BatteryLevelPropertyInstance,
					Value:    99,
				},
				Timestamp: 20,
			},
			{
				Type: model.FloatPropertyType,
				State: model.FloatPropertyState{
					Instance: model.AmperagePropertyInstance,
					Value:    5,
				},
				Timestamp: 20,
			},
			{
				Type: model.FloatPropertyType,
				State: model.FloatPropertyState{
					Instance: model.VoltagePropertyInstance,
					Value:    220,
				},
				Timestamp: 20,
			},
			{
				Type: model.EventPropertyType,
				State: model.EventPropertyState{
					Instance: model.WaterLeakPropertyInstance,
					Value:    model.LeakEvent,
				},
				Timestamp: 20,
			},
			{
				Type: model.EventPropertyType,
				State: model.EventPropertyState{
					Instance: model.WaterLeakPropertyInstance,
					Value:    model.DryEvent,
				},
				Timestamp: 20,
			},
			{
				Type: model.EventPropertyType,
				State: model.EventPropertyState{
					Instance: model.OpenPropertyInstance,
					Value:    model.OpenedEvent,
				},
				Timestamp: 20,
			},
			{
				Type: model.EventPropertyType,
				State: model.EventPropertyState{
					Instance: model.OpenPropertyInstance,
					Value:    model.ClosedEvent,
				},
				Timestamp: 20,
			},
			{
				Type: model.EventPropertyType,
				State: model.EventPropertyState{
					Instance: model.VibrationPropertyInstance,
					Value:    model.VibrationEvent,
				},
				Timestamp: 20,
			},
			{
				Type: model.EventPropertyType,
				State: model.EventPropertyState{
					Instance: model.VibrationPropertyInstance,
					Value:    model.TiltEvent,
				},
				Timestamp: 20,
			},
			{
				Type: model.EventPropertyType,
				State: model.EventPropertyState{
					Instance: model.VibrationPropertyInstance,
					Value:    model.FallEvent,
				},
				Timestamp: 20,
			},
		},
	}
	actualDeviceStateView := ConvertEndpointEventsToDeviceStateView(ctx, logger, deviceEvents)
	assert.Equal(t, expectedDeviceStateView, actualDeviceStateView)
}

func mustNewAny(m proto.Message) *anypb.Any {
	value, err := anypb.New(m)
	if err != nil {
		panic(fmt.Sprintf("failed to create protobuf any value: %s", err))
	}
	return value
}
