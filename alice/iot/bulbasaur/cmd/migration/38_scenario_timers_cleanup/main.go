package main

import (
	"context"
	"fmt"
	"os"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
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

type scenario struct {
	huid        uint64
	id          string
	triggerType string
}

func (db *MigrationDBClient) getScenariosChunk(ctx context.Context, lastScenario *scenario) ([]scenario, bool, error) {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");

		DECLARE $limit AS Uint64;
		DECLARE $lastHuid AS Uint64;
		DECLARE $lastId AS String;

		$part1 = (
			SELECT huid, id, trigger_type FROM Scenarios
			WHERE huid = $lastHuid AND id > $lastId
			ORDER BY huid, id LIMIT $limit
		);

		$part2 = (
			SELECT huid, id, trigger_type FROM Scenarios
			WHERE huid > $lastHuid
			ORDER BY huid, id LIMIT $limit
		);

		$union = (
			SELECT * FROM $part1
			UNION ALL
			SELECT * FROM $part2
		);

		SELECT huid, id, trigger_type FROM $union ORDER BY huid, id LIMIT $limit
	`, db.Prefix)

	var lastScenarioHuid uint64
	var lastScenarioID string

	if lastScenario != nil {
		lastScenarioHuid = lastScenario.huid
		lastScenarioID = lastScenario.id
	}

	limit := 1000
	params := table.NewQueryParameters(
		table.ValueParam("$limit", ydb.Uint64Value(uint64(limit))),
		table.ValueParam("$lastHuid", ydb.Uint64Value(lastScenarioHuid)),
		table.ValueParam("$lastId", ydb.StringValue([]byte(lastScenarioID))),
	)

	scenarios := make([]scenario, 0)

	res, err := db.Read(ctx, query, params)
	if err != nil {
		return scenarios, false, err
	}

	if !res.NextSet() {
		return scenarios, false, nil
	}

	for res.NextRow() {
		var s scenario

		res.NextItem()
		s.huid = res.OUint64()

		res.NextItem()
		s.id = string(res.OString())

		res.NextItem()
		s.triggerType = string(res.OString())

		scenarios = append(scenarios, s)
	}

	hasMoreData := res.RowCount() == limit
	return scenarios, hasMoreData, nil
}

func (db *MigrationDBClient) getScenariosToDelete(launches []scenario) []scenario {
	filtered := make([]scenario, 0, len(launches))
	for _, l := range launches {
		if l.triggerType == "scenario.trigger.timer" || l.triggerType == "scenario.trigger.time" {
			filtered = append(filtered, l)
		}
	}
	return filtered
}

func (db *MigrationDBClient) deleteScenarios(ctx context.Context, launches []scenario) error {
	if len(launches) == 0 {
		return nil
	}

	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");

		DECLARE $scenarios AS List<Struct<
			huid: Uint64,
			id: String
		>>;

		DELETE FROM
			Scenarios
		ON SELECT
			huid, id
		FROM AS_TABLE($scenarios);
	`, db.Prefix)

	deleteParams := make([]ydb.Value, 0, len(launches))
	for _, l := range launches {
		logger.Infof("Prepare to delete Scenario with values (huid: %d, id: %s, trigger_type: %s)", l.huid, l.id, l.triggerType)

		deleteParams = append(deleteParams, ydb.StructValue(
			ydb.StructFieldValue("huid", ydb.Uint64Value(l.huid)),
			ydb.StructFieldValue("id", ydb.StringValue([]byte(l.id))),
		))
	}

	params := table.NewQueryParameters(
		table.ValueParam("$scenarios", ydb.ListValue(deleteParams...)),
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
	var lastScenario *scenario

	for hasMoreData {
		chunk, moreData, err := client.getScenariosChunk(ctx, lastScenario)
		if err != nil {
			return err
		}
		hasMoreData = moreData
		if len(chunk) > 0 {
			lastScenario = &chunk[len(chunk)-1]
		}

		scenarios := client.getScenariosToDelete(chunk)
		if err := client.deleteScenarios(ctx, scenarios); err != nil {
			return err
		}
		deletedCount += len(scenarios)
	}

	logger.Infof("Scenarios cleanup done\n")
	logger.Infof("Scenarios deleted count: %d\n", deletedCount)
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

	msg := fmt.Sprintf("Do you really want to cleanup Scenarios with trigger_type other than 'scenario.trigger.voice' at `%s%s`", endpoint, prefix)
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
