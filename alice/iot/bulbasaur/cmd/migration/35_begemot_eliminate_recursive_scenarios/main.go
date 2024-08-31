package main

import (
	"context"
	"encoding/json"
	"fmt"
	"net/http"
	"os"
	"time"

	uzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"

	"a.yandex-team.ru/alice/iot/bulbasaur/begemot"
	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	libbegemot "a.yandex-team.ru/alice/library/go/begemot"
	"a.yandex-team.ru/alice/library/go/cli"
	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/alice/library/go/zaplogger"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/xerrors"
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

type MigrationDBClient struct {
	*db.DBClient
}

type Scenario struct {
	huid                         uint64
	id                           string
	userID                       uint64
	devices                      model.ScenarioDevices
	requestedSpeakerCapabilities model.ScenarioCapabilities
}

func (s Scenario) ToYdbStructValue() ydb.Value {
	rawDevices, _ := json.Marshal(s.devices)
	rawRequestedSpeakerCapabilities, _ := json.Marshal(s.requestedSpeakerCapabilities)
	return ydb.StructValue(
		ydb.StructFieldValue("huid", ydb.Uint64Value(s.huid)),
		ydb.StructFieldValue("id", ydb.StringValue([]byte(s.id))),
		ydb.StructFieldValue("user_id", ydb.Uint64Value(s.userID)),
		ydb.StructFieldValue("devices", ydb.JSONValue(string(rawDevices))),
		ydb.StructFieldValue("requested_speaker_capabilities", ydb.JSONValue(string(rawRequestedSpeakerCapabilities))),
	)
}

func (db *MigrationDBClient) getScenariosChunk(ctx context.Context, lastScenario *Scenario, chunkSize int) ([]Scenario, bool, error) {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");

		DECLARE $limit AS Uint64;
		DECLARE $lastHuid AS Uint64;
		DECLARE $lastId AS String;

		$scenarios = (
			SELECT
				huid, id, user_id, devices, requested_speaker_capabilities
			FROM
				Scenarios
			WHERE
				trigger_type like "scenario.trigger.voice" AND
				archived = false AND
				(
					JSON_EXISTS(devices, @@ $ ? (@.capabilities ? (@.state.instance == "text_action").size() > 0) @@) OR
					JSON_EXISTS(requested_speaker_capabilities, @@ $ ? (@.state.instance == "text_action") @@)
				) AND
				huid = $lastHuid AND id > $lastId
			ORDER BY
				huid, id
			LIMIT
				$limit

			UNION ALL

			SELECT
				huid, id, user_id, devices, requested_speaker_capabilities
			FROM
				Scenarios
			WHERE
				trigger_type like "scenario.trigger.voice" AND
				archived = false AND
				(
					JSON_EXISTS(devices, @@ $ ? (@.capabilities ? (@.state.instance == "text_action").size() > 0) @@) OR
					JSON_EXISTS(requested_speaker_capabilities, @@ $ ? (@.state.instance == "text_action") @@)
				) AND
				huid > $lastHuid
			ORDER BY
				huid, id
			LIMIT
				$limit
		);

		SELECT
			huid, id, user_id, devices, requested_speaker_capabilities
		FROM
			$scenarios
		ORDER BY
			huid, id
		LIMIT
			$limit
	`, db.Prefix)

	hasMoreData := true
	var lastID string
	var lastHuid uint64
	var limit = chunkSize

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
		var scenario Scenario
		res.NextItem()
		scenario.huid = res.OUint64()

		res.NextItem()
		scenario.id = string(res.OString())

		res.NextItem()
		scenario.userID = res.OUint64()

		res.NextItem()
		var devices []model.ScenarioDevice
		devicesRaw := res.OJSON()
		if len(devicesRaw) > 0 {
			if err := json.Unmarshal([]byte(devicesRaw), &devices); err != nil {
				return nil, false, xerrors.Errorf("failed to parse `devices` field for scenario %s: %w", scenario.id, err)
			}
		}
		scenario.devices = devices

		res.NextItem()
		var rsCapabilities []model.ScenarioCapability
		rsCapabilitiesRaw := res.OJSON()
		if len(rsCapabilitiesRaw) > 0 {
			if err := json.Unmarshal([]byte(rsCapabilitiesRaw), &rsCapabilities); err != nil {
				return nil, false, xerrors.Errorf("failed to parse `requested_speaker_capabilities` field for scenario %s: %w", scenario.id, err)
			}
		}
		scenario.requestedSpeakerCapabilities = rsCapabilities

		if err := res.Err(); err != nil {
			return nil, false, err
		}
		scenarios = append(scenarios, scenario)
	}
	return scenarios, hasMoreData, nil
}

func (db *MigrationDBClient) SelectVoiceScenariosWithPushText(ctx context.Context) (map[uint64][]Scenario, error) {
	userScenarios := make(map[uint64][]Scenario)

	var lastScenario *Scenario
	hasMoreData := true
	selectedScenariosCount := 0
	chunkSize := 1000

	for hasMoreData {
		chunk, moreData, err := db.getScenariosChunk(ctx, lastScenario, chunkSize)
		if err != nil {
			return nil, err
		}
		for _, scenario := range chunk {
			selectedScenariosCount++
			userScenarios[scenario.userID] = append(userScenarios[scenario.userID], scenario)
		}
		hasMoreData = moreData
		if hasMoreData {
			lastScenario = &chunk[len(chunk)-1]
		}
	}

	logger.Infof("Selected %d scenarios from %d users \n", selectedScenariosCount, len(userScenarios))
	return userScenarios, nil
}

func (db *MigrationDBClient) upsertScenarios(ctx context.Context, scenarios []Scenario) error {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");

		DECLARE $values AS List<Struct<
			id: String,
			huid: Uint64,
			user_id: Uint64,
			devices: Json,
			requested_speaker_capabilities: Json
		>>;

		UPSERT INTO
			Scenarios (id, huid, user_id, devices, requested_speaker_capabilities)
		SELECT
			id, huid, user_id, devices, requested_speaker_capabilities
		FROM AS_TABLE($values);
	`, db.Prefix)

	values := make([]ydb.Value, 0, len(scenarios))
	for _, scenario := range scenarios {
		values = append(values, scenario.ToYdbStructValue())
	}
	params := table.NewQueryParameters(
		table.ValueParam("$values", ydb.ListValue(values...)),
	)
	if err := db.Write(ctx, query, params); err != nil {
		return err
	}

	return nil
}

