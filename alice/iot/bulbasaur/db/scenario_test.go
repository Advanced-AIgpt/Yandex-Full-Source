package db

import (
	"time"

	"github.com/gofrs/uuid"

	"a.yandex-team.ru/alice/iot/bulbasaur/db/gotest/data"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/ptr"
)

func (s *DBClientSuite) TestSelectScenarios_UnknownUser() {
	user := data.GenerateUser()

	_, err := s.dbClient.SelectScenario(s.context, user.ID, "scenario-id")
	s.True(xerrors.Is(err, &model.ScenarioNotFoundError{}))

	s.checkUserScenarios(user.ID, nil)
}

func (s *DBClientSuite) TestSelectScenarios_RealUser_NoScenarios() {
	user := data.GenerateUser()
	err := s.dbClient.StoreUser(s.context, user)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	_, err = s.dbClient.SelectScenario(s.context, user.ID, "unknown-scenario-id")
	s.True(xerrors.Is(err, &model.ScenarioNotFoundError{}))

	s.checkUserScenarios(user.ID, nil)
}

func (s *DBClientSuite) TestSelectScenarios_RealUser_WithScenarios() {
	user := data.GenerateUser()
	err := s.dbClient.StoreUser(s.context, user)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	scenario1 := data.GenerateScenario("scenario 1", nil)
	scenario1.ID, err = s.dbClient.CreateScenario(s.context, user.ID, scenario1)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	s.checkUserScenarios(user.ID, []model.Scenario{scenario1})

	devices := make([]model.Device, 3)
	for i := range devices {
		for len(devices[i].Capabilities) == 0 {
			devices[i] = data.GenerateDevice()
		}

		storedDevice, _, err := s.dbClient.StoreUserDevice(s.context, user, devices[i])
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		devices[i], err = s.dbClient.SelectUserDevice(s.context, user.ID, storedDevice.ID)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
	}

	scenario2 := data.GenerateScenario("scenario 2", devices)
	scenario2.ID, err = s.dbClient.CreateScenario(s.context, user.ID, scenario2)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	s.checkUserScenarios(user.ID, []model.Scenario{scenario1, scenario2})
}

func (s *DBClientSuite) TestSelectScenarios_RealUser_DeletedScenarios() {
	user := data.GenerateUser()
	err := s.dbClient.StoreUser(s.context, user)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	scenario1 := data.GenerateScenario("scenario 1", nil)
	scenario1.ID, err = s.dbClient.CreateScenario(s.context, user.ID, scenario1)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	err = s.dbClient.DeleteScenario(s.context, user.ID, scenario1.ID)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	scenario2 := data.GenerateScenario("scenario 2", nil)
	scenario2.ID, err = s.dbClient.CreateScenario(s.context, user.ID, scenario2)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	_, err = s.dbClient.SelectScenario(s.context, user.ID, scenario1.ID)
	s.True(xerrors.Is(err, &model.ScenarioNotFoundError{}))

	s.checkUserScenarios(user.ID, []model.Scenario{scenario2})
}

func (s *DBClientSuite) TestSelectScenarios_EvilUser() {
	alice := data.GenerateUser()
	err := s.dbClient.StoreUser(s.context, alice)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	bob := data.GenerateUser()
	err = s.dbClient.StoreUser(s.context, bob)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	aliceScenario := data.GenerateScenario("scenario", nil)
	aliceScenario.ID, err = s.dbClient.CreateScenario(s.context, alice.ID, aliceScenario)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	_, err = s.dbClient.SelectScenario(s.context, bob.ID, aliceScenario.ID)
	s.True(xerrors.Is(err, &model.ScenarioNotFoundError{}))

	s.checkUserScenarios(bob.ID, nil)
}

func (s *DBClientSuite) TestCreateScenario_WithEmptyName() {
	user := data.GenerateUser()
	err := s.dbClient.StoreUser(s.context, user)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	for _, name := range []model.ScenarioName{
		"",
		"   ",
	} {
		_, err = s.dbClient.CreateScenario(s.context, user.ID, model.Scenario{
			Name:     name,
			Triggers: []model.ScenarioTrigger{model.VoiceScenarioTrigger{Phrase: string(name)}},
		})
		s.True(xerrors.Is(err, &model.InvalidValueError{}))
	}

	s.checkUserScenarios(user.ID, nil)
}

func (s *DBClientSuite) TestCreateScenario_NameNormalization() {
	for _, testCase := range []struct {
		CreateWithName model.ScenarioName
		ExpectedName   model.ScenarioName
	}{
		{
			CreateWithName: "some scenario 123",
			ExpectedName:   "some scenario 123",
		},
		{
			CreateWithName: "some    scenario  123",
			ExpectedName:   "some scenario 123",
		},
		{
			CreateWithName: "  some scenario 123",
			ExpectedName:   "some scenario 123",
		},
		{
			CreateWithName: "some scenario 123    ",
			ExpectedName:   "some scenario 123",
		},
		{
			CreateWithName: "  some scenario 123         ",
			ExpectedName:   "some scenario 123",
		},
		{
			CreateWithName: "  some    scenario        123    ",
			ExpectedName:   "some scenario 123",
		},
		{
			CreateWithName: "  some    scenario123    ",
			ExpectedName:   "some scenario123",
		},
		{
			CreateWithName: "  somescenario123    ",
			ExpectedName:   "somescenario123",
		},
	} {
		user := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, user)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		scenario := model.Scenario{
			Name:     testCase.CreateWithName,
			Triggers: []model.ScenarioTrigger{model.VoiceScenarioTrigger{Phrase: string(testCase.CreateWithName)}},
			Steps:    model.ScenarioSteps{},
			IsActive: true,
		}
		scenario.ID, err = s.dbClient.CreateScenario(s.context, user.ID, scenario)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		scenario.Name = testCase.ExpectedName
		s.checkUserScenarios(user.ID, []model.Scenario{scenario})
	}
}

