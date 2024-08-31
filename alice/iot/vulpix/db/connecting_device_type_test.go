package db

import (
	bmodel "a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/vulpix/model"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (s *ClientSuite) TestConnectingDeviceType() {
	s.Run("SelectAndStoreConnectingDeviceType", func() {
		var userID uint64
		speakerID := "my-speaker"
		_, err := s.dbClient.SelectConnectingDeviceType(s.context, userID, speakerID)
		s.Require().Error(err)
		s.Require().True(xerrors.Is(err, &model.ErrConnectingDeviceTypeNotFound{}))

		err = s.dbClient.StoreConnectingDeviceType(s.context, userID, speakerID, bmodel.LightDeviceType)
		s.Require().NoError(err)
		selectedType, err := s.dbClient.SelectConnectingDeviceType(s.context, userID, speakerID)
		s.Require().NoError(err)
		s.Require().Equal(bmodel.LightDeviceType, selectedType)
	})
}
