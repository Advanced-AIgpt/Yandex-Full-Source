package model

import (
	endpointpb "a.yandex-team.ru/alice/protos/endpoint"
)

var KnownModelSceneToEndpointScene = map[ColorSceneID]endpointpb.TColorCapability_EColorScene{
	ColorSceneIDLavaLamp: endpointpb.TColorCapability_LavaLampScene,
	ColorSceneIDCandle:   endpointpb.TColorCapability_CandleScene,
	ColorSceneIDNight:    endpointpb.TColorCapability_NightScene,
	ColorSceneIDInactive: endpointpb.TColorCapability_Inactive,
}

var KnownEndpointSceneToModelScene = map[endpointpb.TColorCapability_EColorScene]ColorSceneID{
	endpointpb.TColorCapability_LavaLampScene: ColorSceneIDLavaLamp,
	endpointpb.TColorCapability_CandleScene:   ColorSceneIDCandle,
	endpointpb.TColorCapability_NightScene:    ColorSceneIDNight,
	endpointpb.TColorCapability_Inactive:      ColorSceneIDInactive,
}

var PropertyInstanceToEndpointLevelInstanceMap = map[PropertyInstance]endpointpb.TLevelCapability_EInstance{
	TemperaturePropertyInstance:  endpointpb.TLevelCapability_TemperatureInstance,
	HumidityPropertyInstance:     endpointpb.TLevelCapability_HumidityInstance,
	PressurePropertyInstance:     endpointpb.TLevelCapability_PressureInstance,
	IlluminationPropertyInstance: endpointpb.TLevelCapability_IlluminanceInstance,
	TvocPropertyInstance:         endpointpb.TLevelCapability_TVOCInstance,
	AmperagePropertyInstance:     endpointpb.TLevelCapability_AmperageInstance,
	VoltagePropertyInstance:      endpointpb.TLevelCapability_VoltageInstance,
	PowerPropertyInstance:        endpointpb.TLevelCapability_PowerInstance,
}

var EndpointEventToEventValue = map[endpointpb.TCapability_EEventType]EventValue{
	endpointpb.TCapability_ButtonClickEventType:              ClickEvent,
	endpointpb.TCapability_ButtonDoubleClickEventType:        DoubleClickEvent,
	endpointpb.TCapability_ButtonLongPressEventType:          LongPressEvent,
	endpointpb.TCapability_MotionDetectedEventType:           DetectedEvent,
	endpointpb.TCapability_WaterLeakSensorLeakEventType:      LeakEvent,
	endpointpb.TCapability_WaterLeakSensorDryEventType:       DryEvent,
	endpointpb.TCapability_VibrationSensorVibrationEventType: VibrationEvent,
	endpointpb.TCapability_VibrationSensorTiltEventType:      TiltEvent,
	endpointpb.TCapability_VibrationSensorFallEventType:      FallEvent,
	endpointpb.TCapability_OpeningSensorOpenedEventType:      OpenedEvent,
	endpointpb.TCapability_OpeningSensorClosedEventType:      ClosedEvent,
}

var EventValueToEndpointEvent = map[EventValue]endpointpb.TCapability_EEventType{
	ClickEvent:       endpointpb.TCapability_ButtonClickEventType,
	DoubleClickEvent: endpointpb.TCapability_ButtonDoubleClickEventType,
	LongPressEvent:   endpointpb.TCapability_ButtonLongPressEventType,
	DetectedEvent:    endpointpb.TCapability_MotionDetectedEventType,
	LeakEvent:        endpointpb.TCapability_WaterLeakSensorLeakEventType,
	DryEvent:         endpointpb.TCapability_WaterLeakSensorDryEventType,
	VibrationEvent:   endpointpb.TCapability_VibrationSensorVibrationEventType,
	TiltEvent:        endpointpb.TCapability_VibrationSensorTiltEventType,
	FallEvent:        endpointpb.TCapability_VibrationSensorFallEventType,
	OpenedEvent:      endpointpb.TCapability_OpeningSensorOpenedEventType,
	ClosedEvent:      endpointpb.TCapability_OpeningSensorClosedEventType,
}

func ConvertEventValuesToEndpointEvents(events EventValues) []endpointpb.TCapability_EEventType {
	result := make([]endpointpb.TCapability_EEventType, 0, len(events))
	for _, event := range events {
		eventValue, seen := EventValueToEndpointEvent[event]
		if !seen {
			continue
		}
		result = append(result, eventValue)
	}
	return result
}
