package db

import (
	"context"
	"fmt"

	"a.yandex-team.ru/alice/iot/vulpix/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (db *Client) SelectDeviceState(ctx context.Context, userID uint64, speakerID string) (model.DeviceState, error) {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");
		DECLARE $huid AS Uint64;
		DECLARE $user_id AS Uint64;
		DECLARE $id AS String;

		SELECT
			state,
			updated
		FROM
			DeviceStates
		WHERE
			huid == $huid AND
			user_id == $user_id AND
			id == $id;`, db.Prefix)

	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
		table.ValueParam("$user_id", ydb.Uint64Value(userID)),
		table.ValueParam("$id", ydb.StringValue([]byte(speakerID))),
	)

	res, err := db.Read(ctx, query, params)
	if err != nil {
		return model.DeviceState{}, err
	}
	if !res.NextSet() {
		return model.DeviceState{}, &model.ErrDeviceStateNotFound{}
	}
	if !res.NextRow() {
		return model.DeviceState{}, &model.ErrDeviceStateNotFound{}
	}
	var result model.DeviceState
	res.NextItem()
	result.Type = model.DeviceStateType(res.OString())
	res.NextItem()
	result.Updated = timestamp.FromMicro(res.OTimestamp())
	if res.Err() != nil {
		err := xerrors.Errorf("failed to parse YDB response row: %w", res.Err())
		ctxlog.Error(ctx, db.Logger, err.Error())
		return model.DeviceState{}, err
	}
	return result, nil
}

func (db *Client) StoreDeviceState(ctx context.Context, userID uint64, speakerID string, state model.DeviceState) error {
	query := fmt.Sprintf(`
				--!syntax_v1
				PRAGMA TablePathPrefix("%s");
				DECLARE $huid AS Uint64;
				DECLARE $user_id AS Uint64;
				DECLARE $id AS String;
				DECLARE $state AS String;
				DECLARE $updated AS Timestamp;
				UPSERT INTO
					DeviceStates (huid, user_id, id, state, updated)
				VALUES
					($huid, $user_id, $id, $state, $updated)`, db.Prefix)
	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
		table.ValueParam("$user_id", ydb.Uint64Value(userID)),
		table.ValueParam("$id", ydb.StringValue([]byte(speakerID))),
		table.ValueParam("$state", ydb.StringValue([]byte(state.Type))),
		table.ValueParam("$updated", ydb.TimestampValue(state.Updated.YdbTimestamp())),
	)
	if err := db.Write(ctx, query, params); err != nil {
		return err
	}
	ctxlog.Infof(ctx, db.Logger, "Stored device %s state type %s for user %d", speakerID, state.Type, userID)
	return nil
}
