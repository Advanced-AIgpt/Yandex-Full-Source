package quasarconfig

import (
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"sort"
	"sync"
	"sync/atomic"
	"testing"

	"github.com/stretchr/testify/assert"
	"go.uber.org/zap/zaptest"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/quasar"
	"a.yandex-team.ru/alice/library/go/libquasar"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	_ IController = &Controller{}
)

type testControllerEnv struct {
	at         *assert.Assertions
	ctx        context.Context
	clientMock *libquasar.ClientMock
	dbMock     *db.DBClientMock
	logger     log.Logger
	controller *Controller

	// data
	user           model.User
	quasarPlatform string

	leaderDeviceID      string
	leaderQuasarID      string
	leaderVersion       string
	leaderNewVersion    string
	leaderChannel       model.StereopairDeviceChannel
	leaderConfig        libquasar.Config
	leaderDevice        model.Device
	leaderIotDeviceInfo libquasar.IotDeviceInfo

	followerDeviceID      string
	followerQuasarID      string
	followerVersion       string
	followerNewVersion    string
	followerChannel       model.StereopairDeviceChannel
	followerConfig        libquasar.Config
	followerDevice        model.Device
	followerIotDeviceInfo libquasar.IotDeviceInfo

	stereopairID   string
	stereopairName string
	stereopair     model.Stereopair

	firstQuasarDevice           model.Device
	firstQuasarDeviceID         string
	firstQuasarDeviceType       model.DeviceType
	firstQuasarName             string
	firstQuasarDeviceQuasarID   string
	firstQuasarDeviceVersion    string
	firstQuasarDeviceNewVersion string
	firstQuasarDeviceConfig     libquasar.Config
	firstQuasarNewVersion       string
	firstQuasarIotDeviceInfo    libquasar.IotDeviceInfo

	secondQuasarDevice           model.Device
	secondQuasarDeviceID         string
	secondQuasarDeviceType       model.DeviceType
	secondQuasarName             string
	secondQuasarDeviceQuasarID   string
	secondQuasarDeviceVersion    string
	secondQuasarDeviceNewVersion string
	secondQuasarDeviceConfig     libquasar.Config
	secondQuasarNewVersion       string
	secondQuasarIotDeviceInfo    libquasar.IotDeviceInfo

	// counters
	tCounters

	// out-data
	m                   sync.Mutex
	deleteStereopairIDs map[string]bool
	quasarSetConfigs    map[string]libquasar.Config
}

func (e *testControllerEnv) reset() {
	e.tCounters = tCounters{}
	e.quasarSetConfigs = map[string]libquasar.Config{}
	e.deleteStereopairIDs = map[string]bool{}
}

type tCounters struct {
	cntDeleteStereopairs   int64
	cntSelectUserDevice    int64
	cntSelectUserDevices   int64
	cntSelectStereopair    int64
	cntSelectStereopairs   int64
	cntDeviceConfig        int64
	cntUpdateDeviceConfigs int64
	cntSetDeviceConfig     int64
	cntIotDeviceInfos      int64
}

