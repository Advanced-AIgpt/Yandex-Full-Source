package endpoints

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	endpointpb "a.yandex-team.ru/alice/protos/endpoint"
)

var EndpointStatusMap = map[endpointpb.TEndpoint_EEndpointStatus]model.DeviceStatus{
	endpointpb.TEndpoint_Unknown: model.UnknownDeviceStatus,
	endpointpb.TEndpoint_Offline: model.OfflineDeviceStatus,
	endpointpb.TEndpoint_Online:  model.OnlineDeviceStatus,
}

var EndpointTypeToDeviceTypeMap = map[endpointpb.TEndpoint_EEndpointType]model.DeviceType{
	endpointpb.TEndpoint_LightEndpointType:          model.LightDeviceType,
	endpointpb.TEndpoint_SocketEndpointType:         model.SocketDeviceType,
	endpointpb.TEndpoint_SensorEndpointType:         model.SensorDeviceType,
	endpointpb.TEndpoint_SwitchEndpointType:         model.SwitchDeviceType,
	endpointpb.TEndpoint_WindowCoveringEndpointType: model.CurtainDeviceType,
}

var EndpointUnitMap = map[endpointpb.EUnit]model.Unit{
	endpointpb.EUnit_PercentUnit:            model.UnitPercent,
	endpointpb.EUnit_TemperatureKelvinUnit:  model.UnitTemperatureKelvin,
	endpointpb.EUnit_TemperatureCelsiusUnit: model.UnitTemperatureCelsius,
	endpointpb.EUnit_PressureAtmUnit:        model.UnitPressureAtm,
	endpointpb.EUnit_PressurePascalUnit:     model.UnitPressurePascal,
	endpointpb.EUnit_PressureBarUnit:        model.UnitPressureBar,
	endpointpb.EUnit_PressureMmHgUnit:       model.UnitPressureMmHg,
	endpointpb.EUnit_LuxUnit:                model.UnitIlluminationLux,
	endpointpb.EUnit_AmpereUnit:             model.UnitAmpere,
	endpointpb.EUnit_VoltUnit:               model.UnitVolt,
	endpointpb.EUnit_WattUnit:               model.UnitWatt,
}

var EndpointLevelInstanceToCapabilityInstanceMap = map[endpointpb.TLevelCapability_EInstance]model.RangeCapabilityInstance{
	endpointpb.TLevelCapability_TemperatureInstance: model.TemperatureRangeInstance,
	endpointpb.TLevelCapability_HumidityInstance:    model.HumidityRangeInstance,
	endpointpb.TLevelCapability_BrightnessInstance:  model.BrightnessRangeInstance,
	endpointpb.TLevelCapability_CoverInstance:       model.OpenRangeInstance,
}

var EndpointLevelInstanceToPropertyInstanceMap = map[endpointpb.TLevelCapability_EInstance]model.PropertyInstance{
	endpointpb.TLevelCapability_TemperatureInstance: model.TemperaturePropertyInstance,
	endpointpb.TLevelCapability_HumidityInstance:    model.HumidityPropertyInstance,
	endpointpb.TLevelCapability_PressureInstance:    model.PressurePropertyInstance,
	endpointpb.TLevelCapability_IlluminanceInstance: model.IlluminationPropertyInstance,
	endpointpb.TLevelCapability_TVOCInstance:        model.TvocPropertyInstance,
	endpointpb.TLevelCapability_AmperageInstance:    model.AmperagePropertyInstance,
	endpointpb.TLevelCapability_VoltageInstance:     model.VoltagePropertyInstance,
	endpointpb.TLevelCapability_PowerInstance:       model.PowerPropertyInstance,
}

// range capability utilities
func supportsRangeRandomAccess(directiveTypes []endpointpb.TCapability_EDirectiveType) bool {
	for _, directiveType := range directiveTypes {
		if directiveType == endpointpb.TCapability_SetAbsoluteLevelDirectiveType {
			return true
		}
	}
	return false
}

func getRange(r *endpointpb.TRange) *model.Range {
	if r == nil {
		return nil
	}
	return &model.Range{
		Min:       r.GetMin(),
		Max:       r.GetMax(),
		Precision: r.GetPrecision(),
	}
}

func isProperty(meta *endpointpb.TCapability_TMeta) bool {
	return len(meta.GetSupportedDirectives()) == 0
}
