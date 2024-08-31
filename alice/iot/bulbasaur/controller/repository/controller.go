package repository

import (
	"context"
	"fmt"
	"sync"
	"time"

	"github.com/karlseguin/ccache/v2"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/contexter"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Controller struct {
	Logger   log.Logger
	Database db.DB

	IgnoreCache     bool
	YdbPumpkinCache *ccache.Cache // `user_info` pumpkin-cache

	signals signals
}

func (c *Controller) Init(registry metrics.Registry, ignoreCache bool) {
	c.IgnoreCache = ignoreCache
	c.YdbPumpkinCache = ccache.New(ccache.Configure().MaxSize(1000000))

	if registry != nil {
		c.signals = newSignals(registry)
	}
	c.Logger.Info("Repository Controller Client was successfully initialized.")
}

func (c *Controller) SelectUser(ctx context.Context, userID uint64) (model.User, error) {
	return c.Database.SelectUser(ctx, userID)
}

func (c *Controller) SelectUserDevice(ctx context.Context, user model.User, deviceID string) (model.Device, error) {
	device, err := c.Database.SelectUserDevice(ctx, user.ID, deviceID)
	if err != nil {
		return model.Device{}, err
	}

	stereopairs, err := c.Database.SelectStereopairs(ctx, user.ID)
	if err != nil {
		return model.Device{}, err
	}

	device, ok := filterDevice(ctx, user, device, stereopairs)
	if !ok {
		return model.Device{}, xerrors.Errorf("failed to select device: filter policy applied")
	}
	return device, nil
}

func (c *Controller) SelectUserDeviceSimple(ctx context.Context, user model.User, deviceID string) (model.Device, error) {
	device, err := c.Database.SelectUserDeviceSimple(ctx, user.ID, deviceID)
	if err != nil {
		return model.Device{}, err
	}

	stereopairs, err := c.Database.SelectStereopairs(ctx, user.ID)
	if err != nil {
		return model.Device{}, err
	}

	device, ok := filterDevice(ctx, user, device, stereopairs)
	if !ok {
		return model.Device{}, xerrors.Errorf("failed to select device: filter policy applied")
	}
	return device, nil
}

func (c *Controller) SelectUserDevices(ctx context.Context, user model.User) (model.Devices, error) {
	devices, err := c.Database.SelectUserDevices(ctx, user.ID)
	if err != nil {
		return model.Devices{}, err
	}

	stereopairs, err := c.Database.SelectStereopairs(ctx, user.ID)
	if err != nil {
		return nil, err
	}

	return filterDevices(ctx, user, devices, stereopairs), nil
}

func (c *Controller) SelectUserDevicesSimple(ctx context.Context, user model.User) (model.Devices, error) {
	devices, err := c.Database.SelectUserDevicesSimple(ctx, user.ID)
	if err != nil {
		return model.Devices{}, err
	}

	stereopairs, err := c.Database.SelectStereopairs(ctx, user.ID)
	if err != nil {
		return nil, err
	}

	return filterDevices(ctx, user, devices, stereopairs), nil
}

func (c *Controller) SelectUserGroupDevices(ctx context.Context, user model.User, groupID string) ([]model.Device, error) {
	devices, err := c.Database.SelectUserGroupDevices(ctx, user.ID, groupID)
	if err != nil {
		return model.Devices{}, err
	}

	stereopairs, err := c.Database.SelectStereopairs(ctx, user.ID)
	if err != nil {
		return nil, err
	}

	return filterDevices(ctx, user, devices, stereopairs), nil
}

func (c *Controller) SelectUserRoomDevices(ctx context.Context, user model.User, roomID string) ([]model.Device, error) {
	devices, err := c.Database.SelectUserRoomDevices(ctx, user.ID, roomID)
	if err != nil {
		return model.Devices{}, err
	}

	stereopairs, err := c.Database.SelectStereopairs(ctx, user.ID)
	if err != nil {
		return nil, err
	}

	return filterDevices(ctx, user, devices, stereopairs), nil
}

func (c *Controller) SelectUserHouseholdDevices(ctx context.Context, user model.User, householdID string) (model.Devices, error) {
	devices, err := c.Database.SelectUserHouseholdDevices(ctx, user.ID, householdID)
	if err != nil {
		return model.Devices{}, err
	}

	stereopairs, err := c.Database.SelectStereopairs(ctx, user.ID)
	if err != nil {
		return nil, err
	}

	return filterDevices(ctx, user, devices, stereopairs), nil
}

