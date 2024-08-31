package db

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"sort"
	"strings"

	"golang.org/x/exp/slices"

	"a.yandex-team.ru/alice/iot/bulbasaur/db/dao"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/alice/library/go/ydbclient"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type ScenarioQueryCriteria struct {
	ScenarioID string
	UserID     uint64
	IsActive   bool
}

func (db *DBClient) SelectUserScenarios(ctx context.Context, userID uint64) (model.Scenarios, error) {
	criteria := ScenarioQueryCriteria{UserID: userID}
	return db.selectUserScenarios(ctx, criteria)
}

func (db *DBClient) SelectUserScenariosSimple(ctx context.Context, userID uint64) (model.Scenarios, error) {
	criteria := ScenarioQueryCriteria{UserID: userID}
	return db.selectUserScenariosSimple(ctx, criteria)
}

func (db *DBClient) fillScenarioLaunchByStereopairsInfo(ctx context.Context, userID uint64, scenarioLaunch *model.ScenarioLaunch) error {
	stereopairs, err := db.SelectStereopairs(ctx, userID)
	if err != nil {
		return xerrors.Errorf("failed to select stereopairs to fill scenario launch: %w", err)
	}

	for stepIndex, step := range scenarioLaunch.Steps {
		if step.Type() != model.ScenarioStepActionsType {
			continue
		}

		parameters := step.Parameters().(model.ScenarioStepActionsParameters)
		var stepStereopairs model.Stereopairs
		for _, stepDevice := range parameters.Devices {
			if stereopair, exist := stereopairs.GetByDeviceID(stepDevice.ID); exist {
				stepStereopairs = append(stepStereopairs, stereopair)
			}
		}
		parameters.Stereopairs.FromStereopairs(stepStereopairs)
		step.SetParameters(parameters)
		scenarioLaunch.Steps[stepIndex] = step
	}
	return nil
}

func (db *DBClient) StoreScenarioLaunch(ctx context.Context, userID uint64, scenarioLaunch model.ScenarioLaunch) (launchID string, allErr error) {
	if err := db.fillScenarioLaunchByStereopairsInfo(ctx, userID, &scenarioLaunch); err != nil {
		return "", xerrors.Errorf("failed to fill scenario launch by stereopairs: %w", err)
	}

	if scenarioLaunch.ID == "" {
		scenarioLaunch.ID = model.GenerateScenarioLaunchID()
	}
	launchID = scenarioLaunch.ID

	row := &dao.ScenarioLaunch{
		UserID:              userID,
		ScenarioLaunchModel: dao.ScenarioLaunchModel(scenarioLaunch),
	}

	err := db.StoreYDBRows(ctx, "ScenarioLaunches", []ydbclient.YDBRow{row})
	if err != nil {
		return "", xerrors.Errorf("failed to store scenario launch: %w", err)
	}
	return launchID, nil
}

func (db *DBClient) SelectScheduledScenarioLaunches(ctx context.Context, userID uint64) (model.ScenarioLaunches, error) {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");
		DECLARE $huid AS Uint64;
		DECLARE $trigger_types AS List<String>;

		SELECT
			%v
		FROM
			ScenarioLaunches VIEW scenario_launches_huid_status
		WHERE
			huid == $huid AND
			status == "SCHEDULED"
		ORDER BY
			scheduled ASC`, db.Prefix, ydbclient.JoinFieldNames(&dao.ScenarioLaunch{}))

	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
	)

	res, err := db.Read(ctx, query, params)
	if err != nil {
		return nil, err
	}

	var scenarios []model.ScenarioLaunch

	for res.NextSet() {
		for res.NextRow() {
			scenario, err := db.parseScenarioLaunch(ctx, res)
			if err != nil {
				return nil, err
			}

			if res.Err() != nil {
				err := xerrors.Errorf("failed to parse YDB response row: %w", res.Err())
				ctxlog.Error(ctx, db.Logger, err.Error())
				return nil, err
			}

			scenarios = append(scenarios, scenario)
		}
	}
	return scenarios, nil
}

func (db *DBClient) SelectScenarioLaunchList(ctx context.Context, userID uint64, limit uint64, triggerTypes []model.ScenarioTriggerType) (model.ScenarioLaunches, error) {
	ydbTriggerTypes := make([]ydb.Value, 0, len(triggerTypes))
	for _, tType := range triggerTypes {
		ydbTriggerTypes = append(ydbTriggerTypes, ydb.StringValue([]byte(tType)))
	}
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");
		DECLARE $huid AS Uint64;
		DECLARE $limit AS Uint64;
		DECLARE $trigger_types AS List<String>;

		SELECT
			%v
		FROM ScenarioLaunches
		WHERE
			ScenarioLaunches.huid == $huid AND
			ScenarioLaunches.launch_trigger_type in $trigger_types
		ORDER BY
			ScenarioLaunches.finished DESC
		LIMIT $limit`, db.Prefix, ydbclient.JoinFieldNames(&dao.ScenarioLaunch{}))

	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
		table.ValueParam("$limit", ydb.Uint64Value(limit)),
		table.ValueParam("$trigger_types", ydb.ListValue(ydbTriggerTypes...)),
	)

	res, err := db.Read(ctx, query, params)
	if err != nil {
		return nil, err
	}

	var scenarios []model.ScenarioLaunch

	for res.NextSet() {
		for res.NextRow() {
			scenario, err := db.parseScenarioLaunch(ctx, res)
			if err != nil {
				return nil, err
			}

			if res.Err() != nil {
				err := xerrors.Errorf("failed to parse YDB response row: %w", res.Err())
				ctxlog.Error(ctx, db.Logger, err.Error())
				return nil, err
			}

			scenarios = append(scenarios, scenario)
		}
	}
	return scenarios, nil
}

func (db *DBClient) SelectScenarioLaunchesByScenarioID(ctx context.Context, userID uint64, scenarioID string) (model.ScenarioLaunches, error) {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");
		DECLARE $huid AS Uint64;
		DECLARE $scenario_id AS String;
		DECLARE $status AS String;

		SELECT
			%v
		FROM ScenarioLaunches VIEW scenario_launches_huid_scenario_id_status
		WHERE
			huid == $huid AND
			scenario_id == $scenario_id AND
			status == $status
		ORDER BY
			scheduled`, db.Prefix, ydbclient.JoinFieldNames(&dao.ScenarioLaunch{}))

	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
		table.ValueParam("$scenario_id", ydb.StringValue([]byte(scenarioID))),
		table.ValueParam("$status", ydb.StringValue([]byte(model.ScenarioLaunchScheduled))),
	)

	res, err := db.Read(ctx, query, params)
	if err != nil {
		return nil, err
	}

	var scenarios []model.ScenarioLaunch

	for res.NextSet() {
		for res.NextRow() {
			scenario, err := db.parseScenarioLaunch(ctx, res)
			if err != nil {
				return nil, err
			}

			if res.Err() != nil {
				err := xerrors.Errorf("failed to parse YDB response row: %w", res.Err())
				ctxlog.Error(ctx, db.Logger, err.Error())
				return nil, err
			}

			scenarios = append(scenarios, scenario)
		}
	}
	return scenarios, nil
}

func (db *DBClient) SelectScenarioLaunch(ctx context.Context, userID uint64, launchID string) (model.ScenarioLaunch, error) {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");
		DECLARE $huid AS Uint64;
		DECLARE $launch_id AS String;

		SELECT
			%v
		FROM ScenarioLaunches
		WHERE
			ScenarioLaunches.huid == $huid AND
			ScenarioLaunches.id == $launch_id`, db.Prefix, ydbclient.JoinFieldNames(&dao.ScenarioLaunch{}))

	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
		table.ValueParam("$launch_id", ydb.StringValue([]byte(launchID))),
	)

	res, err := db.Read(ctx, query, params)
	if err != nil {
		return model.ScenarioLaunch{}, err
	}

	if !res.NextSet() || !res.NextRow() {
		return model.ScenarioLaunch{}, &model.ScenarioLaunchNotFoundError{}
	}

	scenario, err := db.parseScenarioLaunch(ctx, res)
	if err != nil {
		return scenario, err
	}

	if res.Err() != nil {
		err := xerrors.Errorf("failed to parse YDB response row: %w", res.Err())
		ctxlog.Error(ctx, db.Logger, err.Error())
		return scenario, err
	}

	return scenario, nil
}

