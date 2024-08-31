package db

import (
	"context"
	"fmt"

	bmodel "a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/vulpix/model"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (db *Client) SelectConnectingDeviceType(ctx context.Context, userID uint64, speakerID string) (bmodel.DeviceType, error) {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");
		DECLARE $huid AS Uint64;
		DECLARE $user_id AS Uint64;
		DECLARE $id AS String;

		SELECT
			device_type
		FROM
			ConnectingDeviceTypes
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
		return "", err
	}
	if !res.NextSet() || !res.NextRow() {
		return "", &model.ErrConnectingDeviceTypeNotFound{}
	}
	var result bmodel.DeviceType
	res.NextItem()
	result = bmodel.DeviceType(res.OString())
	if res.Err() != nil {
		err := xerrors.Errorf("failed to parse YDB response row: %w", res.Err())
		ctxlog.Error(ctx, db.Logger, err.Error())
		return "", err
	}
	return result, nil
}

func (db *Client) StoreConnectingDeviceType(ctx context.Context, userID uint64, speakerID string, deviceType bmodel.DeviceType) error {
	query := fmt.Sprintf(`
				--!syntax_v1
				PRAGMA TablePathPrefix("%s");
				DECLARE $huid AS Uint64;
				DECLARE $user_id AS Uint64;
				DECLARE $id AS String;
				DECLARE $device_type AS String;
				UPSERT INTO
					ConnectingDeviceTypes (huid, user_id, id, device_type)
				VALUES
					($huid, $user_id, $id, $device_type)`, db.Prefix)
	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
		table.ValueParam("$user_id", ydb.Uint64Value(userID)),
		table.ValueParam("$id", ydb.StringValue([]byte(speakerID))),
		table.ValueParam("$device_type", ydb.StringValue([]byte(deviceType))),
	)
	if err := db.Write(ctx, query, params); err != nil {
		return err
	}
	ctxlog.Infof(ctx, db.Logger, "Stored device type %s for speaker %s for user %d", deviceType, speakerID, userID)
	return nil
}
