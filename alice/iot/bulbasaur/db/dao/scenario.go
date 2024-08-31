package dao

import (
	"encoding/json"
	"time"

	"a.yandex-team.ru/alice/library/go/ydbclient"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// ScenarioLaunchModel clear model type from methods
type ScenarioLaunchModel model.ScenarioLaunch
type ScenarioLaunch struct {
	Huid   uint64 // autocalc while store
	UserID uint64
	ScenarioLaunchModel
}

func (sl *ScenarioLaunch) GetHuid() uint64 {
	return sl.Huid
}

func (sl ScenarioLaunch) YDBFields() []string {
	return []string{
		"huid",
		"user_id",
		"id",
		"scenario_id",
		"scenario_name",
		"launch_trigger_id",
		"launch_trigger_type",
		"launch_trigger_value",
		"icon",
		"launch_data",
		"steps",
		"current_step_index",
		"created",
		"scheduled",
		"finished",
		"status",
		"error",
		"push_on_invoke",
	}
}

func (sl *ScenarioLaunch) YDBParseResult(res *table.Result) error {
	var created, scheduled, finished time.Time
	var launchTriggerValue, launchDataRaw, stepsRaw []byte
	err := res.ScanWithDefaults(&sl.Huid, &sl.UserID, &sl.ID, &sl.ScenarioID, wrap(&sl.ScenarioName), &sl.LaunchTriggerID, wrap(&sl.LaunchTriggerType), &launchTriggerValue,
		wrap(&sl.Icon), &launchDataRaw, &stepsRaw, wrap(&sl.CurrentStepIndex), &created, &scheduled, &finished, wrap(&sl.Status), &sl.ErrorCode, &sl.PushOnInvoke)

	if err != nil {
		return xerrors.Errorf("failed to scan scenario launch ydb row: %w", err)
	}
	sl.Created = timestamp.FromTime(created)
	sl.Scheduled = timestamp.FromTime(scheduled)

	if !finished.IsZero() {
		sl.Finished = timestamp.FromTime(finished)
	}

	if len(launchTriggerValue) != 0 {
		// context arg will remove
		value, err := model.JSONUnmarshalScenarioTriggerValue(launchTriggerValue)
		if err != nil {
			return xerrors.Errorf("failed to parse `launch_trigger_value` field for scenario launch %s: %w", sl.ID, err)
		}
		sl.LaunchTriggerValue = value
	}

	if len(launchDataRaw) > 0 {
		launchData, err := model.JSONUnmarshalScenarioStepActionsParameters(launchDataRaw)
		if err != nil {
			return xerrors.Errorf("failed to parse `launch_data` field for scenario launch %s: %w", sl.ID, err)
		}
		sl.LaunchData = launchData
	}

	sl.Steps = make(model.ScenarioSteps, 0)
	if len(stepsRaw) > 0 {
		steps, err := model.JSONUnmarshalScenarioSteps(stepsRaw)
		if err != nil {
			return xerrors.Errorf("failed to parse `steps` field for scenario launch %s: %w", sl.ID, err)
		}
		sl.Steps = steps
	}

	return nil
}

func (sl ScenarioLaunch) YDBDeclareStruct() string {
	return `Struct<
			huid: Uint64,
			user_id: Uint64,
			id: String,
			scenario_id: String,
			scenario_name: String,
			launch_trigger_id: String,
			launch_trigger_type: String,
			launch_trigger_value: Optional<JSON>,
			icon: String,
			steps: JSON,
			current_step_index: Int64,
			created: Timestamp,
			scheduled: Timestamp,
			finished: Optional<Timestamp>,
			status: String,
			error: String,
			push_on_invoke: Bool,
>`
}

func (sl ScenarioLaunch) YDBStruct() (ydb.Value, error) {
	sl.Huid = tools.Huidify(sl.UserID)
	triggerValues := ydb.NullValue(ydb.TypeJSON)
	if sl.LaunchTriggerValue != nil {
		triggerValuesB, err := json.Marshal(sl.LaunchTriggerValue)
		if err != nil {
			return nil, xerrors.Errorf("cannot marshal scenarioLaunches.LaunchTriggerValue: %w", err)
		}
		triggerValues = ydb.OptionalValue(ydb.JSONValue(string(triggerValuesB)))
	}

	stepsB, err := json.Marshal(sl.Steps)
	if err != nil {
		return nil, xerrors.Errorf("cannot marshal scenarioLaunches.Steps: %w", err)
	}
	return ydb.StructValue(
		ydb.StructFieldValue("huid", ydb.Uint64Value(sl.Huid)),
		ydb.StructFieldValue("user_id", ydb.Uint64Value(sl.UserID)),
		ydb.StructFieldValue("id", ydb.StringValueFromString(sl.ID)),
		ydb.StructFieldValue("scenario_id", ydb.StringValueFromString(sl.ScenarioID)),
		ydb.StructFieldValue("scenario_name", ydb.StringValueFromString(string(sl.ScenarioName))),
		ydb.StructFieldValue("launch_trigger_id", ydb.StringValueFromString(sl.LaunchTriggerID)),
		ydb.StructFieldValue("launch_trigger_type", ydb.StringValueFromString(string(sl.LaunchTriggerType))),
		ydb.StructFieldValue("launch_trigger_value", triggerValues),
		ydb.StructFieldValue("icon", ydb.StringValueFromString(string(sl.Icon))),
		// ydb.StructFieldValue("launch_data", ydb.JSONValueFromBytes(launchData)), // deprecated
		ydb.StructFieldValue("steps", ydb.JSONValue(string(stepsB))),
		ydb.StructFieldValue("current_step_index", ydb.Int64Value(int64(sl.CurrentStepIndex))),
		ydb.StructFieldValue("created", ydb.TimestampValue(sl.Created.YdbTimestamp())),
		ydb.StructFieldValue("scheduled", ydb.TimestampValue(sl.Scheduled.YdbTimestamp())),
		ydb.StructFieldValue("finished", optionalYdbTimestampValue(sl.Finished)),
		ydb.StructFieldValue("status", ydb.StringValueFromString(string(sl.Status))),
		ydb.StructFieldValue("error", ydb.StringValueFromString(sl.ErrorCode)),
		ydb.StructFieldValue("push_on_invoke", ydb.BoolValue(sl.PushOnInvoke)),
	), nil
}

var (
	_ ydbclient.YDBRow = &ScenarioLaunch{}
)

func optionalYdbTimestampValue(timestamp timestamp.PastTimestamp) ydb.Value {
	if timestamp > 0 {
		return ydb.OptionalValue(ydb.TimestampValue(timestamp.YdbTimestamp()))
	} else {
		return ydb.NullValue(ydb.TypeTimestamp)
	}
}
