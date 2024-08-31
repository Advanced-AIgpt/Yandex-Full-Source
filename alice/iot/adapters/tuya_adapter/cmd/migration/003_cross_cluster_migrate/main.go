package main

import (
	"context"
	"fmt"
	"os"
	"time"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/library/go/core/log/zap"

	"a.yandex-team.ru/alice/library/go/cli"
	uberzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"

	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/db"
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

func do(ctx context.Context, dstClient *db.DBClient, srcClient *db.DBClient) (err error) {
	// Step 1: Create new users table
	err = createUsersTable(ctx, dstClient)
	if err != nil {
		return fmt.Errorf("create table users error: %v", err)
	}
	logger.Infof("create table users done\n")

	// Step 2: Migrate from src cluster to dst cluster
	err = crossClusterMigrate(ctx, dstClient, srcClient)
	if err != nil {
		return fmt.Errorf("populate users table error: %v", err)
	}
	logger.Infof("populate of Users table done\n")
	return nil
}

func initDBClient(endpointEnv, prefixEnv string) (endpoint, prefix string, client *db.DBClient) {
	endpoint = os.Getenv(endpointEnv)
	if len(endpoint) == 0 {
		panic(fmt.Sprintf("%s env is not set", endpointEnv))
	}

	prefix = os.Getenv(prefixEnv)
	if len(prefix) == 0 {
		panic(fmt.Sprintf("%s env is not set", prefixEnv))
	}

	token := os.Getenv("YDB_TOKEN")
	if len(token) == 0 {
		panic("YDB_TOKEN env is not set")
	}

	_, trace := os.LookupEnv("YDB_DEBUG")

	var err error
	client, err = db.NewClient(context.Background(), logger, endpoint, prefix, ydb.AuthTokenCredentials{AuthToken: token}, trace)
	if err != nil {
		panic(err.Error())
	}
	return endpoint, prefix, client
}

func main() {
	l, stop := initLogging()
	logger = l
	defer stop()

	dstEndpoint, dstPrefix, dstClient := initDBClient("DST_YDB_ENDPOINT", "DST_YDB_PREFIX")
	srcEndpoint, srcPrefix, srcClient := initDBClient("SRC_YDB_ENDPOINT", "SRC_YDB_PREFIX")
	msg := fmt.Sprintf("Do you really want to migration from database at `%s%s` to database at `%s%s`", srcEndpoint, srcPrefix, dstEndpoint, dstPrefix)
	c := cli.AskForConfirmation(msg, logger)
	if !c {
		logger.Info("Bye")
		os.Exit(0)
	}

	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	start := time.Now()
	if err := do(ctx, dstClient, srcClient); err != nil {
		logger.Fatal(err.Error())
	}
	logger.Infof("Spent %s time on migration", time.Since(start))
}
