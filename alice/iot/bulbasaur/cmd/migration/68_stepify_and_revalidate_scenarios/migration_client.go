package main

import (
	"context"
	"path"

	"golang.org/x/exp/slices"

	"a.yandex-team.ru/alice/iot/bulbasaur/begemot"
	"a.yandex-team.ru/alice/iot/bulbasaur/cmd/migration/dbmigration"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/goroutines"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type migrationClient struct {
	env           dbmigration.Environment
	begemotClient begemot.IClient

	userIDsFilter  []uint64 // if provided, will migrate only these uids
	checkRecursive bool     // if provided, will

	stats Stats
}

// reading data
func (c *migrationClient) streamScenarioUsers() <-chan User {
	ctx := c.env.Ctx
	logger := c.env.Logger
	db := c.env.Mdb

	userChannel := make(chan User)

	streamUsersFunc := func(ctx context.Context) {
		defer close(userChannel)

		session, err := db.SessionPool.Get(ctx)
		if err != nil {
			logger.Fatalf("failed to get session: %v", err)
		}

		usersTablePath := path.Join(db.Prefix, "Users")
		logger.Infof("reading users from %q", usersTablePath)

		res, err := session.StreamReadTable(ctx, usersTablePath, table.ReadColumn("id"))
		if err != nil {
			logger.Fatalf("failed to read table: %v", err)
		}

		defer func() {
			if err := res.Close(); err != nil {
				logger.Fatalf("failed to close result set: %v", err)
			}
		}()

		logger.Infof("will apply users filter: %+v", c.userIDsFilter)
		for res.NextResultSet(ctx, "id") {
			for res.NextRow() {
				var user User
				if err := res.ScanWithDefaults(&user.ID); err != nil {
					logger.Fatalf("failed to scan user: %v", err)
				}
				if len(c.userIDsFilter) == 0 || slices.Contains(c.userIDsFilter, user.ID) {
					c.stats.TotalUsersCount.Inc()
					userChannel <- user
				}
			}
		}
		if err := res.Err(); err != nil {
			logger.Fatalf("failed to read users: %v", err)
		}
		logger.Infof("finished reading users: read %d", c.stats.TotalUsersCount.Load())
	}

	c.env.Logger.Info("Streaming scenario users...")

	go goroutines.SafeBackground(ctx, logger, streamUsersFunc)

	return userChannel
}

func (c *migrationClient) readScenariosAndDevices(user User) (model.Scenarios, model.Devices, error) {
	ctx := c.env.Ctx
	db := c.env.Mdb
	scenarios, err := db.SelectUserScenarios(ctx, user.ID)
	if err != nil {
		c.stats.FailedUsersCount.Inc()
		return nil, nil, err
	}
	c.stats.TotalScenariosCount.Add(uint64(len(scenarios)))

	if len(scenarios) == 0 {
		return nil, nil, nil
	}

	devices, err := db.SelectUserDevicesSimple(ctx, user.ID)
	if err != nil {
		c.stats.FailedUsersCount.Inc()
		c.stats.FailedScenariosCount.Add(uint64(len(scenarios)))
		return nil, nil, xerrors.Errorf("failed to select user devices: %w", err)
	}
	return scenarios, devices, nil
}

// migrating data
func (c *migrationClient) migrateUserScenarios(user User) error {
	c.env.Logger.Infof("Migrating user %d", user.ID)
	scenarios, devices, err := c.readScenariosAndDevices(user)
	if err != nil {
		return xerrors.Errorf("failed to read scenarios and devices: %w", err)
	}
	if len(scenarios) == 0 {
		return nil
	}

	updatedScenarios := make(model.Scenarios, 0, len(scenarios))
	for i := range scenarios {
		updatesScenario := c.migrateScenario(scenarios[i], scenarios, devices)
		updatedScenarios = append(updatedScenarios, updatesScenario)
	}

	if err := c.env.Mdb.UpdateScenarios(c.env.Ctx, user.ID, updatedScenarios); err != nil {
		c.stats.FailedScenariosCount.Add(uint64(len(scenarios)))
		return xerrors.Errorf("failed to update user scenarios: %w", err)
	}
	return nil
}

func (c *migrationClient) migrateScenario(scenario model.Scenario, scenarios model.Scenarios, devices model.Devices) model.Scenario {
	updatedScenario := c.migrateScenarioSteps(scenario, devices)

	if !c.checkRecursive {
		return updatedScenario
	}

	if !updatedScenario.Steps.HasQuasarTextActionCapabilities() {
		// steps without text actions are not recursive
		return updatedScenario
	}

	textActions := updatedScenario.Steps.GetTextQuasarServerActionCapabilityValues()
	c.env.Logger.Info(
		"Checking recursive scenario",
		log.Any("scenario_id", updatedScenario.ID), log.Any("name", updatedScenario.Name), log.Any("text_actions", textActions),
	)
	err := c.checkRecursiveTextActions(updatedScenario, scenarios, devices)
	if err != nil {
		c.env.Logger.Warn(
			"scenario might be recursive",
			log.Error(err), log.Any("scenario_id", updatedScenario.ID), log.Any("name", updatedScenario.Name), log.Any("text_actions", textActions),
		)
		switch {
		case xerrors.Is(err, new(model.ScenarioTextServerActionNameError)):
			updatedScenario = c.migrateRecursiveScenario(updatedScenario)
			c.stats.MigratedRecursiveScenariosCount.Inc()
		default:
			c.env.Logger.Warnf("failed to check scenario %s text actions: %v", updatedScenario.ID, err)
			c.stats.AddFailedRecursiveScenario(scenario.ID)
		}
	}
	return updatedScenario
}

// migration bits
func (c *migrationClient) migrateScenarioSteps(scenario model.Scenario, devices model.Devices) model.Scenario {
	if len(scenario.Steps) > 0 {
		return scenario
	}
	scenario.Steps = scenario.ScenarioSteps(devices)
	c.stats.MigratedStepsScenariosCount.Inc()
	return scenario
}

func (c *migrationClient) migrateRecursiveScenario(scenario model.Scenario) model.Scenario {
	for i := range scenario.Steps {
		switch scenario.Steps[i].(type) {
		case *model.ScenarioStepActions:
			stepParameters := scenario.Steps[i].Parameters().(model.ScenarioStepActionsParameters)
			scenario.Steps[i].SetParameters(moveTextActionsToPhraseActions(stepParameters))
		default:
			continue
		}
	}
	return scenario
}

func (c *migrationClient) checkRecursiveTextActions(updatedScenario model.Scenario, scenarios model.Scenarios, devices model.Devices) error {
	userInfo := model.UserInfo{Scenarios: scenarios, Devices: devices}
	userScenarioIDs := scenarios.GetIDs()
	pushTexts := updatedScenario.Steps.GetTextQuasarServerActionCapabilityValues()
	for _, pushText := range pushTexts {
		err := begemot.ValidatePushText(c.env.Ctx, c.env.Logger, c.begemotClient, pushText, userScenarioIDs, userInfo)
		if err != nil {
			return err
		}
	}
	return nil
}
