package discovery

import (
	"context"
	"fmt"
	"runtime/debug"

	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/iotapi"
	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/miotspec"
	xmodel "a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/model"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type deviceBuilder struct {
	logger         log.Logger
	miotSpecClient miotspec.APIClient

	workers   int
	workersCh chan buildDeviceMessage
}

type buildDeviceMessage struct {
	ctx         context.Context
	token       string
	apiClient   iotapi.APIClient
	apiDevice   iotapi.Device
	apiHomes    []iotapi.Home
	onProcessed func(xmodel.Device, error)
}

func newDeviceBuilder(logger log.Logger, client miotspec.APIClient, workers int) (builder deviceBuilder, stopFunc func()) {
	builder = deviceBuilder{
		logger:         logger,
		miotSpecClient: client,
		workers:        workers,
		workersCh:      make(chan buildDeviceMessage, workers),
	}
	builder.Start()
	return builder, builder.Stop
}

func (builder *deviceBuilder) Start() {
	for i := 0; i < builder.workers; i++ {
		go func() {
			defer func() {
				if r := recover(); r != nil {
					builder.logger.Warn(fmt.Sprintf("panic in device builder worker: %v", r), log.Any("stacktrace", string(debug.Stack())))
				}
			}()

			for msg := range builder.workersCh {
				device, err := builder.buildDevice(msg.ctx, msg.token, msg.apiClient, msg.apiDevice, msg.apiHomes)
				msg.onProcessed(device, err)
			}
		}()
	}
}

func (builder *deviceBuilder) Stop() {
	close(builder.workersCh)
}

func (builder *deviceBuilder) buildDevices(ctx context.Context, token string, apiClient iotapi.APIClient, apiDevices []iotapi.Device, apiHomes []iotapi.Home, onProcessed func(xmodel.Device, error)) {
	for _, apiDevice := range apiDevices {
		builder.workersCh <- buildDeviceMessage{ctx, token, apiClient, apiDevice, apiHomes, onProcessed}
	}
}

func (builder *deviceBuilder) buildDevice(ctx context.Context, token string, apiClient iotapi.APIClient, apiDevice iotapi.Device, apiHomes []iotapi.Home) (xmodel.Device, error) {
	var device xmodel.Device
	device.PopulateDeviceData(apiDevice)
	device.PopulateRoomData(apiDevice, apiHomes)
	device.PopulateRegion(apiClient.GetRegion())

	services, err := builder.miotSpecClient.GetDeviceServices(ctx, apiDevice.Type)
	if err != nil {
		return xmodel.Device{}, xerrors.Errorf("cannot get device Spec: %w", err)
	}
	device.PopulateServices(services)

	if deviceInfoPropIDs := device.GetDeviceInfoPropertyIDs(); len(deviceInfoPropIDs) > 0 {
		deviceInfoProperties, err := apiClient.GetProperties(ctx, token, deviceInfoPropIDs...)
		if err != nil {
			return xmodel.Device{}, xerrors.Errorf("cannot get device properties: %w", err)
		}
		device.PopulatePropertyStates(deviceInfoProperties)
	}
	return device, nil
}
