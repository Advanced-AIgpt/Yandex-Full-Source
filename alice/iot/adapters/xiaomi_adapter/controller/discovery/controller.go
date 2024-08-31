package discovery

import (
	"context"
	"fmt"
	"runtime/debug"
	"sync"

	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/db"
	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi"
	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/iotapi"
	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/miotspec"
	xmodel "a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/model"
	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/userapi"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Controller struct {
	iotAPIClients  iotapi.APIClients
	userAPIClient  userapi.APIClient
	miotSpecClient miotspec.APIClient
	deviceBuilder  deviceBuilder
	database       db.DB
	logger         log.Logger
}

func NewController(logger log.Logger, buildWorkers int, apiConfig xiaomi.APIConfig, database db.DB) (c *Controller, stopFunc func()) {
	c = &Controller{
		iotAPIClients:  apiConfig.IOTAPIClients,
		userAPIClient:  apiConfig.UserAPIClient,
		miotSpecClient: apiConfig.MIOTSpecClient,
		database:       database,
		logger:         logger,
	}

	c.deviceBuilder, stopFunc = newDeviceBuilder(c.logger, c.miotSpecClient, buildWorkers)
	return c, stopFunc
}

func (c *Controller) getExternalUserID(ctx context.Context, token string) (string, error) {
	userProfile, err := c.userAPIClient.GetUserProfile(ctx, token)
	if err != nil {
		return "", xerrors.Errorf("unable to get user profile: %w", err)
	}
	return userProfile.Data.UnionID, nil
}

func (c *Controller) Discovery(ctx context.Context, token string, userID uint64) (string, []adapter.DeviceInfoView, error) {
	externalUserID, err := c.getExternalUserID(ctx, token)
	if err != nil {
		return "", nil, xerrors.Errorf("unable to get external user id: %w", err)
	}

	ctx = ctxlog.WithFields(ctx, log.String("external_user_id", externalUserID))

	if err := c.database.StoreExternalUser(ctx, externalUserID, userID); err != nil {
		return "", nil, xerrors.Errorf("unable to store external user: %w", err)
	}

	if err := c.database.StoreUserSubscriptions(ctx, externalUserID); err != nil {
		ctxlog.Warnf(ctx, c.logger, "unable to store user subscriptions: %v", err)
	}

	deviceInfoViews, err := c.discovery(ctx, token, externalUserID, noFilter{})
	if err != nil {
		return "", nil, xerrors.Errorf("discovery error: %w", err)
	}
	return externalUserID, deviceInfoViews, err
}

func (c *Controller) DiscoverDevicesByID(ctx context.Context, token, externalUserID string, deviceID string) ([]adapter.DeviceInfoView, error) {
	ctx = ctxlog.WithFields(ctx, log.String("external_user_id", externalUserID))
	return c.discovery(ctx, token, externalUserID, filterByDeviceID{deviceID: deviceID})
}

func (c *Controller) discovery(ctx context.Context, token, externalUserID string, filter deviceFilter) ([]adapter.DeviceInfoView, error) {
	devices := c.collectUserDevices(ctx, token, filter)
	deviceInfoViews := make([]adapter.DeviceInfoView, 0, len(devices))
	for _, device := range devices {
		propertyIDs, eventIDs, deviceInfoView, err := device.ToDeviceInfoViewWithSubscriptions()
		if err != nil {
			ctxlog.Info(ctx, c.logger,
				fmt.Sprintf("skipping device: %s", err),
				log.String("id", device.DID),
				log.String("region", device.Region.String()),
				log.String("xiaomi_type", device.Type),
			)
			continue
		}
		deviceCtx := ctxlog.WithFields(ctx,
			log.String("id", device.DID),
			log.String("region", device.Region.String()),
			log.String("yandex_type", deviceInfoView.Type.String()),
			log.String("xiaomi_type", device.Type),
		)

		if len(deviceInfoView.Capabilities) == 0 && len(deviceInfoView.Properties) == 0 {
			ctxlog.Info(deviceCtx, c.logger, "skipping device: no known capabilities or properties")
			continue
		}

		vctx := adapter.NewDiscoveryInfoViewValidationContext(deviceCtx, c.logger, model.XiaomiSkill, true)
		if _, err := deviceInfoView.Validate(vctx); err != nil {
			ctxlog.Warn(deviceCtx, c.logger, "bad device: validation failed", log.Any("error", err.Error()))
		} else {
			// event ids are not collected as they are not popular in devices at the moment
			err := c.database.StoreDeviceSubscriptions(deviceCtx, externalUserID, device, propertyIDs, eventIDs)
			if err != nil {
				ctxlog.Infof(deviceCtx, c.logger, "unable to store subscriptions: %v", err)
				continue
			}
			ctxlog.Info(deviceCtx, c.logger, "stored device subscriptions")
		}
		deviceInfoViews = append(deviceInfoViews, deviceInfoView)
	}
	return deviceInfoViews, nil
}