func (s *DBClientSuite) TestCreateScenario_VoiceTriggerNormalization() {
	for _, testCase := range []struct {
		CreateVoiceTriggerPhrase   string
		ExpectedVoiceTriggerPhrase string
	}{
		{
			CreateVoiceTriggerPhrase:   "some scenario 123",
			ExpectedVoiceTriggerPhrase: "some scenario 123",
		},
		{
			CreateVoiceTriggerPhrase:   "some    scenario  123",
			ExpectedVoiceTriggerPhrase: "some scenario 123",
		},
		{
			CreateVoiceTriggerPhrase:   "  some scenario 123",
			ExpectedVoiceTriggerPhrase: "some scenario 123",
		},
		{
			CreateVoiceTriggerPhrase:   "some scenario 123    ",
			ExpectedVoiceTriggerPhrase: "some scenario 123",
		},
		{
			CreateVoiceTriggerPhrase:   "  some scenario 123         ",
			ExpectedVoiceTriggerPhrase: "some scenario 123",
		},
		{
			CreateVoiceTriggerPhrase:   "  some    scenario        123    ",
			ExpectedVoiceTriggerPhrase: "some scenario 123",
		},
		{
			CreateVoiceTriggerPhrase:   "  some    scenario123    ",
			ExpectedVoiceTriggerPhrase: "some scenario123",
		},
		{
			CreateVoiceTriggerPhrase:   "  somescenario123    ",
			ExpectedVoiceTriggerPhrase: "somescenario123",
		},
	} {
		user := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, user)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		scenario := model.Scenario{
			Name:     "scenario",
			IsActive: true,
			Triggers: []model.ScenarioTrigger{model.VoiceScenarioTrigger{Phrase: testCase.CreateVoiceTriggerPhrase}},
			Steps:    model.ScenarioSteps{},
		}
		scenario.ID, err = s.dbClient.CreateScenario(s.context, user.ID, scenario)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		s.Len(scenario.Triggers, 1)
		s.IsType(model.VoiceScenarioTrigger{}, scenario.Triggers[0])
		s.Equal(testCase.ExpectedVoiceTriggerPhrase, scenario.Triggers[0].(model.VoiceScenarioTrigger).Phrase)
		s.checkUserScenarios(user.ID, []model.Scenario{scenario})
	}
}

func (s *DBClientSuite) TestCreateScenario_WithDuplicateName() {
	var err error

	user := data.GenerateUser()
	err = s.dbClient.StoreUser(s.context, user)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	scenario := model.Scenario{
		Name:     "some scenario 123",
		Triggers: []model.ScenarioTrigger{model.VoiceScenarioTrigger{Phrase: "some scenario 123"}},
		Steps:    model.ScenarioSteps{},
		IsActive: true,
	}
	scenario.ID, err = s.dbClient.CreateScenario(s.context, user.ID, scenario)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	s.checkUserScenarios(user.ID, []model.Scenario{scenario})

	for _, name := range []model.ScenarioName{
		"some scenario 123",
		"        some scenario 123",
		"some scenario 123      ",
		"    some scenario 123      ",
		"some  scenario  123",
		"   some  scenario  123  ",
	} {
		_, err = s.dbClient.CreateScenario(s.context, user.ID, model.Scenario{
			Name:     name,
			Triggers: []model.ScenarioTrigger{model.VoiceScenarioTrigger{Phrase: string(name)}},
		})
		s.True(xerrors.Is(err, &model.NameIsAlreadyTakenError{}))

		s.checkUserScenarios(user.ID, []model.Scenario{scenario})
	}
}

func (s *DBClientSuite) TestCreateScenario_WithRealDevices() {
	user := data.GenerateUser()
	err := s.dbClient.StoreUser(s.context, user)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	devices := make([]model.Device, 3)
	for i := range devices {
		for len(devices[i].Capabilities) == 0 {
			devices[i] = data.GenerateDevice()
		}

		storedDevice, _, err := s.dbClient.StoreUserDevice(s.context, user, devices[i])
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		devices[i], err = s.dbClient.SelectUserDevice(s.context, user.ID, storedDevice.ID)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
	}

	scenario := data.GenerateScenario("scenario", devices)
	scenario.ID, err = s.dbClient.CreateScenario(s.context, user.ID, scenario)
	s.NoError(err)

	s.checkUserScenarios(user.ID, []model.Scenario{scenario})
}

func (s *DBClientSuite) TestCreateScenario_WithUnknownDevices() {
	user := data.GenerateUser()
	err := s.dbClient.StoreUser(s.context, user)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	devices := make([]model.Device, 3)
	for i := range devices {
		for len(devices[i].Capabilities) == 0 {
			devices[i] = data.GenerateDevice()
		}

		if i%2 == 0 { // save device
			storedDevice, _, err := s.dbClient.StoreUserDevice(s.context, user, devices[i])
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}

			devices[i], err = s.dbClient.SelectUserDevice(s.context, user.ID, storedDevice.ID)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}
		} else { // don't save
			id, err := uuid.NewV4()
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}
			devices[i].ID = id.String()
		}
	}

	scenario := data.GenerateScenario("scenario", devices)
	scenario.ID, err = s.dbClient.CreateScenario(s.context, user.ID, scenario)
	//s.Equal(&model.DeviceNotFoundError{}, errors.Cause(err)) // TODO: QUASAR-4261
	s.NoError(err)

	//s.checkUserScenarios(user.ID, nil) // TODO: QUASAR-4261
	s.checkUserScenarios(user.ID, []model.Scenario{scenario})
}

func (s *DBClientSuite) TestCreateScenario_WithDeletedDevices() {
	user := data.GenerateUser()
	err := s.dbClient.StoreUser(s.context, user)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	devices := make([]model.Device, 3)
	for i := range devices {
		for len(devices[i].Capabilities) == 0 {
			devices[i] = data.GenerateDevice()
		}

		storedDevice, _, err := s.dbClient.StoreUserDevice(s.context, user, devices[i])
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		devices[i], err = s.dbClient.SelectUserDevice(s.context, user.ID, storedDevice.ID)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		if i%2 == 0 { // delete some devices
			err = s.dbClient.DeleteUserDevice(s.context, user.ID, devices[i].ID)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}
		}
	}

	scenario := data.GenerateScenario("scenario", devices)
	scenario.ID, err = s.dbClient.CreateScenario(s.context, user.ID, scenario)
	//s.Equal(&model.DeviceNotFoundError{}, errors.Cause(err)) // TODO: QUASAR-4261
	s.NoError(err)

	//s.checkUserScenarios(user.ID, nil) // TODO: QUASAR-4261
	s.checkUserScenarios(user.ID, []model.Scenario{scenario})
}

