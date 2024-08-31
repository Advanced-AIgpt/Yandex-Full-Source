package main

import (
	"context"
	"sync"

	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/iotapi"
	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/miotspec"
	xmodel "a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/model"
	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/userapi"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
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
	userAPIClient    userapi.APIClient
	miotSpecClient   miotspec.APIClient
	Logger           log.Logger
}

type userDevicesInfo struct {
	devicesXmodel   []xmodel.Device
	devicesInfoView []adapter.DeviceInfoView
}

func (c *Checker) Init(appID string) {
	c.Logger.Info("Initializing Checker")

	c.appID = appID
	c.discoveryWorkers = 10

	mockRegistry := mock.NewRegistry(mock.NewRegistryOpts())
	chinaIOTAPIClient := iotapi.NewClientWithMetrics(logger, appID, iotapi.ChinaRegion, "", mockRegistry, zora.NewClient(tvm.ClientMock{}))
	ruIOTAPIClient := iotapi.NewClientWithMetrics(logger, appID, iotapi.RussiaRegion, "", mockRegistry, zora.NewClient(tvm.ClientMock{}))
	europeIOTAPIClient := iotapi.NewClientWithMetrics(logger, appID, iotapi.EuropeRegion, "", mockRegistry, zora.NewClient(tvm.ClientMock{}))
	singaporeIOTAPIClient := iotapi.NewClientWithMetrics(logger, appID, iotapi.SingaporeRegion, "", mockRegistry, zora.NewClient(tvm.ClientMock{}))
	usWestIOTAPIClient := iotapi.NewClientWithMetrics(logger, appID, iotapi.USWestRegion, "", mockRegistry, zora.NewClient(tvm.ClientMock{}))
	c.iotAPIClients = map[iotapi.Region]iotapi.APIClient{
		iotapi.ChinaRegion:     chinaIOTAPIClient,
		iotapi.RussiaRegion:    ruIOTAPIClient,
		iotapi.EuropeRegion:    europeIOTAPIClient,
		iotapi.SingaporeRegion: singaporeIOTAPIClient,
		iotapi.USWestRegion:    usWestIOTAPIClient,
	}

	userAPIClient := userapi.NewClientWithMetrics(logger, c.appID, mockRegistry, zora.NewClient(tvm.ClientMock{}))
	c.userAPIClient = userAPIClient

	miotSpecClient := miotspec.NewClientWithMetrics(logger, mockRegistry, zora.NewClient(tvm.ClientMock{}))
	c.miotSpecClient = miotSpecClient

	c.Logger.Info("Checker was successfully initialized")
}

func (c *Checker) GetUserDevicesInfo(ctx context.Context, token string) (map[iotapi.Region]userDevicesInfo, error) {
	type regionDevicesMessage struct {
		region        iotapi.Region
		devicesXmodel []xmodel.Device
		err           error
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
					err := xerrors.Errorf("Panic in discovering region %s devicesXmodel: %v", client.GetRegion(), r)
					ctxlog.Warnf(ctx, c.Logger, "%v", err)
					ch <- regionDevicesMessage{region: client.GetRegion(), err: err}
				}
			}()

			ctxlog.Debugf(ctx, c.Logger, "Discovering %s region devicesXmodel", client.GetRegion())
			regionDevices, err := c.getUserRegionDevices(ctx, token, client)

			ch <- regionDevicesMessage{
				region:        client.GetRegion(),
				devicesXmodel: regionDevices,
				err:           err,
			}
		}(ctx, token, apiClient)
	}

	go func() {
		wg.Wait()
		close(ch)
	}()

	rawDevices := make(map[iotapi.Region][]xmodel.Device)
	for msg := range ch {
		if err := msg.err; err != nil {
			ctxlog.Warnf(ctx, c.Logger, "Error in discovering %s region devicesXmodel: %v", msg.region, err)
			continue
		}
		rawDevices[msg.region] = append(rawDevices[msg.region], msg.devicesXmodel...)
	}

	userDevicesInfoByRegion := make(map[iotapi.Region]userDevicesInfo)
	for region := range c.iotAPIClients {
		rawDevices[region] = c.deduplicateDevices(ctx, rawDevices[region])
		devicesInfoViewRegion := make([]adapter.DeviceInfoView, len(rawDevices[region]))
		for i, deviceRaw := range rawDevices[region] {
			_, _, divRegion, err := deviceRaw.ToDeviceInfoViewWithSubscriptions()
			if err != nil {
				ctxlog.Warnf(ctx, c.Logger, "Error in discovering %s region device %s: %s", region, deviceRaw.DID, err)
				continue
			}
			devicesInfoViewRegion[i] = divRegion
		}
		if len(rawDevices[region])+len(devicesInfoViewRegion) != 0 {
			userDevicesInfoByRegion[region] = userDevicesInfo{rawDevices[region], devicesInfoViewRegion}
		}
	}

	return userDevicesInfoByRegion, nil
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
				ctxlog.Infof(ctx, regionLogger, "Panic in populating discovery workers with data: %v", r)
			}
		}()
		for _, device := range devices {
			if !xmodel.KnownDeviceCategories.IsKnown(device.Category) {
				ctxlog.Infof(ctx, regionLogger, "Device %s has unsupported category: %s, type: %s.", device.DID, device.Type, device.Category)
			}
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
					err := xerrors.Errorf("Panic in getting xmodel devicesXmodel: %v", r)
					regionLogger.Warnf("%v", err)
					modelDevicesCh <- modelDevicesMessage{err: err}
				}
			}()
			for apiDevice := range apiDevicesCh {
				regionLogger.Infof("Getting xmodel device %s", apiDevice.DID)
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
			regionLogger.Infof("Unable to get xmodel devicesXmodel, reason: %v", err)
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

//deduplicate devicesXmodel by device.Id, choose one with greater LastUpdateTime
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