func (db *DBClient) UpdateScenarioLaunch(ctx context.Context, userID uint64, scenarioLaunch model.ScenarioLaunch) error {
	row := &dao.ScenarioLaunch{
		UserID:              userID,
		ScenarioLaunchModel: dao.ScenarioLaunchModel(scenarioLaunch),
	}
	return db.StoreYDBRows(ctx, "ScenarioLaunches", []ydbclient.YDBRow{row})
}

func (db *DBClient) UpdateScenarioLaunchScheduledTime(ctx context.Context, userID uint64, launchID string, newScheduled timestamp.PastTimestamp) error {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");
		DECLARE $launch_id AS String;
		DECLARE $huid AS Uint64;
		DECLARE $scheduled AS Timestamp;

		UPDATE
			ScenarioLaunches
		SET
			scheduled = $scheduled
		WHERE
			huid == $huid AND
			id == $launch_id AND
			status == "SCHEDULED"`, db.Prefix)

	params := table.NewQueryParameters(
		table.ValueParam("$launch_id", ydb.StringValue([]byte(launchID))),
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
		table.ValueParam("$scheduled", ydb.TimestampValue(newScheduled.YdbTimestamp())),
	)

	return db.Write(ctx, query, params)
}

func (db *DBClient) DeleteScenarioLaunches(ctx context.Context, userID uint64, launchIDs []string) error {
	if len(launchIDs) == 0 {
		return nil
	}

	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");

		DECLARE $scenario_launches AS List<Struct<
			huid: Uint64,
			id: String
		>>;

		DELETE FROM
			ScenarioLaunches
		ON SELECT
			huid, id
		FROM
			AS_TABLE($scenario_launches);
	`, db.Prefix)

	deleteParams := make([]ydb.Value, 0, len(launchIDs))
	for _, launchID := range launchIDs {
		deleteParams = append(deleteParams, ydb.StructValue(
			ydb.StructFieldValue("huid", ydb.Uint64Value(tools.Huidify(userID))),
			ydb.StructFieldValue("id", ydb.StringValue([]byte(launchID))),
		))
	}

	params := table.NewQueryParameters(
		table.ValueParam("$scenario_launches", ydb.ListValue(deleteParams...)),
	)

	err := db.Write(ctx, query, params)
	if err != nil {
		return xerrors.Errorf("database request has failed: %w", err)
	}
	return nil
}

func (db *DBClient) CancelScenarioLaunchesByTriggerTypeAndStatus(ctx context.Context, userID uint64, triggerType model.ScenarioTriggerType, status model.ScenarioLaunchStatus) error {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");
		DECLARE $huid AS Uint64;
		DECLARE $trigger_type AS String;
		DECLARE $status AS String;
		DECLARE $finished_time AS Timestamp;

		UPSERT INTO
			ScenarioLaunches
		SELECT
			huid, id, "CANCELED" as status, $finished_time AS finished
		FROM
			ScenarioLaunches VIEW scenario_launches_huid_status
		WHERE
			huid == $huid AND
			status == $status AND
			launch_trigger_type == $trigger_type;
   `, db.Prefix)

	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
		table.ValueParam("$trigger_type", ydb.StringValue([]byte(triggerType))),
		table.ValueParam("$status", ydb.StringValue([]byte(status))),
		table.ValueParam("$finished_time", ydb.TimestampValue(db.timestamper.CurrentTimestamp().YdbTimestamp())),
	)

	err := db.Write(ctx, query, params)
	if err != nil {
		return xerrors.Errorf("database request has failed: %w", err)
	}
	return nil
}

func (db *DBClient) SelectScenario(ctx context.Context, userID uint64, scenarioID string) (model.Scenario, error) {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");
		DECLARE $huid AS Uint64;
		DECLARE $scenario_id AS String;

		SELECT
			s.id as id,
			s.name,
			s.triggers,
			s.icon,
			s.devices,
			s.requested_speaker_capabilities,
			s.steps,
			s.is_active,
			s.effective_time,
			s.push_on_invoke,
			f.target_id
		FROM Scenarios as s
		LEFT JOIN
			(SELECT * FROM Favorites WHERE huid = $huid AND type == "scenario") AS f
		ON
			s.huid = f.huid AND
			s.id = f.target_id
		WHERE
			s.huid == $huid AND
			s.id == $scenario_id AND
			s.archived == false`, db.Prefix)

	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
		table.ValueParam("$scenario_id", ydb.StringValue([]byte(scenarioID))),
	)

	res, err := db.Read(ctx, query, params)
	if err != nil {
		return model.Scenario{}, err
	}

	if !res.NextSet() || !res.NextRow() {
		return model.Scenario{}, &model.ScenarioNotFoundError{}
	}

	res.SeekItem("id")
	id := string(res.OString())
	res.NextItem()
	name := model.ScenarioName(res.OString())

	//triggers
	res.NextItem()
	triggers := make(model.ScenarioTriggers, 0)
	triggersRaw := res.OJSON()
	if len(triggersRaw) > 0 {
		triggers, err = model.JSONUnmarshalTriggers([]byte(triggersRaw))
		if err != nil {
			return model.Scenario{}, xerrors.Errorf("failed to parse `triggers` field for scenario %s: %w", scenarioID, err)
		}
		sort.Sort(triggers)
	}

	res.NextItem()
	icon := model.ScenarioIcon(res.OString())

	//devices
	res.NextItem()
	var devices []model.ScenarioDevice
	devicesRaw := res.OJSON()
	if len(devicesRaw) > 0 {
		if err := json.Unmarshal([]byte(devicesRaw), &devices); err != nil {
			return model.Scenario{}, xerrors.Errorf("failed to parse `devices` field for scenario %s: %w", scenarioID, err)
		}
	}

	// requested speaker capabilities
	res.NextItem()
	var rsCapabilities []model.ScenarioCapability
	rsCapabilitiesRaw := res.OJSON()
	if len(rsCapabilitiesRaw) > 0 {
		if err := json.Unmarshal([]byte(rsCapabilitiesRaw), &rsCapabilities); err != nil {
			return model.Scenario{}, xerrors.Errorf("failed to parse `requested_speaker_capabilities` field for scenario %s: %w", scenarioID, err)
		}
	}

	// steps
	res.NextItem()
	steps := make(model.ScenarioSteps, 0)
	stepsRaw := res.OJSON()
	if len(stepsRaw) > 0 {
		if steps, err = model.JSONUnmarshalScenarioSteps([]byte(stepsRaw)); err != nil {
			return model.Scenario{}, xerrors.Errorf("failed to parse `steps` field for scenario %s: %w", scenarioID, err)
		}
	}

	// is_active
	res.NextItem()
	var isActive bool
	if res.IsNull() {
		isActive = true
	} else {
		isActive = res.OBool()
	}

	// effective time
	res.NextItem()
	var effectiveTime *model.EffectiveTime
	if !res.IsNull() {
		if err := json.Unmarshal([]byte(res.OJSON()), &effectiveTime); err != nil {
			return model.Scenario{}, xerrors.Errorf("failed to parse `effective_time` field for scenario %s: %w", scenarioID, err)
		}
	}

	// push_on_invoke
	res.NextItem()
	var pushOnInvoke bool
	if res.IsNull() {
		pushOnInvoke = false
	} else {
		pushOnInvoke = res.OBool()
	}

	// favourite
	res.NextItem()
	inFavoritesID := string(res.OString())
	isFavorite := len(inFavoritesID) > 0

	if res.Err() != nil {
		err := xerrors.Errorf("failed to parse YDB response row: %w", res.Err())
		ctxlog.Error(ctx, db.Logger, err.Error())
		return model.Scenario{}, err
	}

	return model.Scenario{
		ID:                           id,
		Name:                         name,
		Icon:                         icon,
		Triggers:                     triggers,
		Devices:                      devices,
		RequestedSpeakerCapabilities: rsCapabilities,
		Steps:                        steps,
		IsActive:                     isActive,
		EffectiveTime:                effectiveTime,
		Favorite:                     isFavorite,
		PushOnInvoke:                 pushOnInvoke,
	}, nil
}

