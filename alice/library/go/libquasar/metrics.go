package libquasar

import (
	"context"

	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/library/go/core/metrics"
)

type signals struct {
	getConfig         quasarmetrics.RouteSignalsWithTotal
	setConfigsBatch   quasarmetrics.RouteSignalsWithTotal
	iotDeviceInfo     quasarmetrics.RouteSignalsWithTotal
	createDeviceGroup quasarmetrics.RouteSignalsWithTotal
	updateDeviceGroup quasarmetrics.RouteSignalsWithTotal
	deleteDeviceGroup quasarmetrics.RouteSignalsWithTotal
	encrypt           quasarmetrics.RouteSignalsWithTotal
}

func (s signals) GetSignal(context context.Context) quasarmetrics.RouteSignalsWithTotal {
	switch context.Value(signalKey).(ctxMetricsMethod) {
	case callGetConfig:
		return s.getConfig
	case callIotDeviceInfo:
		return s.iotDeviceInfo
	case callSetConfigsBatch:
		return s.setConfigsBatch
	case callCreateDeviceGroup:
		return s.createDeviceGroup
	case callUpdateDeviceGroup:
		return s.updateDeviceGroup
	case callDeleteDeviceGroup:
		return s.deleteDeviceGroup
	case callEncrypt:
		return s.encrypt
	default:
		return nil
	}
}

func NewSignals(registry metrics.Registry) quasarmetrics.Signals {
	return signals{
		getConfig: quasarmetrics.NewRouteSignalsWithTotal(
			registry.WithTags(map[string]string{"call": "get_device_config"}),
			quasarmetrics.DefaultExponentialBucketsPolicy()),
		iotDeviceInfo: quasarmetrics.NewRouteSignalsWithTotal(
			registry.WithTags(map[string]string{"call": "iot_device_info"}),
			quasarmetrics.DefaultExponentialBucketsPolicy()),
		setConfigsBatch: quasarmetrics.NewRouteSignalsWithTotal(
			registry.WithTags(map[string]string{"call": "set_device_config_batch"}),
			quasarmetrics.DefaultExponentialBucketsPolicy()),
		createDeviceGroup: quasarmetrics.NewRouteSignalsWithTotal(
			registry.WithTags(map[string]string{"call": "create_device_group"}),
			quasarmetrics.DefaultExponentialBucketsPolicy()),
		updateDeviceGroup: quasarmetrics.NewRouteSignalsWithTotal(
			registry.WithTags(map[string]string{"call": "update_device_group"}),
			quasarmetrics.DefaultExponentialBucketsPolicy()),
		deleteDeviceGroup: quasarmetrics.NewRouteSignalsWithTotal(
			registry.WithTags(map[string]string{"call": "delete_device_group"}),
			quasarmetrics.DefaultExponentialBucketsPolicy()),
	}
}
