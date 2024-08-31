package main

import (
	"context"
	"flag"
	"fmt"
	"os"
	"sync"
	"time"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/library/go/core/log/zap"

	"a.yandex-team.ru/alice/library/go/cli"
	"a.yandex-team.ru/yt/go/yt"
	"a.yandex-team.ru/yt/go/yt/ythttp"
	"go.uber.org/atomic"
	uberzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
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

type speakerMigration struct {
	logger        *zap.Logger
	dbClient      *DBClient
	backendClient *quasarBackend

	batchSize int

	devicesProcessed atomic.Int32
	usersProcessed   atomic.Int32
}

func (sm *speakerMigration) do(ctx context.Context) {
	var batchNumber int
	var wg sync.WaitGroup
	discoveryChannel := sm.backendClient.Discover()

	for {
		startBatch := time.Now()
		discoveryResults := make([]discoveryResult, 0, sm.batchSize)

		for result := range discoveryChannel {
			if len(discoveryResults) < sm.batchSize {
				discoveryResults = append(discoveryResults, result)
			}
			if len(discoveryResults) >= sm.batchSize {
				break
			}
		}

		if len(discoveryResults) > 0 {
			batchNumber++
			wg.Add(1)
			go func(batchNumber int) {
				defer wg.Done()
				if err := sm.storeDiscoveryResults(ctx, discoveryResults); err != nil {
					sm.logger.Debugf("Error in batch %d: %v", batchNumber, err)
				} else {
					sm.logger.Debugf("Processed batch %d in %v", batchNumber, time.Since(startBatch))
				}
			}(batchNumber)
		} else {
			break
		}
	}
	wg.Wait()
	sm.logger.Debugf("Finished on batch %d", batchNumber)
}

func (sm *speakerMigration) storeDiscoveryResults(ctx context.Context, discoveryResults []discoveryResult) error {
	var users []quasarUser
	var userDevices []quasarUserDevice

	for _, discoveryResult := range discoveryResults {
		sm.usersProcessed.Inc()
		sm.devicesProcessed.Add(int32(len(discoveryResult.devices)))
		users = append(users, discoveryResult.user)
		userDevices = append(userDevices, discoveryResult.devices...)
	}
	if err := sm.dbClient.UpsertUsers(ctx, users); err != nil {
		sm.logger.Warnf("Error during storing users: %v", err)
		return err
	}
	if err := sm.dbClient.UpsertUserDevices(ctx, userDevices); err != nil {
		sm.logger.Warnf("Error during storing devices: %v", err)
		return err
	}
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

	ydbToken := os.Getenv("YDB_TOKEN")
	if len(ydbToken) == 0 {
		panic("YDB_TOKEN env is not set")
	}

	_, trace := os.LookupEnv("YDB_DEBUG")

	baseDBClient, err := db.NewClient(context.Background(), logger, endpoint, prefix, ydb.AuthTokenCredentials{AuthToken: ydbToken}, trace)
	if err != nil {
		panic(err.Error())
	}
	dbcli := &DBClient{DBClient: baseDBClient}

	ytClient, err := ythttp.NewClient(&yt.Config{
		Proxy:             "hahn",
		ReadTokenFromFile: true,
	})
	if err != nil {
		logger.Fatal(err.Error())
	}

	flag.Parse()
	if !cli.IsForceYes() {
		msg := fmt.Sprintf("Do you really want to add speaker users and their devices to `%s%s`", endpoint, prefix)
		if c := cli.AskForConfirmation(msg, logger); !c {
			logger.Info("Bye")
			os.Exit(0)
		}
	}

	ctx := context.Background()
	dbcli.IotDeviceMemoryTool = NewDeviceMemoryTool(ctx, dbcli)
	dbcli.IotUserMemoryTool = NewUserMemoryTool(ctx, dbcli)

	start := time.Now()
	migration := speakerMigration{
		logger:    logger,
		dbClient:  dbcli,
		batchSize: 100,

		backendClient: newQuasarBackend(ctx, ytClient),
	}
	migration.do(ctx)
	logger.Infof("stored %d/%d users", dbcli.userCounter.Load(), migration.usersProcessed.Load())
	logger.Infof("stored %d/%d devices", dbcli.deviceCounter.Load(), migration.devicesProcessed.Load())
	logger.Infof("Migration took: %v", time.Since(start))
}