func (db *DBClient) CreateScenarioWithLaunch(ctx context.Context, userID uint64, scenario model.Scenario, launch model.ScenarioLaunch) error {
	if scenario.ID == "" {
		return &model.InvalidValueError{}
	}

	scenario.Name = model.ScenarioName(tools.StandardizeSpaces(string(scenario.Name)))
	if scenario.Name == "" {
		return &model.InvalidValueError{}
	}

	devicesB, err := json.Marshal(scenario.Devices)
	if err != nil {
		return xerrors.Errorf("cannot marshal scenario.devices: %w", err)
	}

	requestedSpeakerCapabilitiesB, err := json.Marshal(scenario.RequestedSpeakerCapabilities)
	if err != nil {
		return xerrors.Errorf("cannot marshal scenario.requested_speaker_capabilities: %w", err)
	}

	scenario.Triggers.Normalize()
	triggers, err := json.Marshal(scenario.Triggers)
	if err != nil {
		return xerrors.Errorf("cannot marshal scenario.triggers: %w", err)
	}

	scenarioStepsB, err := json.Marshal(scenario.Steps)
	if err != nil {
		return xerrors.Errorf("cannot marshal scenario.Steps: %w", err)
	}

	if launch.ID == "" {
		return &model.InvalidValueError{}
	}

	effectiveTimeParam := ydb.NullValue(ydb.TypeJSON)
	if scenario.EffectiveTime != nil {
		effectiveTimeB, err := json.Marshal(scenario.EffectiveTime)
		if err != nil {
			return xerrors.Errorf("cannot marshal scenario.effective_time: %w", err)
		}

		effectiveTimeParam = ydb.OptionalValue(ydb.JSONValue(string(effectiveTimeB)))
	}

	declares := fmt.Sprintf(`
		DECLARE $scenario_id AS String;
		DECLARE $huid AS Uint64;
		DECLARE $user_id AS Uint64;
		DECLARE $name AS String;
		DECLARE $icon AS String;
		DECLARE $triggers AS JSON;
		DECLARE $devices AS Json;
		DECLARE $requested_speaker_capabilities AS JSON;
		DECLARE $scenario_steps AS JSON;
		DECLARE $timestamp AS Timestamp;
		DECLARE $effective_time AS Optional<JSON>;
		DECLARE $is_active AS Bool;
		DECLARE $push_on_invoke AS Bool;

		DECLARE $launches AS List<%s>;
	`, dao.ScenarioLaunch{}.YDBDeclareStruct())

	query := `
		UPSERT INTO
			Scenarios (id, huid, user_id, name, icon, triggers, devices, requested_speaker_capabilities, steps,
			created, archived, is_active, effective_time, push_on_invoke)
		VALUES
			($scenario_id, $huid, $user_id, $name, $icon, $triggers, $devices, $requested_speaker_capabilities, $scenario_steps,
			$timestamp, false, $is_active, $effective_time, $push_on_invoke);

		UPSERT INTO
			ScenarioLaunches
		SELECT
			*
		FROM
			AS_TABLE ($launches);
	`

	ydbLaunch, err := dao.ScenarioLaunch{
		UserID:              userID,
		ScenarioLaunchModel: dao.ScenarioLaunchModel(launch),
	}.YDBStruct()
	if err != nil {
		return xerrors.Errorf("failed to store launch as ydb struct: %w", err)
	}

	params := table.NewQueryParameters(
		table.ValueParam("$scenario_id", ydb.StringValue([]byte(scenario.ID))),
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
		table.ValueParam("$user_id", ydb.Uint64Value(userID)),
		table.ValueParam("$name", ydb.StringValue([]byte(scenario.Name))),
		table.ValueParam("$icon", ydb.StringValue([]byte(scenario.Icon))),
		table.ValueParam("$triggers", ydb.JSONValue(string(triggers))),
		table.ValueParam("$devices", ydb.JSONValue(string(devicesB))),
		table.ValueParam("$requested_speaker_capabilities", ydb.JSONValue(string(requestedSpeakerCapabilitiesB))),
		table.ValueParam("$scenario_steps", ydb.JSONValue(string(scenarioStepsB))),
		table.ValueParam("$timestamp", ydb.TimestampValue(db.CurrentTimestamp().YdbTimestamp())),
		table.ValueParam("$is_active", ydb.BoolValue(scenario.IsActive)),
		table.ValueParam("$effective_time", effectiveTimeParam),
		table.ValueParam("$push_on_invoke", ydb.BoolValue(scenario.PushOnInvoke)),
		table.ValueParam("$launches", ydb.ListValue(ydbLaunch)),
	)

	if deviceTriggers := scenario.Triggers.GetDevicePropertyTriggers(); len(deviceTriggers) > 0 {
		indexQueryDeclares, indexQuery, param, err := makeDeviceTriggersIndexUpsertQuery(userID, scenario.ID, deviceTriggers)
		if err != nil {
			return err
		}
		declares += indexQueryDeclares
		query += indexQuery
		params.Add(param)
	}

	insertQuery := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");
		%s
		%s
	`, db.Prefix, declares, query)

	err = db.CallInTx(ctx, SerializableReadWrite, func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
		//check scenario already exist
		criteria := ScenarioQueryCriteria{UserID: userID}
		tx, scenarios, err := db.getUserScenariosTx(ctx, s, txControl, criteria)
		if err != nil {
			return tx, err
		}

		err = scenarios.ValidateNewScenario(scenario)
		if err != nil {
			return tx, err
		}

		insertQueryStmt, err := s.Prepare(ctx, insertQuery)
		if err != nil {
			return tx, err
		}
		_, err = tx.ExecuteStatement(ctx, insertQueryStmt, params)
		return tx, err
	})

	if err != nil {
		return xerrors.Errorf("database request has failed: %w", err)
	}

	return nil
}

func (db *DBClient) UpdateScenarioAndCreateLaunch(ctx context.Context, userID uint64, scenario model.Scenario, newLaunch model.ScenarioLaunch) error {
	if err := db.fillScenarioLaunchByStereopairsInfo(ctx, userID, &newLaunch); err != nil {
		return xerrors.Errorf("failed to fill scenario launch by stereopairs: %w", err)
	}

	if scenario.ID == "" {
		return &model.InvalidValueError{}
	}

	scenario.Name = model.ScenarioName(tools.StandardizeSpaces(string(scenario.Name)))
	if scenario.Name == "" {
		return &model.InvalidValueError{}
	}

	devicesB, err := json.Marshal(scenario.Devices)
	if err != nil {
		return xerrors.Errorf("cannot marshal scenario.devices: %w", err)
	}

	requestedSpeakerCapabilitiesB, err := json.Marshal(scenario.RequestedSpeakerCapabilities)
	if err != nil {
		return xerrors.Errorf("cannot marshal scenario.requested_speaker_capabilities: %w", err)
	}

	scenario.Triggers.Normalize()
	triggers, err := json.Marshal(scenario.Triggers)
	if err != nil {
		return xerrors.Errorf("cannot marshal scenario.triggers: %w", err)
	}

	scenarioStepsB, err := json.Marshal(scenario.Steps)
	if err != nil {
		return xerrors.Errorf("cannot marshal scenario.Steps: %w", err)
	}

	triggerValues := ydb.NullValue(ydb.TypeJSON)
	if newLaunch.LaunchTriggerValue != nil {
		triggerValuesB, err := json.Marshal(newLaunch.LaunchTriggerValue)
		if err != nil {
			return xerrors.Errorf("cannot marshal scenarioLaunches.LaunchTriggerValue: %w", err)
		}
		triggerValues = ydb.OptionalValue(ydb.JSONValue(string(triggerValuesB)))
	}

	launchStepsB, err := json.Marshal(newLaunch.Steps)
	if err != nil {
		return xerrors.Errorf("cannot marshal scenarioLaunches.Steps: %w", err)
	}

	if newLaunch.ID == "" {
		return &model.InvalidValueError{}
	}

	effectiveTimeParam := ydb.NullValue(ydb.TypeJSON)
	if scenario.EffectiveTime != nil {
		effectiveTimeB, err := json.Marshal(scenario.EffectiveTime)
		if err != nil {
			return xerrors.Errorf("cannot marshal scenario.effective_time: %w", err)
		}

		effectiveTimeParam = ydb.OptionalValue(ydb.JSONValue(string(effectiveTimeB)))
	}

	updateDeclares := fmt.Sprintf(`
		DECLARE $scenario_id AS String;
		DECLARE $huid AS Uint64;
		DECLARE $user_id AS Uint64;
		DECLARE $name AS String;
		DECLARE $icon AS String;
		DECLARE $triggers AS JSON;
		DECLARE $devices AS Json;
		DECLARE $requested_speaker_capabilities AS JSON;
		DECLARE $scenario_steps AS JSON;
		DECLARE $is_active AS Bool;
		DECLARE $effective_time AS Optional<JSON>;
		DECLARE $push_on_invoke AS Bool;

		DECLARE $launch_trigger_id AS String;
		DECLARE $launch_trigger_type AS String;
		DECLARE $launch_trigger_value AS Optional<JSON>;
		DECLARE $launch_id AS String;
		DECLARE $launch_steps AS JSON;
		DECLARE $launch_current_step_index AS Int64;
		DECLARE $created AS Timestamp;
		DECLARE $scheduled AS Timestamp;
		DECLARE $finished AS Optional<Timestamp>;
		DECLARE $status AS String;
		DECLARE $error AS String;

		DECLARE $launches AS List<%s>;

	`, dao.ScenarioLaunch{}.YDBDeclareStruct())

	updateQuery := `
		UPDATE
			Scenarios
		SET
			name = $name,
			icon = $icon,
			triggers = $triggers,
			devices = $devices,
			requested_speaker_capabilities = $requested_speaker_capabilities,
			steps = $scenario_steps,
			is_active = $is_active,
			effective_time = $effective_time,
			push_on_invoke = $push_on_invoke
		WHERE
			huid == $huid AND
			id == $scenario_id AND
			archived == false;

		UPSERT INTO
			ScenarioLaunches
		SELECT
			*
		FROM
			AS_TABLE ($launches);
	`

	ydbLaunch, err := dao.ScenarioLaunch{
		UserID:              userID,
		ScenarioLaunchModel: dao.ScenarioLaunchModel(newLaunch),
	}.YDBStruct()
	if err != nil {
		return xerrors.Errorf("failed to convert launch to ydb struct: %w", err)
	}

	params := table.NewQueryParameters(
		table.ValueParam("$scenario_id", ydb.StringValue([]byte(scenario.ID))),
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
		table.ValueParam("$user_id", ydb.Uint64Value(userID)),
		table.ValueParam("$name", ydb.StringValue([]byte(scenario.Name))),
		table.ValueParam("$icon", ydb.StringValue([]byte(scenario.Icon))),
		table.ValueParam("$triggers", ydb.JSONValue(string(triggers))),
		table.ValueParam("$devices", ydb.JSONValue(string(devicesB))),
		table.ValueParam("$requested_speaker_capabilities", ydb.JSONValue(string(requestedSpeakerCapabilitiesB))),
		table.ValueParam("$scenario_steps", ydb.JSONValue(string(scenarioStepsB))),
		table.ValueParam("$is_active", ydb.BoolValue(scenario.IsActive)),
		table.ValueParam("$launch_id", ydb.StringValue([]byte(newLaunch.ID))),
		table.ValueParam("$launch_trigger_id", ydb.StringValue([]byte(newLaunch.LaunchTriggerID))),
		table.ValueParam("$launch_trigger_type", ydb.StringValue([]byte(newLaunch.LaunchTriggerType))),
		table.ValueParam("$launch_trigger_value", triggerValues),
		table.ValueParam("$launch_steps", ydb.JSONValue(string(launchStepsB))),
		table.ValueParam("$launch_current_step_index", ydb.Int64Value(int64(newLaunch.CurrentStepIndex))),
		table.ValueParam("$created", ydb.TimestampValue(newLaunch.Created.YdbTimestamp())),
		table.ValueParam("$scheduled", ydb.TimestampValue(newLaunch.Scheduled.YdbTimestamp())),
		table.ValueParam("$finished", optionalYdbTimestampValue(newLaunch.Finished)),
		table.ValueParam("$status", ydb.StringValue([]byte(newLaunch.Status))),
		table.ValueParam("$error", ydb.StringValue([]byte(newLaunch.ErrorCode))),
		table.ValueParam("$effective_time", effectiveTimeParam),
		table.ValueParam("$push_on_invoke", ydb.BoolValue(scenario.PushOnInvoke)),
		table.ValueParam("$launches", ydb.ListValue(ydbLaunch)),
	)

	var insertTriggersIndexDeclares string
	var insertTriggersIndexQuery string
	if deviceTriggers := scenario.Triggers.GetDevicePropertyTriggers(); len(deviceTriggers) > 0 {
		indexQueryDeclares, indexQuery, param, err := makeDeviceTriggersIndexUpsertQuery(userID, scenario.ID, deviceTriggers)
		if err != nil {
			return err
		}
		insertTriggersIndexDeclares = indexQueryDeclares
		insertTriggersIndexQuery = indexQuery
		params.Add(param)
	}

	err = db.CallInTx(ctx, SerializableReadWrite, func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
		criteria := ScenarioQueryCriteria{UserID: userID}
		tx, scenarios, err := db.getUserScenariosTx(ctx, s, txControl, criteria)
		if err != nil {
			return tx, err
		}

		scenariosMap := scenarios.ToMap()
		oldScenario, ok := scenariosMap[scenario.ID]
		if !ok {
			return tx, &model.ScenarioNotFoundError{}
		}

		err = scenarios.ExcludeScenario(scenario.ID).ValidateNewScenario(scenario)
		if err != nil {
			return tx, err
		}

		var deleteOldTriggersIndexDeclares string
		var deleteOldTriggersIndexQuery string
		if devicePropertyTriggers := oldScenario.Triggers.GetDevicePropertyTriggers(); len(devicePropertyTriggers) > 0 {
			indexQueryDeclares, indexQuery, param := makeDeviceTriggersIndexRemoveQuery(userID, scenario.ID, devicePropertyTriggers)
			deleteOldTriggersIndexDeclares = indexQueryDeclares
			deleteOldTriggersIndexQuery = indexQuery
			params.Add(param)
		}

		query := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			%s
		`, db.Prefix, strings.Join([]string{deleteOldTriggersIndexDeclares, updateDeclares, insertTriggersIndexDeclares,
			deleteOldTriggersIndexQuery, updateQuery, insertTriggersIndexQuery}, "\n"))

		stmt, err := s.Prepare(ctx, query)
		if err != nil {
			return tx, err
		}
		_, err = tx.ExecuteStatement(ctx, stmt, params)
		if err != nil {
			return tx, err
		}

		return tx, nil
	})

	if err != nil {
		return xerrors.Errorf("database request has failed: %w", err)
	}
	return nil
}

