package main

import (
	"context"
	"encoding/json"
	"fmt"
	"path"

	"go.uber.org/atomic"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type DBClient struct {
	*db.DBClient

	IotDeviceMemoryTool IotDeviceMemoryTool
	IotUserMemoryTool   IotUserMemoryTool
	userCounter         atomic.Int32
	deviceCounter       atomic.Int32
}

func (db *DBClient) UpsertUserDevices(ctx context.Context, quasarUserDevices []quasarUserDevice) error {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");

		DECLARE $values AS List<Struct<
			id: String,
			huid: Uint64,
			user_id: Uint64,
			name: String,
			external_id: String,
			external_name: String,
			skill_id: String,
			type: String,
			original_type: String,
			room_id: Optional<String>,
			capabilities: Json,
			properties: Json,
			custom_data: Json,
			device_info: Json,
			created: Timestamp,
			updated: Timestamp
		>>;

		UPSERT INTO
			Devices (id, huid, user_id, name, external_id, external_name, skill_id, type, original_type, room_id, capabilities, properties, custom_data, device_info, archived, created, updated)
		SELECT
			id, huid, user_id, name, external_id, external_name, skill_id, type, original_type, room_id, capabilities, properties, custom_data, device_info, false, created, updated
		FROM AS_TABLE($values);
	`, db.Prefix)

	db.deviceCounter.Add(int32(len(quasarUserDevices)))
	values := make([]ydb.Value, 0, len(quasarUserDevices))
	for _, quDevice := range quasarUserDevices {
		d := db.IotDeviceMemoryTool.GetDevice(UniquePair{
			HUID:  tools.Huidify(quDevice.user.GetID()),
			ExtID: quDevice.device.ExternalID,
		})
		ydbValue, err := quDevice.ToYDBValue(d)
		if err != nil {
			return xerrors.Errorf("cannot fill ydb-value for device: `%s`: %w", quDevice.device.ExternalID, err)
		}
		values = append(values, ydbValue)
	}
	params := table.NewQueryParameters(
		table.ValueParam("$values", ydb.ListValue(values...)),
	)

	if err := db.Write(ctx, query, params); err != nil {
		return xerrors.Errorf("failed to upsert deviceCounter: %w", err)
	}

	return nil
}

func (db *DBClient) UpsertUsers(ctx context.Context, quasarUsers []quasarUser) error {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");

		DECLARE $values AS List<Struct<
			hid: Uint64,
			id: Uint64,
			login: String,
			created: Timestamp
		>>;

		UPSERT INTO
			Users (hid, id, login, created)
		SELECT
			hid, id, login, created
		FROM AS_TABLE($values);
	`, db.Prefix)

	db.userCounter.Add(int32(len(quasarUsers)))
	values := make([]ydb.Value, 0, len(quasarUsers))
	for _, qUser := range quasarUsers {
		u := db.IotUserMemoryTool.GetUser(qUser.GetID())
		ydbValue, err := qUser.ToYDBValue(u)
		if err != nil {
			return xerrors.Errorf("cannot fill ydb-value for quasarUser: `%s`: %w", qUser.ID, err)
		}
		values = append(values, ydbValue)
	}
	params := table.NewQueryParameters(
		table.ValueParam("$values", ydb.ListValue(values...)),
	)

	if err := db.Write(ctx, query, params); err != nil {
		return xerrors.Errorf("failed to upsert users: %w", err)
	}

	return nil
}

type IotDevice struct {
	ID           string
	HUID         uint64
	externalID   string
	roomID       string
	created      timestamp.PastTimestamp
	updated      timestamp.PastTimestamp
	customData   interface{}
	deviceInfo   *model.DeviceInfo
	capabilities model.Capabilities
	properties   model.Properties
}

