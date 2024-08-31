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

type Device struct {
	HUID     uint64
	DeviceID string
}

func (db *MigrationDBClient) StreamDevicesWithoutProperties(ctx context.Context) <-chan Device {
	devicesChannel := make(chan Device)

	go func() {
		defer close(devicesChannel)

		s, err := db.SessionPool.Get(ctx)
		if err != nil {
			logger.Fatalf("Can't get session: %v", err)
		}

		devicesTablePath := path.Join(db.Prefix, "Devices")
		logger.Infof("Reading Devices table from path %q", devicesTablePath)

		res, err := s.StreamReadTable(ctx, devicesTablePath,
			table.ReadColumn("huid"),
			table.ReadColumn("id"),
			table.ReadColumn("properties"),
		)
		if err != nil {
			logger.Fatalf("Failed to read table: %v", err)
		}
		defer func() {
			if err := res.Close(); err != nil {
				logger.Fatalf("Error while closing result set: %v", err)
			}
		}()

		var devicesCount int
		for res.NextStreamSet(ctx) {
			for res.NextRow() {
				devicesCount++

				var device Device
				res.SeekItem("huid")
				device.HUID = res.OUint64()

				res.SeekItem("id")
				device.DeviceID = string(res.OString())

				res.SeekItem("properties")
				properties := res.OJSON()

				if err := res.Err(); err != nil {
					logger.Warnf("Error occurred while reading %s: %v", devicesTablePath, err)
					continue
				}
				if len(properties) == 0 {
					devicesChannel <- device
				}
			}
		}

		logger.Infof("Finished reading %d devices from %s", devicesCount, devicesTablePath)
	}()

	return devicesChannel
}

func (db *MigrationDBClient) FillPropertiesOnDevices(ctx context.Context, devices []Device) error {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");

		DECLARE $values AS List<Struct<
			id: String,
			huid: Uint64,
			properties: Json
		>>;

		UPSERT INTO
			Devices (id, huid, properties)
		SELECT
			id, huid, properties
		FROM AS_TABLE($values);
	`, db.Prefix)

	values := make([]ydb.Value, 0, len(devices))
	for _, userDevice := range devices {
		values = append(values, ydb.StructValue(
			ydb.StructFieldValue("huid", ydb.Uint64Value(userDevice.HUID)),
			ydb.StructFieldValue("id", ydb.StringValue([]byte(userDevice.DeviceID))),
			ydb.StructFieldValue("properties", ydb.JSONValue("[]")),
		))
	}
	params := table.NewQueryParameters(
		table.ValueParam("$values", ydb.ListValue(values...)),
	)
	if err := db.Write(ctx, query, params); err != nil {
		return xerrors.Errorf("failed to fill properties on devices: %w", err)
	}

	return nil
}