func (db *DBClient) UpdateScenarioAndDeleteLaunches(ctx context.Context, userID uint64, scenario model.Scenario) error {
	if scenario.ID == "" {
		return &model.InvalidValueError{}
	}

	scenario.Name = model.ScenarioName(tools.StandardizeSpaces(string(scenario.Name)))
	if scenario.Name == "" {
		return &model.InvalidValueError{}
	}

	devicesB, err := json.Marshal(scenario.Devices)
	if err != nil {
		return xerrors.Errorf("cannot marshal scenario.devices: %w", err)
	}

	requestedSpeakerCapabilitiesB, err := json.Marshal(scenario.RequestedSpeakerCapabilities)
	if err != nil {
		return xerrors.Errorf("cannot marshal scenario.requested_speaker_capabilities: %w", err)
	}

	scenario.Triggers.Normalize()
	triggers, err := json.Marshal(scenario.Triggers)
	if err != nil {
		return xerrors.Errorf("cannot marshal scenario.triggers: %w", err)
	}

	stepsB, err := json.Marshal(scenario.Steps)
	if err != nil {
		return xerrors.Errorf("cannot marshal scenario.steps: %w", err)
	}

	effectiveTimeParam := ydb.NullValue(ydb.TypeJSON)
	if scenario.EffectiveTime != nil {
		effectiveTimeB, err := json.Marshal(scenario.EffectiveTime)
		if err != nil {
			return xerrors.Errorf("cannot marshal scenario.effective_time: %w", err)
		}

		effectiveTimeParam = ydb.OptionalValue(ydb.JSONValue(string(effectiveTimeB)))
	}

	updateDeclares := `
		DECLARE $scenario_id AS String;
		DECLARE $huid AS Uint64;
		DECLARE $user_id AS Uint64;
		DECLARE $name AS String;
		DECLARE $icon AS String;
		DECLARE $triggers AS JSON;
		DECLARE $devices AS Json;
		DECLARE $requested_speaker_capabilities AS JSON;
		DECLARE $steps AS JSON;
		DECLARE $is_active AS Bool;
		DECLARE $effective_time AS Optional<JSON>;
		DECLARE $push_on_invoke AS Bool;
	`

	updateQuery := `
		DELETE FROM
			ScenarioLaunches
		ON SELECT
			huid, id
		FROM
			ScenarioLaunches VIEW scenario_launches_huid_scenario_id_status
		WHERE
			huid == $huid AND
			scenario_id == $scenario_id AND
			status == "SCHEDULED";

		UPDATE
			Scenarios
		SET
			name = $name,
			icon = $icon,
			triggers = $triggers,
			devices = $devices,
			requested_speaker_capabilities = $requested_speaker_capabilities,
			steps = $steps,
			is_active = $is_active,
			effective_time = $effective_time,
			push_on_invoke = $push_on_invoke
		WHERE
			huid == $huid AND
			id == $scenario_id AND
			archived == false;
	`

	params := table.NewQueryParameters(
		table.ValueParam("$scenario_id", ydb.StringValue([]byte(scenario.ID))),
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
		table.ValueParam("$user_id", ydb.Uint64Value(userID)),
		table.ValueParam("$name", ydb.StringValue([]byte(scenario.Name))),
		table.ValueParam("$icon", ydb.StringValue([]byte(scenario.Icon))),
		table.ValueParam("$triggers", ydb.JSONValue(string(triggers))),
		table.ValueParam("$devices", ydb.JSONValue(string(devicesB))),
		table.ValueParam("$requested_speaker_capabilities", ydb.JSONValue(string(requestedSpeakerCapabilitiesB))),
		table.ValueParam("$steps", ydb.JSONValue(string(stepsB))),
		table.ValueParam("$is_active", ydb.BoolValue(scenario.IsActive)),
		table.ValueParam("$effective_time", effectiveTimeParam),
		table.ValueParam("$push_on_invoke", ydb.BoolValue(scenario.PushOnInvoke)),
	)

	var insertTriggersIndexDeclares string
	var insertTriggersIndexQuery string
	if deviceTriggers := scenario.Triggers.GetDevicePropertyTriggers(); len(deviceTriggers) > 0 {
		indexQueryDeclares, indexQuery, param, err := makeDeviceTriggersIndexUpsertQuery(userID, scenario.ID, deviceTriggers)
		if err != nil {
			return err
		}
		insertTriggersIndexDeclares = indexQueryDeclares
		insertTriggersIndexQuery = indexQuery
		params.Add(param)
	}

	err = db.CallInTx(ctx, SerializableReadWrite, func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
		criteria := ScenarioQueryCriteria{UserID: userID}
		tx, scenarios, err := db.getUserScenariosTx(ctx, s, txControl, criteria)
		if err != nil {
			return tx, err
		}

		scenariosMap := scenarios.ToMap()
		oldScenario, ok := scenariosMap[scenario.ID]
		if !ok {
			return tx, &model.ScenarioNotFoundError{}
		}

		err = scenarios.ExcludeScenario(scenario.ID).ValidateNewScenario(scenario)
		if err != nil {
			return tx, err
		}

		var deleteOldTriggersIndexDeclares string
		var deleteOldTriggersIndexQuery string
		if devicePropertyTriggers := oldScenario.Triggers.GetDevicePropertyTriggers(); len(devicePropertyTriggers) > 0 {
			indexQueryDeclares, indexQuery, param := makeDeviceTriggersIndexRemoveQuery(userID, scenario.ID, devicePropertyTriggers)
			deleteOldTriggersIndexDeclares = indexQueryDeclares
			deleteOldTriggersIndexQuery = indexQuery
			params.Add(param)
		}

		query := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			%s
		`, db.Prefix, strings.Join([]string{deleteOldTriggersIndexDeclares, updateDeclares, insertTriggersIndexDeclares,
			deleteOldTriggersIndexQuery, updateQuery, insertTriggersIndexQuery}, "\n"))

		stmt, err := s.Prepare(ctx, query)
		if err != nil {
			return tx, err
		}
		_, err = tx.ExecuteStatement(ctx, stmt, params)
		if err != nil {
			return tx, err
		}

		return tx, nil
	})

	if err != nil {
		return xerrors.Errorf("database request has failed: %w", err)
	}
	return nil
}

func (db *DBClient) DeleteScenario(ctx context.Context, userID uint64, scenarioID string) error {
	declares := `
		DECLARE $scenario_id AS String;
		DECLARE $huid AS Uint64;
	`

	query := `
		DELETE FROM
			ScenarioLaunches
		ON SELECT
			huid, id
		FROM
			ScenarioLaunches VIEW scenario_launches_huid_scenario_id_status
		WHERE
			huid == $huid AND
			scenario_id == $scenario_id AND
			status == "SCHEDULED";

		UPDATE
			Scenarios
		SET
			archived = true
		WHERE
			huid == $huid AND
			id == $scenario_id;
	`

	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
		table.ValueParam("$scenario_id", ydb.StringValue([]byte(scenarioID))),
	)

	err := db.CallInTx(ctx, SerializableReadWrite, func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
		criteria := ScenarioQueryCriteria{UserID: userID}
		tx, scenarios, err := db.getUserScenariosTx(ctx, s, txControl, criteria)
		if err != nil {
			return tx, err
		}

		scenariosMap := scenarios.ToMap()
		oldScenario, ok := scenariosMap[scenarioID]
		if !ok {
			return tx, nil // it's ok to remove unknown scenarios
		}

		var deleteOldTriggersIndexDeclares string
		var deleteOldTriggersIndexQuery string
		if devicePropertyTriggers := oldScenario.Triggers.GetDevicePropertyTriggers(); len(devicePropertyTriggers) > 0 {
			indexQueryDeclares, indexQuery, param := makeDeviceTriggersIndexRemoveQuery(userID, scenarioID, devicePropertyTriggers)
			deleteOldTriggersIndexDeclares = indexQueryDeclares
			deleteOldTriggersIndexQuery = indexQuery
			params.Add(param)
		}

		query := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			%s
		`, db.Prefix, strings.Join([]string{deleteOldTriggersIndexDeclares, declares, deleteOldTriggersIndexQuery,
			query}, "\n"))

		stmt, err := s.Prepare(ctx, query)
		if err != nil {
			return tx, err
		}
		_, err = tx.ExecuteStatement(ctx, stmt, params)
		if err != nil {
			return tx, err
		}

		return tx, nil
	})

	if err != nil {
		return xerrors.Errorf("database request has failed: %w", err)
	}
	return nil
}

