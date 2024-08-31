package main

import (
	"context"
	"fmt"
	"runtime/debug"
	"sync"

	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/iotapi"
	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/miotspec"
	xmodel "a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/model"
	"a.yandex-team.ru/alice/library/go/tvm"
	"a.yandex-team.ru/alice/library/go/zora"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/metrics/mock"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Checker struct {
	appID            string
	discoveryWorkers int
	iotAPIClients    map[iotapi.Region]iotapi.APIClient
	miotSpecClient   miotspec.APIClient
	Logger           log.Logger
}

func (c *Checker) Init(appID string) {
	c.Logger.Info("Initializing Checker")

	c.appID = appID
	c.discoveryWorkers = 10

	mockRegistry := mock.NewRegistry(mock.NewRegistryOpts())
	chinaIOTAPIClient := iotapi.NewClientWithMetrics(logger, appID, iotapi.ChinaRegion, "", mockRegistry, zora.NewClient(tvm.ClientMock{}))
	ruIOTAPIClient := iotapi.NewClientWithMetrics(logger, appID, iotapi.RussiaRegion, "", mockRegistry, zora.NewClient(tvm.ClientMock{}))
	c.iotAPIClients = map[iotapi.Region]iotapi.APIClient{
		iotapi.ChinaRegion:  chinaIOTAPIClient,
		iotapi.RussiaRegion: ruIOTAPIClient,
	}

	miotSpecClient := miotspec.NewClientWithMetrics(logger, mockRegistry, zora.NewClient(tvm.ClientMock{}))
	c.miotSpecClient = miotSpecClient

	c.Logger.Info("Checker was successfully initialized")
}

func (c *Checker) GetUserDevices(ctx context.Context, token string) (map[iotapi.Region]xmodel.Devices, error) {
	type regionDevicesMessage struct {
		region  iotapi.Region
		devices []xmodel.Device
		err     error
	}
	ch := make(chan regionDevicesMessage)
	var wg sync.WaitGroup

	//iterate over regions
	for _, apiClient := range c.iotAPIClients {
		wg.Add(1)
		go func(ctx context.Context, token string, client iotapi.APIClient) {
			defer wg.Done()
			defer func() {
				if r := recover(); r != nil {
					err := xerrors.Errorf("caught panic in discovering region %s devicesXmodel: %v", client.GetRegion(), r)
					ctxlog.Warn(ctx, c.Logger, fmt.Sprintf("%v", err), log.Any("stacktrace", string(debug.Stack())))
					ch <- regionDevicesMessage{region: client.GetRegion(), err: err}
				}
			}()

			ctxlog.Debugf(ctx, c.Logger, "Discovering %s region devices", client.GetRegion())
			regionDevices, err := c.getUserRegionDevices(ctx, token, client)

			ch <- regionDevicesMessage{
				region:  client.GetRegion(),
				devices: regionDevices,
				err:     err,
			}
		}(ctx, token, apiClient)
	}

	go func() {
		wg.Wait()
		close(ch)
	}()

	userDevicesByRegion := make(map[iotapi.Region]xmodel.Devices)
	for msg := range ch {
		if err := msg.err; err != nil {
			ctxlog.Warnf(ctx, c.Logger, "Error in discovering %s region devices: %v", msg.region, err)
			continue
		}
		userDevicesByRegion[msg.region] = c.deduplicateDevices(ctx, msg.devices)
	}
	return userDevicesByRegion, nil
}

