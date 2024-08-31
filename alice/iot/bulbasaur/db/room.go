package db

import (
	"bytes"
	"context"
	"fmt"

	"github.com/gofrs/uuid"
	"golang.org/x/exp/slices"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type RoomQueryCriteria struct {
	RoomID      string
	UserID      uint64
	HouseholdID string
	WithShared  bool // that flag allows to select rooms that were shared with user
}

func (db *DBClient) SelectUserRooms(ctx context.Context, userID uint64) ([]model.Room, error) {
	criteria := RoomQueryCriteria{UserID: userID, WithShared: true}
	return db.selectUserRooms(ctx, criteria)
}

func (db *DBClient) SelectUserRoom(ctx context.Context, userID uint64, roomID string) (model.Room, error) {
	criteria := RoomQueryCriteria{UserID: userID, RoomID: roomID, WithShared: true}
	result, err := db.selectUserRooms(ctx, criteria)
	if err != nil {
		return model.Room{}, xerrors.Errorf("cannot get user room from database: %w", err)
	}
	if len(result) > 1 {
		return model.Room{}, fmt.Errorf("found more than one room using criteria: %#v", criteria)
	}

	if len(result) == 0 {
		return model.Room{}, &model.RoomNotFoundError{}
	}
	return result[0], nil
}

func (db *DBClient) SelectUserHouseholdRooms(ctx context.Context, userID uint64, householdID string) (model.Rooms, error) {
	criteria := RoomQueryCriteria{UserID: userID, HouseholdID: householdID, WithShared: true}
	return db.selectUserRooms(ctx, criteria)
}

func (db *DBClient) UpdateUserRoomName(ctx context.Context, user model.User, roomID string, name string) error {
	updateFunc := func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
		tx, currentHousehold, err := db.selectCurrentHouseholdTx(ctx, s, txControl, CurrentHouseholdQueryCriteria{UserID: user.ID})
		if err != nil {
			return tx, xerrors.Errorf("failed to select current household: %w", err)
		}
		criteria := RoomQueryCriteria{UserID: user.ID, HouseholdID: currentHousehold.ID}
		userRooms, err := db.getUserRoomsInTx(ctx, s, tx, criteria)
		if err != nil {
			return tx, err
		}

		checkRoom := model.Room{ID: roomID, Name: name}
		if err = checkRoom.ValidateName(userRooms); err != nil {
			return tx, err
		}

		var roomExists bool
		for _, userRoom := range userRooms {
			if userRoom.ID == roomID {
				roomExists = true
			}
		}
		if !roomExists {
			return tx, &model.RoomNotFoundError{}
		}

		updateQuery := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $huid AS Uint64;
			DECLARE $room_id AS String;
			DECLARE $room_name AS String;

			UPDATE
				Rooms
			SET
				name = $room_name
			WHERE
				huid == $huid AND
				id == $room_id;`, db.Prefix)
		params := table.NewQueryParameters(
			table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(user.ID))),
			table.ValueParam("$room_name", ydb.StringValue([]byte(tools.StandardizeSpaces(name)))),
			table.ValueParam("$room_id", ydb.StringValue([]byte(roomID))),
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

func (db *DBClient) UpdateUserDeviceRoom(ctx context.Context, userID uint64, deviceID string, roomID string) error {
	updateFunc := func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
		// Using getUserDevicesSimpleTx to start transaction to prevent device room changes during current update
		// Also we check that user is an owner of device with id = deviceID
		criteria := DeviceQueryCriteria{UserID: userID, DeviceID: deviceID}
		tx, userDevices, err := db.getUserDevicesSimpleTx(ctx, s, txControl, criteria)
		if err != nil {
			return tx, err
		}

		if len(userDevices) == 0 {
			return tx, &model.DeviceNotFoundError{}
		}
		if len(userDevices) > 1 {
			return tx, fmt.Errorf("found more than one device using criteria: %#v", criteria)
		}
		userDevice := userDevices[0]
		roomCriteria := RoomQueryCriteria{UserID: userID, RoomID: roomID}
		userRooms, err := db.getUserRoomsInTx(ctx, s, tx, roomCriteria)
		if err != nil {
			return tx, xerrors.Errorf("failed to get user %d rooms in tx: %w", userID, err)
		}
		if len(userRooms) == 0 {
			return tx, &model.RoomNotFoundError{}
		}
		userRoom := userRooms[0]
		// if we are moving device from one household to another
		if userDevice.HouseholdID != userRoom.HouseholdID {
			if err := db.removeDeviceFromAllGroupsInTx(ctx, s, tx, userID, userDevice.ID); err != nil {
				return tx, xerrors.Errorf("failed to remove device %s from all groups: %w", deviceID, err)
			}
		}

		// Starting update
		updateQuery := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $huid AS Uint64;
			DECLARE $room_id AS String;
			DECLARE $device_id AS String;
			DECLARE $household_id AS Optional<String>;

			UPDATE
				Devices
			SET
				room_id = $room_id,
				household_id = $household_id
			WHERE
				huid == $huid AND
				id == $device_id AND
				archived == false;`, db.Prefix)
		params := table.NewQueryParameters(
			table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
			table.ValueParam("$room_id", ydb.StringValue([]byte(roomID))),
			table.ValueParam("$device_id", ydb.StringValue([]byte(deviceID))),
			table.ValueParam("$household_id", ydb.OptionalValue(ydb.StringValue([]byte(userRoom.HouseholdID)))),
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

// in room creating we technically can move devices from one household to another
func (db *DBClient) CreateUserRoom(ctx context.Context, user model.User, room model.Room) (roomID string, err error) {
	createFunc := func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
		tx, currentHousehold, err := db.selectCurrentHouseholdTx(ctx, s, txControl, CurrentHouseholdQueryCriteria{UserID: user.ID})
		if err != nil {
			return tx, err
		}
		var roomHouseholdID string
		if room.HouseholdID != "" {
			roomHouseholdID = room.HouseholdID
		} else {
			roomHouseholdID = currentHousehold.ID
		}
		criteria := RoomQueryCriteria{UserID: user.ID, HouseholdID: roomHouseholdID}
		userRooms, err := db.getUserRoomsInTx(ctx, s, tx, criteria)
		if err != nil {
			return tx, err
		}

		householdExists, err := db.checkUserHouseholdExistInTx(ctx, s, tx, HouseholdQueryCriteria{UserID: user.ID, HouseholdID: roomHouseholdID})
		if err != nil {
			return tx, err
		}
		if !householdExists {
			return tx, &model.UserHouseholdNotFoundError{}
		}

		room.ID = ""
		if err := room.ValidateName(userRooms); err != nil {
			return tx, err
		}

		uid, err := uuid.NewV4()
		if err != nil {
			ctxlog.Warnf(ctx, db.Logger, "failed to generate UUID: %v", err)
		}
		roomID = uid.String()

		upsertQuery := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $uid AS Uint64;
			DECLARE $huid AS Uint64;
			DECLARE $room_id AS String;
			DECLARE $room_name AS String;
			DECLARE $timestamp AS Timestamp;
			DECLARE $household_id AS Optional<String>;

			UPSERT INTO
				Rooms (id, huid, name, household_id, user_id, archived, created)
			VALUES
				($room_id, $huid, $room_name, $household_id, $uid, false, $timestamp);`, db.Prefix)
		params := table.NewQueryParameters(
			table.ValueParam("$uid", ydb.Uint64Value(user.ID)),
			table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(user.ID))),
			table.ValueParam("$room_name", ydb.StringValue([]byte(tools.StandardizeSpaces(room.Name)))),
			table.ValueParam("$room_id", ydb.StringValue([]byte(roomID))),
			table.ValueParam("$household_id", ydb.OptionalValue(ydb.StringValue([]byte(roomHouseholdID)))),
			table.ValueParam("$timestamp", ydb.TimestampValue(db.CurrentTimestamp().YdbTimestamp())),
		)
		upsertQueryStmt, err := s.Prepare(ctx, upsertQuery)
		if err != nil {
			return tx, err
		}
		_, err = tx.ExecuteStatement(ctx, upsertQueryStmt, params)
		if err != nil {
			return tx, err
		}

		_, userDevices, err := db.getUserDevicesSimpleTx(ctx, s, table.TxControl(table.WithTx(tx)), DeviceQueryCriteria{UserID: user.ID})
		if err != nil {
			return tx, xerrors.Errorf("failed to get user %d devices in tx: %w", user.ID, err)
		}
		userDevicesMap := userDevices.ToMap()
		// find devices from another household
		otherHouseholdDevices := make([]string, 0, len(room.Devices))
		for _, deviceID := range room.Devices {
			if device, exist := userDevicesMap[deviceID]; exist && device.HouseholdID != roomHouseholdID {
				otherHouseholdDevices = append(otherHouseholdDevices, deviceID)
			}
		}
		// remove devices from groups of previous household
		err = db.removeDevicesFromAllGroupsInTx(ctx, s, tx, user.ID, otherHouseholdDevices)
		if err != nil {
			return tx, xerrors.Errorf("failed to remove devices from all groups: %w", err)
		}
		roomDeviceChangeSet := make([]roomDeviceChange, 0, len(room.Devices))
		for _, deviceID := range room.Devices {
			roomDeviceChangeSet = append(roomDeviceChangeSet, roomDeviceChange{
				UserID:      user.ID,
				DeviceID:    deviceID,
				RoomID:      &roomID,
				HouseholdID: roomHouseholdID,
			})
		}
		err = db.updateRoomDevicesInTx(ctx, s, tx, roomDeviceChangeSet)
		if err != nil {
			return tx, xerrors.Errorf("failed to update room devices: %w", err)
		}

		return tx, nil
	}

	err = db.CallInTx(ctx, SerializableReadWrite, createFunc)
	if err != nil {
		return "", xerrors.Errorf("database request has failed: %w", err)
	}

	return roomID, nil
}

// in room updating we technically can move devices from one household to another
func (db *DBClient) UpdateUserRoomNameAndDevices(ctx context.Context, user model.User, room model.Room) error {
	updateFunc := func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
		roomHouseholdID := room.HouseholdID
		criteria := RoomQueryCriteria{UserID: user.ID, HouseholdID: roomHouseholdID}
		tx, userRooms, err := db.getUserRoomsTx(ctx, s, txControl, criteria)
		if err != nil {
			return tx, err
		}
		if err := room.ValidateName(userRooms); err != nil {
			return tx, err
		}
		var previousRoom *model.Room
		for i, userRoom := range userRooms {
			if userRoom.ID == room.ID {
				previousRoom = &userRooms[i]
			}
		}
		if previousRoom == nil {
			return tx, &model.RoomNotFoundError{}
		}

		var householdID ydb.Value
		if roomHouseholdID != "" {
			householdID = ydb.OptionalValue(ydb.StringValue([]byte(roomHouseholdID)))
		} else {
			householdID = ydb.NullValue(ydb.TypeString)
		}

		upsertQuery := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $huid AS Uint64;
			DECLARE $room_id AS String;
			DECLARE $room_name AS String;
			DECLARE $household_id AS Optional<String>;

			UPSERT INTO
				Rooms (id, huid, name, household_id)
			VALUES
				($room_id, $huid, $room_name, $household_id);`, db.Prefix)
		params := table.NewQueryParameters(
			table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(user.ID))),
			table.ValueParam("$room_name", ydb.StringValue([]byte(tools.StandardizeSpaces(room.Name)))),
			table.ValueParam("$room_id", ydb.StringValue([]byte(room.ID))),
			table.ValueParam("$household_id", householdID),
		)
		upsertQueryStmt, err := s.Prepare(ctx, upsertQuery)
		if err != nil {
			return tx, err
		}
		_, err = tx.ExecuteStatement(ctx, upsertQueryStmt, params)
		if err != nil {
			return tx, err
		}

		_, userDevices, err := db.getUserDevicesSimpleTx(ctx, s, table.TxControl(table.WithTx(tx)), DeviceQueryCriteria{UserID: user.ID})
		if err != nil {
			return tx, xerrors.Errorf("failed to get user %d devices in tx: %w", user.ID, err)
		}
		userDevicesMap := userDevices.ToMap()
		// find devices from another household
		// TODO: IOT-863: Use callables and filter
		otherHouseholdDevices := make([]string, 0, len(room.Devices))
		for _, deviceID := range room.Devices {
			if device, exist := userDevicesMap[deviceID]; exist && device.HouseholdID != roomHouseholdID {
				otherHouseholdDevices = append(otherHouseholdDevices, deviceID)
			}
		}
		// remove devices from groups of previous household
		err = db.removeDevicesFromAllGroupsInTx(ctx, s, tx, user.ID, otherHouseholdDevices)
		if err != nil {
			return tx, xerrors.Errorf("failed to remove devices from all groups: %w", err)
		}
		roomDeviceChangeSet := make([]roomDeviceChange, 0, len(room.Devices))
		for _, deviceID := range room.Devices {
			roomDeviceChangeSet = append(roomDeviceChangeSet, roomDeviceChange{
				UserID:      user.ID,
				DeviceID:    deviceID,
				RoomID:      &room.ID,
				HouseholdID: roomHouseholdID,
			})
		}
		// we need to set no room to all devices that was deleted from room
		for _, deviceID := range previousRoom.Devices {
			if !slices.Contains(room.Devices, deviceID) {
				roomDeviceChangeSet = append(roomDeviceChangeSet, roomDeviceChange{
					UserID:      user.ID,
					DeviceID:    deviceID,
					HouseholdID: roomHouseholdID,
				})
			}
		}
		err = db.updateRoomDevicesInTx(ctx, s, tx, roomDeviceChangeSet)
		if err != nil {
			return tx, xerrors.Errorf("failed to update room devices: %w", err)
		}

		return tx, nil
	}

	err := db.CallInTx(ctx, SerializableReadWrite, updateFunc)
	if err != nil {
		return xerrors.Errorf("database request has failed: %w", err)
	}

	return nil
}