func (s *DBClientSuite) TestCreateScenario_EvilUser() {
	alice := data.GenerateUser()
	err := s.dbClient.StoreUser(s.context, alice)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	aliceDevices := make([]model.Device, 2)
	for i := range aliceDevices {
		for len(aliceDevices[i].Capabilities) == 0 {
			aliceDevices[i] = data.GenerateDevice()
		}

		storedDevice, _, err := s.dbClient.StoreUserDevice(s.context, alice, aliceDevices[i])
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		aliceDevices[i], err = s.dbClient.SelectUserDevice(s.context, alice.ID, storedDevice.ID)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
	}

	bob := data.GenerateUser()
	err = s.dbClient.StoreUser(s.context, bob)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	bobDevices := make([]model.Device, 2)
	for i := range bobDevices {
		for len(bobDevices[i].Capabilities) == 0 {
			bobDevices[i] = data.GenerateDevice()
		}

		storedDevice, _, err := s.dbClient.StoreUserDevice(s.context, bob, bobDevices[i])
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		bobDevices[i], err = s.dbClient.SelectUserDevice(s.context, bob.ID, storedDevice.ID)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
	}

	scenario := data.GenerateScenario("scenario", append(aliceDevices, bobDevices...))
	scenario.ID, err = s.dbClient.CreateScenario(s.context, alice.ID, scenario)
	//s.Equal(&model.DeviceNotFoundError{}, errors.Cause(err)) // TODO: QUASAR-4261
	s.NoError(err)

	//s.checkUserScenarios(alice.ID, nil) // TODO: QUASAR-4261
	s.checkUserScenarios(alice.ID, []model.Scenario{scenario})

	s.checkUserScenarios(bob.ID, nil)
}

func (s *DBClientSuite) TestUpdateScenario_UnknownScenario() {
	user := data.GenerateUser()
	err := s.dbClient.StoreUser(s.context, user)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	devices := make([]model.Device, 6)
	for i := range devices {
		for len(devices[i].Capabilities) == 0 {
			devices[i] = data.GenerateDevice()
		}

		storedDevice, _, err := s.dbClient.StoreUserDevice(s.context, user, devices[i])
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		devices[i], err = s.dbClient.SelectUserDevice(s.context, user.ID, storedDevice.ID)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
	}

	scenario := data.GenerateScenario("scenario", devices[:3])
	scenario.ID, err = s.dbClient.CreateScenario(s.context, user.ID, scenario)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	updatedScenario := data.GenerateScenario("new scenario", devices[3:])
	updatedScenario.ID = "unknown-scenario-id"
	err = s.dbClient.UpdateScenario(s.context, user.ID, updatedScenario)
	s.True(xerrors.Is(err, &model.ScenarioNotFoundError{}))

	s.checkUserScenarios(user.ID, []model.Scenario{scenario})
}

func (s *DBClientSuite) TestUpdateScenario_AllData() {
	user := data.GenerateUser()
	err := s.dbClient.StoreUser(s.context, user)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	devices := make([]model.Device, 6)
	for i := range devices {
		for len(devices[i].Capabilities) == 0 {
			devices[i] = data.GenerateDevice()
		}

		storedDevice, _, err := s.dbClient.StoreUserDevice(s.context, user, devices[i])
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		devices[i], err = s.dbClient.SelectUserDevice(s.context, user.ID, storedDevice.ID)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
	}

	scenario := data.GenerateScenario("scenario", devices[:3])
	scenario.ID, err = s.dbClient.CreateScenario(s.context, user.ID, scenario)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	updatedScenario := data.GenerateScenario("new scenario", devices[3:])
	updatedScenario.ID = scenario.ID
	err = s.dbClient.UpdateScenario(s.context, user.ID, updatedScenario)
	s.NoError(err)

	s.checkUserScenarios(user.ID, []model.Scenario{updatedScenario})
}

func (s *DBClientSuite) TestUpdateScenario_RenameToEmptyName() {
	var err error

	user := data.GenerateUser()
	err = s.dbClient.StoreUser(s.context, user)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	scenario := model.Scenario{
		Name:     "some scenario",
		Triggers: []model.ScenarioTrigger{model.VoiceScenarioTrigger{Phrase: "some scenario"}},
		IsActive: true,
		Steps:    model.ScenarioSteps{},
	}
	scenario.ID, err = s.dbClient.CreateScenario(s.context, user.ID, scenario)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	for _, newName := range []model.ScenarioName{
		"",
		"   ",
	} {
		scenario.Name = newName
		err = s.dbClient.UpdateScenario(s.context, user.ID, scenario)
		s.True(xerrors.Is(err, &model.InvalidValueError{}))

		scenario.Name = "some scenario"
		s.checkUserScenarios(user.ID, []model.Scenario{scenario})
	}
}

func (s *DBClientSuite) TestUpdateScenario_RenameToSameName() {
	var err error

	user := data.GenerateUser()
	err = s.dbClient.StoreUser(s.context, user)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	scenario := model.Scenario{
		Name:     "some scenario 123",
		Triggers: []model.ScenarioTrigger{model.VoiceScenarioTrigger{Phrase: "some scenario 123"}},
		Steps:    model.ScenarioSteps{},
	}
	scenario.ID, err = s.dbClient.CreateScenario(s.context, user.ID, scenario)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	for _, newName := range []model.ScenarioName{
		"some scenario 123",
		"        some scenario 123",
		"some scenario 123      ",
		"    some scenario 123      ",
		"some  scenario  123",
		"   some  scenario  123  ",
	} {
		scenario.Name = newName
		err = s.dbClient.UpdateScenario(s.context, user.ID, scenario)
		s.NoError(err)

		scenario.Name = "some scenario 123"
		s.checkUserScenarios(user.ID, []model.Scenario{scenario})
	}
}

