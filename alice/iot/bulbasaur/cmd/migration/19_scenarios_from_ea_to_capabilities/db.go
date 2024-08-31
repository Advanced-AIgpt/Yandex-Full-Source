package main

import (
	"context"
	"encoding/json"
	"fmt"
	"path"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type MigrationDBClient struct {
	*db.DBClient
	UpdatedScenariosCounter int
}

func (db *MigrationDBClient) prepareRecord(ctx context.Context, record userScenario) userScenario {
	logger.Infof("prepared scenario %s of user %d for sending in next batch", record.Scenario.ID, record.User.ID)
	db.UpdatedScenariosCounter++
	return record
}

func (db *MigrationDBClient) batchUpdateScenarios(ctx context.Context, scenarios []userScenario) error {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");

		DECLARE $values AS List<Struct<
			id: String,
			huid: Uint64,
			requested_speaker_capabilities: Json,
			devices: Json
		>>;

		UPSERT INTO
			Scenarios (id, huid, requested_speaker_capabilities, devices)
		SELECT
			id, huid, requested_speaker_capabilities, devices
		FROM AS_TABLE($values);
	`, db.Prefix)

	values := make([]ydb.Value, 0, len(scenarios))
	for _, userScenario := range scenarios {
		//devices
		devicesB, err := json.Marshal(userScenario.Scenario.Devices)
		if err != nil {
			return xerrors.Errorf("cannot marshal scenario %s devices: %w", userScenario.Scenario.ID, err)
		}

		// requested_speaker_capabilities
		requestedSpeakerCapabilitiesB, err := json.Marshal(userScenario.Scenario.RequestedSpeakerCapabilities)
		if err != nil {
			return xerrors.Errorf("cannot marshal scenario %s requested_speaker_capabilities: %w", userScenario.Scenario.ID, err)
		}

		values = append(values, ydb.StructValue(
			ydb.StructFieldValue("huid", ydb.Uint64Value(tools.Huidify(userScenario.User.ID))),
			ydb.StructFieldValue("id", ydb.StringValue([]byte(userScenario.Scenario.ID))),
			ydb.StructFieldValue("requested_speaker_capabilities", ydb.JSONValue(string(requestedSpeakerCapabilitiesB))),
			ydb.StructFieldValue("devices", ydb.JSONValue(string(devicesB))),
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

func (db *MigrationDBClient) streamRecords(ctx context.Context) <-chan userScenario {
	scenariosChannel := make(chan userScenario)

	go func() {
		defer close(scenariosChannel)

		s, err := db.SessionPool.Get(ctx)
		if err != nil {
			logger.Fatalf("Can't get session: %v", err)
		}

		scenariosTablePath := path.Join(db.Prefix, "Scenarios")
		logger.Infof("Reading Scenarios table from path %q", scenariosTablePath)

		res, err := s.StreamReadTable(ctx, scenariosTablePath,
			table.ReadColumn("archived"),
			table.ReadColumn("user_id"),
			table.ReadColumn("id"),
			table.ReadColumn("devices"),
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
				var us userScenario

				//archived
				res.SeekItem("archived")
				archived := res.OBool()
				if archived {
					continue
				}

				//user
				res.NextItem()
				us.User.ID = res.OUint64()
				if us.User.ID == 0 {
					continue
				}

				res.NextItem()
				us.Scenario.ID = string(res.OString())

				//devices
				res.NextItem()
				var devices []model.ScenarioDevice
				if err := json.Unmarshal([]byte(res.OJSON()), &devices); err != nil {
					logger.Fatalf("failed to parse `devices` field for scenario %s: %v", us.Scenario.ID, err)
				}
				us.Scenario.Devices = devices

				if res.Err() != nil {
					logger.Fatalf("failed to parse YDB response row: %v", res.Err())
				}

				scenariosCount++
				scenariosChannel <- us
			}
		}

		logger.Infof("finished reading %d records from %s", scenariosCount, scenariosTablePath)
	}()

	return scenariosChannel
}
