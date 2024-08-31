package db

import (
	"context"
	"fmt"
	"time"

	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/tuya"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"golang.org/x/xerrors"
)

var ErrNoDeviceOwner = xerrors.New("no owner found")

func (db *DBClient) GetDeviceOwner(ctx context.Context, deviceID string, maxAge time.Duration) (tuya.DeviceOwner, error) {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");

		DECLARE $hid AS Uint64;
		DECLARE $id AS String;
		DECLARE $max_age AS Interval;
		DECLARE $timestamp AS Timestamp;

		SELECT
			tuya_uid, skill_id
		FROM
			DeviceOwnerCache
		WHERE
			hid = $hid
			AND id = $id
			AND ($timestamp - updated) < $max_age;`, db.Prefix)

	params := table.NewQueryParameters(
		table.ValueParam("$hid", ydb.Uint64Value(tools.HuidifyString(deviceID))),
		table.ValueParam("$id", ydb.StringValue([]byte(deviceID))),
		table.ValueParam("$max_age", ydb.IntervalValue(maxAge.Microseconds())),
		table.ValueParam("$timestamp", ydb.TimestampValue(db.CurrentTimestamp().YdbTimestamp())),
	)

	res, err := db.Read(ctx, query, params)
	if err != nil {
		return tuya.DeviceOwner{}, xerrors.Errorf("failed to get device owner from DB: %w", err)
	}

	if !res.NextSet() {
		return tuya.DeviceOwner{}, xerrors.New("result set not found")
	}
	if res.SetRowCount() > 1 {
		return tuya.DeviceOwner{}, xerrors.Errorf("found more than one device with id=%q", deviceID)
	}
	if !res.NextRow() {
		return tuya.DeviceOwner{}, ErrNoDeviceOwner // this error is now used in tests!
	}

	res.SeekItem("tuya_uid")
	ownerUID := string(res.OString())
	res.NextItem()
	skillID := string(res.OString())
	if err := res.Err(); err != nil {
		return tuya.DeviceOwner{}, xerrors.Errorf("failed to parse result row: %w", err)
	}
	return tuya.DeviceOwner{
		TuyaUID: ownerUID,
		SkillID: skillID,
	}, nil
}

func (db *DBClient) SetDevicesOwner(ctx context.Context, deviceIDs []string, owner tuya.DeviceOwner) error {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");

		DECLARE $values AS List<Struct<
			hid: Uint64,
			id: String,
			tuya_uid: String,
			skill_id: String,
			updated: Timestamp
		>>;

		UPSERT INTO
			DeviceOwnerCache (hid, id, tuya_uid, skill_id, updated)
		SELECT
			hid,
			id,
			tuya_uid,
			skill_id,
			updated
		FROM
			AS_TABLE($values);`, db.Prefix)

	values := make([]ydb.Value, 0, len(deviceIDs))
	for _, deviceID := range deviceIDs {
		value := ydb.StructValue(
			ydb.StructFieldValue("hid", ydb.Uint64Value(tools.HuidifyString(deviceID))),
			ydb.StructFieldValue("id", ydb.StringValue([]byte(deviceID))),
			ydb.StructFieldValue("tuya_uid", ydb.StringValue([]byte(owner.TuyaUID))),
			ydb.StructFieldValue("skill_id", ydb.StringValue([]byte(owner.SkillID))),
			ydb.StructFieldValue("updated", ydb.TimestampValue(db.CurrentTimestamp().YdbTimestamp())),
		)
		values = append(values, value)
	}

	params := table.NewQueryParameters(
		table.ValueParam("$values", ydb.ListValue(values...)),
	)

	if err := db.Write(ctx, query, params); err != nil {
		return xerrors.Errorf("failed to store device owner: %w", err)
	}

	return nil
}

func (db *DBClient) InvalidateDeviceOwner(ctx context.Context, deviceID string) error {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");

		DECLARE $hid AS Uint64;
		DECLARE $id AS String;

		DELETE FROM
			DeviceOwnerCache
		WHERE
			hid = $hid AND
			id = $id;`, db.Prefix)

	params := table.NewQueryParameters(
		table.ValueParam("$hid", ydb.Uint64Value(tools.HuidifyString(deviceID))),
		table.ValueParam("$id", ydb.StringValue([]byte(deviceID))),
	)

	if err := db.Write(ctx, query, params); err != nil {
		return xerrors.Errorf("failed to invalidate device owner: %w", err)
	}

	return nil
}