func (s *DBClientSuite) TestUpdateScenario_RenameToDuplicateName() {
	var err error

	user := data.GenerateUser()
	err = s.dbClient.StoreUser(s.context, user)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	scenario1 := model.Scenario{
		Name:     "some scenario 123",
		Triggers: []model.ScenarioTrigger{model.VoiceScenarioTrigger{Phrase: "some scenario 123"}},
		Steps:    model.ScenarioSteps{},
		IsActive: true,
	}
	scenario1.ID, err = s.dbClient.CreateScenario(s.context, user.ID, scenario1)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	scenario2 := model.Scenario{
		Name:     "another scenario",
		Triggers: []model.ScenarioTrigger{model.VoiceScenarioTrigger{Phrase: "another scenario"}},
		Steps:    model.ScenarioSteps{},
		IsActive: true,
	}
	scenario2.ID, err = s.dbClient.CreateScenario(s.context, user.ID, scenario2)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	s.checkUserScenarios(user.ID, []model.Scenario{scenario1, scenario2})

	for _, newName := range []model.ScenarioName{
		"some scenario 123",
		"        some scenario 123",
		"some scenario 123      ",
		"    some scenario 123      ",
		"some  scenario  123",
		"   some  scenario  123  ",
	} {
		scenario2.Name = newName
		err = s.dbClient.UpdateScenario(s.context, user.ID, scenario2)
		s.True(xerrors.Is(err, &model.NameIsAlreadyTakenError{}))
	}

	scenario2.Name = "another scenario"
	s.checkUserScenarios(user.ID, []model.Scenario{scenario1, scenario2})
}

func (s *DBClientSuite) TestUpdateScenario_FromEmptyToDevices() {
	user := data.GenerateUser()
	err := s.dbClient.StoreUser(s.context, user)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	scenario := data.GenerateScenario("scenario", nil)
	scenario.ID, err = s.dbClient.CreateScenario(s.context, user.ID, scenario)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	devices := make([]model.Device, 3)
	for i := range devices {
		for len(devices[i].Capabilities) == 0 {
			devices[i] = data.GenerateDevice()
		}

		storedDevice, _, err := s.dbClient.StoreUserDevice(s.context, user, devices[i])
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		devices[i], err = s.dbClient.SelectUserDevice(s.context, user.ID, storedDevice.ID)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
	}

	updatedScenario := data.GenerateScenario("new scenario", devices)
	updatedScenario.ID = scenario.ID
	err = s.dbClient.UpdateScenario(s.context, user.ID, updatedScenario)
	s.NoError(err)

	s.checkUserScenarios(user.ID, []model.Scenario{updatedScenario})
}

func (s *DBClientSuite) TestUpdateScenario_FromDevicesToEmpty() {
	user := data.GenerateUser()
	err := s.dbClient.StoreUser(s.context, user)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	devices := make([]model.Device, 3)
	for i := range devices {
		for len(devices[i].Capabilities) == 0 {
			devices[i] = data.GenerateDevice()
		}

		storedDevice, _, err := s.dbClient.StoreUserDevice(s.context, user, devices[i])
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		devices[i], err = s.dbClient.SelectUserDevice(s.context, user.ID, storedDevice.ID)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
	}

	scenario := data.GenerateScenario("scenario", devices)
	scenario.ID, err = s.dbClient.CreateScenario(s.context, user.ID, scenario)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	updatedScenario := data.GenerateScenario("new scenario", nil)
	updatedScenario.ID = scenario.ID
	err = s.dbClient.UpdateScenario(s.context, user.ID, updatedScenario)
	s.NoError(err)

	s.checkUserScenarios(user.ID, []model.Scenario{updatedScenario})
}

func (s *DBClientSuite) TestUpdateScenario_ToUnknownDevices() {
	user := data.GenerateUser()
	err := s.dbClient.StoreUser(s.context, user)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	devices := make([]model.Device, 3)
	for i := range devices {
		for len(devices[i].Capabilities) == 0 {
			devices[i] = data.GenerateDevice()
		}

		storedDevice, _, err := s.dbClient.StoreUserDevice(s.context, user, devices[i])
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		devices[i], err = s.dbClient.SelectUserDevice(s.context, user.ID, storedDevice.ID)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
	}

	scenario := data.GenerateScenario("scenario", devices)
	scenario.ID, err = s.dbClient.CreateScenario(s.context, user.ID, scenario)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	newDevices := make([]model.Device, 3)
	for i := range newDevices {
		for len(newDevices[i].Capabilities) == 0 {
			newDevices[i] = data.GenerateDevice()
		}

		if i%2 == 0 { // save device
			storedDevice, _, err := s.dbClient.StoreUserDevice(s.context, user, newDevices[i])
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}

			newDevices[i], err = s.dbClient.SelectUserDevice(s.context, user.ID, storedDevice.ID)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}
		} else { // don't save
			id, err := uuid.NewV4()
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}
			newDevices[i].ID = id.String()
		}
	}

	updatedScenario := data.GenerateScenario("new scenario", newDevices)
	updatedScenario.ID = scenario.ID
	err = s.dbClient.UpdateScenario(s.context, user.ID, updatedScenario)
	//s.Equal(&model.DeviceNotFoundError{}, errors.Cause(err)) // TODO: QUASAR-4261
	s.NoError(err)

	//s.checkUserScenarios(user.ID, []model.Scenario{scenario}) // TODO: QUASAR-4261
	s.checkUserScenarios(user.ID, []model.Scenario{updatedScenario})
}

func (s *DBClientSuite) TestUpdateScenario_ToDeletedDevices() {
	user := data.GenerateUser()
	err := s.dbClient.StoreUser(s.context, user)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	devices := make([]model.Device, 3)
	for i := range devices {
		for len(devices[i].Capabilities) == 0 {
			devices[i] = data.GenerateDevice()
		}

		storedDevice, _, err := s.dbClient.StoreUserDevice(s.context, user, devices[i])
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		devices[i], err = s.dbClient.SelectUserDevice(s.context, user.ID, storedDevice.ID)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
	}

	scenario := data.GenerateScenario("scenario", devices)
	scenario.ID, err = s.dbClient.CreateScenario(s.context, user.ID, scenario)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	newDevices := make([]model.Device, 3)
	for i := range newDevices {
		for len(newDevices[i].Capabilities) == 0 {
			newDevices[i] = data.GenerateDevice()
		}

		storedDevice, _, err := s.dbClient.StoreUserDevice(s.context, user, newDevices[i])
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		newDevices[i], err = s.dbClient.SelectUserDevice(s.context, user.ID, storedDevice.ID)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		if i%2 == 0 { // delete some devices
			err = s.dbClient.DeleteUserDevice(s.context, user.ID, newDevices[i].ID)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}
		}
	}

	updatedScenario := data.GenerateScenario("new scenario", newDevices)
	updatedScenario.ID = scenario.ID
	err = s.dbClient.UpdateScenario(s.context, user.ID, updatedScenario)
	//s.Equal(&model.DeviceNotFoundError{}, errors.Cause(err)) // TODO: QUASAR-4261
	s.NoError(err)

	//s.checkUserScenarios(user.ID, []model.Scenario{scenario}) // TODO: QUASAR-4261
	s.checkUserScenarios(user.ID, []model.Scenario{updatedScenario})
}

