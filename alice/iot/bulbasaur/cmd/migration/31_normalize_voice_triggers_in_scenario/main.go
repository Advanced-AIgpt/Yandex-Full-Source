package main

import (
	"context"
	"encoding/json"
	"fmt"
	"os"

	uzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/cli"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var logger *zap.Logger

type MigrationDBClient struct {
	*db.DBClient
}

type Scenario struct {
	huid     uint64
	id       string
	triggers model.ScenarioTriggers
}

func (db *MigrationDBClient) GetScenariosChunk(ctx context.Context, lastScenario *Scenario) ([]Scenario, bool, error) {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");

		DECLARE $limit AS Uint64;
		DECLARE $lastHuid AS Uint64;
		DECLARE $lastId AS String;

		$scenarios = (
			SELECT huid, id, triggers FROM Scenarios
			WHERE trigger_type = "scenario.trigger.voice" AND huid = $lastHuid AND id > $lastId
			ORDER BY huid, id LIMIT $limit

			UNION ALL

			SELECT huid, id, triggers FROM Scenarios
			WHERE trigger_type = "scenario.trigger.voice" AND huid > $lastHuid
			ORDER BY huid, id LIMIT $limit
		);

		SELECT huid, id, triggers FROM $scenarios ORDER BY huid, id LIMIT $limit
	`, db.Prefix)

	hasMoreData := true
	var lastID string
	var lastHuid uint64
	var limit = 100

	if lastScenario != nil {
		lastID = lastScenario.id
		lastHuid = lastScenario.huid
	}

	scenarios := make([]Scenario, 0, limit)

	params := table.NewQueryParameters(
		table.ValueParam("$lastHuid", ydb.Uint64Value(lastHuid)),
		table.ValueParam("$lastId", ydb.StringValue([]byte(lastID))),
		table.ValueParam("$limit", ydb.Uint64Value(uint64(limit))),
	)

	res, err := db.Read(ctx, query, params)
	if err != nil {
		return scenarios, false, xerrors.Errorf("failed to get scenarios: %w", err)
	}

	if !res.NextSet() {
		return scenarios, false, nil
	}

	hasMoreData = res.RowCount() == limit

	for res.NextRow() {
		res.NextItem()
		lastHuid = res.OUint64()

		res.NextItem()
		lastID = string(res.OString())

		res.NextItem()
		rawTriggers := res.OJSON()

		triggers, err := model.JSONUnmarshalTriggers([]byte(rawTriggers))
		if err != nil {
			return nil, false, err
		}

		scenarios = append(scenarios, Scenario{
			huid:     lastHuid,
			id:       lastID,
			triggers: triggers,
		})
	}

	return scenarios, hasMoreData, nil
}

func (db *MigrationDBClient) UpdateNormalizedScenarioTriggers(ctx context.Context, scenarios []Scenario) (int, error) {
	updateParams := make([]ydb.Value, 0, len(scenarios))
	for _, s := range scenarios {
		s.triggers.Normalize()

		rawTriggers, err := json.Marshal(s.triggers)
		if err != nil {
			return 0, err
		}

		logger.Infof("Trigger values (huid: %d, id: %s): %s", s.huid, s.id, string(rawTriggers))

		updateParams = append(updateParams, ydb.StructValue(
			ydb.StructFieldValue("huid", ydb.Uint64Value(s.huid)),
			ydb.StructFieldValue("id", ydb.StringValue([]byte(s.id))),
			ydb.StructFieldValue("triggers", ydb.JSONValue(string(rawTriggers))),
		))
	}

	if len(updateParams) == 0 {
		return 0, nil
	}

	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");

		DECLARE $scenarios AS List<Struct<
			huid: Uint64,
			id: String,
			triggers: Json
		>>;

		UPDATE Scenarios ON SELECT huid, id, triggers FROM AS_TABLE($scenarios);
	`, db.Prefix)

	params := table.NewQueryParameters(
		table.ValueParam("$scenarios", ydb.ListValue(updateParams...)),
	)

	if err := db.Write(ctx, query, params); err != nil {
		return 0, err
	}

	return len(scenarios), nil
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
	var lastScenario *Scenario
	hasMoreData := true
	processedScenariosCount := 0
	updatedScenariosCount := 0

	for hasMoreData {
		chunk, moreData, err := client.GetScenariosChunk(ctx, lastScenario)
		if err != nil {
			return err
		}
		hasMoreData = moreData
		if hasMoreData {
			lastScenario = &chunk[len(chunk)-1]
		}

		updatedCount, err := client.UpdateNormalizedScenarioTriggers(ctx, chunk)
		if err != nil {
			return err
		}
		updatedScenariosCount += updatedCount
		processedScenariosCount += len(chunk)
	}

	logger.Infof("Scenario triggers update done\n")
	logger.Infof("Scenario processed count: %d\n", processedScenariosCount)
	logger.Infof("Scenario updated count: %d\n", updatedScenariosCount)
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

	msg := fmt.Sprintf("Do you really want to normalize triggers column for Scenarios at `%s%s`", endpoint, prefix)
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