func newEnv(t *testing.T) *testControllerEnv {

	ctx := context.Background()

	e := &testControllerEnv{
		at:         assert.New(t),
		ctx:        ctx,
		clientMock: &libquasar.ClientMock{},
		dbMock:     &db.DBClientMock{},
		logger:     &zap.Logger{L: zaptest.NewLogger(t)},

		// data
		user: model.User{
			ID:     123,
			Login:  "user-login",
			Ticket: "client-tvm-ticket",
		},
		quasarPlatform: "test-quasar-platform",

		leaderDeviceID: "device-id-leader",
		leaderQuasarID: "quasar-leader-id",
		leaderVersion:  "leader-version",
		leaderChannel:  model.LeftChannel,
		leaderConfig:   configFromString(`{"leader": "config"}`),

		followerDeviceID: "device-id-follower",
		followerQuasarID: "quasar-follower-id",
		followerVersion:  "follower-verion",
		followerChannel:  model.RightChannel,
		followerConfig:   configFromString(`{"follower": "config"}`),

		stereopairID:   "stereopair-id",
		stereopairName: "Моя стереопара",

		firstQuasarDeviceID:         "quasar-device-id-1",
		firstQuasarDeviceType:       model.YandexStationDeviceType,
		firstQuasarName:             "quasar-1-name",
		firstQuasarDeviceQuasarID:   "quasar-device-quasar-id-1",
		firstQuasarDeviceVersion:    "quasar-version-1",
		firstQuasarDeviceNewVersion: "quasar-new-version-1",
		firstQuasarDeviceConfig:     configFromString(`{"quasar-1": "config", "stereo_pair": {}}`),

		secondQuasarDeviceID:         "quasar-device-id-2",
		secondQuasarDeviceType:       model.YandexStationDeviceType,
		secondQuasarName:             "quasar-2-name",
		secondQuasarDeviceQuasarID:   "quasar-device-quasar-id-2",
		secondQuasarDeviceVersion:    "quasar-version-2",
		secondQuasarDeviceNewVersion: "quasar-new-version-2",
		secondQuasarDeviceConfig:     configFromString(`{"quasar-2": "config", "stereo_pair": {}}`),

		deleteStereopairIDs: make(map[string]bool),
		quasarSetConfigs:    make(map[string]libquasar.Config),
	}
	e.leaderIotDeviceInfo = libquasar.IotDeviceInfo{
		ID:       e.leaderQuasarID,
		Platform: e.quasarPlatform,
		Config: libquasar.IotDeviceInfoVersionedConfig{
			Content: e.leaderConfig,
			Version: e.leaderVersion,
		},
	}
	e.followerIotDeviceInfo = libquasar.IotDeviceInfo{
		ID:       e.followerQuasarID,
		Platform: e.quasarPlatform,
		Config: libquasar.IotDeviceInfoVersionedConfig{
			Content: e.followerConfig,
			Version: e.followerVersion,
		},
	}
	e.firstQuasarIotDeviceInfo = libquasar.IotDeviceInfo{
		ID:       e.firstQuasarDeviceQuasarID,
		Platform: e.quasarPlatform,
		Config: libquasar.IotDeviceInfoVersionedConfig{
			Content: e.firstQuasarDeviceConfig,
			Version: e.firstQuasarDeviceVersion,
		},
	}
	e.secondQuasarIotDeviceInfo = libquasar.IotDeviceInfo{
		ID:       e.secondQuasarDeviceQuasarID,
		Platform: e.quasarPlatform,
		Config: libquasar.IotDeviceInfoVersionedConfig{
			Content: e.secondQuasarDeviceConfig,
			Version: e.secondQuasarDeviceVersion,
		},
	}

	e.controller = &Controller{
		db:     e.dbMock,
		client: e.clientMock,
		logger: e.logger,
	}

	e.leaderDevice = model.Device{
		ID:         e.leaderDeviceID,
		Name:       "Лидер колонка",
		SkillID:    model.QUASAR,
		CustomData: quasar.CustomData{DeviceID: e.leaderQuasarID, Platform: e.quasarPlatform},
	}

	e.followerDevice = model.Device{
		ID:         e.followerDeviceID,
		Name:       "Фоловер колонка",
		SkillID:    model.QUASAR,
		CustomData: quasar.CustomData{DeviceID: e.followerQuasarID, Platform: e.quasarPlatform},
	}

	e.firstQuasarDevice = model.Device{
		ID:         e.firstQuasarDeviceID,
		Type:       e.firstQuasarDeviceType,
		Name:       e.firstQuasarName,
		SkillID:    model.QUASAR,
		CustomData: quasar.CustomData{DeviceID: e.firstQuasarDeviceQuasarID, Platform: e.quasarPlatform},
	}

	e.secondQuasarDevice = model.Device{
		ID:         e.secondQuasarDeviceID,
		Type:       e.secondQuasarDeviceType,
		Name:       e.secondQuasarName,
		SkillID:    model.QUASAR,
		CustomData: quasar.CustomData{DeviceID: e.secondQuasarDeviceQuasarID, Platform: e.quasarPlatform},
	}

	e.stereopair = model.Stereopair{
		ID:   e.stereopairID,
		Name: e.stereopairName,
		Config: model.StereopairConfig{
			Devices: model.StereopairDeviceConfigs{
				{
					ID:      e.leaderDeviceID,
					Channel: e.leaderChannel,
					Role:    model.LeaderRole,
				},
				{
					ID:      e.followerDeviceID,
					Channel: e.followerChannel,
					Role:    model.FollowerRole,
				},
			},
		},
		Devices: model.Devices{e.followerDevice, e.leaderDevice},
	}

	// prepare db mock
	e.dbMock.SelectUserDevicesMock = func(ctx context.Context, uid uint64) ([]model.Device, error) {
		atomic.AddInt64(&e.tCounters.cntSelectUserDevices, 1)
		return []model.Device{e.firstQuasarDevice, e.secondQuasarDevice, e.leaderDevice, e.followerDevice}, nil
	}

	e.dbMock.DeleteStereopairMock = func(ctx context.Context, userID uint64, id string) error {
		e.at.Equal(e.user.ID, userID)
		atomic.AddInt64(&e.tCounters.cntDeleteStereopairs, 1)

		e.m.Lock()
		defer e.m.Unlock()
		e.deleteStereopairIDs[id] = true
		return nil
	}

	e.dbMock.SelectStereopairMock = func(ctx context.Context, userID uint64, stereopairID string) (model.Stereopair, error) {
		stereopairs, err := e.dbMock.SelectStereopairs(ctx, userID)
		if err != nil {
			return model.Stereopair{}, err
		}
		atomic.AddInt64(&e.cntSelectStereopairs, -1)
		atomic.AddInt64(&e.cntSelectStereopair, 1)
		stereopair, ok := stereopairs.GetByID(stereopairID)
		if !ok {
			return model.Stereopair{}, xerrors.Errorf("failed to select stereopair: %w", &model.DeviceNotFoundError{})
		}
		return stereopair, nil
	}

	e.dbMock.SelectStereopairsMock = func(ctx context.Context, localUserID uint64) (model.Stereopairs, error) {
		e.at.Equal(e.user.ID, localUserID)
		atomic.AddInt64(&e.cntSelectStereopairs, 1)

		return model.Stereopairs{
			model.Stereopair{
				ID:   e.stereopairID,
				Name: "stereo-name",
				Config: model.StereopairConfig{
					Devices: []model.StereopairDeviceConfig{
						{
							ID:      e.leaderDeviceID,
							Channel: e.leaderChannel,
							Role:    model.LeaderRole,
						},
						{
							ID:      e.followerDeviceID,
							Channel: e.followerChannel,
							Role:    model.FollowerRole,
						},
					},
				},
				Devices: model.Devices{e.followerDevice, e.leaderDevice},
			},
		}, nil
	}

	e.dbMock.SelectUserDeviceMock = func(ctx context.Context, uid uint64, deviceID string) (model.Device, error) {
		e.at.Equal(e.user.ID, uid)
		atomic.AddInt64(&e.cntSelectUserDevice, 1)

		switch deviceID {
		case e.leaderDeviceID:
			return e.leaderDevice.Clone(), nil
		case e.followerDeviceID:
			return e.followerDevice.Clone(), nil
		case e.firstQuasarDeviceID:
			return e.firstQuasarDevice.Clone(), nil
		case e.secondQuasarDeviceID:
			return e.secondQuasarDevice.Clone(), nil
		default:
			return model.Device{}, xerrors.Errorf("unknown device: %q", deviceID)
		}
	}

	// prepare client mock
	e.clientMock.DeviceConfigMock = func(ctx context.Context, userTicket string, deviceKey libquasar.DeviceKey) (libquasar.DeviceConfigResult, error) {
		atomic.AddInt64(&e.cntDeviceConfig, 1)

		e.at.Equal(e.user.Ticket, userTicket)
		e.at.Equal(e.quasarPlatform, deviceKey.Platform)
		switch deviceKey.DeviceID {
		case e.leaderQuasarID:
			return libquasar.DeviceConfigResult{
				Status:  "ok",
				Version: e.leaderVersion,
				Config:  e.leaderConfig.Clone(),
			}, nil
		case e.followerQuasarID:
			return libquasar.DeviceConfigResult{
				Status:  "ok",
				Version: e.followerVersion,
				Config:  e.followerConfig.Clone(),
			}, nil
		case e.firstQuasarDeviceQuasarID:
			return libquasar.DeviceConfigResult{
				Status:  "ok",
				Version: e.firstQuasarDeviceVersion,
				Config:  e.firstQuasarDeviceConfig.Clone(),
			}, nil
		case e.secondQuasarDeviceQuasarID:
			return libquasar.DeviceConfigResult{
				Status:  "ok",
				Version: e.secondQuasarDeviceVersion,
				Config:  e.secondQuasarDeviceConfig.Clone(),
			}, nil
		default:
			return libquasar.DeviceConfigResult{}, xerrors.Errorf("client mock DeviceConfig unknown device id: %q", deviceKey.DeviceID)
		}
	}
	e.clientMock.SetDevicesConfigMock = func(ctx context.Context, userTicket string, payload libquasar.SetDevicesConfigPayload) (libquasar.SetDeviceConfigResult, error) {
		atomic.AddInt64(&e.cntSetDeviceConfig, 1)

		e.m.Lock()
		defer e.m.Unlock()

		e.at.Equal(e.user.Ticket, userTicket)

		var res libquasar.SetDeviceConfigResult
		for _, deviceConfig := range payload.Devices {
			e.at.Equal(e.quasarPlatform, deviceConfig.Platform)

			var v libquasar.DeviceVersion
			switch deviceConfig.DeviceID {
			case e.leaderQuasarID:
				e.at.Equal(e.leaderVersion, deviceConfig.FromVersion)
				v = libquasar.DeviceVersion{DeviceID: deviceConfig.DeviceID, Version: e.leaderNewVersion}
			case e.followerQuasarID:
				e.at.Equal(e.followerVersion, deviceConfig.FromVersion)
				v = libquasar.DeviceVersion{DeviceID: deviceConfig.DeviceID, Version: e.followerNewVersion}
			case e.firstQuasarDeviceQuasarID:
				e.at.Equal(e.firstQuasarDeviceVersion, deviceConfig.FromVersion)
				v = libquasar.DeviceVersion{DeviceID: deviceConfig.DeviceID, Version: e.firstQuasarNewVersion}
			case e.secondQuasarDeviceQuasarID:
				e.at.Equal(e.secondQuasarDeviceVersion, deviceConfig.FromVersion)
				v = libquasar.DeviceVersion{DeviceID: deviceConfig.DeviceID, Version: e.secondQuasarNewVersion}
			default:
				return libquasar.SetDeviceConfigResult{}, xerrors.Errorf("unknown device id: %q", deviceConfig.DeviceID)
			}
			res.Devices = append(res.Devices, v)
			e.quasarSetConfigs[deviceConfig.DeviceID] = deviceConfig.Config
		}
		res.Status = "ok"
		return res, nil
	}
	e.clientMock.UpdateDeviceConfigsMock = func(ctx context.Context, userTicket string, deviceIDs []string, update func(configs map[string]*libquasar.Config) error) (err error) {
		atomic.AddInt64(&e.tCounters.cntUpdateDeviceConfigs, 1)
		configs := make(map[string]*libquasar.DeviceConfigResult, len(deviceIDs))

		for _, deviceID := range deviceIDs {
			config, err := e.clientMock.DeviceConfigMock(ctx, userTicket, libquasar.DeviceKey{DeviceID: deviceID, Platform: e.quasarPlatform})
			atomic.AddInt64(&e.tCounters.cntDeviceConfig, -1)
			if err != nil {
				return xerrors.Errorf("failed to get device config: %q: %w", deviceID, err)
			}
			configs[deviceID] = &config
		}

		updateMap := make(map[string]*libquasar.Config, len(configs))
		for deviceID, config := range configs {
			updateMap[deviceID] = &config.Config
		}
		err = update(updateMap)
		if err != nil {
			return xerrors.Errorf("failed to update devices config in callback: %w", err)
		}

		var payload libquasar.SetDevicesConfigPayload
		for deviceID, config := range configs {
			payload.Devices = append(payload.Devices, libquasar.SetDeviceConfig{
				DeviceKey:   libquasar.DeviceKey{DeviceID: deviceID, Platform: e.quasarPlatform},
				FromVersion: config.Version,
				Config:      config.Config,
			})
		}
		sort.Slice(payload.Devices, func(i, j int) bool {
			return payload.Devices[i].DeviceID < payload.Devices[j].DeviceID
		})
		_, err = e.clientMock.SetDeviceConfigs(ctx, userTicket, payload)
		atomic.AddInt64(&e.tCounters.cntSetDeviceConfig, -1)
		if err != nil {
			return xerrors.Errorf("failed to set device config")
		}

		return nil
	}

	e.clientMock.IotDeviceInfoMock = func(ctx context.Context, userTicket string, deviceIDs []string) ([]libquasar.IotDeviceInfo, error) {
		atomic.AddInt64(&e.cntIotDeviceInfos, 1)

		e.m.Lock()
		defer e.m.Unlock()

		result := []libquasar.IotDeviceInfo{
			e.leaderIotDeviceInfo,
			e.followerIotDeviceInfo,
			e.firstQuasarIotDeviceInfo,
			e.secondQuasarIotDeviceInfo,
		}

		if len(deviceIDs) == 0 {
			return result, nil
		}

		filteredRes := make([]libquasar.IotDeviceInfo, 0, len(result))
		for _, deviceID := range deviceIDs {
			switch deviceID {
			case e.leaderDeviceID:
				filteredRes = append(filteredRes, e.leaderIotDeviceInfo)
			case e.followerDeviceID:
				filteredRes = append(filteredRes, e.followerIotDeviceInfo)
			case e.firstQuasarDeviceID:
				filteredRes = append(filteredRes, e.firstQuasarIotDeviceInfo)
			case e.secondQuasarDeviceID:
				filteredRes = append(filteredRes, e.secondQuasarIotDeviceInfo)
			default:
				return nil, xerrors.Errorf("unknown device: %q", deviceID)
			}
		}
		return filteredRes, nil
	}
	return e
}

