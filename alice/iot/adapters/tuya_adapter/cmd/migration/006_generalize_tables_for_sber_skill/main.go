package main

import (
	"context"
	"fmt"
	"os"
	"path"

	"a.yandex-team.ru/alice/library/go/ydbclient"
	"a.yandex-team.ru/library/go/core/log/zap"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	uberzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"

	"a.yandex-team.ru/alice/library/go/cli"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
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

func createGenericUsersTable(ctx context.Context, client *ydbclient.YDBClient) error {
	options := []table.CreateTableOption{
		table.WithColumn("hid", ydb.Optional(ydb.TypeUint64)),
		table.WithColumn("id", ydb.Optional(ydb.TypeUint64)),
		table.WithColumn("skill_id", ydb.Optional(ydb.TypeString)),
		table.WithColumn("login", ydb.Optional(ydb.TypeString)),
		table.WithColumn("tuya_uid", ydb.Optional(ydb.TypeString)),
		table.WithPrimaryKeyColumn("hid", "id", "skill_id"),
		table.WithIndex("tuya_uid_index",
			table.WithIndexType(table.GlobalIndex()),
			table.WithIndexColumns("tuya_uid"),
		),
		table.WithProfile(
			table.WithPartitioningPolicy(
				table.WithPartitioningPolicyUniformPartitions(4),
			),
		),
	}
	return table.Retry(ctx, client.SessionPool, table.OperationFunc(func(ctx context.Context, s *table.Session) error {
		return s.CreateTable(ctx, path.Join(client.Prefix, "GenericUsers"), options...)
	}))
}

func alterDeviceOwnersTable(ctx context.Context, client *ydbclient.YDBClient) error {
	tableOptions := []table.AlterTableOption{
		table.WithAddColumn("skill_id", ydb.Optional(ydb.TypeString)),
	}

	err := table.Retry(ctx, client.SessionPool, table.OperationFunc(func(ctx context.Context, s *table.Session) error {
		if err := s.AlterTable(ctx, path.Join(client.Prefix, "DeviceOwnerCache"), tableOptions...); err != nil {
			return err
		}
		logger.Infof("adding column skill_id to DeviceOwnerCache done\n")
		return nil
	}))

	if err != nil {
		logger.Errorf("adding column skill_id to DeviceOwnerCache went wrong: %v", err)
		return err
	}

	return nil
}

func do(ctx context.Context, client *ydbclient.YDBClient) (err error) {
	err = createGenericUsersTable(ctx, client)
	if err != nil {
		return fmt.Errorf("create table GenericUsers error: %w", err)
	}
	logger.Infof("create table GenericUsers done\n")

	err = alterDeviceOwnersTable(ctx, client)
	if err != nil {
		return fmt.Errorf("alter table DeviceOwnerCache error: %w", err)
	}
	logger.Infof("alter table DeviceOwnerCache done\n")
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

	dbcli, err := ydbclient.NewYDBClient(context.Background(), logger, endpoint, prefix, ydb.AuthTokenCredentials{AuthToken: token}, trace)
	if err != nil {
		panic(err.Error())
	}

	msg := fmt.Sprintf("Do you really want to create table GenericUsers and alter table DeviceOwnerCache at `%s%s`", endpoint, prefix)
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