func (db *MigrationDBClient) StoreUpdatedVoiceScenarios(ctx context.Context, scenariosToUpdate map[uint64][]Scenario) error {
	var flattenedScenarios []Scenario
	for _, scenarios := range scenariosToUpdate {
		flattenedScenarios = append(flattenedScenarios, scenarios...)
	}

	updatedScenariosCount := 0
	errorScenariosCount := 0
	var errorScenarioIDs []string
	batchFrom, batchTo, batchSize := 0, 0, 1000
	for batchFrom < len(flattenedScenarios) {
		batchTo += batchSize
		if batchTo > len(flattenedScenarios) {
			batchTo = len(flattenedScenarios)
		}

		scenarioBatch := flattenedScenarios[batchFrom:batchTo]
		if err := db.upsertScenarios(ctx, scenarioBatch); err != nil {
			var scenarioIDs []string
			for _, s := range scenarioBatch {
				scenarioIDs = append(scenarioIDs, s.id)
				errorScenarioIDs = append(errorScenarioIDs, s.id)
			}
			errorScenariosCount += len(scenarioBatch)
			logger.Warn(fmt.Sprintf("unable to update %d scenarios: %v", len(scenarioIDs), err), log.Any("scenario_ids", scenarioIDs))
		} else {
			updatedScenariosCount += len(scenarioBatch)
		}
		batchFrom = batchTo
	}
	logger.Infof("updated %d/%d scenarios", updatedScenariosCount, len(flattenedScenarios))
	if errorScenariosCount > 0 {
		logger.Warn(
			fmt.Sprintf("was unable to update %d scenarios", errorScenariosCount),
			log.Any("user_scenarios_with_errors", errorScenarioIDs),
		)
	}
	return nil
}