func (db *DBClient) CreateScenario(ctx context.Context, userID uint64, scenario model.Scenario) (scenarioID string, err error) {
	scenarioID = scenario.ID
	if scenarioID == "" {
		scenarioID = model.GenerateScenarioID()
	}

	scenario.Name = model.ScenarioName(tools.StandardizeSpaces(string(scenario.Name)))
	if scenario.Name == "" {
		return "", &model.InvalidValueError{}
	}

	devicesB, err := json.Marshal(scenario.Devices)
	if err != nil {
		return "", xerrors.Errorf("cannot marshal scenario.devices: %w", err)
	}

	requestedSpeakerCapabilitiesB, err := json.Marshal(scenario.RequestedSpeakerCapabilities)
	if err != nil {
		return "", xerrors.Errorf("cannot marshal scenario.requested_speaker_capabilities: %w", err)
	}

	stepsB, err := json.Marshal(scenario.Steps)
	if err != nil {
		return "", xerrors.Errorf("cannot marshal scenario.steps: %w", err)
	}

	scenario.Triggers.Normalize()
	triggers, err := json.Marshal(scenario.Triggers)
	if err != nil {
		return "", xerrors.Errorf("cannot marshal scenario.triggers: %w", err)
	}

	effectiveTimeParam := ydb.NullValue(ydb.TypeJSON)
	if scenario.EffectiveTime != nil {
		effectiveTimeB, err := json.Marshal(scenario.EffectiveTime)
		if err != nil {
			return "", xerrors.Errorf("cannot marshal scenario.effective_time: %w", err)
		}

		effectiveTimeParam = ydb.OptionalValue(ydb.JSONValue(string(effectiveTimeB)))
	}

	declares := `
		DECLARE $scenario_id AS String;
		DECLARE $huid AS Uint64;
		DECLARE $user_id AS Uint64;
		DECLARE $name AS String;
		DECLARE $icon AS String;
		DECLARE $triggers AS JSON;
		DECLARE $devices AS Json;
		DECLARE $requested_speaker_capabilities AS JSON;
		DECLARE $steps AS JSON;
		DECLARE $timestamp AS Timestamp;
		DECLARE $is_active AS Bool;
		DECLARE $effective_time AS Optional<JSON>;
		DECLARE $push_on_invoke AS Bool;
	`

	query := `
		UPSERT INTO
			Scenarios (id, huid, user_id, name, icon, triggers, devices, requested_speaker_capabilities, steps, created, archived, is_active, effective_time, push_on_invoke)
		VALUES
			($scenario_id, $huid, $user_id, $name, $icon, $triggers, $devices, $requested_speaker_capabilities, $steps, $timestamp, false, $is_active, $effective_time, $push_on_invoke);
	`

	params := table.NewQueryParameters(
		table.ValueParam("$scenario_id", ydb.StringValue([]byte(scenarioID))),
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
		table.ValueParam("$user_id", ydb.Uint64Value(userID)),
		table.ValueParam("$name", ydb.StringValue([]byte(scenario.Name))),
		table.ValueParam("$icon", ydb.StringValue([]byte(scenario.Icon))),
		table.ValueParam("$triggers", ydb.JSONValue(string(triggers))),
		table.ValueParam("$devices", ydb.JSONValue(string(devicesB))),
		table.ValueParam("$requested_speaker_capabilities", ydb.JSONValue(string(requestedSpeakerCapabilitiesB))),
		table.ValueParam("$steps", ydb.JSONValue(string(stepsB))),
		table.ValueParam("$timestamp", ydb.TimestampValue(db.CurrentTimestamp().YdbTimestamp())),
		table.ValueParam("$is_active", ydb.BoolValue(scenario.IsActive)),
		table.ValueParam("$effective_time", effectiveTimeParam),
		table.ValueParam("$push_on_invoke", ydb.BoolValue(scenario.PushOnInvoke)),
	)
	if deviceTriggers := scenario.Triggers.GetDevicePropertyTriggers(); len(deviceTriggers) > 0 {
		indexQueryDeclares, indexQuery, param, err := makeDeviceTriggersIndexUpsertQuery(userID, scenarioID, deviceTriggers)
		if err != nil {
			return "", err
		}
		declares += indexQueryDeclares
		query += indexQuery
		params.Add(param)
	}

	insertQuery := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");
		%s
		%s
	`, db.Prefix, declares, query)

	err = db.CallInTx(ctx, SerializableReadWrite, func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
		criteria := ScenarioQueryCriteria{UserID: userID}
		tx, scenarios, err := db.getUserScenariosTx(ctx, s, txControl, criteria)
		if err != nil {
			return tx, err
		}

		err = scenarios.ValidateNewScenario(scenario)
		if err != nil {
			return tx, err
		}

		insertQueryStmt, err := s.Prepare(ctx, insertQuery)
		if err != nil {
			return tx, err
		}
		_, err = tx.ExecuteStatement(ctx, insertQueryStmt, params)
		if err != nil {
			return tx, err
		}

		return tx, nil
	})

	if err != nil {
		return "", xerrors.Errorf("database request has failed: %w", err)
	}

	return scenarioID, nil
}

//Deprecated
//Use UpdateScenarioAndCreateLaunch or UpdateScenarioAndDeleteLaunches instead.
//The former should be used in case when there are timetable triggers in Scenario.Triggers slice.
//The latter should be used in case when there are no timetable triggers in Scenario.Triggers slice.
func (db *DBClient) UpdateScenario(ctx context.Context, userID uint64, scenario model.Scenario) (err error) {
	return db.UpdateScenarios(ctx, userID, model.Scenarios{scenario})
}

func (db *DBClient) UpdateScenarios(ctx context.Context, userID uint64, scenarios model.Scenarios) error {
	// workaround about ydb can't serialize empty list
	if len(scenarios) == 0 {
		return nil
	}

	updateQuery := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");

			DECLARE $values AS List<Struct<
				id: String,
				huid: Uint64,
				name: String,
				icon: String,
				triggers: Json,
				devices: Json,
				requested_speaker_capabilities: Json,
				steps: Json,
				is_active: Bool,
				effective_time: Optional<JSON>,
				push_on_invoke: Bool
			>>;

			UPSERT INTO Scenarios
			SELECT * FROM AS_TABLE($values)
`, db.Prefix)
	huid := ydb.Uint64Value(tools.Huidify(userID))

	updateFunc := func(ctx context.Context, session *table.Session, tc *table.TransactionControl) (tx *table.Transaction, err error) {
		criteria := ScenarioQueryCriteria{UserID: userID}
		tx, userScenarios, err := db.getUserScenariosTx(ctx, session, tc, criteria)
		if err != nil {
			return tx, xerrors.Errorf("failed to select existing sceraios")
		}
		userScenariosIDs := userScenarios.GetIDs()

		listValues := []ydb.Value{}
		for _, scenario := range scenarios {
			//check scenario already exist
			scenario.Name = model.ScenarioName(tools.StandardizeSpaces(string(scenario.Name)))
			if scenario.Name == "" {
				return tx, &model.InvalidValueError{}
			}

			if !slices.Contains(userScenariosIDs, scenario.ID) {
				return tx, &model.ScenarioNotFoundError{}
			}

			err = userScenarios.ExcludeScenario(scenario.ID).ValidateNewScenario(scenario)
			if err != nil {
				return tx, err
			}

			//devices
			devicesB, err := json.Marshal(scenario.Devices)
			if err != nil {
				return tx, xerrors.Errorf("cannot marshal scenario.devices: %w", err)
			}

			scenario.Triggers.Normalize()
			triggers, err := json.Marshal(scenario.Triggers)
			if err != nil {
				return tx, xerrors.Errorf("cannot marshal scenario.triggers: %w", err)
			}

			// requested speaker capabilities
			requestedSpeakerCapabilitiesB, err := json.Marshal(scenario.RequestedSpeakerCapabilities)
			if err != nil {
				return tx, xerrors.Errorf("cannot marshal scenario.requested_speaker_capabilities: %w", err)
			}

			//steps
			stepsB, err := json.Marshal(scenario.Steps)
			if err != nil {
				return tx, xerrors.Errorf("cannot marshal scenario.steps: %w", err)
			}

			effectiveTimeParam := ydb.NullValue(ydb.TypeJSON)
			if scenario.EffectiveTime != nil {
				effectiveTimeB, err := json.Marshal(scenario.EffectiveTime)
				if err != nil {
					return tx, xerrors.Errorf("cannot marshal scenario.effective_time: %w", err)
				}

				effectiveTimeParam = ydb.OptionalValue(ydb.JSONValue(string(effectiveTimeB)))
			}

			ydbValue := ydb.StructValue(
				ydb.StructFieldValue("id", ydb.StringValue([]byte(scenario.ID))),
				ydb.StructFieldValue("huid", huid),
				ydb.StructFieldValue("name", ydb.StringValue([]byte(scenario.Name))),
				ydb.StructFieldValue("icon", ydb.StringValue([]byte(scenario.Icon))),
				ydb.StructFieldValue("triggers", ydb.JSONValue(string(triggers))),
				ydb.StructFieldValue("devices", ydb.JSONValue(string(devicesB))),
				ydb.StructFieldValue("requested_speaker_capabilities", ydb.JSONValue(string(requestedSpeakerCapabilitiesB))),
				ydb.StructFieldValue("steps", ydb.JSONValue(string(stepsB))),
				ydb.StructFieldValue("is_active", ydb.BoolValue(scenario.IsActive)),
				ydb.StructFieldValue("effective_time", effectiveTimeParam),
				ydb.StructFieldValue("push_on_invoke", ydb.BoolValue(scenario.PushOnInvoke)),
			)
			listValues = append(listValues, ydbValue)
		}

		updateQueryStmt, err := session.Prepare(ctx, updateQuery)
		if err != nil {
			return tx, err
		}
		params := table.NewQueryParameters(
			table.ValueParam("$values", ydb.ListValue(listValues...)),
		)
		_, err = tx.ExecuteStatement(ctx, updateQueryStmt, params)
		if err != nil {
			return tx, xerrors.Errorf("failed to execute scenarios update: %w", err)
		}
		return tx, nil
	}

	if err := db.CallInTx(ctx, SerializableReadWrite, updateFunc); err != nil {
		return xerrors.Errorf("failed to save scenarios: %w", err)
	}

	return nil
}

