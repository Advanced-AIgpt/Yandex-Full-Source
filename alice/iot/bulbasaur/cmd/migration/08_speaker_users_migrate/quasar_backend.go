package main

import (
	"context"
	"fmt"
	"sync"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/valid"
	"a.yandex-team.ru/yt/go/ypath"
	"a.yandex-team.ru/yt/go/yt"
)

type quasarBackend struct {
	users         map[string]quasarUser
	userDevices   map[string][]device
	deviceConfigs map[userDeviceKey]deviceConfig
}

func newQuasarBackend(ctx context.Context, yt yt.Client) *quasarBackend {
	q := &quasarBackend{}

	var wg sync.WaitGroup
	wg.Add(1)
	go func() {
		defer wg.Done()
		tablePath := ypath.Path(fmt.Sprintf("//home/quasar-dev/backend/snapshots/%s/account", time.Now().Format("2006-01-02")))
		logger.Infof("reading table `%s`", tablePath.String())
		usersMap := make(map[string]quasarUser)

		type row struct {
			ID    string `yson:"id"`
			Login string `yson:"login"`
		}

		tr, err := yt.ReadTable(ctx, tablePath, nil)
		if err != nil {
			logger.Fatalf("cannot read table at %s: %v", tablePath, err)
		}
		defer func() { _ = tr.Close() }()

		for tr.Next() {
			var r row
			err = tr.Scan(&r)
			if err != nil {
				logger.Fatalf("error while reading row %s: %v", tablePath, err)
			}
			usersMap[r.ID] = quasarUser(r)
		}
		if tr.Err() != nil {
			logger.Fatalf("cannot read table at %s: %v", tablePath, tr.Err())
		}

		logger.Infof("got `%d` users", len(usersMap))
		q.users = usersMap
	}()

	wg.Add(1)
	go func() {
		defer wg.Done()
		tablePath := ypath.Path(fmt.Sprintf("//home/quasar-dev/backend/snapshots/%s/account_device", time.Now().Format("2006-01-02")))
		logger.Infof("reading table `%s`", tablePath.String())
		userDevices := make(map[string][]device)

		type row struct {
			AccountID  string     `yson:"account_id"`
			DeviceID   string     `yson:"device_id"`
			PlatformID platformID `yson:"platform_id"`
		}

		tr, err := yt.ReadTable(ctx, tablePath, nil)
		if err != nil {
			logger.Fatalf("cannot read table at %s: %v", tablePath, err)
		}
		defer func() { _ = tr.Close() }()

		for tr.Next() {
			var r row
			err = tr.Scan(&r)
			if err != nil {
				logger.Fatalf("error while reading row %s: %v", tablePath, err)
			}
			userDevices[r.AccountID] = append(userDevices[r.AccountID], device{r.DeviceID, r.PlatformID, r.AccountID})
		}
		if tr.Err() != nil {
			logger.Fatalf("cannot read table at %s: %v", tablePath, tr.Err())
		}

		logger.Infof("got `%d` quasarUserDevice relationships", len(userDevices))
		q.userDevices = userDevices
	}()

	wg.Add(1)
	go func() {
		defer wg.Done()
		tablePath := ypath.Path(fmt.Sprintf("//home/quasar-dev/backend/snapshots/%s/device_config", time.Now().Format("2006-01-02")))
		logger.Infof("reading table `%s`", tablePath.String())
		deviceConfigsMap := make(map[userDeviceKey]deviceConfig)

		type row struct {
			AccountID  string `yson:"account_id"`
			DeviceID   string `yson:"device_id"`
			JSONConfig string `yson:"json_config"`
		}

		tr, err := yt.ReadTable(ctx, tablePath, nil)
		if err != nil {
			logger.Fatalf("cannot read table at %s: %v", tablePath, err)
		}
		defer func() { _ = tr.Close() }()

		for tr.Next() {
			var r row
			err = tr.Scan(&r)
			if err != nil {
				logger.Fatalf("error while reading row %s: %v", tablePath, err)
			}
			deviceConfigsMap[userDeviceKey{r.AccountID, r.DeviceID}] = deviceConfig{r.AccountID, r.DeviceID, r.JSONConfig}
		}
		if tr.Err() != nil {
			logger.Fatalf("cannot read table at %s: %v", tablePath, tr.Err())
		}

		logger.Infof("got `%d` configs", len(deviceConfigsMap))
		q.deviceConfigs = deviceConfigsMap
	}()

	wg.Wait()
	return q
}

type discoveryResult struct {
	user    quasarUser
	devices []quasarUserDevice
}

func (q *quasarBackend) Discover() <-chan discoveryResult {
	discoveryCh := make(chan discoveryResult)
	vctx := valid.NewValidationCtx()

	go func() {
		defer close(discoveryCh)

		usersProcessed := 0
		usersSkipped := 0
		deviceCount := 0
		for userID := range q.users {
			usersProcessed++
			user, result := q.discover(userID)
			if len(result.Payload.Devices) == 0 {
				usersSkipped++
				continue
			}
			devices := make([]quasarUserDevice, 0, len(result.Payload.Devices))
			for _, deviceInfoView := range result.Payload.Devices {
				deviceCount++
				if _, err := deviceInfoView.Validate(vctx); err != nil {
					err := xerrors.Errorf("can't validate device %v for quasarUser %v: %w", user, deviceInfoView, err)
					logger.Warn(err.Error())
					return
				}
				device := deviceInfoView.ToDevice(model.QUASAR)
				device.Updated = timestamp.Now()
				devices = append(devices, quasarUserDevice{
					user:   user,
					device: device,
				})
			}
			discoveryCh <- discoveryResult{
				user:    user,
				devices: devices,
			}
		}
		logger.Infof("Processed %d users, skipped %d users, will store %d users and %d devices", usersProcessed, usersSkipped, usersProcessed-usersSkipped, deviceCount)
	}()

	return discoveryCh
}

func (q *quasarBackend) discover(uid string) (quasarUser, adapter.DiscoveryResult) {
	var result adapter.DiscoveryResult
	devicesMap := make(map[string]device)
	for _, device := range q.userDevices[uid] {
		devicesMap[device.GetIOTDeviceID()] = device
	}
	for _, device := range devicesMap {
		result.Payload.Devices = append(result.Payload.Devices, device.ToDiscoveryInfoView(q.deviceConfigs))
	}
	return q.users[uid], result
}
