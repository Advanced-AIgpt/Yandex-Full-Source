package db

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"

	"github.com/gofrs/uuid"
	"golang.org/x/exp/slices"

	bulbasaur "a.yandex-team.ru/alice/iot/bulbasaur/errors"
	"a.yandex-team.ru/alice/iot/bulbasaur/logging"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type DeviceQueryCriteria struct {
	DeviceID    string
	UserID      uint64
	SkillID     string
	GroupID     string
	RoomID      string
	HouseholdID string
	Archived    bool
	WithShared  bool // that flag allows to select devices that were shared with user
}

func (db *DBClient) SelectUserDevicesSimple(ctx context.Context, userID uint64) (model.Devices, error) {
	criteria := DeviceQueryCriteria{UserID: userID, WithShared: true}
	devices, err := db.selectUserDevicesSimple(ctx, criteria)
	for i := range devices {
		devices[i].Room = nil
	}
	return devices, err
}

func (db *DBClient) SelectUserDeviceSimple(ctx context.Context, userID uint64, deviceID string) (model.Device, error) {
	criteria := DeviceQueryCriteria{UserID: userID, DeviceID: deviceID, WithShared: true}
	result, err := db.selectUserDevicesSimple(ctx, criteria)
	if err != nil {
		return model.Device{}, xerrors.Errorf("cannot get user device from database: %w", err)
	}
	if len(result) > 1 {
		return model.Device{}, fmt.Errorf("found more than one device using criteria: %#v", criteria)
	}

	if len(result) == 0 {
		return model.Device{}, &model.DeviceNotFoundError{}
	}
	result[0].Room = nil
	return result[0], nil
}

func (db *DBClient) SelectUserDevices(ctx context.Context, userID uint64) (model.Devices, error) {
	criteria := DeviceQueryCriteria{UserID: userID, WithShared: true}
	return db.selectUserDevices(ctx, criteria)
}

func (db *DBClient) SelectUserDevice(ctx context.Context, userID uint64, deviceID string) (model.Device, error) {
	criteria := DeviceQueryCriteria{UserID: userID, DeviceID: deviceID, WithShared: true}
	result, err := db.selectUserDevices(ctx, criteria)
	if err != nil {
		return model.Device{}, xerrors.Errorf("cannot get user device from database: %w", err)
	}
	if len(result) > 1 {
		return model.Device{}, fmt.Errorf("found more than one device using criteria: %#v", criteria)
	}

	if len(result) == 0 {
		return model.Device{}, &model.DeviceNotFoundError{}
	}
	return result[0], nil
}

func (db *DBClient) SelectUserProviderDevicesSimple(ctx context.Context, userID uint64, skillID string) (model.Devices, error) {
	criteria := DeviceQueryCriteria{UserID: userID, SkillID: skillID, WithShared: true}
	return db.selectUserDevicesSimple(ctx, criteria)
}

