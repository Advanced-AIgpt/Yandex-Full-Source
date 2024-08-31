package main

import (
	"context"
	"fmt"
	"os"
	"sync"
	"time"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/library/go/core/log/zap"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/cli"
	uberzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"
)

var logger *zap.Logger

type userScenario struct {
	Scenario model.Scenario
	User     model.User
}

func initLogging() (logger *zap.Logger, stop func()) {
	encoderConfig := uberzap.NewDevelopmentEncoderConfig()
	encoderConfig.EncodeLevel = zapcore.CapitalColorLevelEncoder
	encoderConfig.EncodeCaller = zapcore.ShortCallerEncoder
	encoder := zapcore.NewConsoleEncoder(encoderConfig)

	core := zapcore.NewCore(encoder, zapcore.AddSync(os.Stderr), uberzap.DebugLevel)
	stop = func() {
		_ = core.Sync()
	}
	logger = zap.NewWithCore(core, uberzap.AddStacktrace(uberzap.FatalLevel), uberzap.AddCaller())

	return
}

func do(ctx context.Context, client *MigrationDBClient) error {
	startMigration := time.Now()
	batchSize := 1000
	scenariosChannel := client.streamRecords(ctx)
	var batchNumber int
	var wg sync.WaitGroup
	for {
		startBatch := time.Now()
		scenarios := make([]userScenario, 0, batchSize)

		for result := range scenariosChannel {
			if len(scenarios) < batchSize {
				scenarios = append(scenarios, client.prepareRecord(ctx, result))
			}
			if len(scenarios) >= batchSize {
				break
			}
		}

		if len(scenarios) > 0 {
			batchNumber++
			wg.Add(1)
			go func(batchNumber int) {
				defer wg.Done()
				if err := client.batchUpdateScenarios(ctx, scenarios); err != nil {
					logger.Debugf("Error in batch %d: %v", batchNumber, err)
				} else {
					logger.Debugf("Processed batch %d %d values in %v", batchNumber, len(scenarios), time.Since(startBatch))
				}
			}(batchNumber)
		} else {
			break
		}
	}
	wg.Wait()
	logger.Infof("migrating from external actions to requested speaker capabilities done, finished on batch %d", batchNumber)
	logger.Infof("updated scenarios: %d", client.UpdatedScenariosCounter)
	logger.Infof("elapsed time: %v", time.Since(startMigration))
	logger.Info("done")
	return nil
}

func main() {
	var stop func()
	logger, stop = initLogging()
	defer stop()

	endpoint := os.Getenv("YDB_ENDPOINT")
	if len(endpoint) == 0 {
		panic("YDB_ENDPOINT env is not set")
	}

	prefix := os.Getenv("YDB_PREFIX")
	if len(prefix) == 0 {
		panic("YDB_PREFIX env is not set")
	}

	token := os.Getenv("YDB_TOKEN")
	if len(token) == 0 {
		panic("YDB_TOKEN env is not set")
	}

	_, trace := os.LookupEnv("YDB_DEBUG")

	dbcli, err := db.NewClient(context.Background(), logger, endpoint, prefix, ydb.AuthTokenCredentials{AuthToken: token}, trace)
	if err != nil {
		panic(err.Error())
	}

	msg := fmt.Sprintf("Do you really want to migrate external actions to capabilities from user Scenarios at `%s%s`", endpoint, prefix)
	c := cli.AskForConfirmation(msg, logger)
	if !c {
		logger.Info("Bye")
		os.Exit(0)
	}

	ctx := context.Background()

	if err := do(ctx, &MigrationDBClient{DBClient: dbcli}); err != nil {
		logger.Fatal(err.Error())
	}
}