func (db *DBClient) StreamSpeakerDevices(ctx context.Context) <-chan IotDevice {
	speakerDeviceChannel := make(chan IotDevice)

	go func() {
		defer close(speakerDeviceChannel)

		s, err := db.SessionPool.Get(ctx)
		if err != nil {
			logger.Fatalf("Can't get session: %v", err)
		}

		devicesTablePath := path.Join(db.Prefix, "Devices")
		logger.Infof("Reading Devices table from path %q", devicesTablePath)

		res, err := s.StreamReadTable(ctx, devicesTablePath,
			table.ReadColumn("id"),
			table.ReadColumn("huid"),
			table.ReadColumn("external_id"),
			table.ReadColumn("skill_id"),
			table.ReadColumn("archived"),
			table.ReadColumn("room_id"),
			table.ReadColumn("created"),
			table.ReadColumn("updated"),
			table.ReadColumn("custom_data"),
			table.ReadColumn("device_info"),
			table.ReadColumn("capabilities"),
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

		var speakersCount int
		for res.NextStreamSet(ctx) {
			for res.NextRow() {
				var device IotDevice
				res.SeekItem("id")
				device.ID = string(res.OString())
				res.SeekItem("huid")
				device.HUID = res.OUint64()
				res.SeekItem("external_id")
				device.externalID = string(res.OString())
				res.SeekItem("skill_id")
				skillID := string(res.OString())
				res.SeekItem("archived")
				archived := res.OBool()
				res.SeekItem("room_id")
				device.roomID = string(res.OString())

				res.SeekItem("created")
				rawCreated := res.OTimestamp()
				if rawCreated != 0 {
					device.created = timestamp.FromMicro(rawCreated)
				}
				res.SeekItem("updated")
				rawUpdated := res.OTimestamp()
				if rawUpdated != 0 {
					device.updated = timestamp.FromMicro(rawUpdated)
				}

				res.SeekItem("custom_data")
				customDataRaw := res.OJSON()
				if len(customDataRaw) > 0 {
					var customData interface{}
					if err := json.Unmarshal([]byte(customDataRaw), &customData); err != nil {
						panic(fmt.Sprintf("failed to parse `custom_data` field for device %s: %s", device.ID, err))
					}
					device.customData = customData
				}

				res.SeekItem("device_info")
				deviceInfoRaw := res.OJSON()
				if len(deviceInfoRaw) > 0 {
					device.deviceInfo = &model.DeviceInfo{}
					if err := json.Unmarshal([]byte(deviceInfoRaw), device.deviceInfo); err != nil {
						panic(fmt.Sprintf("failed to parse `device_info` field for device %s: %s", device.ID, err))
					}
				}

				res.SeekItem("capabilities")
				var capabilities model.Capabilities
				capabilitiesRaw := res.OJSON()
				if len(capabilitiesRaw) > 0 {
					capabilities, err = model.JSONUnmarshalCapabilities(json.RawMessage(capabilitiesRaw))
					if err != nil {
						panic(fmt.Sprintf("failed to parse `capabilities` field for device %s: %s", device.ID, err))
					}
				}
				device.capabilities = capabilities

				res.SeekItem("properties")
				var properties model.Properties
				propertiesRaw := res.OJSON()
				if len(propertiesRaw) > 0 {
					properties, err = model.JSONUnmarshalProperties(json.RawMessage(propertiesRaw))
					if err != nil {
						panic(fmt.Sprintf("failed to parse `properties` field for device %s: %s", device.ID, err))
					}
				}
				device.properties = properties

				if !archived && skillID == model.QUASAR {
					speakersCount++
					speakerDeviceChannel <- device
				}
			}
		}

		logger.Infof("Finished reading %d speakers from %s", speakersCount, devicesTablePath)
	}()

	return speakerDeviceChannel
}

type IotUser struct {
	HID     uint64
	ID      uint64
	Login   string
	Created timestamp.PastTimestamp
}

func (db *DBClient) StreamUsers(ctx context.Context) <-chan IotUser {
	userChannel := make(chan IotUser)

	go func() {
		defer close(userChannel)

		s, err := db.SessionPool.Get(ctx)
		if err != nil {
			logger.Fatalf("Can't get session: %v", err)
		}

		usersTablePath := path.Join(db.Prefix, "Users")
		logger.Infof("Reading Users table from path %q", usersTablePath)

		res, err := s.StreamReadTable(ctx, usersTablePath,
			table.ReadColumn("hid"),
			table.ReadColumn("id"),
			table.ReadColumn("login"),
			table.ReadColumn("created"),
		)
		if err != nil {
			logger.Fatalf("Failed to read table: %v", err)
		}
		defer func() {
			if err := res.Close(); err != nil {
				logger.Fatalf("Error while closing result set: %v", err)
			}
		}()

		var usersCount int
		for res.NextStreamSet(ctx) {
			for res.NextRow() {
				var user IotUser
				res.SeekItem("hid")
				user.HID = res.OUint64()
				res.SeekItem("id")
				user.ID = res.OUint64()
				res.SeekItem("login")
				user.Login = string(res.OString())

				res.SeekItem("created")
				rawCreated := res.OTimestamp()
				if rawCreated != 0 {
					user.Created = timestamp.FromMicro(rawCreated)
				}

				usersCount++
				userChannel <- user
			}
		}

		logger.Infof("Finished reading %d users from %s", usersCount, usersTablePath)
	}()

	return userChannel
}