func (db *DBClient) SelectDevicesSimpleByExternalIDs(ctx context.Context, externalIDs []string) (model.DevicesMapByOwnerID, error) {
	if len(externalIDs) == 0 {
		return nil, nil
	}

	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");
		DECLARE $external_ids AS List<String>;

		SELECT
			user_id,
			id,
			name,
			aliases,
			external_id,
			external_name,
			type,
			original_type,
			skill_id,
			capabilities,
			properties,
			custom_data,
			device_info,
			room_id,
			updated,
			created,
			household_id,
			status,
			internal_config
		FROM
			Devices VIEW devices_external_id_index
		WHERE
			external_id IN $external_ids AND
			archived == false;
	`, db.Prefix)

	externalIDValues := make([]ydb.Value, len(externalIDs))
	for i, extID := range externalIDs {
		externalIDValues[i] = ydb.StringValue([]byte(extID))
	}

	params := table.NewQueryParameters(
		table.ValueParam("$external_ids", ydb.ListValue(externalIDValues...)),
	)

	res, err := db.Read(ctx, query, params)
	if err != nil {
		return nil, err
	}

	if !res.NextSet() {
		return nil, nil
	}

	userIDToDevices := make(map[uint64]model.Devices)
	for res.NextRow() {
		var device model.Device

		res.NextItem()
		userID := res.OUint64()

		res.NextItem()
		device.ID = string(res.OString())
		res.NextItem()
		device.Name = string(res.OString())

		res.NextItem()
		rawJSONAliases := res.OJSON()
		device.Aliases = make([]string, 0)
		if len(rawJSONAliases) != 0 {
			var aliases []string
			if err := json.Unmarshal([]byte(rawJSONAliases), &aliases); err != nil {
				return nil, err
			}
			device.Aliases = aliases
		}

		res.NextItem()
		device.ExternalID = string(res.OString())
		res.NextItem()
		device.ExternalName = string(res.OString())
		res.NextItem()
		device.Type = model.DeviceType(res.OString())
		res.NextItem()
		device.OriginalType = model.DeviceType(res.OString())
		res.NextItem()
		device.SkillID = string(res.OString())

		// capabilities
		res.NextItem()
		var capabilities model.Capabilities
		capabilitiesRaw := res.OJSON()
		if len(capabilitiesRaw) > 0 {
			capabilities, err = model.JSONUnmarshalCapabilities(json.RawMessage(capabilitiesRaw))
			if err != nil {
				return nil, xerrors.Errorf("failed to parse `capabilities` field for device %s: %w", device.ID, err)
			}
		}
		device.Capabilities = capabilities

		// properties
		res.NextItem()
		var properties model.Properties
		propertiesRaw := res.OJSON()
		if len(propertiesRaw) > 0 {
			properties, err = model.JSONUnmarshalProperties(json.RawMessage(propertiesRaw))
			if err != nil {
				return nil, xerrors.Errorf("failed to parse `properties` field for device %s: %w", device.ID, err)
			}
		}
		device.Properties = properties

		// custom_data
		res.NextItem()
		customDataRaw := res.OJSON()
		if len(customDataRaw) > 0 {
			var customData interface{}
			if err := json.Unmarshal([]byte(customDataRaw), &customData); err != nil {
				return nil, xerrors.Errorf("failed to parse `custom_data` field for device %s: %w", device.ID, err)
			}
			device.CustomData = customData
		} else {
			device.CustomData = nil
		}

		// device_info
		res.NextItem()
		deviceInfo := res.OJSON()
		if len(deviceInfo) > 0 {
			device.DeviceInfo = &model.DeviceInfo{}
			if err := json.Unmarshal([]byte(deviceInfo), device.DeviceInfo); err != nil {
				return nil, xerrors.Errorf("failed to parse `device_info` field for device %s: %w", device.ID, err)
			}
		}

		// room_id
		res.NextItem()
		roomID := string(res.OString())
		if roomID != "" {
			device.Room = &model.Room{ID: roomID}
		}

		// updated timestamp
		res.NextItem()
		updated := res.OTimestamp()
		if updated != 0 {
			device.Updated = timestamp.FromMicro(updated)
		}

		// created Timestamp
		res.NextItem()
		created := res.OTimestamp()
		if created != 0 {
			device.Created = timestamp.FromMicro(created)
		}

		// household_id
		res.NextItem()
		device.HouseholdID = string(res.OString())

		// status
		res.NextItem()
		statusJSON := res.OJSON()
		device.Status = model.UnknownDeviceStatus
		if len(statusJSON) > 0 {
			var status deviceStatus
			if err := json.Unmarshal([]byte(statusJSON), &status); err != nil {
				return nil, xerrors.Errorf("failed to parse `status` field for device %s: %w", device.ID, err)
			}
			device.Status = model.DeviceStatus(status.Status)
			device.StatusUpdated = timestamp.FromMicro(status.Updated)
		}

		// internal_config
		res.NextItem()
		internalConfigJSON := res.OJSON()
		if len(internalConfigJSON) > 0 {
			if err := json.Unmarshal([]byte(internalConfigJSON), &device.InternalConfig); err != nil {
				return nil, xerrors.Errorf("failed to parse `internal_config` field for device %s: %w", device.ID, err)
			}
		}

		if res.Err() != nil {
			err := xerrors.Errorf("failed to parse YDB response row: %w", res.Err())
			ctxlog.Error(ctx, db.Logger, err.Error())
			return nil, err
		}

		userIDToDevices[userID] = append(userIDToDevices[userID], device)
	}

	return userIDToDevices, nil
}

func (db *DBClient) SelectUserProviderDevices(ctx context.Context, userID uint64, skillID string) ([]model.Device, error) {
	criteria := DeviceQueryCriteria{UserID: userID, SkillID: skillID, WithShared: true}
	return db.selectUserDevices(ctx, criteria)
}

func (db *DBClient) SelectUserProviderArchivedDevicesSimple(ctx context.Context, userID uint64, skillID string) (map[string]model.Device, error) {
	criteria := DeviceQueryCriteria{UserID: userID, SkillID: skillID, Archived: true}
	devices, err := db.selectUserDevicesSimple(ctx, criteria)
	if err != nil {
		return nil, err
	}
	extIDtoDeviceMap := make(map[string]model.Device) // to get only last deleted instance of device
	for _, device := range devices {
		previousDevice, exist := extIDtoDeviceMap[device.ExternalID]
		if !exist || device.Created > previousDevice.Created {
			extIDtoDeviceMap[device.ExternalID] = device
			continue
		}
	}
	return extIDtoDeviceMap, nil
}

func (db *DBClient) SelectUserGroupDevices(ctx context.Context, userID uint64, groupID string) ([]model.Device, error) {
	criteria := DeviceQueryCriteria{UserID: userID, GroupID: groupID, WithShared: true}
	return db.selectUserDevices(ctx, criteria)
}

func (db *DBClient) SelectUserRoomDevices(ctx context.Context, userID uint64, roomID string) ([]model.Device, error) {
	criteria := DeviceQueryCriteria{UserID: userID, RoomID: roomID, WithShared: true}
	return db.selectUserDevices(ctx, criteria)
}

func (db *DBClient) SelectUserHouseholdDevices(ctx context.Context, userID uint64, householdID string) (model.Devices, error) {
	criteria := DeviceQueryCriteria{UserID: userID, HouseholdID: householdID, WithShared: true}
	return db.selectUserDevices(ctx, criteria)
}

func (db *DBClient) SelectUserHouseholdDevicesSimple(ctx context.Context, userID uint64, householdID string) (model.Devices, error) {
	criteria := DeviceQueryCriteria{UserID: userID, HouseholdID: householdID, WithShared: true}
	return db.selectUserDevicesSimple(ctx, criteria)
}

type deviceStoreChange struct {
	ID           string
	UserID       uint64
	HouseholdID  string
	RoomID       *string
	SkillID      string
	ExternalName string
	Capabilities model.Capabilities
	Properties   model.Properties
	CustomData   interface{}
	DeviceInfo   *model.DeviceInfo
	Updated      timestamp.PastTimestamp
	Status       deviceStatus
}

func (d *deviceStoreChange) fromDevice(baseDevice, newDevice model.Device, userID uint64, deviceID string) {
	d.ID = deviceID
	d.UserID = userID
	d.HouseholdID = newDevice.HouseholdID
	if newDevice.Room != nil {
		d.RoomID = &newDevice.Room.ID
	}
	d.SkillID = newDevice.SkillID
	d.ExternalName = newDevice.ExternalName
	d.Capabilities = updateCapabilities(baseDevice.Capabilities, newDevice.Capabilities)
	d.Properties = updateProperties(baseDevice.Properties, newDevice.Properties)
	d.CustomData = newDevice.CustomData
	d.DeviceInfo = newDevice.DeviceInfo
	d.Updated = newDevice.Updated
	d.Status = formatDeviceStatus(newDevice)
}

func updateCapabilities(baseCapabilities, newCapabilities model.Capabilities) model.Capabilities {
	baseCapabilitiesMap := baseCapabilities.AsMap()

	capabilitiesToSave := make(model.Capabilities, 0, len(newCapabilities))
	for _, newCapability := range newCapabilities {
		baseCapability, ok := baseCapabilitiesMap[newCapability.Key()]
		switch {
		case !ok: // newCapability is not in base
			capabilitiesToSave = append(capabilitiesToSave, newCapability)
		case newCapability.State() != nil: // if new state is present, it's better
			capabilitiesToSave = append(capabilitiesToSave, newCapability)
		case baseCapability.State() != nil: // old capability can retain state, if it's still valid
			mergedCapability := newCapability.Clone()
			mergedCapability.SetState(baseCapability.State())
			err := mergedCapability.State().ValidateState(mergedCapability)
			if err == nil { // if old state is still valid - we win
				capabilitiesToSave = append(capabilitiesToSave, mergedCapability)
				continue
			}
			fallthrough // old state is not compatible with new parameters, add new capability
		default: // no matter what we should at least add new capability
			capabilitiesToSave = append(capabilitiesToSave, newCapability)
		}
	}

	return capabilitiesToSave
}

func updateProperties(baseProperties, newProperties model.Properties) model.Properties {
	basePropertiesMap := baseProperties.AsMap()

	propertiesToSave := make(model.Properties, 0, len(newProperties))
	for _, newProperty := range newProperties {
		baseProperty, ok := basePropertiesMap[newProperty.Key()]
		switch {
		case !ok:
			propertiesToSave = append(propertiesToSave, newProperty)
		case newProperty.State() != nil:
			propertiesToSave = append(propertiesToSave, newProperty)
		case baseProperty.State() != nil:
			mergedProperty := newProperty.Clone()
			mergedProperty.SetState(baseProperty.State())
			err := mergedProperty.State().ValidateState(mergedProperty)
			if err == nil {
				propertiesToSave = append(propertiesToSave, mergedProperty)
				continue
			}
			fallthrough
		default:
			propertiesToSave = append(propertiesToSave, newProperty)
		}
	}

	return propertiesToSave
}

func (d *deviceStoreChange) toStructValue() (ydb.Value, error) {
	//update
	capabilitiesB, err := json.Marshal(d.Capabilities)
	if err != nil {
		return nil, xerrors.Errorf("cannot marshal capabilities: %w", err)
	}
	propertiesB, err := json.Marshal(d.Properties)
	if err != nil {
		return nil, xerrors.Errorf("cannot marshal properties: %w", err)
	}
	customDataB, err := json.Marshal(d.CustomData)
	if err != nil {
		return nil, xerrors.Errorf("cannot marshal custom_data: %w", err)
	}
	deviceInfoB, err := json.Marshal(d.DeviceInfo)
	if err != nil {
		return nil, xerrors.Errorf("cannot marshal device_info: %w", err)
	}
	statusB, err := json.Marshal(d.Status)
	if err != nil {
		return nil, xerrors.Errorf("cannot marshal status: %w", err)
	}
	opts := []ydb.StructValueOption{
		ydb.StructFieldValue("external_name", ydb.StringValue([]byte(d.ExternalName))),
		ydb.StructFieldValue("capabilities", ydb.JSONValue(string(capabilitiesB))),
		ydb.StructFieldValue("properties", ydb.JSONValue(string(propertiesB))),
		ydb.StructFieldValue("custom_data", ydb.JSONValue(string(customDataB))),
		ydb.StructFieldValue("device_info", ydb.JSONValue(string(deviceInfoB))),
		ydb.StructFieldValue("huid", ydb.Uint64Value(tools.Huidify(d.UserID))),
		ydb.StructFieldValue("id", ydb.StringValue([]byte(d.ID))),
		ydb.StructFieldValue("updated", ydb.TimestampValue(d.Updated.YdbTimestamp())),
		ydb.StructFieldValue("status", ydb.JSONValue(string(statusB))),
	}
	// https://st.yandex-team.ru/ALICE-17422
	// change of household is needed only for yandexIO devices, because they are usually moved together with speaker
	if d.SkillID == model.YANDEXIO {
		opts = append(opts, ydb.StructFieldValue("household_id", ydb.StringValue([]byte(d.HouseholdID))))
		if d.RoomID != nil {
			opts = append(opts, ydb.StructFieldValue("room_id", ydb.OptionalValue(ydb.StringValue([]byte(*d.RoomID)))))
		} else {
			opts = append(opts, ydb.StructFieldValue("room_id", ydb.NullValue(ydb.TypeString)))
		}
	}
	return ydb.StructValue(opts...), nil
}

func (d *deviceStoreChange) YDBStructLiteralValue() string {
	// https://st.yandex-team.ru/ALICE-17422
	// change of household is needed only for yandexIO devices, because they are usually moved together with speaker
	if d.SkillID == model.YANDEXIO {
		result := `external_name: String, capabilities: Json, properties: Json, custom_data: Json, device_info: Json, huid: Uint64, id: String, household_id: String, room_id: String?, updated: Timestamp, status: Json`
		return result
	} else {
		result := `external_name: String, capabilities: Json, properties: Json, custom_data: Json, device_info: Json, huid: Uint64, id: String, updated: Timestamp, status: Json`
		return result
	}
}

//Selects device from db and decides weather to update it or insert new one
//All the things does in the same transaction
func (db *DBClient) StoreUserDevice(ctx context.Context, user model.User, device model.Device) (model.Device, model.StoreResult, error) {
	storeResult := model.StoreResultUnknownError
	storeFunc := func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
		//check device already exist
		query := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $huid AS Uint64;
			DECLARE $external_id AS String;
			DECLARE $skill_id AS String;

			SELECT
				COUNT(*) AS cnt
			FROM
				Devices
			WHERE
				Devices.huid == $huid AND
				Devices.archived == false;

			SELECT
				Devices.id as id,
				Devices.skill_id,
				Devices.external_name,
				Devices.household_id,
				Devices.capabilities,
		        Devices.properties
			FROM
				Devices
			WHERE
				Devices.huid == $huid AND
				Devices.skill_id == $skill_id AND
				Devices.external_id == $external_id AND
				Devices.archived == false;`, db.Prefix)
		params := table.NewQueryParameters(
			table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(user.ID))),
			table.ValueParam("$external_id", ydb.StringValue([]byte(device.ExternalID))),
			table.ValueParam("$skill_id", ydb.StringValue([]byte(device.SkillID))),
		)

		stmt, err := s.Prepare(ctx, query)
		if err != nil {
			return nil, err
		}

		tx, res, err := stmt.Execute(ctx, txControl, params)
		if err != nil {
			return tx, err
		}

		// read current user devices count
		if !res.NextSet() {
			return tx, xerrors.New("can't count unarchived devices: result set does not exist")
		}
		if !res.NextRow() {
			return tx, xerrors.New("can't read count of unarchived devices: result set has no rows")
		}
		res.SeekItem("cnt")
		userDeviceCount := res.Uint64()
		if err := res.Err(); err != nil {
			return tx, xerrors.Errorf("can't read count of unarchived devices: %w", err)
		}

		// read query
		if !res.NextSet() {
			return tx, xerrors.New("can't read unarchived devices: result set does not exist")
		}

		if res.SetRowCount() > 1 {
			return tx, fmt.Errorf("database contains non-unique triplet: <%d, %s, %s>", user.ID, device.ExternalID, device.SkillID)
		}

		if device.HouseholdID == "" {
			currentHousehold, err := db.selectCurrentHouseholdInTx(ctx, s, tx, CurrentHouseholdQueryCriteria{UserID: user.ID})
			if err != nil {
				return tx, err
			}
			device.HouseholdID = currentHousehold.ID
		} else {
			householdExists, err := db.checkUserHouseholdExistInTx(ctx, s, tx, HouseholdQueryCriteria{UserID: user.ID, HouseholdID: device.HouseholdID})
			if err != nil {
				return tx, err
			}
			if !householdExists {
				return tx, &model.UserHouseholdNotFoundError{}
			}
		}

		if res.SetRowCount() > 0 && res.NextRow() {
			//found one, lets update it
			//read res
			var d model.Device
			res.SeekItem("id")
			d.ID = string(res.OString())
			res.NextItem()
			d.SkillID = string(res.OString())
			res.NextItem()
			d.ExternalName = string(res.OString())
			res.NextItem()
			d.HouseholdID = string(res.OString())

			// capabilities
			res.NextItem()
			var capabilities model.Capabilities
			capabilitiesRaw := res.OJSON()
			if len(capabilitiesRaw) > 0 {
				capabilities, err = model.JSONUnmarshalCapabilities(json.RawMessage(capabilitiesRaw))
				if err != nil {
					return tx, xerrors.Errorf("failed to parse `capabilities` field for device %s: %w", device.ID, err)
				}
			}
			d.Capabilities = capabilities

			// properties
			res.NextItem()
			var properties model.Properties
			propertiesRaw := res.OJSON()
			if len(propertiesRaw) > 0 {
				properties, err = model.JSONUnmarshalProperties(json.RawMessage(propertiesRaw))
				if err != nil {
					return tx, xerrors.Errorf("failed to parse `properties` field for device %s: %w", device.ID, err)
				}
			}
			d.Properties = properties

			if res.Err() != nil {
				return tx, xerrors.Errorf("failed to parse YDB response row: %w", res.Err())
			}
			device.ID = d.ID

			if d.SkillID == model.YANDEXIO {
				// we need to do special things for speaker-controller devices

				// speaker-controlled devices can change households and rooms on re-insertion.
				if device.HouseholdID != d.HouseholdID {
					// when household is changed, we need to remove device from all groups of old household
					if err := db.removeDeviceFromAllGroupsInTx(ctx, s, tx, user.ID, device.ID); err != nil {
						return tx, xerrors.Errorf("failed to delete device from groups before moving it into diff")
					}
				}
				// we need to place device in some room of its household
				if device.Room != nil && device.Room.ID == "" {
					device.Room.Name = tools.StandardizeSpaces(device.Room.Name)
					room, found, err := db.findRoomByNameInTx(ctx, s, tx, user.ID, device.HouseholdID, device.Room.Name)
					if err != nil {
						return tx, xerrors.Errorf("failed to find room in which device should be placed: %w", err)
					}
					if found {
						device.Room.ID = room.ID
					} else {
						device.Room = nil
					}
				}
			}

			var storeStruct deviceStoreChange
			storeStruct.fromDevice(d, device, user.ID, d.ID)
			ydbStoreValue, err := storeStruct.toStructValue()
			if err != nil {
				return tx, err
			}
			updateQuery := fmt.Sprintf(`
				--!syntax_v1
				PRAGMA TablePathPrefix("%s");
				DECLARE $changeset AS List<Struct<
					%s
				>>;
				UPDATE Devices ON SELECT * FROM AS_TABLE($changeset);`, db.Prefix, storeStruct.YDBStructLiteralValue())
			queryChangeSetParams := []ydb.Value{ydbStoreValue}
			params := table.NewQueryParameters(
				table.ValueParam("$changeset", ydb.ListValue(queryChangeSetParams...)),
			)
			updateQueryStmt, err := s.Prepare(ctx, updateQuery)
			if err != nil {
				return tx, err
			}
			_, err = tx.ExecuteStatement(ctx, updateQueryStmt, params)
			if err != nil {
				return tx, xerrors.Errorf("cannot execute update query: %w", err)
			}

			ctx = logging.GetContextWithDevices(ctx, []model.Device{device})
			ctxlog.Infof(ctx, db.Logger, "Device `%s` was updated successfully for user %d", device.ID, user.ID)
			storeResult = model.StoreResultUpdated
		} else {
			//this is a new device, lets insert it, if limit is not reached
			if userDeviceCount >= model.ConstDeviceLimit {
				return tx, &model.DeviceLimitReachedError{}
			}

			newDeviceID, err := uuid.NewV4()
			if err != nil {
				return tx, err
			}
			device.ID = newDeviceID.String()

			//insert Room
			if device.Room != nil {
				device.Room.Name = tools.StandardizeSpaces(device.Room.Name)

				if len(device.Room.Name) > 0 {
					//check room exists
					room, found, err := db.findRoomByNameInTx(ctx, s, tx, user.ID, device.HouseholdID, device.Room.Name)
					if err != nil {
						return tx, err
					}
					if found {
						device.Room.ID = room.ID
					} else {
						newRoomID, err := db.insertDeviceRoomInTx(ctx, s, tx, user.ID, device.Room.Name, device.HouseholdID)
						if err != nil {
							return tx, err
						}
						device.Room.ID = newRoomID
					}
				}
			}

			//insert Device
			capabilitiesB, err := json.Marshal(device.Capabilities)
			if err != nil {
				return tx, xerrors.Errorf("cannot marshal capabilities: %w", err)
			}
			propertiesB, err := json.Marshal(device.Properties)
			if err != nil {
				return tx, xerrors.Errorf("cannot marshal properties: %w", err)
			}
			customDataB, err := json.Marshal(device.CustomData)
			if err != nil {
				return tx, xerrors.Errorf("cannot marshal custom_data: %w", err)
			}
			deviceInfoB, err := json.Marshal(device.DeviceInfo)
			if err != nil {
				return tx, xerrors.Errorf("cannot marshal device_info: %w", err)
			}
			var roomID ydb.Value
			if device.Room != nil {
				roomID = ydb.OptionalValue(ydb.StringValue([]byte(device.Room.ID)))
			} else {
				roomID = ydb.NullValue(ydb.TypeString)
			}

			device.Name = tools.StandardizeSpaces(device.ExternalName)

			if device.IsQuasarDevice() && len(device.Aliases) != 0 {
				return tx, &model.DeviceTypeAliasesUnsupportedError{}
			}

			normalizedAliases := make([]string, 0, len(device.Aliases))
			for _, a := range device.Aliases {
				alias := tools.StandardizeSpaces(a)
				if alias == "" {
					return tx, &model.InvalidValueError{}
				}
				normalizedAliases = append(normalizedAliases, alias)
			}
			marshalledAliases, err := json.Marshal(normalizedAliases)
			if err != nil {
				return tx, err
			}

			statusB, err := json.Marshal(formatDeviceStatus(device))
			if err != nil {
				return tx, err
			}

			device.Created = db.CurrentTimestamp()

			insertDeviceQuery := fmt.Sprintf(`
				--!syntax_v1
				PRAGMA TablePathPrefix("%s");
				DECLARE $device_id AS String;
				DECLARE $huid AS Uint64;
				DECLARE $user_id AS Uint64;
				DECLARE $device_name AS String;
				DECLARE $device_aliases AS Json;
				DECLARE $external_id AS String;
				DECLARE $external_name AS String;
				DECLARE $skill_id AS String;
				DECLARE $type AS String;
				DECLARE $original_type AS String;
				DECLARE $room_id AS Optional<String>;
				DECLARE $household_id AS Optional<String>;
				DECLARE $capabilities AS Json;
				DECLARE $properties AS Json;
				DECLARE $custom_data AS Json;
				DECLARE $device_info AS Json;
				DECLARE $updated AS Timestamp;
				DECLARE $created AS Timestamp;
				DECLARE $status AS Json;

				UPSERT INTO
					Devices (id, huid, user_id, name, aliases, external_id, external_name, skill_id, type, original_type, room_id, capabilities, properties, custom_data, device_info, household_id, archived, created, updated, status)
				VALUES
					($device_id, $huid, $user_id, $device_name, $device_aliases, $external_id, $external_name, $skill_id, $type, $original_type, $room_id, $capabilities, $properties, $custom_data, $device_info, $household_id, false, $created, $updated, $status)`, db.Prefix)
			params := table.NewQueryParameters(
				table.ValueParam("$device_id", ydb.StringValue([]byte(device.ID))),
				table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(user.ID))),
				table.ValueParam("$user_id", ydb.Uint64Value(user.ID)),
				table.ValueParam("$device_name", ydb.StringValue([]byte(device.Name))), // take ExternalName at initial import
				table.ValueParam("$device_aliases", ydb.JSONValue(string(marshalledAliases))),
				table.ValueParam("$external_id", ydb.StringValue([]byte(device.ExternalID))),
				table.ValueParam("$external_name", ydb.StringValue([]byte(device.ExternalName))),
				table.ValueParam("$skill_id", ydb.StringValue([]byte(device.SkillID))),
				table.ValueParam("$type", ydb.StringValue([]byte(device.Type))),
				table.ValueParam("$original_type", ydb.StringValue([]byte(device.OriginalType))),
				table.ValueParam("$room_id", roomID),
				table.ValueParam("$household_id", ydb.OptionalValue(ydb.StringValue([]byte(device.HouseholdID)))),
				table.ValueParam("$capabilities", ydb.JSONValue(string(capabilitiesB))),
				table.ValueParam("$properties", ydb.JSONValue(string(propertiesB))),
				table.ValueParam("$custom_data", ydb.JSONValue(string(customDataB))),
				table.ValueParam("$device_info", ydb.JSONValue(string(deviceInfoB))),
				table.ValueParam("$updated", ydb.TimestampValue(device.Updated.YdbTimestamp())),
				table.ValueParam("$created", ydb.TimestampValue(device.Created.YdbTimestamp())),
				table.ValueParam("$status", ydb.JSONValue(string(statusB))),
			)
			insertDeviceQueryStmt, err := s.Prepare(ctx, insertDeviceQuery)
			if err != nil {
				return tx, err
			}
			_, err = tx.ExecuteStatement(ctx, insertDeviceQueryStmt, params)
			if err != nil {
				return tx, err
			}

			ctx = logging.GetContextWithDevices(ctx, []model.Device{device})
			ctxlog.Infof(ctx, db.Logger, "Inserted new device %s for user %d", device.ID, user.ID)
			storeResult = model.StoreResultNew
		}

		return tx, nil
	}

	err := db.CallInTx(ctx, SerializableReadWrite, storeFunc)
	if err != nil {
		switch {
		case xerrors.Is(err, &model.DeviceLimitReachedError{}):
			storeResult = model.StoreResultLimitReached
			return model.Device{}, storeResult, nil
		}
		return model.Device{}, storeResult, xerrors.Errorf("database request has failed: %w", err)
	}
	return device.Clone(), storeResult, nil
}

