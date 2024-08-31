package iotapi

import (
	"context"

	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/library/go/core/metrics"
)

type signals struct {
	getUserDevices               quasarmetrics.RouteSignalsWithTotal
	getUserHomes                 quasarmetrics.RouteSignalsWithTotal
	getUserDeviceInfo            quasarmetrics.RouteSignalsWithTotal
	getProperties                quasarmetrics.RouteSignalsWithTotal
	setProperties                quasarmetrics.RouteSignalsWithTotal
	setActions                   quasarmetrics.RouteSignalsWithTotal
	subscribeToPropertiesChanged quasarmetrics.RouteSignalsWithTotal
	subscribeToUserEvents        quasarmetrics.RouteSignalsWithTotal
	subscribeToDeviceEvents      quasarmetrics.RouteSignalsWithTotal
}

func (s signals) GetSignal(context context.Context) quasarmetrics.RouteSignalsWithTotal {
	switch context.Value(signalKey) {
	case getUserDevicesSignal:
		return s.getUserDevices
	case getUserHomesSignal:
		return s.getUserHomes
	case getUserDeviceInfoSignal:
		return s.getUserDeviceInfo
	case getPropertiesSignal:
		return s.getProperties
	case setPropertiesSignal:
		return s.setProperties
	case setActionsSignal:
		return s.setActions
	case subscribeToPropertiesChangedSignal:
		return s.subscribeToPropertiesChanged
	case subscribeToUserEventsSignal:
		return s.subscribeToUserEvents
	case subscribeToDeviceEventsSignal:
		return s.subscribeToDeviceEvents
	default:
		return nil
	}
}

func newSignals(registry metrics.Registry) signals {
	return signals{
		getUserDevices: quasarmetrics.NewRouteSignalsWithTotal(
			registry.WithTags(map[string]string{"api": "iot", "call": "get_user_devices"}),
			quasarmetrics.DefaultExponentialBucketsPolicy(),
		),
		getUserHomes: quasarmetrics.NewRouteSignalsWithTotal(
			registry.WithTags(map[string]string{"api": "iot", "call": "get_user_homes"}),
			quasarmetrics.DefaultExponentialBucketsPolicy(),
		),
		getUserDeviceInfo: quasarmetrics.NewRouteSignalsWithTotal(
			registry.WithTags(map[string]string{"api": "iot", "call": "get_user_device_info"}),
			quasarmetrics.DefaultExponentialBucketsPolicy(),
		),
		getProperties: quasarmetrics.NewRouteSignalsWithTotal(
			registry.WithTags(map[string]string{"api": "iot", "call": "get_properties"}),
			quasarmetrics.DefaultExponentialBucketsPolicy(),
		),
		setProperties: quasarmetrics.NewRouteSignalsWithTotal(
			registry.WithTags(map[string]string{"api": "iot", "call": "set_properties"}),
			quasarmetrics.DefaultExponentialBucketsPolicy(),
		),
		setActions: quasarmetrics.NewRouteSignalsWithTotal(
			registry.WithTags(map[string]string{"api": "iot", "call": "set_actions"}),
			quasarmetrics.DefaultExponentialBucketsPolicy(),
		),
		subscribeToPropertiesChanged: quasarmetrics.NewRouteSignalsWithTotal(
			registry.WithTags(map[string]string{"api": "iot", "call": "subscribe_to_properties_changed"}),
			quasarmetrics.DefaultExponentialBucketsPolicy(),
		),
		subscribeToUserEvents: quasarmetrics.NewRouteSignalsWithTotal(
			registry.WithTags(map[string]string{"api": "iot", "call": "subscribe_to_user_events"}),
			quasarmetrics.DefaultExponentialBucketsPolicy(),
		),
		subscribeToDeviceEvents: quasarmetrics.NewRouteSignalsWithTotal(
			registry.WithTags(map[string]string{"api": "iot", "call": "subscribe_to_device_events"}),
			quasarmetrics.DefaultExponentialBucketsPolicy(),
		),
	}
}