func configFromString(s string) libquasar.Config {
	var c libquasar.Config
	err := json.Unmarshal([]byte(s), &c)
	if err != nil {
		panic(err)
	}
	return c
}
func configToString(c libquasar.Config) string {
	resBytes, err := json.Marshal(c)
	if err != nil {
		panic(err)
	}
	return string(resBytes)
}

func TestConstantValues(t *testing.T) {
	at := assert.New(t)
	at.EqualValues(model.LeftChannel, "left")
	at.EqualValues(model.RightChannel, "right")
	at.EqualValues(model.LeaderRole, "leader")
	at.EqualValues(model.FollowerRole, "follower")
}

func TestCreateStereopair(t *testing.T) {
	e := newEnv(t)
	spConfig := model.StereopairConfig{Devices: []model.StereopairDeviceConfig{
		{
			ID:      e.firstQuasarDeviceID,
			Channel: model.RightChannel,
			Role:    model.LeaderRole,
		},
		{
			ID:      e.secondQuasarDeviceID,
			Channel: model.LeftChannel,
			Role:    model.FollowerRole,
		},
	}}
	sp, err := e.controller.CreateStereopair(e.ctx, e.user, spConfig)
	e.at.NoError(err)
	e.at.NotEmpty(sp.ID)
	e.at.NotEmpty(sp.Name)
	e.at.Equal(spConfig, sp.Config)

	expectedFirstConfig := fmt.Sprintf(`{
	"quasar-1": "config",
    "name": "%s",
	"stereo_pair": {
		"role": "%s",
		"channel": "%s",
		"partnerDeviceId": "%s"
	}
}`, e.firstQuasarDevice.Name, model.LeaderRole, model.RightChannel, e.secondQuasarDeviceQuasarID)
	e.at.JSONEq(expectedFirstConfig, configToString(e.quasarSetConfigs[e.firstQuasarDeviceQuasarID]))

	// "quasar-1": "config": config from leader device
	expectedSecondConfig := fmt.Sprintf(`{
	"quasar-1": "config",
    "name": "%s",
	"stereo_pair": {
		"role": "%s",
		"channel": "%s",
		"partnerDeviceId": "%s"
	}
}`, e.secondQuasarDevice.Name, model.FollowerRole, model.LeftChannel, e.firstQuasarDeviceQuasarID)
	e.at.JSONEq(expectedSecondConfig, configToString(e.quasarSetConfigs[e.secondQuasarDeviceQuasarID]))

	e.at.Equal(tCounters{
		cntSelectUserDevices:   1,
		cntUpdateDeviceConfigs: 1,
		cntSelectStereopairs:   1,
	}, e.tCounters)
}