func (db *DBClient) findRoomByNameInTx(ctx context.Context, s *table.Session, tx *table.Transaction, userID uint64, householdID, roomName string) (model.Room, bool, error) {
	criteria := RoomQueryCriteria{UserID: userID, HouseholdID: householdID}
	rooms, err := db.getUserRoomsInTx(ctx, s, tx, criteria)
	if err != nil {
		return model.Room{}, false, err
	}
	targetRoomName := tools.Standardize(roomName)
	for _, room := range rooms {
		if targetRoomName == tools.Standardize(room.Name) {
			return room, true, nil
		}
	}
	return model.Room{}, false, nil
}

func (db *DBClient) StoreDeviceState(ctx context.Context, userID uint64, device model.Device) (model.Capabilities, model.Properties, error) {
	var updatedProperties model.Properties
	var updatedCapabilities model.Capabilities
	updateFunc := func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
		criteria := DeviceQueryCriteria{UserID: userID, DeviceID: device.ID}
		tx, devicesForUpdate, err := db.getUserDevicesSimpleTx(ctx, s, txControl, criteria)
		if err != nil {
			return tx, err
		}

		if len(devicesForUpdate) == 0 {
			return tx, &model.DeviceNotFoundError{}
		} else if len(devicesForUpdate) > 1 {
			return tx, fmt.Errorf("found more than one device using criteria: %#v", criteria)
		}
		deviceForUpdate := devicesForUpdate[0]

		updatedCapabilities, updatedProperties = deviceForUpdate.UpdateState(device.Capabilities, device.Properties)
		hasUpdatedCapabilities, hasUpdatedProperties := len(updatedCapabilities) > 0, len(updatedProperties) > 0

		if !hasUpdatedCapabilities && !hasUpdatedProperties {
			ctxlog.Infof(ctx, db.Logger, "Skipping device %s update cause device has newer state", device.ID)
			return tx, nil
		}
		txControl = table.TxControl(table.WithTx(tx))

		var resTx *table.Transaction
		resTx, err = db.storeDevicesStates(ctx, s, txControl, userID, model.Devices{deviceForUpdate})
		if resTx != nil {
			tx = resTx
		}

		return tx, err
	}

	if err := db.CallInTx(ctx, SerializableReadWrite, updateFunc); err != nil {
		return updatedCapabilities, updatedProperties, xerrors.Errorf("database request has failed: %w", err)
	}

	return updatedCapabilities, updatedProperties, nil
}

