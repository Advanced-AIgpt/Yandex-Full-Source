package main

import (
	"context"
	"fmt"
	"path"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type MigrationDBClient struct {
	*db.DBClient
}

type Scenario struct {
	HUID       uint64
	ScenarioID string
}

func (db *MigrationDBClient) StreamScenariosWithoutRequestedSpeakerCapabilities(ctx context.Context) <-chan Scenario {
	scenariosChannel := make(chan Scenario)

	go func() {
		defer close(scenariosChannel)

		s, err := db.SessionPool.Get(ctx)
		if err != nil {
			logger.Fatalf("Can't get session: %v", err)
		}

		scenariosTablePath := path.Join(db.Prefix, "Scenarios")
		logger.Infof("Reading Scenarios table from path %q", scenariosTablePath)

		res, err := s.StreamReadTable(ctx, scenariosTablePath,
			table.ReadColumn("huid"),
			table.ReadColumn("id"),
			table.ReadColumn("requested_speaker_capabilities"),
		)
		if err != nil {
			logger.Fatalf("Failed to read table: %v", err)
		}
		defer func() {
			if err := res.Close(); err != nil {
				logger.Fatalf("Error while closing result set: %v", err)
			}
		}()

		var scenariosCount int
		for res.NextStreamSet(ctx) {
			for res.NextRow() {
				scenariosCount++

				var scenario Scenario
				res.SeekItem("huid")
				scenario.HUID = res.OUint64()

				res.SeekItem("id")
				scenario.ScenarioID = string(res.OString())

				res.SeekItem("requested_speaker_capabilities")
				rsCapabilities := res.OJSON()

				if err := res.Err(); err != nil {
					logger.Warnf("Error occurred while reading %s: %v", scenariosTablePath, err)
					continue
				}
				if len(rsCapabilities) == 0 {
					scenariosChannel <- scenario
				}
			}
		}

		logger.Infof("Finished reading %d scenarios from %s", scenariosCount, scenariosTablePath)
	}()

	return scenariosChannel
}

func (db *MigrationDBClient) FillRequestedSpeakerCapabilitiesOnScenarios(ctx context.Context, scenarios []Scenario) error {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");

		DECLARE $values AS List<Struct<
			id: String,
			huid: Uint64,
			requested_speaker_capabilities: Json
		>>;

		UPSERT INTO
			Scenarios (id, huid, requested_speaker_capabilities)
		SELECT
			id, huid, requested_speaker_capabilities
		FROM AS_TABLE($values);
	`, db.Prefix)

	values := make([]ydb.Value, 0, len(scenarios))
	for _, userScenario := range scenarios {
		values = append(values, ydb.StructValue(
			ydb.StructFieldValue("huid", ydb.Uint64Value(userScenario.HUID)),
			ydb.StructFieldValue("id", ydb.StringValue([]byte(userScenario.ScenarioID))),
			ydb.StructFieldValue("requested_speaker_capabilities", ydb.JSONValue("[]")),
		))
	}
	params := table.NewQueryParameters(
		table.ValueParam("$values", ydb.ListValue(values...)),
	)
	if err := db.Write(ctx, query, params); err != nil {
		return xerrors.Errorf("failed to fill requested_speaker_capabilities on scenarios: %w", err)
	}

	return nil
}