func (db *DBClient) DeleteUserRoom(ctx context.Context, userID uint64, roomID string) error {
	updateFunc := func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
		deleteRoomQuery := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $room_id AS String;
			DECLARE $huid AS Uint64;

			UPDATE
				Rooms
			SET
				archived = true
			WHERE
				huid == $huid AND
				archived == false AND
				id == $room_id;`, db.Prefix)
		params := table.NewQueryParameters(
			table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
			table.ValueParam("$room_id", ydb.StringValue([]byte(roomID))),
		)

		stmt, err := s.Prepare(ctx, deleteRoomQuery)
		if err != nil {
			return nil, err
		}

		tx, _, err := stmt.Execute(ctx, txControl, params)
		return tx, err
	}

	err := db.CallInTx(ctx, SerializableReadWrite, updateFunc)
	if err != nil {
		return xerrors.Errorf("database request has failed: %w", err)
	}

	return nil
}

type roomDeviceChange struct {
	UserID      uint64
	DeviceID    string
	RoomID      *string
	HouseholdID string
}

func (c roomDeviceChange) ToYDBValue() ydb.Value {
	var roomID ydb.Value
	if c.RoomID != nil {
		roomID = ydb.OptionalValue(ydb.StringValue([]byte(*c.RoomID)))
	} else {
		roomID = ydb.NullValue(ydb.TypeString)
	}
	return ydb.StructValue(
		ydb.StructFieldValue("huid", ydb.Uint64Value(tools.Huidify(c.UserID))),
		ydb.StructFieldValue("id", ydb.StringValue([]byte(c.DeviceID))),
		ydb.StructFieldValue("room_id", roomID),
		ydb.StructFieldValue("household_id", ydb.StringValue([]byte(c.HouseholdID))),
	)
}

func (db *DBClient) updateRoomDevicesInTx(ctx context.Context, s *table.Session, tx *table.Transaction, changeSet []roomDeviceChange) error {
	if len(changeSet) == 0 {
		return nil
	}
	upsertQuery := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");

			DECLARE $values AS List<Struct<
				huid: Uint64,
				id: String,
				room_id: Optional<String>,
				household_id: String
			>>;

			UPSERT INTO
				Devices ( huid, id, room_id, household_id)
			SELECT
				huid, id, room_id, household_id
			FROM AS_TABLE($values);
		`, db.Prefix)
	values := make([]ydb.Value, 0, len(changeSet))
	for _, roomDevice := range changeSet {
		values = append(values, roomDevice.ToYDBValue())
	}
	upsertParams := table.NewQueryParameters(
		table.ValueParam("$values", ydb.ListValue(values...)),
	)
	upsertQueryStmt, err := s.Prepare(ctx, upsertQuery)
	if err != nil {
		return err
	}
	_, err = tx.ExecuteStatement(ctx, upsertQueryStmt, upsertParams)
	if err != nil {
		return err
	}
	return nil
}

