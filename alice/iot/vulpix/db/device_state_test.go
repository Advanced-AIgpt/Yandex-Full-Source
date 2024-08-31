package db

import (
	"a.yandex-team.ru/alice/iot/vulpix/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (s *ClientSuite) TestDeviceState() {
	s.Run("SelectUnexistingDeviceState", func() {
		var userID uint64
		speakerID := "my-speaker"
		_, err := s.dbClient.SelectDeviceState(s.context, userID, speakerID)
		s.Require().Error(err)
		s.Require().True(xerrors.Is(err, &model.ErrDeviceStateNotFound{}))

		ts := timestamp.Now()
		deviceState := model.DeviceState{
			Type:    model.ReadyDeviceState,
			Updated: ts,
		}
		err = s.dbClient.StoreDeviceState(s.context, userID, speakerID, deviceState)
		s.Require().NoError(err)
		selectedState, err := s.dbClient.SelectDeviceState(s.context, userID, speakerID)
		s.Require().NoError(err)
		s.Require().Equal(deviceState.ActualStateType(ts), selectedState.ActualStateType(ts))
	})

	s.Run("StoreAndSelectDeviceState", func() {
		var userID uint64
		speakerID := "my-speaker"
		deviceState := model.DeviceState{
			Type:    model.BusyDeviceState,
			Updated: 1,
		}
		err := s.dbClient.StoreDeviceState(s.context, userID, speakerID, deviceState)
		s.Require().NoError(err)

		selectedState, err := s.dbClient.SelectDeviceState(s.context, userID, speakerID)
		s.Require().NoError(err)
		s.Equal(deviceState, selectedState)
	})
}