func (db *DBClient) selectUserScenarios(ctx context.Context, criteria ScenarioQueryCriteria) ([]model.Scenario, error) {
	var scenarios []model.Scenario
	selectFunc := func(ctx context.Context, session *table.Session, tc *table.TransactionControl) (tx *table.Transaction, err error) {
		tx, scenarios, err = db.getUserScenariosTx(ctx, session, tc, criteria)
		if err != nil {
			return tx, err
		}
		return tx, nil
	}

	err := db.CallInTx(ctx, OnlineReadOnly, selectFunc)
	if err != nil {
		return scenarios, xerrors.Errorf("database request has failed: %w", err)
	}

	return scenarios, nil
}

func (db *DBClient) getUserScenariosTx(ctx context.Context, s *table.Session, tc *table.TransactionControl, criteria ScenarioQueryCriteria) (tx *table.Transaction, scenarios model.Scenarios, err error) {
	var queryB bytes.Buffer
	if err := SelectUserScenariosTemplate.Execute(&queryB, criteria); err != nil {
		return nil, scenarios, err
	}

	query := fmt.Sprintf(queryB.String(), db.Prefix)
	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(criteria.UserID))),
		table.ValueParam("$scenario_id", ydb.StringValue([]byte(criteria.ScenarioID))),
		table.ValueParam("$is_active", ydb.BoolValue(criteria.IsActive)),
	)

	stmt, err := s.Prepare(ctx, query)
	if err != nil {
		return nil, scenarios, err
	}

	tx, res, err := stmt.Execute(ctx, tc, params)
	defer func() {
		if err != nil {
			db.RollbackTransaction(ctx, tx, err)
		}
	}()
	if err != nil {
		return nil, scenarios, err
	}

	for res.NextSet() {
		for res.NextRow() {
			var scenario model.Scenario
			res.SeekItem("id")
			scenario.ID = string(res.OString())
			res.NextItem()
			scenario.Name = model.ScenarioName(res.OString())
			res.NextItem()
			scenario.Icon = model.ScenarioIcon(res.OString())

			//triggers
			res.NextItem()
			triggers := make(model.ScenarioTriggers, 0)
			triggersRaw := res.OJSON()
			if len(triggersRaw) > 0 {
				triggers, err = model.JSONUnmarshalTriggers([]byte(triggersRaw))
				if err != nil {
					return nil, scenarios, xerrors.Errorf("failed to parse `triggers` field for scenario %s: %w", scenario.ID, err)
				}
				sort.Sort(triggers)
			}
			scenario.Triggers = triggers

			//devices
			res.NextItem()
			var devices []model.ScenarioDevice
			devicesRaw := res.OJSON()
			if len(devicesRaw) > 0 {
				if err := json.Unmarshal([]byte(devicesRaw), &devices); err != nil {
					return nil, scenarios, xerrors.Errorf("failed to parse `devices` field for scenario %s: %w", scenario.ID, err)
				}
			}
			scenario.Devices = devices

			// requested speaker capabilities
			res.NextItem()
			var rsCapabilities []model.ScenarioCapability
			rsCapabilitiesRaw := res.OJSON()
			if len(rsCapabilitiesRaw) > 0 {
				if err := json.Unmarshal([]byte(rsCapabilitiesRaw), &rsCapabilities); err != nil {
					return nil, scenarios, xerrors.Errorf("failed to parse `requested_speaker_capabilities` field for scenario %s: %w", scenario.ID, err)
				}
			}
			scenario.RequestedSpeakerCapabilities = rsCapabilities

			// steps
			res.NextItem()
			steps := make(model.ScenarioSteps, 0)
			stepsRaw := res.OJSON()
			if len(stepsRaw) > 0 {
				if steps, err = model.JSONUnmarshalScenarioSteps([]byte(stepsRaw)); err != nil {
					return nil, scenarios, xerrors.Errorf("failed to parse `steps` field for scenario %s: %w", scenario.ID, err)
				}
			}
			scenario.Steps = steps

			// is_active
			res.NextItem()
			if res.IsNull() {
				scenario.IsActive = true
			} else {
				scenario.IsActive = res.OBool()
			}

			// effective time
			res.NextItem()
			var effectiveTime *model.EffectiveTime
			if !res.IsNull() {
				if err := json.Unmarshal([]byte(res.OJSON()), &effectiveTime); err != nil {
					return nil, scenarios, xerrors.Errorf("failed to parse `effective_time` field for scenario %s: %w", scenario.ID, err)
				}
			}

			// push_on_invoke
			res.NextItem()
			if res.IsNull() {
				scenario.PushOnInvoke = false
			} else {
				scenario.PushOnInvoke = res.OBool()
			}

			// favorite
			res.NextItem()
			inFavoritesID := string(res.OString())
			scenario.Favorite = len(inFavoritesID) > 0

			if res.Err() != nil {
				err := xerrors.Errorf("failed to parse YDB response row: %w", res.Err())
				ctxlog.Error(ctx, db.Logger, err.Error())
				return nil, scenarios, err
			}

			scenarios = append(scenarios, scenario)
		}
	}

	return tx, scenarios, nil
}