func (s *DBClientSuite) TestUpdateScenario_EvilUser() {
	alice := data.GenerateUser()
	err := s.dbClient.StoreUser(s.context, alice)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	aliceDevices := make([]model.Device, 2)
	for i := range aliceDevices {
		for len(aliceDevices[i].Capabilities) == 0 {
			aliceDevices[i] = data.GenerateDevice()
		}

		storedDevice, _, err := s.dbClient.StoreUserDevice(s.context, alice, aliceDevices[i])
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		aliceDevices[i], err = s.dbClient.SelectUserDevice(s.context, alice.ID, storedDevice.ID)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
	}

	scenario := data.GenerateScenario("scenario", aliceDevices)
	scenario.ID, err = s.dbClient.CreateScenario(s.context, alice.ID, scenario)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	bob := data.GenerateUser()
	err = s.dbClient.StoreUser(s.context, bob)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	bobDevices := make([]model.Device, 2)
	for i := range bobDevices {
		for len(bobDevices[i].Capabilities) == 0 {
			bobDevices[i] = data.GenerateDevice()
		}

		storedDevice, _, err := s.dbClient.StoreUserDevice(s.context, bob, bobDevices[i])
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		bobDevices[i], err = s.dbClient.SelectUserDevice(s.context, bob.ID, storedDevice.ID)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
	}

	updatedScenario := data.GenerateScenario("new scenario", append(aliceDevices, bobDevices...))
	updatedScenario.ID = scenario.ID
	err = s.dbClient.UpdateScenario(s.context, alice.ID, updatedScenario)
	//s.Equal(&model.DeviceNotFoundError{}, errors.Cause(err)) // TODO: QUASAR-4261
	s.NoError(err)

	//s.checkUserScenarios(alice.ID, []model.Scenario{scenario}) // TODO: QUASAR-4261
	s.checkUserScenarios(alice.ID, []model.Scenario{updatedScenario})

	s.checkUserScenarios(bob.ID, nil)
}

func (s *DBClientSuite) TestDeleteUserScenario_UnknownUser() {
	user := data.GenerateUser()

	s.checkUserScenarios(user.ID, nil)

	err := s.dbClient.DeleteScenario(s.context, user.ID, "scenario-id")
	s.NoError(err)

	s.checkUserScenarios(user.ID, nil)
}

func (s *DBClientSuite) TestDeleteUserScenario_UnknownScenario() {
	user := data.GenerateUser()
	err := s.dbClient.StoreUser(s.context, user)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	s.checkUserScenarios(user.ID, nil)

	err = s.dbClient.DeleteScenario(s.context, user.ID, "unknown-scenario-id")
	s.NoError(err)

	s.checkUserScenarios(user.ID, nil)
}

func (s *DBClientSuite) TestDeleteUserScenario_RealScenario() {
	user := data.GenerateUser()
	err := s.dbClient.StoreUser(s.context, user)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	scenario1 := data.GenerateScenario("scenario 1", nil)
	scenario1.ID, err = s.dbClient.CreateScenario(s.context, user.ID, scenario1)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	devices := make([]model.Device, 3)
	for i := range devices {
		for len(devices[i].Capabilities) == 0 {
			devices[i] = data.GenerateDevice()
		}

		storedDevice, _, err := s.dbClient.StoreUserDevice(s.context, user, devices[i])
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		devices[i], err = s.dbClient.SelectUserDevice(s.context, user.ID, storedDevice.ID)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
	}

	scenario2 := data.GenerateScenario("scenario 2", devices)
	scenario2.ID, err = s.dbClient.CreateScenario(s.context, user.ID, scenario2)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	s.checkUserScenarios(user.ID, []model.Scenario{scenario1, scenario2})

	err = s.dbClient.DeleteScenario(s.context, user.ID, scenario1.ID)
	s.NoError(err)

	s.checkUserScenarios(user.ID, []model.Scenario{scenario2})

	err = s.dbClient.DeleteScenario(s.context, user.ID, scenario2.ID)
	s.NoError(err)

	s.checkUserScenarios(user.ID, nil)
}

func (s *DBClientSuite) TestDeleteUserScenario_Repeat() {
	user := data.GenerateUser()
	err := s.dbClient.StoreUser(s.context, user)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	scenario := data.GenerateScenario("scenario", nil)
	scenario.ID, err = s.dbClient.CreateScenario(s.context, user.ID, scenario)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	s.checkUserScenarios(user.ID, []model.Scenario{scenario})

	err = s.dbClient.DeleteScenario(s.context, user.ID, scenario.ID)
	s.NoError(err)

	s.checkUserScenarios(user.ID, nil)

	err = s.dbClient.DeleteScenario(s.context, user.ID, scenario.ID)
	s.NoError(err)

	s.checkUserScenarios(user.ID, nil)
}

func (s *DBClientSuite) TestDeleteUserScenario_EvilUser() {
	alice := data.GenerateUser()
	err := s.dbClient.StoreUser(s.context, alice)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	aliceScenario := data.GenerateScenario("scenario", nil)
	aliceScenario.ID, err = s.dbClient.CreateScenario(s.context, alice.ID, aliceScenario)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	bob := data.GenerateUser()
	err = s.dbClient.StoreUser(s.context, bob)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	err = s.dbClient.DeleteScenario(s.context, bob.ID, aliceScenario.ID)
	s.NoError(err)

	s.checkUserScenarios(alice.ID, []model.Scenario{aliceScenario})

	s.checkUserScenarios(bob.ID, nil)

	// repeat, just in case
	err = s.dbClient.DeleteScenario(s.context, bob.ID, aliceScenario.ID)
	s.NoError(err)

	s.checkUserScenarios(alice.ID, []model.Scenario{aliceScenario})

	s.checkUserScenarios(bob.ID, nil)
}