func (c *Controller) collectUserDevices(ctx context.Context, token string, filter deviceFilter) []xmodel.Device {
	// get user devices from different regions in one channel
	regionDevicesCh := c.userDevicesByRegion(ctx, token, filter)

	// collect user devices from all regions
	devices := make([]xmodel.Device, 0)
	for msg := range regionDevicesCh {
		if err := msg.err; err != nil {
			ctxlog.Warnf(ctx, c.logger, "error while discovering %s region devices: %v", msg.region, err)
			continue
		}
		devices = append(devices, msg.devices...)
	}

	// remove devices with duplicate ids, choose those with greater last update time
	type SeenDeviceIndex struct {
		index          int
		lastUpdateTime uint64
	}
	seenDevices := make(map[string]SeenDeviceIndex, len(devices))
	index := 0
	for _, device := range devices {
		if seenDevice, ok := seenDevices[device.DID]; ok {
			if seenDevice.lastUpdateTime < device.LastUpdateTime {
				ctxlog.Info(
					ctx, c.logger,
					"duplicate device",
					log.String("id", device.DID), log.String("region", device.Region.String()),
					log.String("type", device.Type), log.String("category", device.Category),
				)
				devices[seenDevice.index] = device
			}
			continue
		}
		seenDevices[device.DID] = SeenDeviceIndex{index: index, lastUpdateTime: device.LastUpdateTime}
		devices[index] = device
		index++
	}
	return devices[:index]
}

type regionDevicesMessage struct {
	region  iotapi.Region
	devices []xmodel.Device
	err     error
}

func (c *Controller) userDevicesByRegion(ctx context.Context, token string, filter deviceFilter) <-chan regionDevicesMessage {
	regionDevicesMessagesCh := make(chan regionDevicesMessage, len(c.iotAPIClients.Clients))
	var wg sync.WaitGroup
	for region := range c.iotAPIClients.Clients {
		wg.Add(1)
		go func(ctx context.Context, token string, region iotapi.Region) {
			defer wg.Done()
			defer func() {
				if r := recover(); r != nil {
					err := xerrors.Errorf("panic in discovering region %s devices: %v", region, r)
					ctxlog.Warn(ctx, c.logger, fmt.Sprintf("%v", err), log.Any("stacktrace", string(debug.Stack())))
					regionDevicesMessagesCh <- regionDevicesMessage{
						region: region,
						err:    err,
					}
				}
			}()
			devices, err := c.getRegionDevices(ctx, token, region, filter)
			regionDevicesMessagesCh <- regionDevicesMessage{
				region:  region,
				devices: devices,
				err:     err,
			}
		}(ctx, token, region)
	}
	go func() {
		wg.Wait()
		close(regionDevicesMessagesCh)
	}()
	return regionDevicesMessagesCh
}

func (c *Controller) getRegionDevices(ctx context.Context, token string, region iotapi.Region, filter deviceFilter) ([]xmodel.Device, error) {
	ctx = ctxlog.WithFields(ctx, log.String("region", region.String()))
	apiClient := c.iotAPIClients.GetAPIClient(region)

	apiHomes, err := apiClient.GetUserHomes(ctx, token)
	if err != nil {
		return nil, xerrors.Errorf("cannot get homes of UserRegionDevices: %w", err)
	}

	apiDevices, err := apiClient.GetUserDevices(ctx, token)
	if err != nil {
		return nil, xerrors.Errorf("cannot get devices of UserRegionDevices: %w", err)
	}

	filteredAPIDevices := make([]iotapi.Device, 0, len(apiDevices))
	for _, apiDevice := range apiDevices {
		if filter.skipDevice(apiDevice) {
			continue
		}
		filteredAPIDevices = append(filteredAPIDevices, apiDevice)
	}

	var resultDevices []xmodel.Device
	for msg := range c.buildRegionDevices(ctx, token, apiClient, filteredAPIDevices, apiHomes) {
		if err := msg.err; err != nil {
			ctxlog.Infof(ctx, c.logger, "unable to build device: %v", err)
			continue
		}
		resultDevices = append(resultDevices, msg.device.Split()...)
	}
	return resultDevices, nil
}

type deviceMessage struct {
	device xmodel.Device
	err    error
}

func (c *Controller) buildRegionDevices(ctx context.Context, token string, apiClient iotapi.APIClient, apiDevices []iotapi.Device, apiHomes []iotapi.Home) chan deviceMessage {
	filteredAPIDevices := make([]iotapi.Device, 0, len(apiDevices))
	for _, apiDevice := range apiDevices {
		if !xmodel.KnownDeviceCategories.IsKnown(apiDevice.Category) {
			ctxlog.Infof(ctx, c.logger, "skipped device %s: unsupported category: %s, type: %s", apiDevice.DID, apiDevice.Category, apiDevice.Type)
			continue
		}
		filteredAPIDevices = append(filteredAPIDevices, apiDevice)
	}

	var wg sync.WaitGroup
	wg.Add(len(filteredAPIDevices))

	devicesCh := make(chan deviceMessage, len(filteredAPIDevices))
	onDeviceProcessed := func(device xmodel.Device, err error) {
		defer wg.Done()
		devicesCh <- deviceMessage{device: device, err: err}
	}
	c.deviceBuilder.buildDevices(ctx, token, apiClient, filteredAPIDevices, apiHomes, onDeviceProcessed)
	go func() {
		wg.Wait()
		close(devicesCh)
	}()

	return devicesCh
}
