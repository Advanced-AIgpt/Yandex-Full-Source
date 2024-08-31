package main

import (
	"context"
	"fmt"
	"os"
	"sync"
	"time"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/library/go/core/log/zap"
	uberzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/library/go/cli"
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

func do(ctx context.Context, client *MigrationDBClient, tableName string) error {
	batchSize := 1000
	workers := 20
	objectsChannel := client.StreamObjects(ctx, tableName)
	var batchNumber int
	var wg sync.WaitGroup
	for i := 0; i < workers; i++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			for {
				startBatch := time.Now()
				objects := make([]Object, 0, batchSize)

				for result := range objectsChannel {
					if len(objects) < batchSize {
						objects = append(objects, result)
					}
					if len(objects) >= batchSize {
						break
					}
				}
				if len(objects) > 0 {
					currentBatchNumber := batchNumber
					batchNumber++
					if err := client.FillHouseholdID(ctx, objects, tableName); err != nil {
						logger.Debugf("Error in batch %d: %v", currentBatchNumber, err)
					} else {
						logger.Debugf("Processed batch %d %d values in %v", currentBatchNumber, len(objects), time.Since(startBatch))
					}
				} else {
					return
				}
			}
		}()
	}
	wg.Wait()
	logger.Infof("Filling household_id columns done, finished on batch %d", batchNumber)
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

	tableName := "Groups"
	if os.Getenv("MIGRATION_TABLE") != "" {
		tableName = os.Getenv("MIGRATION_TABLE")
	}

	_, trace := os.LookupEnv("YDB_DEBUG")

	dbcli, err := db.NewClient(context.Background(), logger, endpoint, prefix, ydb.AuthTokenCredentials{AuthToken: token}, trace)
	if err != nil {
		panic(err.Error())
	}

	msg := fmt.Sprintf("Do you really want to fill nulls of household_id column by current_household value at table %s at `%s%s`", tableName, endpoint, prefix)
	c := cli.AskForConfirmation(msg, logger)
	if !c {
		logger.Info("Bye")
		os.Exit(0)
	}

	ctx := context.Background()
	start := time.Now()
	if err := do(ctx, &MigrationDBClient{dbcli}, tableName); err != nil {
		logger.Fatal(err.Error())
	}
	logger.Infof("Time elapsed: %v", time.Since(start))
}
