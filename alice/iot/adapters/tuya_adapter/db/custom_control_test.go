package db

import (
	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/db/gotest/data"
	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/tuya"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (s *Suite) TestCustomControl() {
	s.Run("SelectCustomControl", func() {
		s.RunDBTest("Success", func(client *TestingDBClient) {
			randomCustomControl := data.GenerateIRCustomControl()
			userID := "successful_hub_owner"
			err := client.StoreCustomControl(client.ctx, userID, randomCustomControl)
			s.NoError(err, client.Logs())

			expected := tuya.IRCustomControl{
				Name:       randomCustomControl.Name,
				DeviceType: randomCustomControl.DeviceType,
				Buttons:    randomCustomControl.Buttons,
			}
			actual, err := client.SelectCustomControl(client.ctx, userID, randomCustomControl.ID)
			s.NoError(err, client.Logs())

			actual.ID = ""
			s.Equal(expected, actual)
		})
		s.RunDBTest("CustomControlNotFound", func(client *TestingDBClient) {
			_, err := client.SelectCustomControl(client.ctx, "blablaba", "blbalblabla")
			s.Error(err, client.Logs())
			s.True(xerrors.Is(err, &tuya.ErrCustomControlNotFound{}))
		})
	})
	s.Run("DeleteCustomControl", func() {
		s.RunDBTest("Success", func(client *TestingDBClient) {
			randomCustomControl := data.GenerateIRCustomControl()
			userID := "blbablabla"
			err := client.StoreCustomControl(client.ctx, userID, randomCustomControl)
			s.NoError(err, client.Logs())

			expected := tuya.IRCustomControl{
				Name:       randomCustomControl.Name,
				DeviceType: randomCustomControl.DeviceType,
				Buttons:    randomCustomControl.Buttons,
			}
			actual, err := client.SelectCustomControl(client.ctx, userID, randomCustomControl.ID)
			s.NoError(err, client.Logs())

			actual.ID = ""
			s.Equal(expected, actual)

			err = client.DeleteCustomControl(client.ctx, userID, randomCustomControl.ID)
			s.NoError(err, client.Logs())

			_, err = client.SelectCustomControl(client.ctx, userID, randomCustomControl.ID)
			s.Error(err, client.Logs())
			s.True(xerrors.Is(err, &tuya.ErrCustomControlNotFound{}))
		})
	})
}