func TestDeleteStereopair(t *testing.T) {
	e := newEnv(t)

	err := e.controller.DeleteStereopair(e.ctx, e.user, e.stereopairID)
	e.at.NoError(err)
	e.at.Equal(map[string]bool{e.stereopairID: true}, e.deleteStereopairIDs)
	e.at.Equal(tCounters{
		cntDeleteStereopairs:   1,
		cntSelectStereopair:    1,
		cntUpdateDeviceConfigs: 1,
	}, e.tCounters)
}

func TestDeviceConfig(t *testing.T) {
	t.Run("get_quasar_device_config", func(t *testing.T) {
		e := newEnv(t)

		quasarFromVersion := libquasar.DeviceVersions{
			libquasar.DeviceVersion{DeviceID: e.firstQuasarDeviceQuasarID, Version: e.firstQuasarDeviceVersion},
		}.JoinVersions()

		res, err := e.controller.DeviceConfig(e.ctx, e.user, e.firstQuasarDeviceID)
		e.at.NoError(err)
		e.at.Equal(DeviceConfig{
			Config:  e.firstQuasarDeviceConfig,
			Version: quasarFromVersion,
		}, res)
		e.at.Equal(tCounters{
			cntDeviceConfig:      1,
			cntSelectUserDevice:  1,
			cntSelectStereopairs: 1,
		}, e.tCounters)
	})

	t.Run("get_device_infos_config", func(t *testing.T) {
		e := newEnv(t)

		quasarFromVersion := libquasar.DeviceVersions{
			libquasar.DeviceVersion{DeviceID: e.firstQuasarDeviceQuasarID, Version: e.firstQuasarDeviceVersion},
		}.JoinVersions()

		stereopairFromVersion := libquasar.DeviceVersions{
			libquasar.DeviceVersion{DeviceID: e.leaderQuasarID, Version: e.leaderVersion},
			libquasar.DeviceVersion{DeviceID: e.followerQuasarID, Version: e.followerVersion},
		}.JoinVersions()

		deviceInfos, err := e.controller.DeviceInfos(e.ctx, e.user)
		e.at.NoError(err)

		firstDeviceInfo := deviceInfos.GetByDeviceID(e.firstQuasarDeviceID)
		e.at.NotNil(firstDeviceInfo)
		e.at.Equal(DeviceConfig{
			Config:  e.firstQuasarDeviceConfig,
			Version: quasarFromVersion,
		}, firstDeviceInfo.Config)

		leaderDeviceInfo := deviceInfos.GetByDeviceID(e.leaderDeviceID)
		e.at.NotNil(leaderDeviceInfo)
		e.at.Equal(DeviceConfig{
			Config:  e.leaderConfig,
			Version: stereopairFromVersion,
		}, leaderDeviceInfo.Config)

		e.at.Equal(tCounters{
			cntSelectUserDevices: 1,
			cntIotDeviceInfos:    1,
			cntSelectStereopairs: 1,
		}, e.tCounters)
	})

	t.Run("get_stereopair_config", func(t *testing.T) {
		e := newEnv(t)

		stereopairFromVersions := libquasar.DeviceVersions{
			libquasar.DeviceVersion{DeviceID: e.leaderQuasarID, Version: e.leaderVersion},
			libquasar.DeviceVersion{DeviceID: e.followerQuasarID, Version: e.followerVersion},
		}
		stereopairFromVersion := stereopairFromVersions.JoinVersions()

		// get leader config
		e.reset()
		res, err := e.controller.DeviceConfig(e.ctx, e.user, e.leaderDeviceID)
		e.at.NoError(err)
		e.at.Equal(DeviceConfig{
			Config:  e.leaderConfig,
			Version: stereopairFromVersion,
		}, res)
		e.at.Equal(tCounters{
			cntSelectUserDevices: 1,
			cntSelectStereopairs: 1,
			cntDeviceConfig:      2,
		}, e.tCounters)

		// get follower config
		e.reset()
		res, err = e.controller.DeviceConfig(e.ctx, e.user, e.followerDeviceID)
		e.at.NoError(err)
		e.at.Equal(DeviceConfig{
			Config:  e.followerConfig,
			Version: stereopairFromVersion,
		}, res)
		e.at.Equal(tCounters{
			cntSelectUserDevices: 1,
			cntSelectStereopairs: 1,
			cntDeviceConfig:      2,
		}, e.tCounters)

	})

	t.Run("get_other_config", func(t *testing.T) {
		e := newEnv(t)
		_, err := e.controller.DeviceConfig(e.ctx, e.user, "asd")
		e.at.Error(err)
	})
}

