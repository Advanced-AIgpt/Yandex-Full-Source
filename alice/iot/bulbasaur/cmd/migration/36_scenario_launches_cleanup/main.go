package main

import (
	"context"
	"fmt"
	"os"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/cli"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/log/zap"
	uzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"
)

var logger *zap.Logger

type MigrationDBClient struct {
	*db.DBClient
}

type scenarioLaunch struct {
	huid uint64
	id   string
}

func (db *MigrationDBClient) getScenarioLaunchesChunk(ctx context.Context) ([]scenarioLaunch, bool, error) {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");

		DECLARE $limit AS Uint64;
		DECLARE $scheduled_status AS String;

		SELECT
			huid, id
		FROM
			ScenarioLaunches
		WHERE
			status != $scheduled_status
		LIMIT
			$limit
	`, db.Prefix)

	limit := 1000
	params := table.NewQueryParameters(
		table.ValueParam("$scheduled_status", ydb.StringValue([]byte(model.ScenarioLaunchScheduled))),
		table.ValueParam("$limit", ydb.Uint64Value(uint64(limit))),
	)

	launches := make([]scenarioLaunch, 0)

	res, err := db.Read(ctx, query, params)
	if err != nil {
		return launches, false, err
	}

	if !res.NextSet() {
		return launches, false, nil
	}

	for res.NextRow() {
		var launch scenarioLaunch

		res.NextItem()
		launch.huid = res.OUint64()

		res.NextItem()
		launch.id = string(res.OString())

		launches = append(launches, launch)
	}

	hasMoreData := res.RowCount() == limit
	return launches, hasMoreData, nil
}

func (db *MigrationDBClient) deleteScenarioLaunches(ctx context.Context, launches []scenarioLaunch) error {
	if len(launches) == 0 {
		return nil
	}

	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");

		DECLARE $launches AS List<Struct<
			huid: Uint64,
			id: String
		>>;

		DELETE FROM
			ScenarioLaunches
		ON SELECT
			huid, id
		FROM AS_TABLE($launches);
	`, db.Prefix)

	deleteParams := make([]ydb.Value, 0, len(launches))
	for _, l := range launches {
		logger.Infof("Prepare to delete ScenarioLaunch with values (huid: %d, id: %s)", l.huid, l.id)

		deleteParams = append(deleteParams, ydb.StructValue(
			ydb.StructFieldValue("huid", ydb.Uint64Value(l.huid)),
			ydb.StructFieldValue("id", ydb.StringValue([]byte(l.id))),
		))
	}

	params := table.NewQueryParameters(
		table.ValueParam("$launches", ydb.ListValue(deleteParams...)),
	)

	return db.Write(ctx, query, params)
}

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

func do(ctx context.Context, client *MigrationDBClient) error {
	hasMoreData := true
	deletedCount := 0

	for hasMoreData {
		chunk, moreData, err := client.getScenarioLaunchesChunk(ctx)
		if err != nil {
			return err
		}
		hasMoreData = moreData

		if err := client.deleteScenarioLaunches(ctx, chunk); err != nil {
			return err
		}
		deletedCount += len(chunk)
	}

	logger.Infof("ScenarioLaunches cleanup done\n")
	logger.Infof("ScenarioLaunches deleted count: %d\n", deletedCount)
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

	msg := fmt.Sprintf("Do you really want to cleanup ScenarioLaunches at `%s%s`", endpoint, prefix)
	c := cli.AskForConfirmation(msg, logger)
	if !c {
		logger.Info("Bye")
		os.Exit(0)
	}

	ctx := context.Background()

	if err := do(ctx, &MigrationDBClient{dbcli}); err != nil {
		logger.Fatal(err.Error())
	}
}