// StoreDevicesStates store many device states into db
// devicePropertiesForHistory - used only if loadDevicesForUpdate is false
// addToHistory - remove after experiments.StorePropertiesOnCallback finished
// TODO: use request source here instead of flags combination
func (db *DBClient) StoreDevicesStates(ctx context.Context, userID uint64, devices []model.Device, loadDevicesForUpdate bool) (model.DeviceCapabilitiesMap, model.DevicePropertiesMap, error) {
	if len(devices) == 0 {
		return nil, nil, nil
	}

	inTx := db.HasTransaction(ctx)
	if inTx && loadDevicesForUpdate {
		return nil, nil, xerrors.Errorf("not implemented: load many devices for update in single transaction.")
	}
	if !inTx && !loadDevicesForUpdate {
		return nil, nil, xerrors.Errorf("need prepare device for update and update it in one transaction")
	}

	// trivial approach
	var errs bulbasaur.Errors
	deviceCapabilitiesMap := make(model.DeviceCapabilitiesMap)
	devicePropertiesMap := make(model.DevicePropertiesMap)
	if loadDevicesForUpdate {
		// trivial approach
		for _, device := range devices {
			updatedCapabilities, updatedProperties, err := db.StoreDeviceState(ctx, userID, device)
			if err != nil {
				errs = append(errs, err)
			} else {
				deviceCapabilitiesMap[device.ID] = updatedCapabilities.Clone()
				devicePropertiesMap[device.ID] = updatedProperties.Clone()
			}
		}
	} else {
		err := db.CallInTx(ctx, SerializableReadWrite, func(ctx context.Context, session *table.Session, tc *table.TransactionControl) (tx *table.Transaction, err error) {
			// that method upserts properties and capabilities to db without checking
			resTx, err := db.storeDevicesStates(ctx, session, tc, userID, devices)

			if resTx != nil {
				tx = resTx
			}
			if err != nil {
				return tx, xerrors.Errorf("store device states: %w", err)
			}
			return tx, nil
		})
		if err != nil {
			errs = append(errs, err)
		}
		for _, device := range devices {
			deviceCapabilitiesMap[device.ID] = device.Capabilities.Clone()
			devicePropertiesMap[device.ID] = device.Properties.Clone()
		}
	}

	if len(errs) > 0 {
		return nil, nil, errs
	}
	return deviceCapabilitiesMap, devicePropertiesMap, nil
}

// storeDevicesStates bulk save states in db without any checks.
// return nil, nil if devices is empty
func (db *DBClient) storeDevicesStates(ctx context.Context, s *table.Session, txControl *table.TransactionControl, userID uint64, devices model.Devices) (tx *table.Transaction, err error) {
	if len(devices) == 0 {
		return nil, nil
	}

	query := fmt.Sprintf(`
	--!syntax_v1
	PRAGMA TablePathPrefix("%s");

	DECLARE $values AS List<Struct<
		huid: Uint64,
 		id: String,
		capabilities: Json,
		properties: Json
	>>;

	UPSERT INTO
		Devices (huid, id, capabilities, properties)
	SELECT
		huid, id, capabilities, properties
	FROM
		AS_TABLE($values)`, db.Prefix)

	values := make([]ydb.Value, 0, len(devices))
	huid := ydb.Uint64Value(tools.Huidify(userID))
	for _, device := range devices {
		capabilitiesB, err := json.Marshal(device.Capabilities)
		if err != nil {
			return nil, xerrors.Errorf("marshal capabilities for device '%v': %w", device.ID, err)
		}

		propertiesB, err := json.Marshal(device.Properties)
		if err != nil {
			return nil, xerrors.Errorf("marshal capabilities for device '%v': %w", device.ID, err)
		}

		value := ydb.StructValue(
			ydb.StructFieldValue("huid", huid),
			ydb.StructFieldValue("id", ydb.StringValue([]byte(device.ID))),
			ydb.StructFieldValue("capabilities", ydb.JSONValue(string(capabilitiesB))),
			ydb.StructFieldValue("properties", ydb.JSONValue(string(propertiesB))),
		)
		values = append(values, value)
	}
	tx, _, err = s.Execute(ctx, txControl, query, table.NewQueryParameters(table.ValueParam("$values", ydb.ListValue(values...))))
	if err != nil {
		err = xerrors.Errorf("store devices state: %w", err)
	}
	return tx, err
}

