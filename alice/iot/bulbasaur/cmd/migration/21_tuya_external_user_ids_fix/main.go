package main

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/xerrors"

	"context"
	"fmt"
	"os"
	"sync"
	"time"

	uberzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"

	"a.yandex-team.ru/alice/library/go/cli"
	"a.yandex-team.ru/alice/library/go/ydbclient"
)

var logger *zap.Logger

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
	externalUsers, err := client.SelectExternalUsers(ctx, model.TUYA)
	if err != nil {
		return xerrors.Errorf("select external users err: %w", err)
	}

	tuyaUsers, err := client.SelectTuyaUsers(ctx)
	if err != nil {
		return xerrors.Errorf("select tuya users err: %w", err)
	}

	usersChan := make(chan TuyaUser, len(tuyaUsers))
	var usersFoundCount int
	for _, user := range tuyaUsers {
		if _, userFound := externalUsers[user.YandexID]; userFound {
			usersFoundCount++
			usersChan <- user
		}
	}
	close(usersChan)
	logger.Debugf("Found %d users to upsert", usersFoundCount)

	batchSize := 1000
	var batchNumber int
	var wg sync.WaitGroup
	for {
		startBatch := time.Now()
		users := make([]TuyaUser, 0, batchSize)

		for user := range usersChan {
			if len(users) < batchSize {
				users = append(users, user)
			}
			if len(users) >= batchSize {
				break
			}
		}

		if len(users) > 0 {
			batchNumber++
			wg.Add(1)
			go func(batchNumber int) {
				defer wg.Done()
				defer func() {
					if r := recover(); r != nil {
						logger.Warnf("Batch processing caught panic: %+v", r)
					}
				}()
				logger.Infof("Fixing tuya uids for batch %d, users %+v", batchNumber, users)
				if err := client.FixBrokenTuyaUIDs(ctx, users); err != nil {
					logger.Debugf("Error in batch %d: %v", batchNumber, err)
				} else {
					logger.Debugf("Processed batch %d %d values in %v", batchNumber, len(users), time.Since(startBatch))
				}
			}(batchNumber)
		} else {
			break
		}
	}
	wg.Wait()
	logger.Infof("Fixing tuya external ids done, finished on batch %d", batchNumber)
	return nil
}

func envPrefixedYDBClient(envPrefix string) (string, *ydbclient.YDBClient) {
	endpoint := os.Getenv(fmt.Sprintf("%s_YDB_ENDPOINT", envPrefix))
	if len(endpoint) == 0 {
		panic(fmt.Sprintf("%s_YDB_ENDPOINT env is not set", envPrefix))
	}

	prefix := os.Getenv(fmt.Sprintf("%s_YDB_PREFIX", envPrefix))
	if len(prefix) == 0 {
		panic(fmt.Sprintf("%s_YDB_PREFIX env is not set", envPrefix))
	}

	token := os.Getenv("YDB_TOKEN")
	if len(token) == 0 {
		panic("YDB_TOKEN env is not set")
	}

	client, err := ydbclient.NewYDBClient(context.Background(), logger, endpoint, prefix, ydb.AuthTokenCredentials{AuthToken: token}, false)
	if err != nil {
		panic(err.Error())
	}
	return endpoint, client
}

func main() {
	var stop func()
	logger, stop = initLogging()
	defer stop()

	tuyaEndpoint, tuyaClient := envPrefixedYDBClient("TUYA")
	bulbasaurEndpoint, bulbasaurClient := envPrefixedYDBClient("BULBASAUR")
	migrationClient := &MigrationDBClient{
		tuyaDB:      tuyaClient,
		bulbasaurDB: bulbasaurClient,
	}

	msg := fmt.Sprintf(
		"Do you really want to move tuya uids from Users table at '%s%s' to ExternalUsers table at '%s%s'",
		tuyaEndpoint, tuyaClient.Prefix,
		bulbasaurEndpoint, bulbasaurClient.Prefix,
	)
	c := cli.AskForConfirmation(msg, logger)
	if !c {
		logger.Info("Bye")
		os.Exit(0)
	}

	ctx := context.Background()
	start := time.Now()
	if err := do(ctx, migrationClient); err != nil {
		logger.Fatal(err.Error())
	}
	logger.Infof("Time elapsed: %v", time.Since(start))
}