func (db *DBClient) selectUserRoomsSimple(ctx context.Context, criteria RoomQueryCriteria) (model.Rooms, error) {
	var queryB bytes.Buffer
	if err := SelectUserRoomsSimpleTemplate.Execute(&queryB, criteria); err != nil {
		return nil, err
	}

	query := fmt.Sprintf(queryB.String(), db.Prefix)

	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(criteria.UserID))),
		table.ValueParam("$room_id", ydb.StringValue([]byte(criteria.RoomID))),
		table.ValueParam("$household_id", ydb.StringValue([]byte(criteria.HouseholdID))),
	)

	res, err := db.Read(ctx, query, params)
	if err != nil {
		return nil, err
	}

	if !res.NextSet() {
		return nil, xerrors.New("result set not found")
	}

	rooms := make([]model.Room, 0, res.SetRowCount())
	for res.NextRow() {
		room := model.Room{Devices: []string{}}

		res.NextItem()
		room.ID = string(res.OString())
		res.NextItem()
		room.Name = string(res.OString())
		res.NextItem()
		room.HouseholdID = string(res.OString())

		if res.Err() != nil {
			err := xerrors.Errorf("failed to parse YDB response row: %w", res.Err())
			ctxlog.Error(ctx, db.Logger, err.Error())
			return nil, err
		}

		rooms = append(rooms, room)
	}

	return rooms, nil
}