type deviceStatusChange struct {
	ID     string
	UserID uint64
	Device model.Device
}

func (c deviceStatusChange) toStructValue() (ydb.Value, error) {
	statusB, err := json.Marshal(formatDeviceStatus(c.Device))
	if err != nil {
		return nil, err
	}

	return ydb.StructValue(
		ydb.StructFieldValue("id", ydb.StringValue([]byte(c.ID))),
		ydb.StructFieldValue("huid", ydb.Uint64Value(tools.Huidify(c.UserID))),
		ydb.StructFieldValue("status", ydb.JSONValue(string(statusB))),
	), nil
}

func (db *DBClient) UpdateDeviceStatuses(ctx context.Context, userID uint64, deviceStates model.DeviceStatusMap) error {
	if len(deviceStates) == 0 {
		return nil
	}
	updateFunc := func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
		criteria := DeviceQueryCriteria{UserID: userID}
		tx, userDevices, err := db.getUserDevicesSimpleTx(ctx, s, txControl, criteria)
		if err != nil {
			return tx, err
		}
		updatedDevices := userDevices.UpdateStatuses(deviceStates, db.timestamper.CurrentTimestamp())
		if len(updatedDevices) == 0 {
			return tx, nil
		}

		updateQuery := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $changeset AS List<Struct<
				huid: Uint64,
				id: String,
				status: Json
			>>;

			UPDATE Devices ON
			SELECT * FROM AS_TABLE($changeset);`, db.Prefix)

		queryChangeSetParams := make([]ydb.Value, 0, len(updatedDevices))
		for _, d := range updatedDevices {
			value, err := deviceStatusChange{d.ID, userID, d}.toStructValue()
			if err != nil {
				return tx, err
			}
			queryChangeSetParams = append(queryChangeSetParams, value)
		}

		updateParams := table.NewQueryParameters(
			table.ValueParam("$changeset", ydb.ListValue(queryChangeSetParams...)),
		)
		updateQueryStmt, err := s.Prepare(ctx, updateQuery)
		if err != nil {
			return tx, err
		}
		_, err = tx.ExecuteStatement(ctx, updateQueryStmt, updateParams)
		if err != nil {
			return tx, err
		}

		return tx, nil
	}

	err := db.CallInTx(ctx, SerializableReadWrite, updateFunc)
	if err != nil {
		return xerrors.Errorf("database request has failed: %w", err)
	}

	return nil
}

func (db *DBClient) UpdateUserDeviceName(ctx context.Context, userID uint64, deviceID string, name string) error {
	updateFunc := func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
		// Using getUserDevicesTx to start transaction to prevent device name changes during current update
		criteria := DeviceQueryCriteria{UserID: userID, DeviceID: deviceID}
		tx, devicesForUpdate, err := db.getUserDevicesSimpleTx(ctx, s, txControl, criteria)
		if err != nil {
			return tx, err
		}

		if len(devicesForUpdate) == 0 {
			return tx, &model.DeviceNotFoundError{}
		} else if len(devicesForUpdate) > 1 {
			return tx, fmt.Errorf("found more than one device using criteria: %#v", criteria)
		}

		normalizedName := tools.StandardizeSpaces(name)
		if normalizedName == "" {
			return tx, &model.InvalidValueError{}
		}

		updateQuery := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $huid AS Uint64;
			DECLARE $device_id AS String;
			DECLARE $device_name AS String;

			UPDATE
				Devices
			SET
				name = $device_name
			WHERE
				Devices.huid == $huid AND
				Devices.id == $device_id AND
				Devices.archived == false;`, db.Prefix)
		params := table.NewQueryParameters(
			table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
			table.ValueParam("$device_name", ydb.StringValue([]byte(normalizedName))),
			table.ValueParam("$device_id", ydb.StringValue([]byte(deviceID))),
		)
		updateQueryStmt, err := s.Prepare(ctx, updateQuery)
		if err != nil {
			return tx, err
		}
		_, err = tx.ExecuteStatement(ctx, updateQueryStmt, params)
		return tx, err
	}

	err := db.CallInTx(ctx, SerializableReadWrite, updateFunc)
	if err != nil {
		return xerrors.Errorf("database request has failed: %w", err)
	}

	return nil
}

func (db *DBClient) UpdateUserDeviceNameAndAliases(ctx context.Context, userID uint64, deviceID string, name string, aliases []string) error {
	updateFunc := func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
		criteria := DeviceQueryCriteria{UserID: userID, DeviceID: deviceID}
		tx, devicesForUpdate, err := db.getUserDevicesSimpleTx(ctx, s, txControl, criteria)
		if err != nil {
			return tx, err
		}

		if len(devicesForUpdate) == 0 {
			return tx, &model.DeviceNotFoundError{}
		} else if len(devicesForUpdate) > 1 {
			return tx, fmt.Errorf("found more than one device using criteria: %#v", criteria)
		}

		normalizedName := tools.StandardizeSpaces(name)
		if normalizedName == "" {
			return tx, &model.InvalidValueError{}
		}

		device := devicesForUpdate[0]
		if device.IsQuasarDevice() && len(aliases) != 0 {
			return tx, &model.DeviceTypeAliasesUnsupportedError{}
		}

		normalizedAliases := make([]string, 0, len(aliases))
		for _, a := range aliases {
			alias := tools.StandardizeSpaces(a)
			if alias == "" {
				return tx, &model.InvalidValueError{}
			}
			normalizedAliases = append(normalizedAliases, alias)
		}

		marshalledAliases, err := json.Marshal(normalizedAliases)
		if err != nil {
			return tx, err
		}

		updateQuery := fmt.Sprintf(`
			PRAGMA TablePathPrefix("%s");
			DECLARE $huid AS Uint64;
			DECLARE $device_id AS String;
			DECLARE $device_name AS String;
			DECLARE $device_aliases AS Json;

			UPDATE
				Devices
			SET
				name = $device_name,
				aliases = $device_aliases
			WHERE
				Devices.huid == $huid AND
				Devices.id == $device_id AND
				Devices.archived == false;`, db.Prefix)
		params := table.NewQueryParameters(
			table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
			table.ValueParam("$device_name", ydb.StringValue([]byte(normalizedName))),
			table.ValueParam("$device_aliases", ydb.JSONValue(string(marshalledAliases))),
			table.ValueParam("$device_id", ydb.StringValue([]byte(deviceID))),
		)
		updateQueryStmt, err := s.Prepare(ctx, updateQuery)
		if err != nil {
			return tx, err
		}
		_, err = tx.ExecuteStatement(ctx, updateQueryStmt, params)
		if err != nil {
			return tx, err
		}

		return tx, nil
	}

	err := db.CallInTx(ctx, SerializableReadWrite, updateFunc)
	if err != nil {
		return xerrors.Errorf("database request has failed: %w", err)
	}

	return nil
}

func (db *DBClient) DeleteUserDevice(ctx context.Context, userID uint64, deviceID string) error {
	return db.DeleteUserDevices(ctx, userID, []string{deviceID})
}