func (c *Controller) SelectUserHouseholdDevicesSimple(ctx context.Context, user model.User, householdID string) (model.Devices, error) {
	devices, err := c.Database.SelectUserHouseholdDevicesSimple(ctx, user.ID, householdID)
	if err != nil {
		return model.Devices{}, err
	}

	stereopairs, err := c.Database.SelectStereopairs(ctx, user.ID)
	if err != nil {
		return nil, err
	}

	return filterDevices(ctx, user, devices, stereopairs), nil
}

func (c *Controller) SelectUserHousehold(ctx context.Context, user model.User, householdID string) (model.Household, error) {
	return c.Database.SelectUserHousehold(ctx, user.ID, householdID)
}

func (c *Controller) SelectHouseholdResidents(ctx context.Context, userID uint64, household model.Household) (model.HouseholdResidents, error) {
	return c.Database.SelectHouseholdResidents(ctx, userID, household)
}

func (c *Controller) DevicesAndScenarios(ctx context.Context, user model.User) (model.Devices, model.Scenarios, error) {
	var wg sync.WaitGroup
	var userDevices []model.Device
	var errUserDevices error
	var userScenarios []model.Scenario
	var errScenarios error
	var userStereopairs model.Stereopairs
	var errStereopairs error

	wg.Add(1)
	go func(ctx context.Context) {
		defer wg.Done()
		userDevices, errUserDevices = c.Database.SelectUserDevices(ctx, user.ID)
	}(contexter.NoCancel(ctx))

	wg.Add(1)
	go func(ctx context.Context) {
		defer wg.Done()
		userScenarios, errScenarios = c.Database.SelectUserScenarios(ctx, user.ID)
	}(contexter.NoCancel(ctx))

	wg.Add(1)
	go func(ctx context.Context) {
		defer wg.Done()
		userStereopairs, errStereopairs = c.Database.SelectStereopairs(ctx, user.ID)
	}(contexter.NoCancel(ctx))

	wg.Wait()

	if errUserDevices != nil {
		return userDevices, userScenarios, errUserDevices
	}
	if errScenarios != nil {
		return userDevices, userScenarios, errScenarios
	}
	if errStereopairs != nil {
		return userDevices, userScenarios, errStereopairs
	}

	userDevices = filterDevices(ctx, user, userDevices, userStereopairs)

	return userDevices, userScenarios, nil
}

func (c *Controller) UserInfo(ctx context.Context, user model.User) (model.UserInfo, error) {
	var userInfo model.UserInfo

	// select user to check existence before throwing parallel requests to empty result sets
	_, err := c.Database.SelectUser(ctx, user.ID)
	if err != nil {
		return userInfo, err
	}

	userInfo, err = c.Database.SelectUserInfo(ctx, user.ID)
	if err != nil {
		return userInfo, err
	}

	return userInfo, err
}

type cacheResult string

const (
	cacheNotUsed cacheResult = "not_used"
	cacheIgnored cacheResult = "ignored"
	cacheMissed  cacheResult = "missed"
	cacheHit     cacheResult = "hit"
)