func (s *DBClientSuite) TestScenarioTriggers() {
	alice := data.GenerateUser()
	err := s.dbClient.StoreUser(s.context, alice)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	scenario := data.GenerateScenario("scenario", nil)
	scenario.Triggers = []model.ScenarioTrigger{
		model.VoiceScenarioTrigger{Phrase: "Hello, world!"},
		model.TimerScenarioTrigger{Time: timestamp.Now()},
	}
	scenario.ID, err = s.dbClient.CreateScenario(s.context, alice.ID, scenario)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	scenarios, err := s.dbClient.SelectUserScenarios(s.context, alice.ID)
	s.Require().NoError(err)
	s.Len(scenarios, 1)
	s.Len(scenarios[0].Triggers, 2)
	s.IsType(model.VoiceScenarioTrigger{}, scenarios[0].Triggers[0])
	s.IsType(model.TimerScenarioTrigger{}, scenarios[0].Triggers[1])
}

func (s *DBClientSuite) TestCancelScenarioLaunchesByTriggerTypeAndStatus() {
	alice := data.GenerateUser()
	err := s.dbClient.StoreUser(s.context, alice)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	scenario := data.GenerateScenario("scenario", nil)
	launch, err := scenario.ToScheduledLaunch(
		s.dbClient.timestamper.CurrentTimestamp(),
		s.dbClient.CurrentTimestamp(),
		model.TimetableScenarioTrigger{
			Condition: model.SpecificTimeCondition{},
		},
		nil,
	)
	s.Require().NoError(err)
	launch.ID, err = s.dbClient.StoreScenarioLaunch(s.context, alice.ID, launch)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	launches, err := s.dbClient.SelectScenarioLaunchList(s.context, alice.ID, 100, []model.ScenarioTriggerType{model.TimetableScenarioTriggerType})
	s.Require().NoError(err)
	s.Len(launches, 1)
	s.Equal(launches[0].Status, model.ScenarioLaunchScheduled)

	err = s.dbClient.CancelScenarioLaunchesByTriggerTypeAndStatus(s.context, alice.ID, model.TimetableScenarioTriggerType, model.ScenarioLaunchScheduled)
	s.Require().NoError(err)

	launches, err = s.dbClient.SelectScenarioLaunchList(s.context, alice.ID, 100, []model.ScenarioTriggerType{model.TimetableScenarioTriggerType})
	s.Require().NoError(err)
	s.Len(launches, 1)
	s.Equal(launches[0].Status, model.ScenarioLaunchCanceled)
	s.Equal(launches[0].Finished, s.dbClient.timestamper.CurrentTimestamp())
}

func (s *DBClientSuite) TestSelectScenariosLaunchList() {
	alice := data.GenerateUser()
	err := s.dbClient.StoreUser(s.context, alice)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	now := timestamp.Now()

	scenarioIDs := make(map[string]string)

	scenarioInputs := []struct {
		Name         string
		Status       model.ScenarioLaunchStatus
		ScheduleTime timestamp.PastTimestamp
	}{
		{
			Name:         "done scenario 1",
			Status:       model.ScenarioLaunchDone,
			ScheduleTime: now.Add(-30 * time.Minute),
		},
		{
			Name:         "done scenario 2",
			Status:       model.ScenarioLaunchDone,
			ScheduleTime: now.Add(-15 * time.Minute),
		},
		{
			Name:         "done scenario 3",
			Status:       model.ScenarioLaunchDone,
			ScheduleTime: now.Add(-10 * time.Minute),
		},
		{
			Name:         "done scenario 4",
			Status:       model.ScenarioLaunchDone,
			ScheduleTime: now.Add(-45 * time.Minute),
		},
		{
			Name:         "failed scenario",
			Status:       model.ScenarioLaunchFailed,
			ScheduleTime: now.Add(-60 * time.Minute),
		},
		{
			Name:         "scheduled scenario",
			Status:       model.ScenarioLaunchScheduled,
			ScheduleTime: now.Add(1 * time.Minute),
		},
	}

	for _, input := range scenarioInputs {
		launch := model.NewScenarioLaunch().
			WithTriggerType(model.TimerScenarioTriggerType).
			WithCreatedTime(now.Add(-24 * time.Hour)).
			WithScheduledTime(input.ScheduleTime).
			WithStatus(input.Status)

		if input.Status != model.ScenarioLaunchScheduled {
			launch = launch.WithFinishedTime(input.ScheduleTime)
		}

		launchID, err := s.dbClient.StoreScenarioLaunch(s.context, alice.ID, *launch)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
		scenarioIDs[input.Name] = launchID
	}

	scenarios, err := s.dbClient.SelectScenarioLaunchList(s.context, alice.ID, 1000, []model.ScenarioTriggerType{model.TimerScenarioTriggerType})
	s.Require().NoError(err)

	expectedScenarioOrder := []string{
		scenarioIDs["done scenario 3"],
		scenarioIDs["done scenario 2"],
		scenarioIDs["done scenario 1"],
		scenarioIDs["done scenario 4"],
		scenarioIDs["failed scenario"],
		scenarioIDs["scheduled scenario"],
	}

	actualScenarioOrder := make([]string, 0, len(scenarios))
	for _, s := range scenarios {
		actualScenarioOrder = append(actualScenarioOrder, s.ID)
	}

	s.Equal(expectedScenarioOrder, actualScenarioOrder)
}

func (s *DBClientSuite) TestCreateScenario_DevicePropertyTriggerScenario() {
	user := data.GenerateUser()
	err := s.dbClient.StoreUser(s.context, user)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	device := data.GenerateDevice()
	device.Properties = model.Properties{
		model.MakePropertyByType(model.FloatPropertyType).
			WithState(model.FloatPropertyState{
				Instance: model.TemperaturePropertyInstance,
				Value:    36.6,
			}),
		model.MakePropertyByType(model.EventPropertyType).
			WithState(model.EventPropertyState{
				Instance: model.MotionPropertyInstance,
				Value:    model.ClosedEvent,
			}),
	}
	device, _, err = s.dbClient.StoreUserDevice(s.context, user, device)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	scenario := data.GenerateScenario("scenario", nil)
	scenario.Triggers = []model.ScenarioTrigger{
		model.DevicePropertyScenarioTrigger{
			DeviceID:     device.ID,
			PropertyType: model.FloatPropertyType,
			Instance:     string(model.TemperaturePropertyInstance),
			Condition: model.FloatPropertyCondition{
				LowerBound: ptr.Float64(36.7),
				UpperBound: ptr.Float64(36.9),
			},
		},
		model.DevicePropertyScenarioTrigger{
			DeviceID:     device.ID,
			PropertyType: model.EventPropertyType,
			Instance:     string(model.MotionPropertyInstance),
			Condition: model.EventPropertyCondition{
				Values: []model.EventValue{model.OpenedEvent},
			},
		},
	}
	scenario.PushOnInvoke = true

	scenario.ID, err = s.dbClient.CreateScenario(s.context, user.ID, scenario)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	temperatureIndexKey := model.DeviceTriggerIndexKey{
		DeviceID:      device.ID,
		TriggerEntity: model.PropertyEntity,
		Type:          model.FloatPropertyType.String(),
		Instance:      model.TemperaturePropertyInstance.String(),
	}
	indexes, err := s.dbClient.SelectDeviceTriggersIndexes(s.context, user.ID, temperatureIndexKey)
	s.Require().NoError(err)
	s.Len(indexes, 1)

	motionIndexKey := model.DeviceTriggerIndexKey{
		DeviceID:      device.ID,
		TriggerEntity: model.PropertyEntity,
		Type:          model.EventPropertyType.String(),
		Instance:      model.MotionPropertyInstance.String(),
	}
	indexes, err = s.dbClient.SelectDeviceTriggersIndexes(s.context, user.ID, motionIndexKey)
	s.Require().NoError(err)
	s.Len(indexes, 1)

	s.checkUserScenarios(user.ID, []model.Scenario{scenario})
}