func (db *DBClient) DeleteUserDevices(ctx context.Context, userID uint64, deviceIDList []string) error {
	deleteFunc := func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
		// Select user groups to start transaction and update groups types after device deleting
		criteria := GroupQueryCriteria{UserID: userID}
		tx, userGroups, err := db.getUserGroupsTx(ctx, s, txControl, criteria)
		if err != nil {
			return tx, err
		}

		stereopairs, err := db.SelectStereopairsSimple(ctx, userID)
		if err != nil {
			return tx, xerrors.Errorf("failed to select stereopairs simple: %w", err)
		}

		userDevicesQuery := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $huid AS Uint64;

			SELECT
				id
			FROM
				Devices
			WHERE
				huid == $huid AND
				archived == false;`, db.Prefix)
		params := table.NewQueryParameters(
			table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
		)

		stmt, err := s.Prepare(ctx, userDevicesQuery)
		if err != nil {
			return tx, err
		}

		res, err := tx.ExecuteStatement(ctx, stmt, params)
		if err != nil {
			return tx, err
		}

		userDevicesIDsMap := make(map[string]bool, 0)
		for res.NextSet() {
			for res.NextRow() {
				res.SeekItem("id")
				userDevicesIDsMap[string(res.OString())] = true
			}
		}

		deviceIDMap := make(map[string]bool)
		for _, deviceID := range deviceIDList {
			deviceIDMap[deviceID] = true
		}
		currentTimestamp := db.timestamper.CurrentTimestamp()
		changeSet := make([]deviceDeleteChange, 0, len(deviceIDList))
		for _, deviceID := range deviceIDList {
			if stereopairs.GetDeviceRole(deviceID) != model.NoStereopair {
				stereopair, _ := stereopairs.GetByDeviceID(deviceID)
				// check, if all remaining devices of stereopair are subjects to delete
				for _, stereopairDeviceID := range stereopair.DevicesIDs() {
					if userDevicesIDsMap[stereopairDeviceID] && !deviceIDMap[stereopairDeviceID] {
						return tx, xerrors.Errorf(
							"failed to delete device: device is part of stereopair: %q: %w", stereopair.ID,
							&model.DisassembleStereopairBeforeDeviceDeletionError{})
					}
				}
			}
			if userDevicesIDsMap[deviceID] {
				changeSet = append(changeSet, deviceDeleteChange{ID: deviceID, UserID: userID, Archived: true, ArchivedAt: currentTimestamp})
			}
		}

		if len(changeSet) == 0 {
			ctxlog.Warn(ctx, db.Logger, "Delete changeset is empty")
			return tx, nil
		}

		updateQuery := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $changeset AS List<Struct<
				id: String,
				huid: Uint64,
				archived: Bool,
				archived_at: Timestamp
			>>;

			UPDATE Devices ON
			SELECT * FROM AS_TABLE($changeset);`, db.Prefix)

		queryChangeSetParams := make([]ydb.Value, 0, len(changeSet))
		for _, change := range changeSet {
			queryChangeSetParams = append(queryChangeSetParams, change.toStructValue())
		}

		updateParams := table.NewQueryParameters(
			table.ValueParam("$changeset", ydb.ListValue(queryChangeSetParams...)),
		)
		updateQueryStmt, err := s.Prepare(ctx, updateQuery)
		if err != nil {
			return tx, err
		}
		_, err = tx.ExecuteStatement(ctx, updateQueryStmt, updateParams)
		if err != nil {
			return tx, err
		}

		// Delete DeviceGroups links
		deleteSet := make([]deviceGroupsDeleteChange, 0, len(userDevicesIDsMap))
		for _, deviceID := range deviceIDList {
			if !userDevicesIDsMap[deviceID] {
				continue
			}
			for _, group := range userGroups {
				if slices.Contains(group.Devices, deviceID) {
					deleteSet = append(deleteSet, deviceGroupsDeleteChange{
						UserID:   userID,
						DeviceID: deviceID,
						GroupID:  group.ID,
					})
				}
			}
		}

		if err := db.removeDevicesFromGroupsInTx(ctx, s, tx, userID, deleteSet); err != nil {
			return tx, xerrors.Errorf("failed to clean device groups: %w", err)
		}

		// Updating groups types after device has been archived
		groupsTypesChangeSet := make([]groupChangeSet, 0, len(userGroups))
		for _, group := range userGroups {
			if len(group.Devices) > 0 && tools.ContainsAll(deviceIDList, group.Devices) {
				groupsTypesChangeSet = append(groupsTypesChangeSet, groupChangeSet{ID: group.ID, Name: group.Name, Type: "", UserID: userID, Archived: false})
			}
		}

		if err := db.updateGroupsInTx(ctx, s, tx, groupsTypesChangeSet); err != nil {
			return tx, xerrors.Errorf("failed to update groups types: %w", err)
		}

		return tx, nil
	}

	if len(deviceIDList) == 0 {
		return nil
	}

	err := db.CallInTx(ctx, SerializableReadWrite, deleteFunc)
	if err != nil {
		return xerrors.Errorf("database request has failed: %w", err)
	}

	return nil
}

type deviceDeleteChange struct {
	ID         string
	UserID     uint64
	Archived   bool
	ArchivedAt timestamp.PastTimestamp
}

func (d *deviceDeleteChange) toStructValue() ydb.Value {
	return ydb.StructValue(
		ydb.StructFieldValue("id", ydb.StringValue([]byte(d.ID))),
		ydb.StructFieldValue("huid", ydb.Uint64Value(tools.Huidify(d.UserID))),
		ydb.StructFieldValue("archived", ydb.BoolValue(d.Archived)),
		ydb.StructFieldValue("archived_at", ydb.TimestampValue(d.ArchivedAt.YdbTimestamp())),
	)
}

func (db *DBClient) selectUserDevices(ctx context.Context, criteria DeviceQueryCriteria) ([]model.Device, error) {
	var devices []model.Device
	selectFunc := func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (tx *table.Transaction, err error) {
		tx, devices, err = db.getUserDevicesTx(ctx, s, txControl, criteria)
		return tx, err
	}

	err := db.CallInTx(ctx, SerializableReadWrite, selectFunc)
	if err != nil {
		return devices, xerrors.Errorf("database request has failed: %w", err)
	}

	return devices, nil
}

func (db *DBClient) UpdateUserDeviceType(ctx context.Context, userID uint64, deviceID string, newDeviceType model.DeviceType) error {
	updateFunc := func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
		// Using getUserDevicesTx to start transaction to prevent device type and group changes during current update
		criteria := DeviceQueryCriteria{UserID: userID, DeviceID: deviceID}
		tx, userDevices, err := db.getUserDevicesTx(ctx, s, txControl, criteria)
		if err != nil {
			return tx, err
		}

		if len(userDevices) > 1 {
			return tx, fmt.Errorf("found more than one device using criteria: %#v", criteria)
		}

		if len(userDevices) == 0 {
			return tx, &model.DeviceNotFoundError{}
		}

		device := userDevices[0]

		if allowedTypes, switchable := model.DeviceSwitchTypeMap[device.OriginalType]; !switchable || !tools.Contains(string(newDeviceType), allowedTypes) {
			ctxlog.Warnf(ctx, db.Logger, "Can't switch device type from %s to %s, allowed switches: %v", device.OriginalType, newDeviceType, allowedTypes)
			return tx, &model.DeviceTypeSwitchError{}
		}

		if len(device.Groups) > 0 {
			return tx, &model.GroupListIsNotEmptyError{}
		}

		// Starting update
		updateQuery := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $huid AS Uint64;
			DECLARE $device_type AS String;
			DECLARE $device_id AS String;

			UPDATE
				Devices
			SET
				type = $device_type
			WHERE
				huid == $huid AND
				id == $device_id AND
				archived == false;`, db.Prefix)
		params := table.NewQueryParameters(
			table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
			table.ValueParam("$device_type", ydb.StringValue([]byte(newDeviceType))),
			table.ValueParam("$device_id", ydb.StringValue([]byte(deviceID))),
		)

		updateQueryStmt, err := s.Prepare(ctx, updateQuery)
		if err != nil {
			return tx, err
		}
		_, err = tx.ExecuteStatement(ctx, updateQueryStmt, params)
		return tx, err
	}

	err := db.CallInTx(ctx, SerializableReadWrite, updateFunc)
	if err != nil {
		return xerrors.Errorf("database request has failed: %w", err)
	}

	return nil
}

type deviceConfigChange struct {
	ID     string
	UserID uint64
	Config model.DeviceConfig
}

func (d deviceConfigChange) toStructValue() (ydb.Value, error) {
	configB, err := json.Marshal(d.Config)
	if err != nil {
		return nil, xerrors.Errorf("cannot marshal config: %w", err)
	}
	opts := []ydb.StructValueOption{
		ydb.StructFieldValue("huid", ydb.Uint64Value(tools.Huidify(d.UserID))),
		ydb.StructFieldValue("id", ydb.StringValue([]byte(d.ID))),
		ydb.StructFieldValue("internal_config", ydb.JSONValue(string(configB))),
	}

	return ydb.StructValue(opts...), nil
}

func (db *DBClient) StoreUserDeviceConfigs(ctx context.Context, userID uint64, deviceConfigs model.DeviceConfigs) error {
	changeSet := make([]deviceConfigChange, 0, len(deviceConfigs))
	for deviceID, config := range deviceConfigs {
		changeSet = append(changeSet, deviceConfigChange{
			ID:     deviceID,
			UserID: userID,
			Config: config,
		})
	}

	updateQuery := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $changeset AS List<Struct<
				huid: Uint64,
				id: String,
				internal_config: Json>
			>;

			UPDATE Devices ON
			SELECT * FROM AS_TABLE($changeset);`, db.Prefix)

	queryChangeSetParams := make([]ydb.Value, 0, len(changeSet))
	for _, change := range changeSet {
		structValue, err := change.toStructValue()
		if err != nil {
			return xerrors.Errorf("failed to get struct value from deviceConfigChange: %w", err)
		}
		queryChangeSetParams = append(queryChangeSetParams, structValue)
	}

	updateParams := table.NewQueryParameters(
		table.ValueParam("$changeset", ydb.ListValue(queryChangeSetParams...)),
	)

	if err := db.Write(ctx, updateQuery, updateParams); err != nil {
		return xerrors.Errorf("failed to update user %d device configs: %w", userID, err)
	}

	return nil
}