func (c *Checker) getUserRegionDevices(ctx context.Context, token string, apiClient iotapi.APIClient) ([]xmodel.Device, error) {
	regionLogger := log.With(c.Logger, log.Any("region", apiClient.GetRegion()))

	//get homes
	homes, err := apiClient.GetUserHomes(ctx, token)
	if err != nil {
		return nil, xerrors.Errorf("cannot get homes of UserRegionDevices: %w", err)
	}

	//get devices
	devices, err := apiClient.GetUserDevices(ctx, token)
	if err != nil {
		return nil, xerrors.Errorf("cannot get devices of UserRegionDevices: %w", err)
	}

	// populate workers with data
	apiDevicesCh := make(chan iotapi.Device)
	go func() {
		defer close(apiDevicesCh)
		defer func() {
			if r := recover(); r != nil {
				ctxlog.Warn(ctx, regionLogger, fmt.Sprintf("caught panic in populating discovery workers with data: %v", r), log.Any("stacktrace", string(debug.Stack())))
			}
		}()
		for _, device := range devices {
			apiDevicesCh <- device
		}
	}()

	// start workers
	type modelDevicesMessage struct {
		devices []xmodel.Device
		err     error
	}
	modelDevicesCh := make(chan modelDevicesMessage)
	var wg sync.WaitGroup
	for i := 0; i < c.discoveryWorkers; i++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			defer func() {
				if r := recover(); r != nil {
					err := xerrors.Errorf("caught panic in getting xmodel devicesXmodel: %v", r)
					ctxlog.Warn(ctx, regionLogger, fmt.Sprintf("%v", err), log.Any("stacktrace", string(debug.Stack())))
					modelDevicesCh <- modelDevicesMessage{err: err}
				}
			}()
			for apiDevice := range apiDevicesCh {
				device, err := c.getDevice(ctx, token, apiClient, apiDevice, homes)
				if err != nil {
					modelDevicesCh <- modelDevicesMessage{err: xerrors.Errorf("Unable to get xmodel device %s, reason: %w", apiDevice.DID, err)}
					continue
				}
				modelDevicesCh <- modelDevicesMessage{devices: device.Split()} // Split xmodel devicesXmodel
			}
		}()
	}

	// close workers output channel
	go func() {
		wg.Wait()
		close(modelDevicesCh)
	}()

	// combine regionInfo
	var userRegionDevices []xmodel.Device
	for msg := range modelDevicesCh {
		if err := msg.err; err != nil {
			regionLogger.Warnf("Unable to get xmodel devicesXmodel, reason: %v", err)
			continue
		}
		userRegionDevices = append(userRegionDevices, msg.devices...)
	}
	return userRegionDevices, nil
}

func (c *Checker) getDevice(ctx context.Context, token string, apiClient iotapi.APIClient, apiDevice iotapi.Device, homes []iotapi.Home) (xmodel.Device, error) {
	//fill in device and room
	var device xmodel.Device
	device.PopulateDeviceData(apiDevice)
	device.PopulateRoomData(apiDevice, homes)
	device.PopulateRegion(apiClient.GetRegion())

	//fill in services
	services, err := c.miotSpecClient.GetDeviceServices(ctx, apiDevice.Type)
	if err != nil {
		return xmodel.Device{}, xerrors.Errorf("cannot get device Spec: %w", err)
	}
	device.PopulateServices(services)

	//fill in deviceInfo
	if deviceInfoPropIDs := device.GetDeviceInfoPropertyIDs(); len(deviceInfoPropIDs) > 0 {
		deviceInfoProperties, err := apiClient.GetProperties(ctx, token, deviceInfoPropIDs...)
		if err != nil {
			return xmodel.Device{}, xerrors.Errorf("cannot get device properties: %w", err)
		}
		device.PopulatePropertyStates(deviceInfoProperties)
	}
	return device, nil
}

//deduplicate devices by device.Id, choose one with greater LastUpdateTime
func (c *Checker) deduplicateDevices(ctx context.Context, ds []xmodel.Device) []xmodel.Device {
	type T struct {
		index int
		time  uint64
	}

	seen := make(map[string]T, len(ds))
	j := 0
	for _, v := range ds {
		if t, ok := seen[v.DID]; ok {
			if t.time < v.LastUpdateTime {
				ctxlog.Debugf(ctx, c.Logger, "Deduplicating device %s", v.DID)
				ds[t.index] = v
			}
			continue
		}
		seen[v.DID] = T{j, v.LastUpdateTime}
		ds[j] = v
		j++
	}
	return ds[:j]
}
