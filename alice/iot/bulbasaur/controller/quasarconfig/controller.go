package quasarconfig

import (
	"context"
	"sync"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/quasar"
	"a.yandex-team.ru/alice/library/go/goroutines"
	"a.yandex-team.ru/alice/library/go/libquasar"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Controller struct {
	db     db.DB
	client libquasar.IClient
	logger log.Logger
}

func NewController(client libquasar.IClient, db db.DB, logger log.Logger) *Controller {
	return &Controller{
		client: client,
		db:     db,
		logger: logger,
	}
}

func (c *Controller) CreateStereopair(ctx context.Context, user model.User, stereopairConfig model.StereopairConfig) (stereopair model.Stereopair, err error) {
	ctxlog.Infof(ctx, c.logger, "Creating stereopair")

	if _, err = stereopairConfig.Validate(nil); err != nil {
		return model.Stereopair{}, xerrors.Errorf("invalid stereopair config: %w", err)
	}

	return stereopair, c.db.Transaction(ctx, "create-stereopair", func(ctx context.Context) error {
		devices, err := c.db.SelectUserDevicesSimple(ctx, user.ID)
		if err != nil {
			return xerrors.Errorf("failed to select devices: %w", err)
		}

		stereopairs, err := c.db.SelectStereopairs(ctx, user.ID)
		if err != nil {
			return xerrors.Errorf("failed to get stereopairs: %w", err)
		}

		devicesMap := devices.ToMap()

		// set stereopair var out of function
		err = stereopair.New(stereopairConfig, devicesMap, stereopairs, timestamp.CurrentTimestampCtx(ctx))
		if err != nil {
			return xerrors.Errorf("failed to create stereopair object from config: %w", err)
		}

		stereopairQuasarDevicesData := stereopair.Devices.ToQuasarCustomDataMap()
		stereopairQuasarIDs := make([]string, 0, len(stereopairQuasarDevicesData))
		for _, quasarInfo := range stereopairQuasarDevicesData {
			stereopairQuasarIDs = append(stereopairQuasarIDs, quasarInfo.DeviceID)
		}
		leaderQuasarID := stereopairQuasarDevicesData[stereopair.Config.GetLeaderID()].DeviceID

		err = c.client.UpdateDeviceConfigs(ctx, user.Ticket, stereopairQuasarIDs, func(configs map[string]*libquasar.Config) error {
			leaderConfig, leaderConfigExist := configs[leaderQuasarID]
			if !leaderConfigExist {
				return xerrors.Errorf("failed to get leader config from map: %q", leaderQuasarID)
			}

			leftSpConfig := stereopair.Config.GetByChannel(model.LeftChannel)
			leftQuasarInfo, ok := stereopairQuasarDevicesData[leftSpConfig.ID]
			if !ok {
				return xerrors.Errorf("failed to get stereopair left channel device quasar info: %q", leftSpConfig.ID)
			}
			leftDevice, _ := stereopair.Devices.GetDeviceByID(leftSpConfig.ID)

			rightSpConfig := stereopair.Config.GetByChannel(model.RightChannel)
			rightQuasarInfo, ok := stereopairQuasarDevicesData[rightSpConfig.ID]
			if !ok {
				return xerrors.Errorf("failed to get stereopair right channel device quasar info: %q", rightSpConfig.ID)
			}
			rightDevice, _ := stereopair.Devices.GetDeviceByID(rightSpConfig.ID)

			leftNewQuasarConfig, rightNewQuasarConfig := leaderConfig.SplitConfigForStereopair(
				leftQuasarInfo.DeviceID, leftDevice.Name, string(leftSpConfig.Role),
				rightQuasarInfo.DeviceID, rightDevice.Name, string(rightSpConfig.Role),
			)
			if _, exist := configs[leftQuasarInfo.DeviceID]; !exist {
				return xerrors.Errorf("doesn't exist left quasar config: %q", leftQuasarInfo.DeviceID)
			}
			*configs[leftQuasarInfo.DeviceID] = leftNewQuasarConfig

			if _, exist := configs[rightQuasarInfo.DeviceID]; !exist {
				return xerrors.Errorf("doesn't exist right quasar config: %q", rightQuasarInfo.DeviceID)
			}
			*configs[rightQuasarInfo.DeviceID] = rightNewQuasarConfig
			return nil
		})

		if err != nil {
			return quasarErrorToModelError(ctx, c.logger, xerrors.Errorf("failed to update devices config in quasar backend: %w", err))
		}

		if err = c.db.StoreStereopair(ctx, user.ID, stereopair); err != nil {
			return xerrors.Errorf("failed to store stereopair config: %w", err)
		}
		return nil
	})
}

func (c *Controller) DeleteStereopair(ctx context.Context, user model.User, stereopairID string) (err error) {
	ctxlog.Info(ctx, c.logger, "delete stereopair", log.String("stereopair_id", stereopairID))

	return c.db.Transaction(ctx, "delete-stereopair", func(ctx context.Context) error {
		stereopair, err := c.db.SelectStereopair(ctx, user.ID, stereopairID)
		if err != nil {
			return xerrors.Errorf("failed to select stereopair: %w", err)
		}

		quasarCustomDataMap := stereopair.Devices.ToQuasarCustomDataMap()

		quasarIDs := make([]string, 0, len(quasarCustomDataMap))
		for _, quasarCustomData := range quasarCustomDataMap {
			quasarIDs = append(quasarIDs, quasarCustomData.DeviceID)
		}

		err = c.client.UpdateDeviceConfigs(ctx, user.Ticket, quasarIDs, func(configs map[string]*libquasar.Config) error {
			for _, config := range configs {
				config.DeleteStereopairConfig()
			}
			return nil
		})

		if err != nil {
			modelErr := quasarErrorToModelError(ctx, c.logger, xerrors.Errorf("failed to set device config: %w", err))
			if !xerrors.Is(modelErr, &model.DeviceNotFoundError{}) {
				return modelErr
			}
			// ALICE-17719: cannot delete stereopair if one of the devices is not owned by user
			// best effort here: try to delete every device config on its own
			// quite rare case
			hasAtLeastOneSuccess := false
			quasarConfigsDeletionResults := make(map[string]bool, len(quasarIDs))
			for _, quasarID := range quasarIDs {
				quasarIDErr := c.client.UpdateDeviceConfigs(ctx, user.Ticket, []string{quasarID}, func(configs map[string]*libquasar.Config) error {
					for _, config := range configs {
						config.DeleteStereopairConfig()
					}
					return nil
				})
				quasarConfigsDeletionResults[quasarID] = quasarIDErr == nil
				if quasarIDErr != nil {
					ctxlog.Warnf(ctx, c.logger, "failed to update quasar device %s config: %v", quasarID, quasarIDErr)
				} else {
					hasAtLeastOneSuccess = true
				}
			}
			if !hasAtLeastOneSuccess {
				return modelErr
			}
			ctxlog.Info(ctx, c.logger, "deleted stereopair config from available devices, deleting stereopair from our db here", log.Any("config_deletion_results", quasarConfigsDeletionResults))
		}

		if err = c.db.DeleteStereopair(ctx, user.ID, stereopairID); err != nil {
			return quasarErrorToModelError(ctx, c.logger, xerrors.Errorf("failed to delete stereopair from db: %w", err))
		}
		return nil
	})
}

func (c *Controller) DeviceConfig(ctx context.Context, user model.User, deviceID string) (_ DeviceConfig, err error) {
	ctxlog.Info(ctx, c.logger, "get quasar device config", log.String("device_id", deviceID))

	stereopairs, err := c.db.SelectStereopairs(ctx, user.ID)
	if err != nil {
		return DeviceConfig{}, xerrors.Errorf("failed to select stereopairs from db: %w", err)
	}

	if stereopair, ok := stereopairs.GetByDeviceID(deviceID); ok {
		return c.deviceConfigStereopair(ctx, user, stereopair, deviceID)
	}

	return c.deviceConfigQuasar(ctx, user, deviceID)
}

func (c *Controller) DeviceInfos(ctx context.Context, user model.User) (DeviceInfos, error) {
	iotDeviceInfos, err := c.client.IotDeviceInfos(ctx, user.Ticket, []string{})
	if err != nil {
		return nil, quasarErrorToModelError(ctx, c.logger, xerrors.Errorf("failed to get iot device infos from quasar: %w", err))
	}
	userDevices, err := c.db.SelectUserDevicesSimple(ctx, user.ID)
	if err != nil {
		return nil, xerrors.Errorf("failed to get user %d devices: %w", user.ID, err)
	}

	stereopairs, err := c.db.SelectStereopairs(ctx, user.ID)
	if err != nil {
		return nil, xerrors.Errorf("failed to get user %d stereopairs: %w", user.ID, err)
	}

	deviceInfos := make(DeviceInfos, 0, len(iotDeviceInfos))
	for _, iotDeviceInfo := range iotDeviceInfos {
		var deviceInfo DeviceInfo
		deviceInfo.FromIotDeviceInfo(iotDeviceInfo, userDevices)
		deviceInfos = append(deviceInfos, deviceInfo)
	}
	deviceInfos.EncodeVersions(stereopairs)
	return deviceInfos, nil
}

func (c *Controller) SetDeviceConfig(ctx context.Context, user model.User, deviceID string, fromVersion string, config libquasar.Config) (version string, err error) {
	ctxlog.Infof(ctxlog.WithFields(ctx, log.String("device_id", deviceID)), c.logger, "Set quasar device config")

	stereopairs, err := c.db.SelectStereopairs(ctx, user.ID)
	if err != nil {
		return "", xerrors.Errorf("failed to select stereopairs from db: %w", err)
	}

	var configs []libquasar.SetDeviceConfig
	if stereopair, ok := stereopairs.GetByDeviceID(deviceID); ok {
		configs, err = c.genStereopairConfigs(ctx, user.ID, stereopair, fromVersion, config)
	} else {
		configs, err = c.genQuasarDeviceClientConfig(ctx, user.ID, deviceID, fromVersion, config)
	}
	if err != nil {
		return "", xerrors.Errorf("failed to  generate client configs: %w", err)
	}

	cRes, err := c.client.SetDeviceConfigs(ctx, user.Ticket, libquasar.SetDevicesConfigPayload{Devices: configs})
	if err != nil {
		return "", quasarErrorToModelError(ctx, c.logger, xerrors.Errorf("failed to set quasar config: %w", err))
	}

	if cRes.Status != "ok" {
		return "", xerrors.Errorf("failed to set quasar config, recieve response with status status: %q", cRes.Status)
	}

	return cRes.Devices.JoinVersions(), nil
}

func (c *Controller) SetStereopairChannels(ctx context.Context, user model.User, stereopair model.Stereopair, deviceChannels []DeviceChannel) error {
	return c.db.Transaction(ctx, "stereopair-set-channels", func(ctx context.Context) error {
		stereopairs, err := c.db.SelectStereopairs(ctx, user.ID)
		if err != nil {
			return xerrors.Errorf("failed to load stereopairs: %w", err)
		}

		stereopair, ok := stereopairs.GetByID(stereopair.ID)
		if !ok {
			return &model.DeviceNotFoundError{}
		}

		for _, deviceChannel := range deviceChannels {
			if cfg, exist := stereopair.Config.Devices.GetConfigByID(deviceChannel.DeviceID); exist {
				cfg.Channel = deviceChannel.Channel
				if err = stereopair.Config.Devices.SetConfig(cfg); err != nil {
					return xerrors.Errorf("failed to set device config in stereopair: device %q, error: %w", deviceChannel.DeviceID, err)
				}
			} else {
				return xerrors.Errorf("failed to find device in stereopair: %q", deviceChannel.DeviceID)
			}
		}

		if _, err = stereopair.Config.Validate(nil); err != nil {
			return xerrors.Errorf("failed to validate stereopair config after apply changes: %w", err)
		}

		if err = c.db.StoreStereopair(ctx, user.ID, stereopair); err != nil {
			return xerrors.Errorf("failed to store stereopair: %w", err)
		}

		err = c.pushStereopairConfig(ctx, user, stereopair)
		if err != nil {
			return xerrors.Errorf("failed to push stereopair config to quasar backend: %w", err)
		}
		return nil
	})

}

func (c *Controller) CreateTandem(ctx context.Context, user model.User, display model.Device, speaker model.Device) error {
	if !display.IsTandemDisplay() {
		return xerrors.Errorf("failed to create tandem of display %q and speaker %q: %q is not display", display.ID, speaker.ID, display.ID)
	}
	if !speaker.IsTandemSpeaker() {
		return xerrors.Errorf("failed to create tandem of display %q and speaker %q: %q is not speaker", display.ID, speaker.ID, speaker.ID)
	}
	if !display.IsTandemCompatibleWith(speaker) {
		return xerrors.Errorf("failed to create tandem of device %q and device %q: not compatible", display.ID, speaker.ID)
	}
	ctxlog.Infof(ctx, c.logger, "Creating tandem for display %q and speaker %q", display.ID, speaker.ID)

	deviceInfos, err := c.DeviceInfos(ctx, user)
	if err != nil {
		return xerrors.Errorf("failed to get device infos for user %d: %w", user.ID, err)
	}

	err = c.db.Transaction(ctx, "create-tandem", func(ctx context.Context) error {
		stereopairs, err := c.db.SelectStereopairs(ctx, user.ID)
		if err != nil {
			return xerrors.Errorf("failed to select stereopairs for user %d: %w", user.ID, err)
		}

		if err := CanCreateTandem(display, speaker, stereopairs, deviceInfos); err != nil {
			return xerrors.Errorf("failed to create tandem for user %d: %w", user.ID, err)
		}
		if deviceIsInTandem(display, deviceInfos) {
			return xerrors.Errorf("failed to create tandem for user %d: device %s is already in tandem", user.ID, display.ID)
		}
		if deviceIsInTandem(speaker, deviceInfos) {
			return xerrors.Errorf("failed to create tandem for user %d: device %s is already in tandem", user.ID, speaker.ID)
		}

		request := newGroupCreateRequest(display, speaker)
		response, err := c.client.CreateDeviceGroup(ctx, user.Ticket, request)
		if err != nil {
			return quasarErrorToModelError(ctx, c.logger, xerrors.Errorf("failed to create device group for user %d: %w", user.ID, err))
		}
		configsMap := make(model.DeviceConfigs)
		configsMap[display.ID] = newDeviceConfig(display, speaker, response.GroupID)
		configsMap[speaker.ID] = newDeviceConfig(speaker, display, response.GroupID)
		if err := c.db.StoreUserDeviceConfigs(ctx, user.ID, configsMap); err != nil {
			return xerrors.Errorf("failed to store user %d device configs: %w", user.ID, err)
		}

		return nil
	})

	if err != nil {
		return xerrors.Errorf("failed to create tandem for user %d: %w", user.ID, err)
	}
	return nil
}

func (c *Controller) DeleteTandem(ctx context.Context, user model.User, device model.Device) error {
	if device.InternalConfig.Tandem == nil {
		return nil
	}

	err := c.db.Transaction(ctx, "delete-tandem", func(ctx context.Context) error {
		groupID := device.InternalConfig.Tandem.Group.ID
		if err := c.client.DeleteDeviceGroup(ctx, user.Ticket, groupID); err != nil {
			return quasarErrorToModelError(ctx, c.logger, xerrors.Errorf("failed to delete device group %d: %w", groupID, err))
		}
		devices, err := c.db.SelectUserDevicesSimple(ctx, user.ID)
		if err != nil {
			return xerrors.Errorf("failed to select user %d devices: %w", user.ID, err)
		}
		devicesMap := devices.ToMap()

		deviceConfigs := make(model.DeviceConfigs)
		partner, partnerExist := devicesMap[device.InternalConfig.Tandem.Partner.ID]
		if partnerExist {
			deviceConfigs[partner.ID] = deleteTandemFromDeviceConfig(partner.InternalConfig)
		}
		dbDevice, deviceExist := devicesMap[device.ID]
		if deviceExist {
			deviceConfigs[dbDevice.ID] = deleteTandemFromDeviceConfig(dbDevice.InternalConfig)
		}
		if err := c.db.StoreUserDeviceConfigs(ctx, user.ID, deviceConfigs); err != nil {
			return xerrors.Errorf("failed to store user %d device configs: %w", user.ID, err)
		}
		return nil
	})

	if err != nil {
		return xerrors.Errorf("failed to delete tandem on device %s for user %d: %w", device.ID, user.ID, err)
	}
	return nil
}

func (c *Controller) UpdateDevicesLocation(ctx context.Context, user model.User, devices model.Devices) error {
	if !experiments.UpdateQuasarDevicesLocations.IsEnabled(ctx) {
		return nil
	}

	quasarDevices := devices.QuasarDevices()
	if len(quasarDevices) == 0 {
		return nil
	}

	households, err := c.db.SelectUserHouseholds(ctx, user.ID)
	if err != nil {
		return err
	}

	householdsMap := households.ToMap()

	quasarIDToLocation := map[string]*model.HouseholdLocation{}

	for _, device := range quasarDevices {
		quasarID, err := device.GetExternalID()
		if err != nil {
			ctxlog.Warnf(ctx, c.logger, "Failed to get quasarID for user %d device %s: %s", user.ID, device.ID, err)
			continue
		}

		if household, ok := householdsMap[device.HouseholdID]; ok {
			quasarIDToLocation[quasarID] = household.Location
		} else {
			ctxlog.Warnf(ctx, c.logger, "Device's %s household %s doesn't exist for user %d", device.ID, device.HouseholdID, user.ID)
			quasarIDToLocation[quasarID] = nil
		}
	}

	return c.updateDevicesLocation(ctx, user, quasarIDToLocation)
}

func (c *Controller) UnsetDevicesLocation(ctx context.Context, user model.User, devices model.Devices) error {
	if !experiments.UpdateQuasarDevicesLocations.IsEnabled(ctx) {
		return nil
	}

	quasarIDToLocation := map[string]*model.HouseholdLocation{}

	for _, device := range devices.QuasarDevices() {
		quasarID, err := device.GetExternalID()
		if err != nil {
			ctxlog.Warnf(ctx, c.logger, "Failed to get quasarID for user %d device %s: %s", user.ID, device.ID, err)
			continue
		}

		quasarIDToLocation[quasarID] = nil
	}

	return c.updateDevicesLocation(ctx, user, quasarIDToLocation)
}

func (c *Controller) updateDevicesLocation(ctx context.Context, user model.User, locations map[string]*model.HouseholdLocation) error {
	ctxlog.Infof(ctx, c.logger, "Update devices location: %v", locations)

	if len(locations) == 0 {
		return nil
	}

	quasarIDs := make([]string, 0, len(locations))
	for quasarID := range locations {
		quasarIDs = append(quasarIDs, quasarID)
	}

	return c.client.UpdateDeviceConfigsRobust(ctx, user.Ticket, quasarIDs, func(configs map[string]*libquasar.Config) error {
		for quasarID, location := range locations {
			cfg, ok := configs[quasarID]
			if !ok {
				ctxlog.Warnf(ctx, c.logger, "Can't update quasar config for quasarID %s: no quasarID in configs", quasarID)
				continue
			}

			if location != nil {
				cfg.SetLocation(location.Latitude, location.Longitude)
			} else {
				cfg.UnsetLocation()
			}
		}

		return nil
	})
}

func (c *Controller) deviceConfigQuasar(ctx context.Context, user model.User, deviceID string) (DeviceConfig, error) {
	device, err := c.db.SelectUserDeviceSimple(ctx, user.ID, deviceID)
	if err != nil {
		return DeviceConfig{}, xerrors.Errorf("failed to select device from db: %w", err)
	}
	quasarCustomData, err := device.QuasarCustomData()
	if err != nil {
		return DeviceConfig{}, xerrors.Errorf("failed to get quasar info: %w", err)
	}
	cRes, err := c.client.DeviceConfig(ctx, user.Ticket, createDeviceKey(*quasarCustomData))
	if err != nil {
		return DeviceConfig{}, quasarErrorToModelError(ctx, c.logger, xerrors.Errorf("failed to get config from quasar: %w", err))
	}

	devicesVersion := libquasar.DeviceVersions{
		libquasar.DeviceVersion{
			DeviceID: quasarCustomData.DeviceID,
			Version:  cRes.Version,
		},
	}

	return DeviceConfig{
		Config:  cRes.Config,
		Version: devicesVersion.JoinVersions(),
	}, nil
}

func (c *Controller) deviceConfigStereopair(ctx context.Context, user model.User, sp model.Stereopair, firstDeviceID string) (DeviceConfig, error) {
	deviceIDs := sp.Config.Devices.DeviceIDs()

	if _, exist := sp.Config.Devices.GetConfigByID(firstDeviceID); !exist {
		return DeviceConfig{}, xerrors.Errorf("firstDeviceID doesn't contained in stereopair, firstDeviceID: %q", firstDeviceID)
	}

	devices, err := c.db.SelectUserDevicesSimple(ctx, user.ID)
	if err != nil {
		return DeviceConfig{}, xerrors.Errorf("failed to select devices: %w", err)
	}

	type deviceConfigResultWithQuasarID struct {
		DeviceID string
		libquasar.DeviceConfigResult
	}

	var grp goroutines.Group
	var cResultsMutex sync.Mutex
	var firstDeviceResultIndex int
	cResults := make([]deviceConfigResultWithQuasarID, 0, len(deviceIDs))

	createGetDeviceConfigFunc := func(deviceID string) func() error {
		return func() error {
			var err error

			localDevice, _ := devices.GetDeviceByID(deviceID)
			quasarInfo, err := localDevice.QuasarCustomData()
			if err != nil {
				return xerrors.Errorf("failed to get quasar info for device %q: %w", deviceID, err)
			}

			res, err := c.client.DeviceConfig(ctx, user.Ticket, createDeviceKey(*quasarInfo))
			cResultsMutex.Lock()
			cResults = append(cResults, deviceConfigResultWithQuasarID{DeviceID: quasarInfo.DeviceID, DeviceConfigResult: res})
			if deviceID == firstDeviceID {
				firstDeviceResultIndex = len(cResults) - 1
			}
			cResultsMutex.Unlock()
			if err != nil {
				return quasarErrorToModelError(ctx, c.logger, xerrors.Errorf("failed to get device config. DeviceID %q: %w", deviceID, err))
			}

			return nil

		}
	}

	for _, deviceID := range deviceIDs {
		grp.Go(createGetDeviceConfigFunc(deviceID))
	}
	err = grp.Wait()
	if err != nil {
		return DeviceConfig{}, xerrors.Errorf("failed to get quasar config for stereopair: %w", err)
	}

	devicesVersion := make(libquasar.DeviceVersions, 0, len(cResults))
	for _, cRes := range cResults {
		devicesVersion = append(devicesVersion, libquasar.DeviceVersion{
			DeviceID: cRes.DeviceID,
			Version:  cRes.Version,
		})
	}

	return DeviceConfig{
		Config:  cResults[firstDeviceResultIndex].Config,
		Version: devicesVersion.JoinVersions(),
	}, nil
}

func (c *Controller) pushStereopairConfig(ctx context.Context, user model.User, stereopair model.Stereopair) error {
	ctxlog.Infof(ctx, c.logger, "push stereopair config for quasar backend: %q", stereopair.ID)

	stereopairDevices := stereopair.Devices.ToMap()
	deviceQuasarCustomData := make(map[string]*quasar.CustomData, len(stereopair.Devices))
	deviceIDs := make([]string, 0, len(stereopair.Devices))
	for _, cfg := range stereopair.Devices {
		device := stereopairDevices[cfg.ID]
		quasarCustomData, err := device.QuasarCustomData()
		if err != nil {
			return xerrors.Errorf("failed to get quasar custom data for device %q: %w", device.ID, err)
		}
		deviceQuasarCustomData[device.ID] = quasarCustomData
		deviceIDs = append(deviceIDs, quasarCustomData.DeviceID)
	}

	return c.client.UpdateDeviceConfigs(ctx, user.Ticket, deviceIDs, func(configs map[string]*libquasar.Config) error {
		leaderID := stereopair.GetLeaderDevice().ID
		leftCfg := stereopair.Config.GetByChannel(model.LeftChannel)
		rightCfg := stereopair.Config.GetByChannel(model.RightChannel)

		leaderQuasarID := deviceQuasarCustomData[leaderID].DeviceID
		leftQuasarID := deviceQuasarCustomData[leftCfg.ID].DeviceID
		rightQuasarID := deviceQuasarCustomData[rightCfg.ID].DeviceID

		leaderConfig := *configs[leaderQuasarID]
		newLeftConfig, newRightConfig := leaderConfig.SplitConfigForStereopair(
			leftQuasarID, stereopairDevices[leftCfg.ID].Name, string(leftCfg.Role),
			rightQuasarID, stereopairDevices[rightCfg.ID].Name, string(rightCfg.Role),
		)
		*configs[leftQuasarID] = newLeftConfig
		*configs[rightQuasarID] = newRightConfig
		return nil
	})
}

func (c *Controller) genQuasarDeviceClientConfig(ctx context.Context, userID uint64, deviceID string, fromVersion string, config libquasar.Config) ([]libquasar.SetDeviceConfig, error) {
	device, err := c.db.SelectUserDeviceSimple(ctx, userID, deviceID)
	if err != nil {
		return nil, xerrors.Errorf("failed to select user device with id %q: %w", deviceID, err)
	}
	quasarCustomData, err := device.QuasarCustomData()
	if err != nil {
		return nil, xerrors.Errorf("failed to get quasar info: %w", err)
	}

	var versions libquasar.DeviceVersions
	if err = versions.FromJoined(fromVersion); err != nil {
		ctxlog.Warnf(ctx, c.logger, "failed to deserialize version: %+v", err)
		return nil, &model.InvalidConfigVersionError{}
	}

	if len(versions) != 1 {
		return nil, xerrors.Errorf("bad from version: internal versions count (must be one): %v", len(versions))
	}

	if versions[0].DeviceID != quasarCustomData.DeviceID {
		return nil, xerrors.Errorf("bad version: internal quasar device id %q", versions[0].DeviceID)
	}

	return []libquasar.SetDeviceConfig{
		{
			DeviceKey:   createDeviceKey(*quasarCustomData),
			FromVersion: versions[0].Version,
			Config:      config,
		},
	}, nil
}

func (c *Controller) genStereopairConfigs(ctx context.Context, userID uint64, stereopair model.Stereopair, fromVersion string, config libquasar.Config) ([]libquasar.SetDeviceConfig, error) {
	leftPairCfg := stereopair.Config.GetByChannel(model.LeftChannel)
	rightPairCfg := stereopair.Config.GetByChannel(model.RightChannel)

	devices, err := c.db.SelectUserDevicesSimple(ctx, userID)
	if err != nil {
		return nil, xerrors.Errorf("failed to select devices: %w", err)
	}

	leftDevice, ok := devices.GetDeviceByID(leftPairCfg.ID)
	if !ok {
		return nil, xerrors.Errorf("failed to get left device: %v", &model.DeviceNotFoundError{})
	}
	rightDevice, ok := devices.GetDeviceByID(rightPairCfg.ID)
	if !ok {
		return nil, xerrors.Errorf("failed to get right device: %v", &model.DeviceNotFoundError{})
	}

	leftQuasar, err := leftDevice.QuasarCustomData()
	if err != nil {
		return nil, xerrors.Errorf("failed to get left quasar info: %w", err)
	}
	rightQuasar, err := rightDevice.QuasarCustomData()
	if err != nil {
		return nil, xerrors.Errorf("failed to get right quasar info: %w", err)
	}

	var fromVersions libquasar.DeviceVersions
	err = fromVersions.FromJoined(fromVersion)
	if err != nil {
		ctxlog.Warnf(ctx, c.logger, "failed to split versions: %+v", err)
		return nil, &model.InvalidConfigVersionError{}
	}
	ctxlog.Debug(ctx, c.logger, "received versions", log.Any("versions", fromVersions))

	versions := make(map[string]string)
	for _, quasarID := range []string{leftQuasar.DeviceID, rightQuasar.DeviceID} {
		version, ok := fromVersions.GetByDeviceID(quasarID)
		if !ok {
			ctxlog.Warnf(ctx, c.logger, "version field doesn't contain config version for device with quasar_id: %q", quasarID)
			return nil, &model.InvalidConfigVersionError{}
		}
		versions[quasarID] = version
	}

	leftConfig, rightConfig := config.SplitConfigForStereopair(
		leftQuasar.DeviceID, leftDevice.Name, string(leftPairCfg.Role),
		rightQuasar.DeviceID, rightDevice.Name, string(rightPairCfg.Role))
	return []libquasar.SetDeviceConfig{
		{
			DeviceKey:   createDeviceKey(*leftQuasar),
			FromVersion: versions[leftQuasar.DeviceID],
			Config:      leftConfig,
		},
		{
			DeviceKey:   createDeviceKey(*rightQuasar),
			FromVersion: versions[rightQuasar.DeviceID],
			Config:      rightConfig,
		},
	}, nil
}

// FIXME: placed here due to dependency cycle
// FIXME: purge after tandem transferring to our model
func CanCreateTandem(currentDevice model.Device, candidate model.Device, stereopairs model.Stereopairs, deviceInfos DeviceInfos) error {
	if err := model.CanCreateTandem(currentDevice, candidate, stereopairs); err != nil {
		return err
	}
	if deviceInfos.TandemPartnerID(candidate.ID) != "" && deviceInfos.TandemPartnerID(candidate.ID) != currentDevice.ID {
		return xerrors.Errorf("device %s and device %s could not be tandemized: device %s already in tandem with other device", currentDevice.ID, candidate.ID, candidate.ID)
	}
	return nil
}

func deviceIsInTandem(device model.Device, deviceInfos DeviceInfos) bool {
	return device.IsInTandem() || deviceInfos.TandemInfo(device.ID) != nil
}

func createDeviceKey(customData quasar.CustomData) libquasar.DeviceKey {
	return libquasar.DeviceKey{
		DeviceID: customData.DeviceID,
		Platform: customData.Platform,
	}
}

func quasarErrorToModelError(ctx context.Context, logger log.Logger, err error) error {
	var targetErr error
	switch {
	case xerrors.Is(err, libquasar.ErrForbidden) || xerrors.Is(err, libquasar.ErrNotFound):
		targetErr = &model.DeviceNotFoundError{}
	case xerrors.Is(err, libquasar.ErrConflict):
		targetErr = &model.InvalidConfigVersionError{}
	default:
		targetErr = err
	}

	if targetErr != err {
		ctxlog.Info(ctx, logger, "libquasar error was replaced by model error", log.NamedError("source_error", err), log.NamedError("target_error", targetErr))
	}
	return targetErr
}
