package main

import (
	"context"
	"encoding/json"
	"fmt"
	"path"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type MigrationDBClient struct {
	*db.DBClient
}

type Device struct {
	HUID         uint64
	ID           string
	Type         model.DeviceType
	Capabilities model.Capabilities
}

func (d *Device) FillCapabilities() bool {
	if !d.Type.IsSmartSpeaker() {
		return false
	}
	expManager := experiments.MockManager{
		experiments.DropNewCapabilitiesForOldSpeakers: true,
	}
	quasarCapabilities := model.GenerateQuasarCapabilities(experiments.ContextWithManager(context.Background(), expManager), d.Type)

	// if device already has all capabilities - no need to do anything
	if intersection := intersectCapabilities(d.Capabilities, quasarCapabilities); len(intersection) == len(quasarCapabilities) {
		return false
	}
	d.Capabilities = quasarCapabilities
	return true
}

func (db *MigrationDBClient) StreamSpeakers(ctx context.Context) <-chan Device {
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
			table.ReadColumn("skill_id"),
			table.ReadColumn("type"),
			table.ReadColumn("capabilities"),
			table.ReadColumn("archived"),
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
		var unfilledDevicesCount int
		for res.NextStreamSet(ctx) {
			for res.NextRow() {
				devicesCount++

				var device Device
				res.SeekItem("huid")
				device.HUID = res.OUint64()

				res.SeekItem("id")
				device.ID = string(res.OString())

				res.SeekItem("skill_id")
				skillID := string(res.OString())

				res.SeekItem("type")
				device.Type = model.DeviceType(res.OString())

				// capabilities
				res.SeekItem("capabilities")
				var capabilities model.Capabilities
				capabilitiesRaw := res.OJSON()
				if len(capabilitiesRaw) > 0 {
					capabilities, err = model.JSONUnmarshalCapabilities(json.RawMessage(capabilitiesRaw))
					if err != nil {
						ctxlog.Errorf(ctx, logger, "failed to parse `capabilities` field for device %s: %v", device.ID, err)
						continue
					}
				}
				device.Capabilities = capabilities

				res.SeekItem("archived")
				archived := res.OBool()

				if err := res.Err(); err != nil {
					logger.Warnf("Error occurred while reading %s: %v", devicesTablePath, err)
					continue
				}

				if skillID == model.QUASAR && !archived && device.FillCapabilities() {
					devicesChannel <- device
					unfilledDevicesCount++
				}
			}
		}

		logger.Infof("Finished reading %d devices from %s, unfilled speakers: %d", devicesCount, devicesTablePath, unfilledDevicesCount)
	}()

	return devicesChannel
}

func (db *MigrationDBClient) FillCapabilitiesOnSpeakers(ctx context.Context, devices []Device) error {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");

		DECLARE $values AS List<Struct<
			id: String,
			huid: Uint64,
			capabilities: Json
		>>;

		UPSERT INTO
			Devices (id, huid, capabilities)
		SELECT
			id, huid, capabilities
		FROM AS_TABLE($values);
	`, db.Prefix)

	values := make([]ydb.Value, 0, len(devices))
	for _, userDevice := range devices {
		capabilitiesB, err := json.Marshal(userDevice.Capabilities)
		if err != nil {
			ctxlog.Errorf(ctx, logger, "cannot marshal capabilities for device %s: %v", userDevice.ID, err)
			continue
		}
		values = append(values, ydb.StructValue(
			ydb.StructFieldValue("huid", ydb.Uint64Value(userDevice.HUID)),
			ydb.StructFieldValue("id", ydb.StringValue([]byte(userDevice.ID))),
			ydb.StructFieldValue("capabilities", ydb.JSONValue(string(capabilitiesB))),
		))
	}
	params := table.NewQueryParameters(
		table.ValueParam("$values", ydb.ListValue(values...)),
	)
	if err := db.Write(ctx, query, params); err != nil {
		return xerrors.Errorf("failed to fill capabilities on devices: %w", err)
	}

	return nil
}

func intersectCapabilities(a model.Capabilities, b model.Capabilities) model.Capabilities {
	aMap := a.AsMap()
	bMap := b.AsMap()
	result := make(model.Capabilities, 0, len(a)+len(b))
	for _, capability := range aMap {
		if _, exist := bMap[capability.Key()]; exist {
			result = append(result, capability.Clone())
		}
	}
	return result
}