func (db *DBClient) getUserDevicesTx(ctx context.Context, s *table.Session, tc *table.TransactionControl, criteria DeviceQueryCriteria) (*table.Transaction, model.Devices, error) {
	var queryB bytes.Buffer
	var devices model.Devices

	// xxx(galecore): is joining favorites inside really necessary?
	if err := SelectUserDevicesTemplate.Execute(&queryB, criteria); err != nil {
		return nil, devices, err
	}

	query := fmt.Sprintf(queryB.String(), db.Prefix)
	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(criteria.UserID))),
		table.ValueParam("$group_id", ydb.StringValue([]byte(criteria.GroupID))),
		table.ValueParam("$device_id", ydb.StringValue([]byte(criteria.DeviceID))),
		table.ValueParam("$skill_id", ydb.StringValue([]byte(criteria.SkillID))),
		table.ValueParam("$household_id", ydb.StringValue([]byte(criteria.HouseholdID))),
		table.ValueParam("$room_id", ydb.StringValue([]byte(criteria.RoomID))),
	)

	stmt, err := s.Prepare(ctx, query)
	if err != nil {
		return nil, devices, err
	}

	tx, res, err := stmt.Execute(ctx, tc, params)
	defer func() {
		if err != nil {
			db.RollbackTransaction(ctx, tx, err)
		}
	}()
	if err != nil {
		return nil, devices, err
	}

	for res.NextSet() {
		for res.NextRow() {
			device, err := db.parseDevice(res)
			if err != nil {
				ctxlog.Errorf(ctx, db.Logger, err.Error())
				return nil, devices, err
			}

			if res.Err() != nil {
				err := xerrors.Errorf("failed to parse YDB response row: %w", res.Err())
				ctxlog.Error(ctx, db.Logger, err.Error())
				return nil, devices, err
			}

			devices = append(devices, device)
		}
	}

	if criteria.WithShared {
		sharedDevices, err := db.selectSharedDevicesInTx(ctx, s, tx, criteria)
		if err != nil {
			return nil, nil, xerrors.Errorf("failed to select shared devices in tx: %w", err)
		}
		devices = append(devices, sharedDevices...)
	}

	return tx, devices, nil
}

func (db *DBClient) getUserDevicesInTx(ctx context.Context, s *table.Session, tx *table.Transaction, criteria DeviceQueryCriteria) (model.Devices, error) {
	var devices model.Devices
	var queryB bytes.Buffer

	if err := SelectUserDevicesTemplate.Execute(&queryB, criteria); err != nil {
		return devices, err
	}

	query := fmt.Sprintf(queryB.String(), db.Prefix)
	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(criteria.UserID))),
		table.ValueParam("$group_id", ydb.StringValue([]byte(criteria.GroupID))),
		table.ValueParam("$device_id", ydb.StringValue([]byte(criteria.DeviceID))),
		table.ValueParam("$skill_id", ydb.StringValue([]byte(criteria.SkillID))),
		table.ValueParam("$household_id", ydb.StringValue([]byte(criteria.HouseholdID))),
		table.ValueParam("$room_id", ydb.StringValue([]byte(criteria.RoomID))),
	)

	stmt, err := s.Prepare(ctx, query)
	if err != nil {
		return devices, err
	}

	res, err := tx.ExecuteStatement(ctx, stmt, params)
	if err != nil {
		return devices, err
	}

	for res.NextSet() {
		for res.NextRow() {
			device, err := db.parseDevice(res)
			if err != nil {
				ctxlog.Errorf(ctx, db.Logger, err.Error())
				return devices, err
			}

			if res.Err() != nil {
				err := xerrors.Errorf("failed to parse YDB response row: %w", res.Err())
				ctxlog.Error(ctx, db.Logger, err.Error())
				return devices, err
			}

			devices = append(devices, device)
		}
	}

	if criteria.WithShared {
		sharedDevices, err := db.selectSharedDevicesInTx(ctx, s, tx, criteria)
		if err != nil {
			return nil, xerrors.Errorf("failed to select shared devices in tx: %w", err)
		}
		devices = append(devices, sharedDevices...)
	}

	return devices, nil
}

func (db *DBClient) selectUserDevicesSimple(ctx context.Context, criteria DeviceQueryCriteria) (model.Devices, error) {
	var devices []model.Device
	selectFunc := func(ctx context.Context, session *table.Session, txControl *table.TransactionControl) (tx *table.Transaction, err error) {
		tx, devices, err = db.getUserDevicesSimpleTx(ctx, session, txControl, criteria)
		return tx, err
	}

	if err := db.CallInTx(ctx, SerializableReadWrite, selectFunc); err != nil {
		return devices, xerrors.Errorf("database request has failed: %w", err)
	}

	return devices, nil
}

func (db *DBClient) getUserDevicesSimpleTx(ctx context.Context, s *table.Session, tc *table.TransactionControl, criteria DeviceQueryCriteria) (*table.Transaction, model.Devices, error) {
	var queryB bytes.Buffer
	var devices model.Devices
	if err := SelectUserDevicesSimpleTemplate.Execute(&queryB, criteria); err != nil {
		return nil, nil, err
	}

	query := fmt.Sprintf(queryB.String(), db.Prefix)
	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(criteria.UserID))),
		table.ValueParam("$device_id", ydb.StringValue([]byte(criteria.DeviceID))),
		table.ValueParam("$skill_id", ydb.StringValue([]byte(criteria.SkillID))),
		table.ValueParam("$household_id", ydb.StringValue([]byte(criteria.HouseholdID))),
		table.ValueParam("$room_id", ydb.StringValue([]byte(criteria.RoomID))),
	)

	stmt, err := s.Prepare(ctx, query)
	if err != nil {
		return nil, nil, err
	}

	tx, res, err := stmt.Execute(ctx, tc, params)
	defer func() {
		if err != nil {
			db.RollbackTransaction(ctx, tx, err)
		}
	}()
	if err != nil {
		return tx, nil, err
	}

	if !res.NextSet() {
		return tx, nil, xerrors.New("result set not found")
	}

	for res.NextRow() {
		device, err := db.parseDeviceSimple(res)
		if err != nil {
			ctxlog.Errorf(ctx, db.Logger, err.Error())
			return tx, devices, err
		}

		if res.Err() != nil {
			err := xerrors.Errorf("failed to parse YDB response row: %w", res.Err())
			ctxlog.Error(ctx, db.Logger, err.Error())
			return tx, nil, err
		}

		devices = append(devices, device)
	}

	if criteria.WithShared {
		sharedDevices, err := db.selectSharedDevicesSimpleInTx(ctx, s, tx, criteria)
		if err != nil {
			return tx, nil, xerrors.Errorf("failed to select shared devices in tx: %w", err)
		}
		devices = append(devices, sharedDevices...)
	}

	return tx, devices, nil
}

func (db *DBClient) getUserDevicesSimpleInTx(ctx context.Context, s *table.Session, tx *table.Transaction, criteria DeviceQueryCriteria) (model.Devices, error) {
	var queryB bytes.Buffer
	var devices model.Devices

	if err := SelectUserDevicesSimpleTemplate.Execute(&queryB, criteria); err != nil {
		return devices, err
	}

	query := fmt.Sprintf(queryB.String(), db.Prefix)
	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(criteria.UserID))),
		table.ValueParam("$group_id", ydb.StringValue([]byte(criteria.GroupID))),
		table.ValueParam("$device_id", ydb.StringValue([]byte(criteria.DeviceID))),
		table.ValueParam("$skill_id", ydb.StringValue([]byte(criteria.SkillID))),
		table.ValueParam("$household_id", ydb.StringValue([]byte(criteria.HouseholdID))),
		table.ValueParam("$room_id", ydb.StringValue([]byte(criteria.RoomID))),
	)

	stmt, err := s.Prepare(ctx, query)
	if err != nil {
		return devices, err
	}

	res, err := tx.ExecuteStatement(ctx, stmt, params)
	if err != nil {
		return devices, err
	}

	for res.NextSet() {
		for res.NextRow() {
			device, err := db.parseDeviceSimple(res)
			if err != nil {
				ctxlog.Errorf(ctx, db.Logger, err.Error())
				return devices, err
			}

			if res.Err() != nil {
				err := xerrors.Errorf("failed to parse YDB response row: %w", res.Err())
				ctxlog.Error(ctx, db.Logger, err.Error())
				return devices, err
			}

			devices = append(devices, device)
		}
	}

	if criteria.WithShared {
		sharedDevices, err := db.selectSharedDevicesSimpleInTx(ctx, s, tx, criteria)
		if err != nil {
			return nil, xerrors.Errorf("failed to select shared devices in tx: %w", err)
		}
		devices = append(devices, sharedDevices...)
	}

	return devices, nil
}

