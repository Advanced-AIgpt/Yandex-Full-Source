package db

import (
	"context"
	"encoding/json"
	"fmt"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/alice/library/go/ydbclient"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (db *Client) DevicePropertyHistory(ctx context.Context, userID uint64, deviceID string, propertyType model.PropertyType, instance model.PropertyInstance) ([]model.PropertyLogData, error) {
	query := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");

			DECLARE $huid AS Uint64;
			DECLARE $device_id AS String;
			DECLARE $ts_threshold AS Timestamp;
			DECLARE $entity_type AS String;
			DECLARE $type AS String;
			DECLARE $instance AS String;

			SELECT
				ts, source, state, parameters
			FROM
				DeviceStates
			WHERE
				huid = $huid AND
				device_id = $device_id AND
				entity_type = $entity_type AND
				type = $type AND
				instance = $instance
			ORDER BY
				ts DESC
			LIMIT
				1000;`, db.Prefix)

	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
		table.ValueParam("$device_id", ydb.StringValue([]byte(deviceID))),
		table.ValueParam("$entity_type", ydb.StringValue([]byte(model.PropertyEntity))),
		table.ValueParam("$type", ydb.StringValue([]byte(propertyType))),
		table.ValueParam("$instance", ydb.StringValue([]byte(instance))),
	)

	res, err := db.Read(ctx, query, params)
	if err != nil {
		return nil, err
	}

	result := make([]model.PropertyLogData, 0, 1000)
	for res.NextSet() {
		for res.NextRow() {
			var data model.PropertyLogData

			res.NextItem()
			data.Timestamp = timestamp.FromMicro(res.OTimestamp())

			res.NextItem()
			data.Source = string(res.OString())

			res.NextItem()
			rawState := res.OJSON()

			res.NextItem()
			rawParameters := res.OJSON()

			switch propertyType {
			case model.FloatPropertyType:
				var state model.FloatPropertyState
				if err := json.Unmarshal([]byte(rawState), &state); err != nil {
					return nil, err
				}
				data.State = state

				var parameters model.FloatPropertyParameters
				if err := json.Unmarshal([]byte(rawParameters), &parameters); err != nil {
					return nil, err
				}
				data.Parameters = parameters
			case model.EventPropertyType:
				var state model.EventPropertyState
				if err := json.Unmarshal([]byte(rawState), &state); err != nil {
					return nil, err
				}
				data.State = state

				var parameters model.EventPropertyParameters
				if err := json.Unmarshal([]byte(rawParameters), &parameters); err != nil {
					return nil, err
				}
				parameters.FillByWellKnownEvents()
				data.Parameters = parameters
			default:
				return nil, xerrors.Errorf("db: unknown property type: %s", propertyType)
			}

			result = append(result, data)
		}
	}

	return result, nil
}

func (db *Client) StoreDeviceProperties(ctx context.Context, userID uint64, deviceProperties model.DevicePropertiesMap, source model.RequestSource) error {
	if len(deviceProperties) == 0 {
		return nil
	}
	updateFunc := func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
		var values []ydb.Value
		for deviceID, properties := range deviceProperties {
			for _, property := range properties {
				state := property.State()
				if state == nil {
					continue
				}

				parameters := property.Parameters()
				if parameters == nil {
					continue
				}

				stateB, err := json.Marshal(state)
				if err != nil {
					return nil, err
				}

				parametersB, err := json.Marshal(parameters)
				if err != nil {
					return nil, err
				}

				value := ydb.StructValue(
					ydb.StructFieldValue("device_id", ydb.StringValue([]byte(deviceID))),
					ydb.StructFieldValue("ts", ydb.TimestampValue(property.LastUpdated().YdbTimestamp())),
					ydb.StructFieldValue("type", ydb.StringValue([]byte(property.Type()))),
					ydb.StructFieldValue("instance", ydb.StringValue([]byte(property.Instance()))),
					ydb.StructFieldValue("state", ydb.JSONValue(string(stateB))),
					ydb.StructFieldValue("parameters", ydb.JSONValue(string(parametersB))),
				)
				values = append(values, value)
			}
		}

		if len(values) == 0 {
			return nil, nil
		}

		query := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");

			DECLARE $huid AS Uint64;
			DECLARE $entity_type AS String;
			DECLARE $user_id AS Uint64;
			DECLARE $source AS String;
			DECLARE $values AS List<Struct<
				device_id: String,
				ts: Timestamp,
				type: String,
				instance: String,
				state: Json,
				parameters: Json
			>>;

			UPSERT INTO
				DeviceStates (huid, device_id, ts, entity_type, type, instance, state, parameters, user_id, source)
			SELECT
				$huid AS huid,
				device_id,
				ts,
				$entity_type AS entity_type,
				type,
				instance,
				state,
				parameters,
				$user_id AS user_id,
				$source AS source
			FROM
				AS_TABLE($values);`, db.Prefix)

		params := table.NewQueryParameters(
			table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
			table.ValueParam("$entity_type", ydb.StringValue([]byte(model.PropertyEntity))),
			table.ValueParam("$user_id", ydb.Uint64Value(userID)),
			table.ValueParam("$source", ydb.StringValue([]byte(source))),
			table.ValueParam("$values", ydb.ListValue(values...)),
		)

		stmt, err := s.Prepare(ctx, query)
		if err != nil {
			return nil, err
		}

		tx, _, err := stmt.Execute(ctx, txControl, params)
		if err != nil {
			return nil, err
		}

		return tx, nil
	}

	if err := db.CallInTx(ctx, ydbclient.SerializableReadWrite, updateFunc); err != nil {
		return xerrors.Errorf("database request has failed: %w", err)
	}

	return nil
}