func (db *DBClient) selectUserRooms(ctx context.Context, criteria RoomQueryCriteria) (model.Rooms, error) {
	var rooms model.Rooms
	selectFunc := func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (tx *table.Transaction, err error) {
		tx, rooms, err = db.getUserRoomsTx(ctx, s, txControl, criteria)
		return tx, err
	}

	err := db.CallInTx(ctx, SerializableReadWrite, selectFunc)
	if err != nil {
		return rooms, xerrors.Errorf("database request has failed: %w", err)
	}

	return rooms, nil
}

func (db *DBClient) insertDeviceRoomInTx(ctx context.Context, s *table.Session, tx *table.Transaction, userID uint64, roomName string, householdID string) (string, error) {
	newRoomID, err := uuid.NewV4()
	if err != nil {
		return "", err
	}
	insertRoomQuery := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");
		DECLARE $room_id AS String;
		DECLARE $room_name AS String;
		DECLARE $user_id AS Uint64;
		DECLARE $huid AS Uint64;
		DECLARE $timestamp AS Timestamp;
		DECLARE $household_id AS Optional<String>;

		UPSERT INTO
			Rooms (id, huid, name, user_id, household_id, archived, created)
		VALUES
			($room_id, $huid, $room_name, $user_id, $household_id, false, $timestamp)`, db.Prefix)
	params := table.NewQueryParameters(
		table.ValueParam("$room_id", ydb.StringValue([]byte(newRoomID.String()))),
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
		table.ValueParam("$room_name", ydb.StringValue([]byte(roomName))),
		table.ValueParam("$user_id", ydb.Uint64Value(userID)),
		table.ValueParam("$household_id", ydb.OptionalValue(ydb.StringValue([]byte(householdID)))),
		table.ValueParam("$timestamp", ydb.TimestampValue(db.CurrentTimestamp().YdbTimestamp())),
	)
	insertRoomQueryStmt, err := s.Prepare(ctx, insertRoomQuery)
	if err != nil {
		return "", err
	}
	_, err = tx.ExecuteStatement(ctx, insertRoomQueryStmt, params)
	if err != nil {
		return "", err
	}
	return newRoomID.String(), nil
}

func (db *DBClient) getUserRoomsTx(ctx context.Context, s *table.Session, tc *table.TransactionControl, criteria RoomQueryCriteria) (tx *table.Transaction, rooms model.Rooms, err error) {
	var queryB bytes.Buffer
	if err := SelectUserRoomTemplate.Execute(&queryB, criteria); err != nil {
		return nil, rooms, err
	}

	query := fmt.Sprintf(queryB.String(), db.Prefix)
	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(criteria.UserID))),
		table.ValueParam("$room_id", ydb.StringValue([]byte(criteria.RoomID))),
		table.ValueParam("$household_id", ydb.StringValue([]byte(criteria.HouseholdID))),
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
		return nil, nil, err
	}

	for res.NextSet() {
		for res.NextRow() {
			var room model.Room
			res.SeekItem("id")
			room.ID = string(res.OString())
			res.NextItem()
			room.Name = string(res.OString())

			// devices
			res.NextItem()
			devices := make([]string, 0)
			for i, n := 0, res.ListIn(); i < n; i++ {
				res.ListItem(i)
				devices = append(devices, string(res.String()))
			}
			res.ListOut()
			room.Devices = devices

			// household_id
			res.NextItem()
			room.HouseholdID = string(res.OString())

			if res.Err() != nil {
				err := xerrors.Errorf("failed to parse YDB response row: %w", res.Err())
				ctxlog.Error(ctx, db.Logger, err.Error())
				return tx, nil, err
			}

			rooms = append(rooms, room)
		}
	}

	if criteria.WithShared {
		sharedRooms, err := db.selectSharedRoomsInTx(ctx, s, tx, criteria)
		if err != nil {
			return tx, nil, xerrors.Errorf("failed to select shared rooms in tx: %w", err)
		}
		rooms = append(rooms, sharedRooms...)
	}

	return tx, rooms, nil
}

func (db *DBClient) getUserRoomsInTx(ctx context.Context, s *table.Session, tx *table.Transaction, criteria RoomQueryCriteria) (model.Rooms, error) {
	var rooms model.Rooms
	var queryB bytes.Buffer
	if err := SelectUserRoomTemplate.Execute(&queryB, criteria); err != nil {
		return rooms, err
	}

	query := fmt.Sprintf(queryB.String(), db.Prefix)
	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(criteria.UserID))),
		table.ValueParam("$room_id", ydb.StringValue([]byte(criteria.RoomID))),
		table.ValueParam("$household_id", ydb.StringValue([]byte(criteria.HouseholdID))),
	)

	stmt, err := s.Prepare(ctx, query)
	if err != nil {
		return nil, err
	}

	res, err := tx.ExecuteStatement(ctx, stmt, params)
	if err != nil {
		return nil, err
	}

	for res.NextSet() {
		for res.NextRow() {
			var room model.Room
			res.SeekItem("id")
			room.ID = string(res.OString())
			res.NextItem()
			room.Name = string(res.OString())

			// devices
			res.NextItem()
			devices := make([]string, 0)
			for i, n := 0, res.ListIn(); i < n; i++ {
				res.ListItem(i)
				devices = append(devices, string(res.String()))
			}
			res.ListOut()
			room.Devices = devices

			// household_id
			res.NextItem()
			room.HouseholdID = string(res.OString())

			if res.Err() != nil {
				err := xerrors.Errorf("failed to parse YDB response row: %w", res.Err())
				ctxlog.Error(ctx, db.Logger, err.Error())
				return nil, err
			}

			rooms = append(rooms, room)
		}
	}

	if criteria.WithShared {
		sharedRooms, err := db.selectSharedRoomsInTx(ctx, s, tx, criteria)
		if err != nil {
			return nil, xerrors.Errorf("failed to select shared rooms in tx: %w", err)
		}
		rooms = append(rooms, sharedRooms...)
	}

	return rooms, nil
}
