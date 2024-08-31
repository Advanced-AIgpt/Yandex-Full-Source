package db

import (
	"fmt"
	"time"

	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/tuya"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (s *Suite) TestDeviceOwner() {
	s.RunDBTest("GetDeviceOwner", func(client *TestingDBClient) {
		randomUser := s.randomTuyaUser()
		err := client.CreateUser(client.ctx, randomUser.id, randomUser.skillID, randomUser.login, randomUser.tuyaUID)
		s.NoError(err, client.Logs())

		deviceIDs := []string{"tuya-device-id-1", "tuya-device-id-2"}
		owner := tuya.DeviceOwner{
			TuyaUID: randomUser.tuyaUID,
			SkillID: randomUser.skillID,
		}

		cacheMaxAge := 10 * time.Minute

		deviceOwnerSetTime := time.Date(2021, 02, 27, 1, 0, 0, 0, time.Local)
		client.timestamper = &timestamp.TimestamperMock{
			CurrentTimestampValue: timestamp.FromTime(deviceOwnerSetTime),
		}
		err = client.SetDevicesOwner(client.ctx, deviceIDs, owner)
		s.NoError(err, client.Logs())

		s.Run("Success", func() {
			deviceOwnerGetTime := time.Date(2021, 02, 27, 1, 5, 0, 0, time.Local)
			client.timestamper = &timestamp.TimestamperMock{
				CurrentTimestampValue: timestamp.FromTime(deviceOwnerGetTime),
			}
			actualOwner, err := client.GetDeviceOwner(client.ctx, deviceIDs[0], cacheMaxAge)
			s.NoError(err, client.Logs())
			s.Equal(owner, actualOwner)
			actualOwner, err = client.GetDeviceOwner(client.ctx, deviceIDs[1], cacheMaxAge)
			s.NoError(err, client.Logs())
			s.Equal(owner, actualOwner)
		})
		s.RunDBTest("MaxAgeExceeded", func(client *TestingDBClient) {
			deviceOwnerGetTime := time.Date(2021, 02, 27, 1, 15, 0, 0, time.Local)
			client.timestamper = &timestamp.TimestamperMock{
				CurrentTimestampValue: timestamp.FromTime(deviceOwnerGetTime),
			}
			_, err := client.GetDeviceOwner(client.ctx, "unknown-device-id", cacheMaxAge)
			if err == nil {
				s.Fail("device owner should not be found as max age is exceeded by 5 mins")
				return
			}
			if !xerrors.Is(err, ErrNoDeviceOwner) {
				s.Fail(fmt.Sprintf("no device owner err should be returned, but found `%v`", err))
				return
			}
		})
		s.RunDBTest("UnknownDeviceID", func(client *TestingDBClient) {
			deviceOwnerGetTime := time.Date(2021, 02, 27, 1, 5, 0, 0, time.Local)
			client.timestamper = &timestamp.TimestamperMock{
				CurrentTimestampValue: timestamp.FromTime(deviceOwnerGetTime),
			}
			_, err := client.GetDeviceOwner(client.ctx, "unknown-device-id", cacheMaxAge)
			if err == nil {
				s.Fail("device owner should not be found as unknown device id was passed")
				return
			}
			if !xerrors.Is(err, ErrNoDeviceOwner) {
				s.Fail(fmt.Sprintf("no device owner err should be returned, but found `%v`", err))
				return
			}
		})
	})
	s.RunDBTest("InvalidateDeviceOwner", func(client *TestingDBClient) {
		randomUser := s.randomTuyaUser()
		err := client.CreateUser(client.ctx, randomUser.id, randomUser.skillID, randomUser.login, randomUser.tuyaUID)
		s.NoError(err, client.Logs())

		deviceIDs := []string{"tuya-device-id-1", "tuya-device-id-2"}
		owner := tuya.DeviceOwner{
			TuyaUID: randomUser.tuyaUID,
			SkillID: randomUser.skillID,
		}

		// set owner for both devices
		cacheMaxAge := 10 * time.Minute
		deviceOwnerSetTime := time.Date(2021, 02, 27, 1, 0, 0, 0, time.Local)
		client.timestamper = &timestamp.TimestamperMock{
			CurrentTimestampValue: timestamp.FromTime(deviceOwnerSetTime),
		}
		err = client.SetDevicesOwner(client.ctx, deviceIDs, owner)
		s.NoError(err, client.Logs())

		// owners are present
		deviceOwnerGetTime := time.Date(2021, 02, 27, 1, 5, 0, 0, time.Local)
		client.timestamper = &timestamp.TimestamperMock{
			CurrentTimestampValue: timestamp.FromTime(deviceOwnerGetTime),
		}
		actualOwner, err := client.GetDeviceOwner(client.ctx, deviceIDs[0], cacheMaxAge)
		s.NoError(err, client.Logs())
		s.Equal(owner, actualOwner)
		actualOwner, err = client.GetDeviceOwner(client.ctx, deviceIDs[1], cacheMaxAge)
		s.NoError(err, client.Logs())
		s.Equal(owner, actualOwner)

		// invalidate first device
		err = client.InvalidateDeviceOwner(client.ctx, deviceIDs[0])
		s.NoError(err, client.Logs())
		// first device owner should be gone
		_, err = client.GetDeviceOwner(client.ctx, deviceIDs[0], cacheMaxAge)
		if err == nil {
			s.Fail("device owner should not be found as owner was invalidated")
			return
		}
		if !xerrors.Is(err, ErrNoDeviceOwner) {
			s.Fail(fmt.Sprintf("no device owner err should be returned, but found `%v`", err))
			return
		}
		// second device is still alive and breathing
		actualOwner, err = client.GetDeviceOwner(client.ctx, deviceIDs[1], cacheMaxAge)
		s.NoError(err, client.Logs())
		s.Equal(owner, actualOwner)

		// invalidate second device
		err = client.InvalidateDeviceOwner(client.ctx, deviceIDs[1])
		s.NoError(err, client.Logs())
		// second device owner should now be gone too
		_, err = client.GetDeviceOwner(client.ctx, deviceIDs[1], cacheMaxAge)
		if err == nil {
			s.Fail("device owner should not be found as owner was invalidated")
			return
		}
		if !xerrors.Is(err, ErrNoDeviceOwner) {
			s.Fail(fmt.Sprintf("no device owner err should be returned, but found `%v`", err))
			return
		}
	})
}
