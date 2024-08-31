package main

import (
	"context"
	"net/http"

	"a.yandex-team.ru/alice/iot/bulbasaur/begemot"
	"a.yandex-team.ru/alice/iot/bulbasaur/cmd/migration/dbmigration"
	libbegemot "a.yandex-team.ru/alice/library/go/begemot"
	"a.yandex-team.ru/alice/library/go/goroutines"
)

func newMigrationClient(env dbmigration.Environment, userIDsFilter []uint64, checkRecursive bool) *migrationClient {
	begemotClient := libbegemot.NewClient(
		"http://hamzard.yandex.net:8891",
		http.DefaultClient,
		env.Logger,
	)
	return &migrationClient{
		env:            env,
		begemotClient:  begemot.NewClient(begemotClient, env.Logger),
		userIDsFilter:  userIDsFilter,
		checkRecursive: checkRecursive,
		stats:          newStats(),
	}
}

func do(migrationClient *migrationClient, workersCount int) error {
	ctx := migrationClient.env.Ctx
	db := migrationClient.env.Mdb

	userChannel := migrationClient.streamScenarioUsers()
	migrateUsersFunc := func(ctx context.Context, user User) func(ctx context.Context) error {
		return func(ctx context.Context) error {
			return migrationClient.migrateUserScenarios(user)
		}
	}

	var workers goroutines.Group
	for i := 0; i < workersCount; i++ {
		workers.Go(func() error {
			for user := range userChannel {
				if err := db.Transaction(ctx, "Migrating scenarios", migrateUsersFunc(ctx, user)); err != nil {
					migrationClient.env.Logger.Warnf("User %d migration failed: %v", user.ID, err)
				}
			}
			return nil
		})
	}
	return workers.Wait()
}

func main() {
	migrationEnvironment := dbmigration.AskMigrationWithDescription(
		"Migrate scenarios to steps and fix recursive scenarios",
		"Devices and RequestedSpeakerCapabilities will be moved to steps. Recursive text actions will optionally be moved to phrase actions.",
	)
	defer migrationEnvironment.Close()

	// []
	uidsToMigrate := []uint64{}
	migrationClient := newMigrationClient(migrationEnvironment, uidsToMigrate, false)

	err := do(migrationClient, 50)
	migrationClient.env.Logger.Info(migrationClient.stats.String())
	if err != nil {
		migrationClient.env.Logger.Fatalf("failed to migrate: %v", err)
	}
}
