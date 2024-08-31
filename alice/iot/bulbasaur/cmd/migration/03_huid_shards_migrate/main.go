package main

import (
	"context"
	"fmt"
	"os"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/library/go/core/log/zap"

	"a.yandex-team.ru/alice/library/go/cli"
	uberzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/db/schema"
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

func do(ctx context.Context, client *db.DBClient) (err error) {
	err = cleanupDatabase(ctx, client)
	if err != nil {
		return err
	}
	logger.Infof("cleanup done\n")

	err = schema.CreateTables(ctx, client.SessionPool, client.Prefix, "_hcopy")
	if err != nil {
		return fmt.Errorf("create tables error: %v", err)
	}
	logger.Infof("create done\n")

	err = backupOldTables(ctx, client.SessionPool, client.Prefix)
	if err != nil {
		return fmt.Errorf("copy tables error: %v", err)
	}
	logger.Infof("backup old tables done\n")

	//HERE YOU SHOULD:
	//1. upsert from `_old` databases to `_hcopy`
	//2. drop databases without suffixes manually
	//3. continue to run this script

	err = copyHcopy(ctx, client.SessionPool, client.Prefix)
	if err != nil {
		return fmt.Errorf("copy tables error: %v", err)
	}
	logger.Infof("_hcopy tables have been successfully copied, woohoo!\n")

	//HERE YOU SHOULD:
	//1. drop `_hcopy` tables manually

	return nil
}

func main() {
	logger, stop := initLogging()
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

	msg := fmt.Sprintf("You cannot perform this operation at `%s%s`", endpoint, prefix)
	cli.Ban(`production|quasar`, prefix, msg, logger)

	msg = fmt.Sprintf("Do you really want to redeploy Database at `%s%s`", endpoint, prefix)
	c := cli.AskForConfirmation(msg, logger)
	if !c {
		logger.Info("Bye")
		os.Exit(0)
	}

	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	if err := do(ctx, dbcli); err != nil {
		logger.Fatal(err.Error())
	}
}