func checkRecursiveScenarioAndUpdate(ctx context.Context, begemotClient begemot.IClient, scenario Scenario, userInfo model.UserInfo) (Scenario, bool, error) {
	updatedScenario := Scenario{huid: scenario.huid, id: scenario.id, userID: scenario.userID}

	var shouldUpdate = false
	for _, device := range scenario.devices {
		updatedDevice := model.ScenarioDevice{ID: device.ID}
		for _, capability := range device.Capabilities {
			cType, cInstance := capability.Type, capability.State.GetInstance()
			isTextAction := cType == model.QuasarServerActionCapabilityType && cInstance == model.TextActionCapabilityInstance.String()
			if isTextAction {
				state := capability.State.(model.QuasarServerActionCapabilityState)
				pushText := state.Value
				allScenarioIDs := userInfo.Scenarios.GetIDs()
				ctx := requestid.WithRequestID(ctx, requestid.New())
				scenarioID, err := begemot.ScenarioIDByPushText(ctx, logger, begemotClient, pushText, allScenarioIDs, userInfo)
				if err != nil {
					return updatedScenario, false, err
				}
				if scenarioID != "" {
					// this is prohibited - push text calls another scenario
					// this capability should be changed in the updated scenario
					updatedDevice.Capabilities = append(updatedDevice.Capabilities, model.ScenarioCapability{
						Type: model.QuasarServerActionCapabilityType,
						State: model.QuasarServerActionCapabilityState{
							Instance: model.PhraseActionCapabilityInstance,
							Value:    pushText,
						},
					})
					shouldUpdate = true
					continue
				}
			}
			updatedDevice.Capabilities = append(updatedDevice.Capabilities, capability)
		}
		updatedScenario.devices = append(updatedScenario.devices, updatedDevice)
	}
	for _, capability := range scenario.requestedSpeakerCapabilities {
		cType, cInstance := capability.Type, capability.State.GetInstance()
		isTextAction := cType == model.QuasarServerActionCapabilityType && cInstance == model.TextActionCapabilityInstance.String()
		if isTextAction {
			state := capability.State.(model.QuasarServerActionCapabilityState)
			pushText := state.Value
			allScenarioIDs := userInfo.Scenarios.GetIDs()
			ctx := requestid.WithRequestID(ctx, requestid.New())
			scenarioID, err := begemot.ScenarioIDByPushText(ctx, logger, begemotClient, pushText, allScenarioIDs, userInfo)
			if err != nil {
				return updatedScenario, false, err
			}
			if scenarioID != "" {
				// this is prohibited - push text calls another scenario
				// this capability should be changed in the updated scenario
				updatedScenario.requestedSpeakerCapabilities = append(updatedScenario.requestedSpeakerCapabilities, model.ScenarioCapability{
					Type: model.QuasarServerActionCapabilityType,
					State: model.QuasarServerActionCapabilityState{
						Instance: model.PhraseActionCapabilityInstance,
						Value:    pushText,
					},
				})
				shouldUpdate = true
				continue
			}
		}
		updatedScenario.requestedSpeakerCapabilities = append(updatedScenario.requestedSpeakerCapabilities, capability)
	}

	return updatedScenario, shouldUpdate, nil
}

func do(ctx context.Context, dbClient *MigrationDBClient, begemotClient begemot.IClient) error {
	startAt := time.Now()
	defer func() {
		logger.Infof("Time elapsed: %v", time.Since(startAt))
	}()
	userScenarios, err := dbClient.SelectVoiceScenariosWithPushText(ctx)
	if err != nil {
		return err
	}

	scenariosTotal := 0
	scenariosToUpdateCount := 0
	scenariosErrorsCount := 0
	userCount := 0
	userScenariosToUpdate := make(map[uint64][]Scenario)
	userScenariosErrors := make(map[uint64][]string)
	for userID, scenarios := range userScenarios {
		userCount++
		if userCount > 0 && userCount%100 == 0 {
			logger.Infof("processed %d/%d users, time running: %v", userCount, len(userScenarios), time.Since(startAt))
		}
		userInfo, err := dbClient.SelectUserInfo(ctx, userID)
		if err != nil {
			logger.Warnf("unable to select user info of user %d: %v", userID, err)
			for _, scenario := range scenarios {
				scenariosTotal++
				scenariosErrorsCount++
				userScenariosErrors[userID] = append(userScenariosErrors[userID], scenario.id)
				logger.Warnf("unable to check scenario %s of user %d for recursivity: %v", scenario.id, userID, err)
			}
			continue
		}
		for _, scenario := range scenarios {
			scenariosTotal++
			updatedScenario, shouldUpdate, err := checkRecursiveScenarioAndUpdate(ctx, begemotClient, scenario, userInfo)
			if err != nil {
				scenariosErrorsCount++
				userScenariosErrors[userID] = append(userScenariosErrors[userID], scenario.id)
				logger.Warnf("unable to check scenario %s of user %d for recursivity: %v", scenario.id, userID, err)
				continue
			}
			if shouldUpdate {
				scenariosToUpdateCount++
				userScenariosToUpdate[userID] = append(userScenariosToUpdate[userID], updatedScenario)
			}
		}
	}
	logger.Infof("found %d/%d scenarios of %d users to be recursive", scenariosToUpdateCount, scenariosTotal, len(userScenariosToUpdate))
	if scenariosErrorsCount > 0 {
		logger.Warn(
			fmt.Sprintf("was unable to check %d scenarios for recursivity", scenariosErrorsCount),
			log.Any("user_scenarios_with_errors", userScenariosErrors),
		)
	}
	return dbClient.StoreUpdatedVoiceScenarios(ctx, userScenariosToUpdate)
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

	libbegemotClient := libbegemot.NewClient("http://hamzard.yandex.net:8891", http.DefaultClient, zaplogger.NewNop())
	begemotClient := begemot.NewClient(libbegemotClient, logger)

	msg := fmt.Sprintf("Do you really want to eliminate recursive scenarios at `%s%s`", endpoint, prefix)
	c := cli.AskForConfirmation(msg, logger)
	if !c {
		logger.Info("Bye")
		os.Exit(0)
	}

	ctx := context.Background()
	if err := do(ctx, &MigrationDBClient{dbcli}, begemotClient); err != nil {
		logger.Fatal(err.Error())
	}
}