func (db *DBClient) selectUserScenariosSimple(ctx context.Context, criteria ScenarioQueryCriteria) ([]model.Scenario, error) {
	var scenarios []model.Scenario
	selectFunc := func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (tx *table.Transaction, err error) {
		tx, scenarios, err = db.getUserScenariosSimpleTx(ctx, s, txControl, criteria)
		if err != nil {
			return tx, err
		}
		return tx, nil
	}

	err := db.CallInTx(ctx, OnlineReadOnly, selectFunc)
	if err != nil {
		return scenarios, xerrors.Errorf("database request has failed: %w", err)
	}

	return scenarios, nil
}

func (db *DBClient) getUserScenariosSimpleTx(ctx context.Context, s *table.Session, tc *table.TransactionControl, criteria ScenarioQueryCriteria) (tx *table.Transaction, scenarios model.Scenarios, err error) {
	var queryB bytes.Buffer
	if err := SelectUserScenariosSimpleTemplate.Execute(&queryB, criteria); err != nil {
		return nil, scenarios, err
	}

	query := fmt.Sprintf(queryB.String(), db.Prefix)
	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(criteria.UserID))),
		table.ValueParam("$scenario_id", ydb.StringValue([]byte(criteria.ScenarioID))),
		table.ValueParam("$is_active", ydb.BoolValue(criteria.IsActive)),
	)

	stmt, err := s.Prepare(ctx, query)
	if err != nil {
		return nil, scenarios, err
	}

	tx, res, err := stmt.Execute(ctx, tc, params)
	defer func() {
		if err != nil {
			db.RollbackTransaction(ctx, tx, err)
		}
	}()
	if err != nil {
		return nil, scenarios, err
	}

	for res.NextSet() {
		for res.NextRow() {
			var scenario model.Scenario
			res.SeekItem("id")
			scenario.ID = string(res.OString())
			res.NextItem()
			scenario.Name = model.ScenarioName(res.OString())
			res.NextItem()
			scenario.Icon = model.ScenarioIcon(res.OString())

			//triggers
			res.NextItem()
			triggers := make(model.ScenarioTriggers, 0)
			triggersRaw := res.OJSON()
			if len(triggersRaw) > 0 {
				triggers, err = model.JSONUnmarshalTriggers([]byte(triggersRaw))
				if err != nil {
					return nil, scenarios, xerrors.Errorf("failed to parse `triggers` field for scenario %s: %w", scenario.ID, err)
				}
				sort.Sort(triggers)
			}
			scenario.Triggers = triggers

			//devices
			res.NextItem()
			var devices []model.ScenarioDevice
			devicesRaw := res.OJSON()
			if len(devicesRaw) > 0 {
				if err := json.Unmarshal([]byte(devicesRaw), &devices); err != nil {
					return nil, scenarios, xerrors.Errorf("failed to parse `devices` field for scenario %s: %w", scenario.ID, err)
				}
			}
			scenario.Devices = devices

			// requested speaker capabilities
			res.NextItem()
			var rsCapabilities []model.ScenarioCapability
			rsCapabilitiesRaw := res.OJSON()
			if len(rsCapabilitiesRaw) > 0 {
				if err := json.Unmarshal([]byte(rsCapabilitiesRaw), &rsCapabilities); err != nil {
					return nil, scenarios, xerrors.Errorf("failed to parse `requested_speaker_capabilities` field for scenario %s: %w", scenario.ID, err)
				}
			}
			scenario.RequestedSpeakerCapabilities = rsCapabilities

			// steps
			res.NextItem()
			steps := make(model.ScenarioSteps, 0)
			stepsRaw := res.OJSON()
			if len(stepsRaw) > 0 {
				if steps, err = model.JSONUnmarshalScenarioSteps([]byte(stepsRaw)); err != nil {
					return nil, scenarios, xerrors.Errorf("failed to parse `steps` field for scenario %s: %w", scenario.ID, err)
				}
			}
			scenario.Steps = steps

			// is_active
			res.NextItem()
			if res.IsNull() {
				scenario.IsActive = true
			} else {
				scenario.IsActive = res.OBool()
			}

			// effective time
			res.NextItem()
			var effectiveTime *model.EffectiveTime
			if !res.IsNull() {
				if err := json.Unmarshal([]byte(res.OJSON()), &effectiveTime); err != nil {
					return nil, scenarios, xerrors.Errorf("failed to parse `effective_time` field for scenario %s: %w", scenario.ID, err)
				}
			}

			// push_on_invoke
			res.NextItem()
			if res.IsNull() {
				scenario.PushOnInvoke = false
			} else {
				scenario.PushOnInvoke = res.OBool()
			}

			if res.Err() != nil {
				err := xerrors.Errorf("failed to parse YDB response row: %w", res.Err())
				ctxlog.Error(ctx, db.Logger, err.Error())
				return nil, scenarios, err
			}

			scenarios = append(scenarios, scenario)
		}
	}

	return tx, scenarios, nil
}