func (db *DBClient) parseDeviceSimple(res *table.Result) (model.Device, error) {
	var device model.Device

	res.NextItem()
	device.ID = string(res.OString())
	res.NextItem()
	device.Name = string(res.OString())

	res.NextItem()
	rawJSONAliases := res.OJSON()
	device.Aliases = make([]string, 0)
	if len(rawJSONAliases) != 0 {
		var aliases []string
		if err := json.Unmarshal([]byte(rawJSONAliases), &aliases); err != nil {
			return model.Device{}, err
		}
		device.Aliases = aliases
	}

	res.NextItem()
	device.ExternalID = string(res.OString())
	res.NextItem()
	device.ExternalName = string(res.OString())
	res.NextItem()
	device.Type = model.DeviceType(res.OString())
	res.NextItem()
	device.OriginalType = model.DeviceType(res.OString())
	res.NextItem()
	device.SkillID = string(res.OString())

	// capabilities
	res.NextItem()
	capabilitiesRaw := res.OJSON()
	if len(capabilitiesRaw) > 0 {
		capabilities, err := model.JSONUnmarshalCapabilities(json.RawMessage(capabilitiesRaw))
		if err != nil {
			return model.Device{}, xerrors.Errorf("failed to parse `capabilities` field for device %s: %w", device.ID, err)
		}
		device.Capabilities = capabilities
	}

	// properties
	res.NextItem()
	propertiesRaw := res.OJSON()
	if len(propertiesRaw) > 0 {
		properties, err := model.JSONUnmarshalProperties(json.RawMessage(propertiesRaw))
		if err != nil {
			return model.Device{}, xerrors.Errorf("failed to parse `properties` field for device %s: %w", device.ID, err)
		}
		device.Properties = properties
	}

	// custom_data
	res.NextItem()
	customDataRaw := res.OJSON()
	if len(customDataRaw) > 0 {
		var customData interface{}
		if err := json.Unmarshal([]byte(customDataRaw), &customData); err != nil {
			return model.Device{}, xerrors.Errorf("failed to parse `custom_data` field for device %s: %w", device.ID, err)
		}
		device.CustomData = customData
	} else {
		device.CustomData = nil
	}

	// device_info
	res.NextItem()
	deviceInfo := res.OJSON()
	if len(deviceInfo) > 0 {
		device.DeviceInfo = &model.DeviceInfo{}
		if err := json.Unmarshal([]byte(deviceInfo), device.DeviceInfo); err != nil {
			return model.Device{}, xerrors.Errorf("failed to parse `device_info` field for device %s: %w", device.ID, err)
		}
	}

	// room_id
	res.NextItem()
	roomID := string(res.OString())
	if roomID != "" {
		device.Room = &model.Room{ID: roomID}
	}

	// updated timestamp
	res.NextItem()
	updated := res.OTimestamp()
	if updated != 0 {
		device.Updated = timestamp.FromMicro(updated)
	}

	// created Timestamp
	res.NextItem()
	created := res.OTimestamp()
	if created != 0 {
		device.Created = timestamp.FromMicro(created)
	}

	// household_id
	res.NextItem()
	device.HouseholdID = string(res.OString())

	// status
	res.NextItem()
	statusJSON := res.OJSON()
	device.Status = model.UnknownDeviceStatus
	if len(statusJSON) > 0 {
		var status deviceStatus
		if err := json.Unmarshal([]byte(statusJSON), &status); err != nil {
			return model.Device{}, xerrors.Errorf("failed to parse `status` field for device %s: %w", device.ID, err)
		}
		device.Status = model.DeviceStatus(status.Status)
		device.StatusUpdated = timestamp.FromMicro(status.Updated)
	}

	// internal_config
	res.NextItem()
	internalConfigJSON := res.OJSON()
	if len(internalConfigJSON) > 0 {
		if err := json.Unmarshal([]byte(internalConfigJSON), &device.InternalConfig); err != nil {
			return model.Device{}, xerrors.Errorf("failed to parse `internal_config` field for device %s: %w", device.ID, err)
		}
	}
	return device, nil
}

func (db *DBClient) parseDevice(res *table.Result) (model.Device, error) {
	var device model.Device
	res.SeekItem("id")
	device.ID = string(res.OString())
	res.NextItem()
	device.Name = string(res.OString())

	res.NextItem()
	rawJSONAliases := res.OJSON()
	device.Aliases = make([]string, 0)
	if len(rawJSONAliases) != 0 {
		var aliases []string
		if err := json.Unmarshal([]byte(rawJSONAliases), &aliases); err != nil {
			return model.Device{}, err
		}
		device.Aliases = aliases
	}

	res.NextItem()
	device.ExternalID = string(res.OString())
	res.NextItem()
	device.ExternalName = string(res.OString())
	res.NextItem()
	device.Type = model.DeviceType(res.OString())
	res.NextItem()
	device.OriginalType = model.DeviceType(res.OString())
	res.NextItem()
	device.SkillID = string(res.OString())

	//Room
	res.NextItem()
	roomID := string(res.OString())
	res.NextItem()
	roomName := string(res.OString())
	if len(roomID) > 0 {
		device.Room = &model.Room{ID: roomID, Name: roomName}
	}

	// capabilities
	res.NextItem()
	capabilitiesRaw := res.OJSON()
	if len(capabilitiesRaw) > 0 {
		capabilities, err := model.JSONUnmarshalCapabilities(json.RawMessage(capabilitiesRaw))
		if err != nil {
			return model.Device{}, xerrors.Errorf("failed to parse `capabilities` field for device %s: %w", device.ID, err)
		}
		device.Capabilities = capabilities
	}

	// properties
	res.NextItem()
	propertiesRaw := res.OJSON()
	if len(propertiesRaw) > 0 {
		properties, err := model.JSONUnmarshalProperties(json.RawMessage(propertiesRaw))
		if err != nil {
			return model.Device{}, xerrors.Errorf("failed to parse `properties` field for device %s: %w", device.ID, err)
		}
		device.Properties = properties
	}

	// custom_data
	res.NextItem()
	customDataRaw := res.OJSON()
	if len(customDataRaw) > 0 {
		var customData interface{}
		if err := json.Unmarshal([]byte(customDataRaw), &customData); err != nil {
			return model.Device{}, xerrors.Errorf("failed to parse `custom_data` field for device %s: %w", device.ID, err)
		}
		device.CustomData = customData
	} else {
		device.CustomData = nil
	}

	// groups
	res.NextItem()
	var groups []model.Group
	if !res.IsNull() {
		//query depends on template parameters
		if res.IsOptional() {
			res.Unwrap()
		}
		for i, n := 0, res.ListIn(); i < n; i++ {
			var group model.Group
			res.ListItem(i)
			for i, n := 0, res.StructIn(); i < n; i++ {
				switch res.StructField(i) {
				case "id":
					group.ID = string(res.OString())
				case "name":
					group.Name = string(res.OString())
				case "aliases":
					rawJSONAliases := res.OJSON()
					group.Aliases = make([]string, 0)
					if len(rawJSONAliases) != 0 {
						var aliases []string
						if err := json.Unmarshal([]byte(rawJSONAliases), &aliases); err != nil {
							return model.Device{}, err
						}
						group.Aliases = aliases
					}
				case "type":
					group.Type = model.DeviceType(res.OString())
				}
			}
			res.StructOut()
			groups = append(groups, group)
		}
		res.ListOut()
	}
	device.Groups = groups

	// device_info
	res.NextItem()
	deviceInfo := res.OJSON()
	if len(deviceInfo) > 0 {
		device.DeviceInfo = &model.DeviceInfo{}
		if err := json.Unmarshal([]byte(deviceInfo), device.DeviceInfo); err != nil {
			return model.Device{}, xerrors.Errorf("failed to parse `device_info` field for device %s: %w", device.ID, err)
		}
	}

	// TODO: Hide all optional reading behind if
	// updated Timestamp
	res.NextItem()
	updated := res.OTimestamp()
	if updated != 0 {
		device.Updated = timestamp.FromMicro(updated)
	}

	// created Timestamp
	res.NextItem()
	created := res.OTimestamp()
	if created != 0 {
		device.Created = timestamp.FromMicro(created)
	}

	// household_id
	res.NextItem()
	device.HouseholdID = string(res.OString())

	res.NextItem()
	statusJSON := res.OJSON()
	device.Status = model.UnknownDeviceStatus
	if len(statusJSON) > 0 {
		var status deviceStatus
		if err := json.Unmarshal([]byte(statusJSON), &status); err != nil {
			return model.Device{}, xerrors.Errorf("failed to parse `status` field for device %s: %w", device.ID, err)
		}
		device.Status = model.DeviceStatus(status.Status)
		device.StatusUpdated = timestamp.FromMicro(status.Updated)
	}

	// favorite
	res.NextItem()
	inFavoritesID := res.OString()
	device.Favorite = len(inFavoritesID) > 0

	// internal_config
	res.NextItem()
	internalConfigJSON := res.OJSON()
	if len(internalConfigJSON) > 0 {
		if err := json.Unmarshal([]byte(internalConfigJSON), &device.InternalConfig); err != nil {
			return model.Device{}, xerrors.Errorf("failed to parse `internal_config` field for device %s: %w", device.ID, err)
		}
	}
	return device, nil
}

type deviceStatus struct {
	Status  string `json:"status"`
	Updated uint64 `json:"updated"`
}

func formatDeviceStatus(d model.Device) deviceStatus {
	if d.Status == "" {
		d.Status = model.UnknownDeviceStatus
	}
	return deviceStatus{
		Status:  string(d.Status),
		Updated: d.StatusUpdated.YdbTimestamp(),
	}
}