func (s *DBClientSuite) TestCreateScenario_DevicePropertyTriggerScenarioWithTimetable() {
	user := data.GenerateUser()
	err := s.dbClient.StoreUser(s.context, user)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	device := data.GenerateDevice()
	device.Properties = model.Properties{
		model.MakePropertyByType(model.EventPropertyType).
			WithState(model.EventPropertyState{
				Instance: model.MotionPropertyInstance,
				Value:    model.ClosedEvent,
			}),
	}
	device, _, err = s.dbClient.StoreUserDevice(s.context, user, device)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	scenario := data.GenerateScenario("scenario", nil)
	scenario.ID = model.GenerateScenarioID()
	scenario.Triggers = []model.ScenarioTrigger{
		model.DevicePropertyScenarioTrigger{
			DeviceID:     device.ID,
			PropertyType: model.EventPropertyType,
			Instance:     string(model.MotionPropertyInstance),
			Condition: model.EventPropertyCondition{
				Values: []model.EventValue{model.OpenedEvent},
			},
		},
		model.MakeTimetableTrigger(15, 42, 0, time.Wednesday),
	}
	scenario.PushOnInvoke = true

	now := timestamp.Now()
	launch, err := scenario.ToScheduledLaunch(
		now,
		now.Add(5*time.Minute),
		model.TimetableScenarioTrigger{},
		nil,
	)
	s.Require().NoError(err)
	launch.ID = model.GenerateScenarioLaunchID()

	err = s.dbClient.CreateScenarioWithLaunch(s.context, user.ID, scenario, launch)
	s.Require().NoError(err)

	motionIndexKey := model.DeviceTriggerIndexKey{
		DeviceID:      device.ID,
		TriggerEntity: model.PropertyEntity,
		Type:          model.EventPropertyType.String(),
		Instance:      model.MotionPropertyInstance.String(),
	}
	indexes, err := s.dbClient.SelectDeviceTriggersIndexes(s.context, user.ID, motionIndexKey)
	s.Require().NoError(err)
	s.Len(indexes, 1)

	launches, err := s.dbClient.SelectScenarioLaunchesByScenarioID(s.context, user.ID, scenario.ID)
	s.Require().NoError(err)
	s.Len(launches, 1)
	s.Equal(launch.ID, launches[0].ID)
}

func (s *DBClientSuite) TestUpdateScenario_DevicePropertyTriggerScenario() {
	user := data.GenerateUser()
	err := s.dbClient.StoreUser(s.context, user)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	device := data.GenerateDevice()
	device.Properties = model.Properties{
		model.MakePropertyByType(model.EventPropertyType).
			WithState(model.EventPropertyState{
				Instance: model.MotionPropertyInstance,
				Value:    model.NotDetectedEvent,
			}),
		model.MakePropertyByType(model.EventPropertyType).
			WithState(model.EventPropertyState{
				Instance: model.OpenPropertyInstance,
				Value:    model.ClosedEvent,
			}),
	}
	device, _, err = s.dbClient.StoreUserDevice(s.context, user, device)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	scenario := data.GenerateScenario("scenario", nil)
	scenario.ID, err = s.dbClient.CreateScenario(s.context, user.ID, scenario)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	scenario.Triggers = []model.ScenarioTrigger{
		model.DevicePropertyScenarioTrigger{
			DeviceID:     device.ID,
			PropertyType: model.EventPropertyType,
			Instance:     string(model.MotionPropertyInstance),
			Condition: model.EventPropertyCondition{
				Values: []model.EventValue{model.DetectedEvent},
			},
		},
		model.MakeTimetableTrigger(15, 42, 0, time.Wednesday),
	}

	now := timestamp.Now()
	launch, err := scenario.ToScheduledLaunch(
		now,
		now.Add(5*time.Minute),
		model.TimetableScenarioTrigger{},
		nil,
	)
	s.Require().NoError(err)
	launch.ID = model.GenerateScenarioLaunchID()

	err = s.dbClient.UpdateScenarioAndCreateLaunch(s.context, user.ID, scenario, launch)
	s.Require().NoError(err)

	motionIndexKey := model.DeviceTriggerIndexKey{
		DeviceID:      device.ID,
		TriggerEntity: model.PropertyEntity,
		Type:          model.EventPropertyType.String(),
		Instance:      model.MotionPropertyInstance.String(),
	}
	indexes, err := s.dbClient.SelectDeviceTriggersIndexes(s.context, user.ID, motionIndexKey)
	s.Require().NoError(err)
	s.Len(indexes, 1)

	launches, err := s.dbClient.SelectScenarioLaunchesByScenarioID(s.context, user.ID, scenario.ID)
	s.Require().NoError(err)
	s.Len(launches, 1)
	s.Equal(launch.ID, launches[0].ID)

	scenario.Triggers = []model.ScenarioTrigger{
		model.DevicePropertyScenarioTrigger{
			DeviceID:     device.ID,
			PropertyType: model.EventPropertyType,
			Instance:     string(model.MotionPropertyInstance),
			Condition: model.EventPropertyCondition{
				Values: []model.EventValue{model.DetectedEvent},
			},
		},
		model.DevicePropertyScenarioTrigger{
			DeviceID:     device.ID,
			PropertyType: model.EventPropertyType,
			Instance:     string(model.OpenPropertyInstance),
			Condition: model.EventPropertyCondition{
				Values: []model.EventValue{model.OpenedEvent},
			},
		},
	}

	err = s.dbClient.UpdateScenarioAndDeleteLaunches(s.context, user.ID, scenario)
	s.Require().NoError(err)

	indexes, err = s.dbClient.SelectDeviceTriggersIndexes(s.context, user.ID, motionIndexKey)
	s.Require().NoError(err)
	s.Len(indexes, 1)

	openIndexKey := model.DeviceTriggerIndexKey{
		DeviceID:      device.ID,
		TriggerEntity: model.PropertyEntity,
		Type:          model.EventPropertyType.String(),
		Instance:      model.OpenPropertyInstance.String(),
	}
	indexes, err = s.dbClient.SelectDeviceTriggersIndexes(s.context, user.ID, openIndexKey)
	s.Require().NoError(err)
	s.Len(indexes, 1)

	launches, err = s.dbClient.SelectScenarioLaunchesByScenarioID(s.context, user.ID, scenario.ID)
	s.Require().NoError(err)
	s.Empty(launches)
}

