package db

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/db/gotest/data"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

func (s *DBClientSuite) TestUserStorageConfig() {
	user := data.GenerateUser()
	err := s.dbClient.StoreUser(s.context, user)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	// Check empty config
	config, err := s.dbClient.SelectUserStorageConfig(s.context, user)
	s.NoError(err)
	expectedConfig := make(model.UserStorageConfig)
	s.Equal(expectedConfig, config)

	closedStoriesTooltip := model.UserStorageValue{
		Type:    model.BoolUserStorageValueType,
		Created: 1,
		Updated: 2,
		Value:   model.BoolUserStorageValue(true),
	}
	err = config.AddValue("closed-stories", closedStoriesTooltip)
	s.NoError(err)

	// check storing
	err = s.dbClient.StoreUserStorageConfig(s.context, user, config)
	s.NoError(err)
	expectedConfig = model.UserStorageConfig{
		"closed-stories": closedStoriesTooltip,
	}
	config, err = s.dbClient.SelectUserStorageConfig(s.context, user)
	s.NoError(err)
	s.Equal(expectedConfig, config)

	// check merge on update
	householdsTooltip := model.UserStorageValue{
		Type:    model.BoolUserStorageValueType,
		Created: 1,
		Updated: 1,
		Value:   model.BoolUserStorageValue(true),
	}
	closedStoriesTooltipOlder := model.UserStorageValue{
		Type:    model.BoolUserStorageValueType,
		Created: 1,
		Updated: 1,
		Value:   model.BoolUserStorageValue(true),
	}
	appStoryTS := model.UserStorageValue{
		Type:    model.FloatUserStorageValueType,
		Created: 1,
		Updated: 1,
		Value:   model.FloatUserStorageValue(1000.0),
	}
	somethingTooltip := model.UserStorageValue{
		Type:    model.StringUserStorageValueType,
		Created: 2,
		Updated: 2,
		Value:   model.StringUserStorageValue("something"),
	}
	structureValue := model.UserStorageValue{
		Type:    model.StructUserStorageValueType,
		Created: 3,
		Updated: 3,
		Value:   model.StructUserStorageValue(`{"kek":"lol"}`),
	}
	newConfig := model.UserStorageConfig{
		"closed-stories": closedStoriesTooltipOlder,
		"households":     householdsTooltip,
		"app-story-ts":   appStoryTS,
		"something":      somethingTooltip,
		"struct":         structureValue,
	}
	err = s.dbClient.StoreUserStorageConfig(s.context, user, newConfig)
	s.NoError(err)

	expectedConfig = model.UserStorageConfig{
		"closed-stories": closedStoriesTooltip,
		"households":     householdsTooltip,
		"app-story-ts":   appStoryTS,
		"something":      somethingTooltip,
		"struct":         structureValue,
	}
	config, err = s.dbClient.SelectUserStorageConfig(s.context, user)
	s.NoError(err)
	s.Equal(expectedConfig, config)

	// deletion
	err = s.dbClient.DeleteUserStorageConfig(s.context, user)
	s.NoError(err)
	expectedConfig = make(model.UserStorageConfig)
	config, err = s.dbClient.SelectUserStorageConfig(s.context, user)
	s.NoError(err)
	s.Equal(expectedConfig, config)
}
