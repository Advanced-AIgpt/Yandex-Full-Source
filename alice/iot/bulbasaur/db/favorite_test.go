package db

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/db/gotest/data"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

func (s *DBClientSuite) TestFavorites() {
	user := data.GenerateUser()
	err := s.dbClient.StoreUser(s.context, user)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	humidity := model.MakePropertyByType(model.FloatPropertyType)
	humidity.SetParameters(model.FloatPropertyParameters{
		Instance: model.HumidityPropertyInstance,
		Unit:     model.UnitPercent,
	})

	voltage := model.MakePropertyByType(model.FloatPropertyType)
	voltage.SetParameters(model.FloatPropertyParameters{
		Instance: model.VoltagePropertyInstance,
		Unit:     model.UnitVolt,
	})
	generatedDevice := data.GenerateDevice()
	generatedDevice.Properties = model.Properties{humidity, voltage}
	storedDevice, _, err := s.dbClient.StoreUserDevice(s.context, user, generatedDevice)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	device, err := s.dbClient.SelectUserDevice(s.context, user.ID, storedDevice.ID)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	scenarioID, err := s.dbClient.CreateScenario(s.context, user.ID, data.GenerateScenario("Любимчик", model.Devices{device}))
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	scenario, err := s.dbClient.SelectScenario(s.context, user.ID, scenarioID)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	generatedGroup := data.GenerateGroup()
	generatedGroup.Devices = []string{device.ID}
	groupID, err := s.dbClient.CreateUserGroup(s.context, user, generatedGroup)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	group, err := s.dbClient.SelectUserGroup(s.context, user.ID, groupID)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	emptyFavorites := model.Favorites{
		Scenarios:  make(model.Scenarios, 0),
		Devices:    make(model.Devices, 0),
		Groups:     make(model.Groups, 0),
		Properties: make([]model.FavoritesDeviceProperty, 0),
	}

	favorites, err := s.dbClient.SelectFavorites(s.context, user)
	s.NoError(err)
	s.Equal(emptyFavorites, favorites)

	// make them all favourite
	err = s.dbClient.StoreFavoriteScenarios(s.context, user, model.Scenarios{scenario})
	s.NoError(err)
	err = s.dbClient.StoreFavoriteDevices(s.context, user, model.Devices{device})
	s.NoError(err)
	err = s.dbClient.StoreFavoriteGroups(s.context, user, model.Groups{group})
	s.NoError(err)
	err = s.dbClient.StoreFavoriteProperties(s.context, user, model.FavoritesDeviceProperties{
		{
			DeviceID: device.ID,
			Property: humidity.Clone(),
		},
		{
			DeviceID: device.ID,
			Property: voltage.Clone(),
		},
	})
	s.NoError(err)

	// these flags were actually never filled in favorites lol.
	scenario.Favorite = true
	group.Favorite = true
	device.Favorite = true

	device.Groups = model.Groups{group}
	expected := model.Favorites{
		Scenarios: model.Scenarios{scenario},
		Devices:   model.Devices{device},
		Groups:    model.Groups{group},
		Properties: []model.FavoritesDeviceProperty{
			{
				DeviceID: device.ID,
				Property: humidity.Clone(),
			},
			{
				DeviceID: device.ID,
				Property: voltage.Clone(),
			},
		},
	}
	favorites, err = s.dbClient.SelectFavorites(s.context, user)
	s.NoError(err)
	s.Equal(expected, favorites)

	// delete them from favourites
	err = s.dbClient.DeleteFavoriteScenarios(s.context, user, model.Scenarios{scenario})
	s.NoError(err)
	err = s.dbClient.DeleteFavoriteDevices(s.context, user, model.Devices{device})
	s.NoError(err)
	err = s.dbClient.DeleteFavoriteGroups(s.context, user, model.Groups{group})
	s.NoError(err)
	err = s.dbClient.DeleteFavoriteProperties(s.context, user, model.FavoritesDeviceProperties{
		{
			DeviceID: device.ID,
			Property: humidity.Clone(),
		},
		{
			DeviceID: device.ID,
			Property: voltage.Clone(),
		},
	})
	s.NoError(err)

	favorites, err = s.dbClient.SelectFavorites(s.context, user)
	s.NoError(err)
	s.Equal(emptyFavorites, favorites)
}

func (s *DBClientSuite) TestReplaceFavoriteDevices() {
	user := data.GenerateUser()
	err := s.dbClient.StoreUser(s.context, user)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	humidity := model.MakePropertyByType(model.FloatPropertyType)
	humidity.SetParameters(model.FloatPropertyParameters{
		Instance: model.HumidityPropertyInstance,
		Unit:     model.UnitPercent,
	})

	voltage := model.MakePropertyByType(model.FloatPropertyType)
	voltage.SetParameters(model.FloatPropertyParameters{
		Instance: model.VoltagePropertyInstance,
		Unit:     model.UnitVolt,
	})
	generatedDevice := data.GenerateDevice()
	generatedDevice.Properties = model.Properties{humidity, voltage}
	storedDevice, _, err := s.dbClient.StoreUserDevice(s.context, user, generatedDevice)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	device, err := s.dbClient.SelectUserDevice(s.context, user.ID, storedDevice.ID)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	device.Groups = []model.Group{}
	err = s.dbClient.StoreFavoriteDevices(s.context, user, model.Devices{device})
	s.NoError(err)

	// these flags were actually never filled in favorites lol.
	device.Favorite = true
	expected := model.Favorites{
		Scenarios:  model.Scenarios{},
		Devices:    model.Devices{device},
		Groups:     model.Groups{},
		Properties: []model.FavoritesDeviceProperty{},
	}
	favorites, err := s.dbClient.SelectFavorites(s.context, user)
	s.NoError(err)
	s.Equal(expected, favorites)

	err = s.dbClient.ReplaceFavoriteDevices(s.context, user, model.Devices{})
	s.NoError(err)

	expected = model.Favorites{
		Scenarios:  model.Scenarios{},
		Devices:    model.Devices{},
		Groups:     model.Groups{},
		Properties: []model.FavoritesDeviceProperty{},
	}
	favorites, err = s.dbClient.SelectFavorites(s.context, user)
	s.NoError(err)
	s.Equal(expected, favorites)
}