func (s *DBClientSuite) TestDeleteScenario_DevicePropertyTriggerScenario() {
	user := data.GenerateUser()
	err := s.dbClient.StoreUser(s.context, user)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	device := data.GenerateDevice()
	device.Properties = model.Properties{
		model.MakePropertyByType(model.EventPropertyType).
			WithState(model.EventPropertyState{
				Instance: model.MotionPropertyInstance,
				Value:    model.NotDetectedEvent,
			}),
	}
	device, _, err = s.dbClient.StoreUserDevice(s.context, user, device)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	scenario := data.GenerateScenario("scenario", nil)
	scenario.ID = model.GenerateScenarioID()
	scenario.Triggers = []model.ScenarioTrigger{
		model.DevicePropertyScenarioTrigger{
			DeviceID:     device.ID,
			PropertyType: model.EventPropertyType,
			Instance:     string(model.MotionPropertyInstance),
			Condition: model.EventPropertyCondition{
				Values: []model.EventValue{model.DetectedEvent},
			},
		},
		model.MakeTimetableTrigger(15, 42, 0, time.Wednesday),
	}
	scenario.PushOnInvoke = true

	now := timestamp.Now()
	launch, err := scenario.ToScheduledLaunch(
		now,
		now.Add(5*time.Minute),
		model.TimetableScenarioTrigger{},
		nil,
	)
	s.Require().NoError(err)
	launch.ID = model.GenerateScenarioLaunchID()

	err = s.dbClient.CreateScenarioWithLaunch(s.context, user.ID, scenario, launch)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	motionIndexKey := model.DeviceTriggerIndexKey{
		DeviceID:      device.ID,
		TriggerEntity: model.PropertyEntity,
		Type:          model.EventPropertyType.String(),
		Instance:      model.MotionPropertyInstance.String(),
	}
	indexes, err := s.dbClient.SelectDeviceTriggersIndexes(s.context, user.ID, motionIndexKey)
	s.Require().NoError(err)
	s.Len(indexes, 1)

	launches, err := s.dbClient.SelectScenarioLaunchesByScenarioID(s.context, user.ID, scenario.ID)
	s.Require().NoError(err)
	s.Len(launches, 1)
	s.Equal(launch.ID, launches[0].ID)

	err = s.dbClient.DeleteScenario(s.context, user.ID, scenario.ID)
	s.Require().NoError(err)

	indexes, err = s.dbClient.SelectDeviceTriggersIndexes(s.context, user.ID, motionIndexKey)
	s.Require().NoError(err)
	s.Empty(indexes)

	launches, err = s.dbClient.SelectScenarioLaunchesByScenarioID(s.context, user.ID, scenario.ID)
	s.Require().NoError(err)
	s.Empty(launches)
}

func (s *DBClientSuite) TestCreateScenario_VoiceTriggerScenario() {
	user := data.GenerateUser()
	err := s.dbClient.StoreUser(s.context, user)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	scenario := data.GenerateScenario("scenario", nil)
	scenario.Triggers = []model.ScenarioTrigger{
		model.VoiceScenarioTrigger{Phrase: "Hello, world!"},
		model.VoiceScenarioTrigger{Phrase: "Hello, world again!"},
		model.VoiceScenarioTrigger{Phrase: "And again hello, world!"},
	}
	scenario.ID, err = s.dbClient.CreateScenario(s.context, user.ID, scenario)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	scenario.PushOnInvoke = true

	launch := scenario.ToInvokedLaunch(scenario.Triggers[0], s.dbClient.timestamper.CurrentTimestamp(), nil)
	launch.ID, err = s.dbClient.StoreScenarioLaunch(s.context, user.ID, launch)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	launches, err := s.dbClient.SelectScenarioLaunchList(s.context, user.ID, 100, []model.ScenarioTriggerType{model.VoiceScenarioTriggerType})

	s.Require().NoError(err)
	s.Len(launches, 1)
	s.Equal(launches[0].LaunchTriggerType, model.VoiceScenarioTriggerType)
	s.Equal(launches[0].LaunchTriggerValue, model.VoiceTriggerValue{
		Phrases: []string{
			"Hello, world!",
			"Hello, world again!",
			"And again hello, world!",
		},
	})
	s.True(launches[0].PushOnInvoke)
}

func (s *DBClientSuite) TestSelectUserScenariosSimple() {
	user := data.GenerateUser()
	err := s.dbClient.StoreUser(s.context, user)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	device := data.GenerateDevice()
	device, _, err = s.dbClient.StoreUserDevice(s.context, user, device)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	scenario := data.GenerateScenario("scenario", model.Devices{device})
	scenario.ID = model.GenerateScenarioID()
	scenario.Triggers = []model.ScenarioTrigger{
		model.VoiceScenarioTrigger{Phrase: "фразочка"},
	}
	scenario.ID, err = s.dbClient.CreateScenario(s.context, user.ID, scenario)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	dbScenarios, err := s.dbClient.SelectUserScenariosSimple(s.context, user.ID)
	s.Require().NoError(err)
	s.Equal(model.Scenarios{scenario}, dbScenarios)
}