func (c *Controller) userInfoWithPumpkinTimeout(ctx context.Context, user model.User, timeout time.Duration) (model.UserInfo, error) {
	cacheKey := fmt.Sprintf("userinfo:%d", user.ID)
	getCachedUserInfo := func() (model.UserInfo, cacheResult) {
		if c.IgnoreCache {
			return model.NewEmptyUserInfo(), cacheIgnored
		}
		if item := c.YdbPumpkinCache.Get(cacheKey); item != nil && !item.Expired() {
			cachedUserInfo, ok := item.Value().(model.UserInfo)
			if !ok {
				return model.NewEmptyUserInfo(), cacheMissed
			}
			return cachedUserInfo, cacheHit
		}
		return model.NewEmptyUserInfo(), cacheMissed
	}
	setCachedUserInfo := func(userInfo model.UserInfo, duration time.Duration) {
		if !c.IgnoreCache {
			c.YdbPumpkinCache.Set(cacheKey, userInfo, time.Hour*12)
		}
	}

	traceCacheUsage := func(userInfo model.UserInfo, cacheResult cacheResult) {
		switch cacheResult {
		case cacheIgnored:
			c.signals.ydbPumpkinCacheIgnored.Inc()
			cacheLogField := log.Any("ydb_pumpkin_cache", map[string]interface{}{
				"key": cacheKey,
			})
			ctxlog.Debug(ctx, c.Logger, fmt.Sprintf("cache ignored for key %s", cacheKey), cacheLogField)
		case cacheNotUsed:
			c.signals.ydbPumpkinCacheNotUsed.Inc()
		case cacheHit:
			c.signals.ydbPumpkinCacheHit.Inc()
			cacheLogField := log.Any("ydb_pumpkin_cache", map[string]interface{}{
				"key":       cacheKey,
				"user_info": userInfo,
			})
			ctxlog.Debug(ctx, c.Logger, fmt.Sprintf("got data from cache using key %s", cacheKey), cacheLogField)
		case cacheMissed:
			c.signals.ydbPumpkinCacheMiss.Inc()
			cacheLogField := log.Any("ydb_pumpkin_cache", map[string]interface{}{
				"key": cacheKey,
			})
			ctxlog.Debug(ctx, c.Logger, fmt.Sprintf("failed to get data from cache using key %s", cacheKey), cacheLogField)
		default:
			cacheLogField := log.Any("ydb_pumpkin_cache", map[string]interface{}{
				"key": cacheKey,
			})
			ctxlog.Warn(ctx, c.Logger, fmt.Sprintf("unknown cache result for key %s: %v", cacheKey, cacheResult), cacheLogField)
		}
		c.signals.ydbPumpkinCacheTotal.Inc()
	}

	type userInfoMsg struct {
		userInfo model.UserInfo
		err      error
	}
	userInfoCh := make(chan userInfoMsg, 1)
	go func() {
		timeoutCtx, cancelFunc := context.WithTimeout(ctx, time.Second)
		defer cancelFunc()
		userInfo, err := c.UserInfo(timeoutCtx, user)
		userInfoCh <- userInfoMsg{userInfo, err}
		if err == nil {
			setCachedUserInfo(userInfo, time.Hour*12)
		}
	}()

	timer := time.NewTimer(timestamp.AdjustChildRequestTimeout(ctx, c.Logger, timeout))
	select {
	case <-timer.C:
		userInfo, cacheResult := getCachedUserInfo()
		traceCacheUsage(userInfo, cacheResult)
		return userInfo, nil

	case userInfoMsg := <-userInfoCh:
		timer.Stop()
		userInfo, err := userInfoMsg.userInfo, userInfoMsg.err
		if err == nil {
			traceCacheUsage(userInfo, cacheNotUsed)
			return userInfo, nil
		}
		ctxlog.Warnf(ctx, c.Logger, "failed to get user info from database: %v", err)
		if c.Database.IsDatabaseError(err) {
			userInfo, cacheResult := getCachedUserInfo()
			traceCacheUsage(userInfo, cacheResult)
			return userInfo, nil
		}
		traceCacheUsage(userInfo, cacheNotUsed)
		return model.UserInfo{}, err
	}
}

func (c *Controller) UserInfoWithPumpkin(ctx context.Context, user model.User) (model.UserInfo, error) {
	timeout := time.Millisecond * 295 // will use 295ms as base timeout, see https://st.yandex-team.ru/IOT-332
	return c.userInfoWithPumpkinTimeout(ctx, user, timeout)
}

func (c *Controller) SelectScenarioDeviceTriggers(ctx context.Context, user model.User) (model.Devices, error) {
	userDevices, err := c.SelectUserDevices(ctx, user)
	if err != nil {
		return nil, err
	}

	stereopairs, err := c.Database.SelectStereopairs(ctx, user.ID)
	if err != nil {
		return nil, err
	}

	devices := make(model.Devices, 0, len(userDevices))
	for _, device := range userDevices {
		filteredDevice, ok := filterDeviceTrigger(ctx, user, device, stereopairs)
		if ok {
			devices = append(devices, filteredDevice)
		}
	}

	return devices, nil
}

func filterDevices(ctx context.Context, user model.User, devices model.Devices, stereopairs model.Stereopairs) model.Devices {
	filteredDevices := make(model.Devices, 0, len(devices))

	for _, device := range devices {
		filteredDevice, ok := filterDevice(ctx, user, device, stereopairs)
		if ok {
			filteredDevices = append(filteredDevices, filteredDevice)
		}
	}

	return filteredDevices
}

func filterDevice(ctx context.Context, user model.User, device model.Device, stereopairs model.Stereopairs) (model.Device, bool) {
	filteredDevice := device.Clone()
	if stereopairs.GetDeviceRole(device.ID) == model.FollowerRole {
		return model.Device{}, false
	}
	return filteredDevice, true
}

func filterDeviceTrigger(ctx context.Context, user model.User, device model.Device, stereopairs model.Stereopairs) (model.Device, bool) {
	filteredDevice, ok := filterDevice(ctx, user, device, stereopairs)
	if !ok {
		return model.Device{}, false
	}
	return filteredDevice, true
}
