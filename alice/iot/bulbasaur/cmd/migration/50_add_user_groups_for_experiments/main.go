package main

import (
	"context"
	"fmt"
	"os"
	"path"

	"a.yandex-team.ru/library/go/core/log/zap"

	uzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/library/go/cli"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
)

var logger *zap.Logger

func initLogging() (*zap.Logger, func()) {
	encoderConfig := uzap.NewDevelopmentEncoderConfig()
	encoderConfig.EncodeLevel = zapcore.CapitalColorLevelEncoder
	encoderConfig.EncodeCaller = zapcore.ShortCallerEncoder
	encoder := zapcore.NewConsoleEncoder(encoderConfig)

	core := zapcore.NewCore(encoder, zapcore.AddSync(os.Stderr), uzap.DebugLevel)
	stop := func() {
		_ = core.Sync()
	}
	logger = zap.NewWithCore(core, uzap.AddStacktrace(uzap.FatalLevel), uzap.AddCaller())

	return logger, stop
}

func do(ctx context.Context, client *db.DBClient) (err error) {
	createTableName := "ExperimentsUserGroup"
	tableCreateOptions := []table.CreateTableOption{
		table.WithColumn("group_id", ydb.Optional(ydb.TypeString)),
		table.WithColumn("user_ids", ydb.Optional(ydb.TypeJSON)),
		table.WithColumn("comment", ydb.Optional(ydb.TypeString)),
		table.WithPrimaryKeyColumn("group_id"),
		table.WithProfile(
			table.WithPartitioningPolicy(
				table.WithPartitioningPolicyUniformPartitions(1),
			),
		),
	}

	alterTableName := "Experiments"
	alterTableOptions := []table.AlterTableOption{
		table.WithAddColumn("group_ids", ydb.Optional(ydb.TypeJSON)),
	}

	return table.Retry(ctx, client.SessionPool, table.OperationFunc(func(ctx context.Context, s *table.Session) error {
		if err := s.CreateTable(ctx, path.Join(client.Prefix, createTableName), tableCreateOptions...); err != nil {
			return err
		}
		logger.Infof("create done\n")

		if err := s.AlterTable(ctx, path.Join(client.Prefix, alterTableName), alterTableOptions...); err != nil {
			return err
		}
		logger.Infof("adding column done\n")

		return nil
	}))
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

	msg := fmt.Sprintf("Do you really want to create table 'ExperimentsUserGroup' and add column 'group_ids' to table 'Experiments' at `%s%s`?", endpoint, prefix)
	c := cli.AskForConfirmation(msg, logger)
	if !c {
		logger.Info("Bye")
		os.Exit(0)
	}

	ctx := context.Background()

	if err := do(ctx, dbcli); err != nil {
		logger.Fatal(err.Error())
	}
}
