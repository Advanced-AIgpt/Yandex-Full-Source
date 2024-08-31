package main

import (
	"context"
	"fmt"
	"os"
	"path"

	"a.yandex-team.ru/library/go/core/log/zap"

	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/db"
	"a.yandex-team.ru/alice/library/go/cli"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
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

func do(ctx context.Context, client *db.DBClient) (err error) {
	return table.Retry(ctx, client.SessionPool, table.OperationFunc(func(ctx context.Context, s *table.Session) error {
		if err := s.DropTable(ctx, path.Join(client.Prefix, "devices")); err != nil {
			return err
		}
		logger.Infof("drop done\n")

		tableOptions := []table.CreateTableOption{
			table.WithColumn("hid", ydb.Optional(ydb.TypeUint64)),
			table.WithColumn("id", ydb.Optional(ydb.TypeString)),
			table.WithColumn("tuya_uid", ydb.Optional(ydb.TypeString)),
			table.WithColumn("updated", ydb.Optional(ydb.TypeTimestamp)),
			table.WithPrimaryKeyColumn("hid", "id"),
			table.WithProfile(
				table.WithPartitioningPolicy(
					table.WithPartitioningPolicyUniformPartitions(4),
				),
			),
		}
		if err := s.CreateTable(ctx, path.Join(client.Prefix, "DeviceOwnerCache"), tableOptions...); err != nil {
			return err
		}
		logger.Infof("create done\n")

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

	msg := fmt.Sprintf("Do you really want to create table devices at `%s%s`", endpoint, prefix)
	if !cli.AskForConfirmation(msg, logger) {
		logger.Info("Bye")
		os.Exit(0)
	}

	ctx := context.Background()

	if err := do(ctx, dbcli); err != nil {
		logger.Fatal(err.Error())
	}
}
