package main

import (
	"context"
	"fmt"
	"os"
	"sync"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/library/go/cli"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/library/go/core/log/zap"
	uberzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"
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
	batchSize := 1000
	usersChannel := client.StreamUsers(ctx)
	var batchNumber int
	var wg sync.WaitGroup
	for {
		startBatch := time.Now()
		users := make([]uint64, 0, batchSize)

		for result := range usersChannel {
			if len(users) < batchSize {
				users = append(users, result)
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
				if err := client.CreateCurrentHouseholdForUser(ctx, users); err != nil {
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
	logger.Infof("Creating first household done, finished on batch %d", batchNumber)
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

	msg := fmt.Sprintf("Do you really want to create first household for all users at `%s%s`", endpoint, prefix)
	c := cli.AskForConfirmation(msg, logger)
	if !c {
		logger.Info("Bye")
		os.Exit(0)
	}

	ctx := context.Background()
	start := time.Now()
	if err := do(ctx, &MigrationDBClient{dbcli}); err != nil {
		logger.Fatal(err.Error())
	}
	logger.Infof("Time elapsed: %v", time.Since(start))
}