func (db *DBClient) parseScenarioLaunch(ctx context.Context, res *table.Result) (model.ScenarioLaunch, error) {
	var sl dao.ScenarioLaunch
	err := sl.YDBParseResult(res)
	return model.ScenarioLaunch(sl.ScenarioLaunchModel), err
}

func (db *DBClient) SelectDeviceTriggersIndexes(ctx context.Context, userID uint64, deviceTriggerIndexKey model.DeviceTriggerIndexKey) (model.DeviceTriggersIndexes, error) {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");

		DECLARE $huid AS Uint64;
		DECLARE $device_id AS String;
		DECLARE $device_trigger_entity AS String;
		DECLARE $type AS String;
		DECLARE $instance AS String;

		SELECT
			user_id,
			device_id,
			device_trigger_entity,
			type,
			instance,
			scenario_id,
			trigger
		FROM DeviceTriggersIndex
		WHERE
			huid == $huid AND
			device_id == $device_id AND
			device_trigger_entity == $device_trigger_entity AND
			type == $type AND
			instance == $instance`, db.Prefix)

	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
		table.ValueParam("$device_id", ydb.StringValue([]byte(deviceTriggerIndexKey.DeviceID))),
		table.ValueParam("$device_trigger_entity", ydb.StringValue([]byte(deviceTriggerIndexKey.TriggerEntity))),
		table.ValueParam("$type", ydb.StringValue([]byte(deviceTriggerIndexKey.Type))),
		table.ValueParam("$instance", ydb.StringValue([]byte(deviceTriggerIndexKey.Instance))),
	)

	res, err := db.Read(ctx, query, params)
	if err != nil {
		return nil, err
	}

	var result model.DeviceTriggersIndexes

	for res.NextSet() {
		for res.NextRow() {
			var entry model.DeviceTriggersIndex

			res.NextItem()
			entry.UserID = res.OUint64()

			res.NextItem()
			entry.DeviceID = string(res.OString())

			res.NextItem()
			entry.TriggerEntity = model.DeviceTriggerEntity(res.OString())

			res.NextItem()
			entry.Type = string(res.OString())

			res.NextItem()
			entry.Instance = string(res.OString())

			res.NextItem()
			entry.ScenarioID = string(res.OString())

			res.NextItem()
			trigger, err := model.JSONUnmarshalTrigger([]byte(res.OJSON()))
			if err != nil {
				return nil, err
			}

			devicePropertyTrigger, ok := trigger.(model.DevicePropertyScenarioTrigger)
			if !ok {
				return nil, xerrors.Errorf("failed to convert trigger to DevicePropertyScenarioTrigger")
			}
			entry.Trigger = devicePropertyTrigger

			if res.Err() != nil {
				err := xerrors.Errorf("failed to parse YDB response row: %w", res.Err())
				ctxlog.Error(ctx, db.Logger, err.Error())
				return nil, err
			}

			result = append(result, entry)
		}
	}
	return result, nil
}

func (db *DBClient) StoreDeviceTriggersIndexes(ctx context.Context, userID uint64, scenarioTriggers map[string]model.DevicePropertyScenarioTriggers) error {
	if !db.HasTransaction(ctx) {
		return xerrors.Errorf("update triggers must be in one transaction with read them")
	}

	upsertFunc := func(ctx context.Context, session *table.Session, tc *table.TransactionControl) (tx *table.Transaction, err error) {
		for scenarioID, triggers := range scenarioTriggers {
			if len(triggers) == 0 {
				continue
			}

			var declares, query string
			declares, query, param, err := makeDeviceTriggersIndexUpsertQuery(userID, scenarioID, triggers)
			if err != nil {
				return tx, xerrors.Errorf("make store query for scenarioID %q: %w", scenarioID, err)
			}
			upsertQuery := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");
		%s
		%s
	`, db.Prefix, declares, query)
			txRes, _, err := session.Execute(ctx, tc, upsertQuery, table.NewQueryParameters(param), table.WithQueryCachePolicy(table.WithQueryCachePolicyKeepInCache()))
			if txRes != nil {
				tx = txRes
			}
			if err != nil {
				return tx, xerrors.Errorf("store triggers for scenarioID %q: %w", scenarioID, err)
			}

			// prepare tc for next iteration
			tc = table.TxControl(table.WithTx(tx))
		}
		return tx, nil
	}
	return db.CallInTx(ctx, SerializableReadWrite, upsertFunc)
}

func makeDeviceTriggersIndexUpsertQuery(userID uint64, scenarioID string, triggers []model.DevicePropertyScenarioTrigger) (string, string, table.ParameterOption, error) {
	declares := `
		DECLARE $triggers_index AS List<Struct<
			huid: Uint64,
			user_id: Uint64,
			device_id: String,
			device_trigger_entity: String,
			type: String,
			instance: String,
			scenario_id: String,
			trigger: JSON
		>>;
	`

	query := `
		UPSERT INTO
			DeviceTriggersIndex (huid, user_id, device_id, device_trigger_entity, type, instance, scenario_id, trigger)
		SELECT
			huid, user_id, device_id, device_trigger_entity, type, instance, scenario_id, trigger
		FROM
			AS_TABLE($triggers_index);
	`

	triggerValues := make([]ydb.Value, 0, len(triggers))

	for _, trigger := range triggers {
		triggerD, err := json.Marshal(trigger)
		if err != nil {
			err := xerrors.Errorf("cannot marshal scenario.trigger: %w", err)
			return "", "", nil, err
		}

		triggerValues = append(triggerValues, ydb.StructValue(
			ydb.StructFieldValue("huid", ydb.Uint64Value(tools.Huidify(userID))),
			ydb.StructFieldValue("user_id", ydb.Uint64Value(userID)),
			ydb.StructFieldValue("device_id", ydb.StringValue([]byte(trigger.DeviceID))),
			ydb.StructFieldValue("device_trigger_entity", ydb.StringValue([]byte("property"))),
			ydb.StructFieldValue("type", ydb.StringValue([]byte(trigger.PropertyType))),
			ydb.StructFieldValue("instance", ydb.StringValue([]byte(trigger.Instance))),
			ydb.StructFieldValue("scenario_id", ydb.StringValue([]byte(scenarioID))),
			ydb.StructFieldValue("trigger", ydb.JSONValue(string(triggerD))),
		))
	}

	return declares, query, table.ValueParam("$triggers_index", ydb.ListValue(triggerValues...)), nil
}

func makeDeviceTriggersIndexRemoveQuery(userID uint64, scenarioID string, triggers []model.DevicePropertyScenarioTrigger) (string, string, table.ParameterOption) {
	declares := `
		DECLARE $triggers_index_to_remove AS List<Struct<
			huid: Uint64,
			device_id: String,
			device_trigger_entity: String,
			type: String,
			instance: String,
			scenario_id: String
		>>;
	`
	query := `
		DELETE FROM
			DeviceTriggersIndex
		ON SELECT
			huid, device_id, device_trigger_entity, type, instance, scenario_id
		FROM
			AS_TABLE($triggers_index_to_remove);
	`

	triggerValues := make([]ydb.Value, 0, len(triggers))
	for _, trigger := range triggers {
		triggerValues = append(triggerValues, ydb.StructValue(
			ydb.StructFieldValue("huid", ydb.Uint64Value(tools.Huidify(userID))),
			ydb.StructFieldValue("device_id", ydb.StringValue([]byte(trigger.DeviceID))),
			ydb.StructFieldValue("device_trigger_entity", ydb.StringValue([]byte("property"))),
			ydb.StructFieldValue("type", ydb.StringValue([]byte(trigger.PropertyType))),
			ydb.StructFieldValue("instance", ydb.StringValue([]byte(trigger.Instance))),
			ydb.StructFieldValue("scenario_id", ydb.StringValue([]byte(scenarioID))),
		))
	}

	return declares, query, table.ValueParam("$triggers_index_to_remove", ydb.ListValue(triggerValues...))
}
