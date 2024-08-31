package db

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/db/gotest/data"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (s *DBClientSuite) TestUserNetworks() {
	user := data.GenerateUser()
	network := data.GenerateNetwork()
	secondNetwork := data.GenerateNetwork()

	_, err := s.dbClient.SelectUserNetwork(s.context, user.ID, network.SSID)
	s.True(xerrors.Is(err, &model.UserNetworkNotFoundError{}))

	// store both network in db
	err = s.dbClient.StoreUserNetwork(s.context, user.ID, network)
	s.NoError(err)

	err = s.dbClient.StoreUserNetwork(s.context, user.ID, secondNetwork)
	s.NoError(err)

	// check valid select work
	selectedNetwork, err := s.dbClient.SelectUserNetwork(s.context, user.ID, network.SSID)
	s.NoError(err)
	s.Equal(network, selectedNetwork)

	// check all user network selection
	s.checkNetworks(user.ID, []model.Network{network, secondNetwork})

	// delete both networks
	err = s.dbClient.DeleteUserNetwork(s.context, user.ID, network.SSID)
	s.NoError(err)

	s.checkNetworks(user.ID, []model.Network{secondNetwork})

	err = s.dbClient.DeleteUserNetwork(s.context, user.ID, secondNetwork.SSID)
	s.NoError(err)

	s.checkNetworks(user.ID, []model.Network{})
}
