package main

import (
	"context"
	"encoding/json"
	"fmt"
	"path"
	"sort"

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

func (d *Device) FillColorsettingCapabilities() bool {
	if d.Type != model.YandexStationMidiDeviceType {
		return false
	}

	if _, ok := d.Capabilities.GetCapabilityByTypeAndInstance(model.ColorSettingCapabilityType, model.SceneCapabilityInstance.String()); ok {
		return false
	}

	colorScenes := make(model.ColorScenes, 0, len(model.KnownYandexmidiColorScenes))
	for _, colorSceneID := range model.KnownYandexmidiColorScenes {
		colorScenes = append(colorScenes, model.KnownColorScenes[model.ColorSceneID(colorSceneID)])
	}
	sort.Sort(model.ColorSceneSorting(colorScenes))
	midiColorSetting := model.MakeCapabilityByType(model.ColorSettingCapabilityType).
		WithParameters(model.ColorSettingCapabilityParameters{
			ColorSceneParameters: &model.ColorSceneParameters{
				Scenes: colorScenes,
			},
		})

	d.Capabilities = append(d.Capabilities, midiColorSetting)

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
		var yandexmidiCount int
		var unfilledYandexmidiCount int

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

				if skillID == model.QUASAR && !archived && device.Type == model.YandexStationMidiDeviceType {
					yandexmidiCount++

					if device.FillColorsettingCapabilities() {
						devicesChannel <- device
						unfilledYandexmidiCount++
					}
				}
			}
		}

		logger.Infof("Finished reading %d devices from %s, yandexmidi count: %d(%d unfilled)", devicesCount, devicesTablePath, yandexmidiCount, unfilledYandexmidiCount)
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
