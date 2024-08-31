package main

import (
	"context"
	"fmt"
	"path"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/ptr"
)

type MigrationDBClient struct {
	*db.DBClient
}

type Device struct {
	HID     uint64
	ID      string
	SkillID *string
}

func (db *MigrationDBClient) StreamDevices(ctx context.Context) <-chan Device {
	devicesCh := make(chan Device)

	go func() {
		defer close(devicesCh)

		s, err := db.SessionPool.Get(ctx)
		if err != nil {
			logger.Fatalf("Can't get session: %v", err)
		}

		deviceOwnerCacheTablePath := path.Join(db.Prefix, "DeviceOwnerCache")
		logger.Infof("reading DeviceOwnerCache table from path %q", deviceOwnerCacheTablePath)

		res, err := s.StreamReadTable(ctx, deviceOwnerCacheTablePath,
			table.ReadColumn("hid"),
			table.ReadColumn("id"),
			table.ReadColumn("skill_id"),
		)
		if err != nil {
			logger.Fatalf("Failed to read table: %v", err)
		}
		defer func() {
			if err := res.Close(); err != nil {
				logger.Fatalf("Error while closing result set: %v", err)
			}
		}()

		var count int
		for res.NextStreamSet(ctx) {
			for res.NextRow() {
				count++

				var device Device
				res.SeekItem("hid")
				device.HID = res.OUint64()

				res.SeekItem("id")
				device.ID = string(res.OString())

				res.SeekItem("skill_id")
				if !res.IsNull() {
					device.SkillID = ptr.String(string(res.OString()))
				}

				if err := res.Err(); err != nil {
					logger.Warnf("error occurred while reading %s: %v", deviceOwnerCacheTablePath, err)
					continue
				}

				if device.SkillID == nil {
					devicesCh <- device
				}
			}
		}

		logger.Infof("finished reading %d rows from %s", count, deviceOwnerCacheTablePath)
	}()

	return devicesCh
}

func (db *MigrationDBClient) FillSkillIDOnDeviceOwnerCache(ctx context.Context, devices []Device) error {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");

		DECLARE $values AS List<Struct<
			hid: Uint64,
			id: String,
			skill_id: String
		>>;

		UPSERT INTO
			DeviceOwnerCache (hid, id, skill_id)
		SELECT
			hid, id, skill_id
		FROM AS_TABLE($values);
	`, db.Prefix)

	values := make([]ydb.Value, 0, len(devices))
	for _, device := range devices {
		values = append(values, ydb.StructValue(
			ydb.StructFieldValue("hid", ydb.Uint64Value(device.HID)),
			ydb.StructFieldValue("id", ydb.StringValue([]byte(device.ID))),
			ydb.StructFieldValue("skill_id", ydb.StringValue([]byte(model.TUYA))),
		))
	}
	params := table.NewQueryParameters(
		table.ValueParam("$values", ydb.ListValue(values...)),
	)
	if err := db.Write(ctx, query, params); err != nil {
		return xerrors.Errorf("failed to fill is_active column on scenarios: %w", err)
	}

	return nil
}