func TestSetDeviceConfig(t *testing.T) {
	t.Run("set_quasar_config", func(t *testing.T) {
		e := newEnv(t)

		quasarFromVersion := libquasar.DeviceVersions{
			libquasar.DeviceVersion{DeviceID: e.firstQuasarDeviceQuasarID, Version: e.firstQuasarDeviceVersion},
		}.JoinVersions()
		quasarExpectedVersion := libquasar.DeviceVersions{
			libquasar.DeviceVersion{DeviceID: e.firstQuasarDeviceQuasarID, Version: e.firstQuasarNewVersion},
		}.JoinVersions()

		c := configFromString(`{"aaa": "bbb"}`)
		version, err := e.controller.SetDeviceConfig(e.ctx, e.user, e.firstQuasarDeviceID, quasarFromVersion, c)

		e.at.NoError(err)
		e.at.Equal(c, e.quasarSetConfigs[e.firstQuasarDeviceQuasarID])
		e.at.Equal(version, quasarExpectedVersion)
		e.at.Equal(tCounters{
			cntSetDeviceConfig:   1,
			cntSelectUserDevice:  1,
			cntSelectStereopairs: 1,
		}, e.tCounters)
	})

	t.Run("set_stereopair_config", func(t *testing.T) {
		e := newEnv(t)

		stereopairFromVersions := libquasar.DeviceVersions{
			libquasar.DeviceVersion{DeviceID: e.leaderQuasarID, Version: e.leaderVersion},
			libquasar.DeviceVersion{DeviceID: e.followerQuasarID, Version: e.followerVersion},
		}
		sort.Sort(stereopairFromVersions)
		stereopairFromVersion := stereopairFromVersions.JoinVersions()

		stereopairExpectedVersions := libquasar.DeviceVersions{
			libquasar.DeviceVersion{DeviceID: e.leaderQuasarID, Version: e.leaderNewVersion},
			libquasar.DeviceVersion{DeviceID: e.followerQuasarID, Version: e.followerNewVersion},
		}
		sort.Sort(stereopairExpectedVersions)
		stereopairExpectedVersion := stereopairExpectedVersions.JoinVersions()

		c := configFromString(`{"aaa":"bbb"}`)

		expectedLeaderConfig := c.Clone()
		expectedLeaderConfig.SetStereopairConfig(libquasar.StereopairConfig{
			Role:            string(model.LeaderRole),
			Channel:         string(e.leaderChannel),
			PartnerDeviceID: e.followerQuasarID,
		})
		expectedLeaderConfig["name"] = []byte(`"` + e.leaderDevice.Name + `"`)

		expectedFollowerConfig := c.Clone()
		expectedFollowerConfig.SetStereopairConfig(libquasar.StereopairConfig{
			Role:            string(model.FollowerRole),
			Channel:         string(e.followerChannel),
			PartnerDeviceID: e.leaderQuasarID,
		})
		expectedFollowerConfig["name"] = []byte(`"` + e.followerDevice.Name + `"`)

		// set with leader device
		e.reset()
		version, err := e.controller.SetDeviceConfig(e.ctx, e.user, e.leaderDeviceID, stereopairFromVersion, c)
		e.at.NoError(err)
		e.at.Equal(stereopairExpectedVersion, version)
		e.at.Equal(expectedLeaderConfig, e.quasarSetConfigs[e.leaderQuasarID])
		e.at.Equal(expectedFollowerConfig, e.quasarSetConfigs[e.followerQuasarID])
		e.at.Equal(tCounters{
			cntSelectUserDevices: 1,
			cntSelectStereopairs: 1,
			cntSetDeviceConfig:   1,
		}, e.tCounters)

		// set with follower device
		e.reset()
		version, err = e.controller.SetDeviceConfig(e.ctx, e.user, e.followerDeviceID, stereopairFromVersion, c)
		e.at.NoError(err)
		e.at.Equal(stereopairExpectedVersion, version)
		e.at.Equal(expectedLeaderConfig, e.quasarSetConfigs[e.leaderQuasarID])
		e.at.Equal(expectedFollowerConfig, e.quasarSetConfigs[e.followerQuasarID])
		e.at.Equal(tCounters{
			cntSelectUserDevices: 1,
			cntSelectStereopairs: 1,
			cntSetDeviceConfig:   1,
		}, e.tCounters)
	})
}

func TestPushStereopairConfig(t *testing.T) {
	e := newEnv(t)
	//swap channels
	e.stereopair.Config.Devices[0].Channel, e.stereopair.Config.Devices[1].Channel = e.stereopair.Config.Devices[1].Channel, e.stereopair.Config.Devices[0].Channel

	err := e.controller.pushStereopairConfig(e.ctx, e.user, e.stereopair)
	e.at.NoError(err)

	expectedLeaderConfig := fmt.Sprintf(`{
"name": "%s",
"leader": "config",
"stereo_pair": {
	"role": "leader",
	"channel": "right",
	"partnerDeviceId": "%s"
}
}`, e.leaderDevice.Name, e.followerQuasarID)

	actualLeaderConfig, _ := json.Marshal(e.quasarSetConfigs[e.leaderQuasarID])
	e.at.JSONEq(expectedLeaderConfig, string(actualLeaderConfig))

	expectedFollowerConfig := fmt.Sprintf(`{
"name": "%s",
"leader": "config",
"stereo_pair": {
	"role": "follower",
	"channel": "left",
	"partnerDeviceId": "%s"
}
}`, e.followerDevice.Name, e.leaderQuasarID)
	actualFollowerConfig, _ := json.Marshal(e.quasarSetConfigs[e.followerQuasarID])
	e.at.JSONEq(expectedFollowerConfig, string(actualFollowerConfig))
}

func TestHumanizeError(t *testing.T) {
	testErr := errors.New("test")
	input := []struct {
		name        string
		originalErr error
		resultErr   error
	}{
		{
			name:        "nil",
			originalErr: nil,
			resultErr:   nil,
		},
		{
			name:        "unknown-err",
			originalErr: testErr,
			resultErr:   testErr,
		},
		{
			name:        "forbidden",
			originalErr: xerrors.Errorf("asdf: %w", libquasar.ErrForbidden),
			resultErr:   &model.DeviceNotFoundError{},
		},
		{
			name:        "not_found",
			originalErr: xerrors.Errorf("asdf: %w", libquasar.ErrNotFound),
			resultErr:   &model.DeviceNotFoundError{},
		},
	}

	for _, tCase := range input {
		t.Run(tCase.name, func(t *testing.T) {
			at := assert.New(t)
			logger := &zap.Logger{L: zaptest.NewLogger(t)}
			at.Equal(tCase.resultErr, quasarErrorToModelError(context.Background(), logger, tCase.originalErr))
		})
	}
}
