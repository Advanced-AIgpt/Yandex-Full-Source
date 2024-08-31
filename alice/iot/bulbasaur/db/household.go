package db

import (
	"bytes"
	"context"
	"fmt"
	"sort"

	"github.com/gofrs/uuid"
	"golang.org/x/exp/slices"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type HouseholdQueryCriteria struct {
	UserID      uint64
	HouseholdID string
	WithShared  bool // that flag allows to select households that were shared with user
}

type CurrentHouseholdQueryCriteria struct {
	UserID     uint64
	WithShared bool
}

func (criteria CurrentHouseholdQueryCriteria) ToHouseholdQueryCriteria() HouseholdQueryCriteria {
	return HouseholdQueryCriteria{
		UserID:     criteria.UserID,
		WithShared: criteria.WithShared,
	}
}

func (db *DBClient) getUserHouseholdsTx(ctx context.Context, s *table.Session, tc *table.TransactionControl, criteria HouseholdQueryCriteria) (tx *table.Transaction, households model.Households, err error) {
	var queryB bytes.Buffer
	if err := SelectUserHouseholdsTemplate.Execute(&queryB, criteria); err != nil {
		return nil, households, err
	}

	query := fmt.Sprintf(queryB.String(), db.Prefix)
	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(criteria.UserID))),
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

	households = make([]model.Household, 0, res.RowCount())
	for res.NextSet() {
		for res.NextRow() {
			household := db.parseHousehold(res)
			if res.Err() != nil {
				err := xerrors.Errorf("failed to parse YDB response row: %w", res.Err())
				ctxlog.Error(ctx, db.Logger, err.Error())
				return nil, nil, err
			}

			households = append(households, household)
		}
	}

	if criteria.WithShared {
		var sharedHouseholds model.Households
		sharedHouseholds, err = db.selectSharedHouseholdsInTx(ctx, s, tx, criteria)
		if err != nil {
			return nil, nil, xerrors.Errorf("failed to select shared households in tx: %w", err)
		}
		households = append(households, sharedHouseholds...)
	}

	return tx, households, nil
}

func (db *DBClient) getUserHouseholdsInTx(ctx context.Context, s *table.Session, tx *table.Transaction, criteria HouseholdQueryCriteria) (households model.Households, err error) {
	var queryB bytes.Buffer
	if err := SelectUserHouseholdsTemplate.Execute(&queryB, criteria); err != nil {
		return households, err
	}

	query := fmt.Sprintf(queryB.String(), db.Prefix)
	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(criteria.UserID))),
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

	households = make([]model.Household, 0, res.RowCount())
	for res.NextSet() {
		for res.NextRow() {
			household := db.parseHousehold(res)
			if res.Err() != nil {
				err := xerrors.Errorf("failed to parse YDB response row: %w", res.Err())
				ctxlog.Error(ctx, db.Logger, err.Error())
				return nil, err
			}

			households = append(households, household)
		}
	}

	if criteria.WithShared {
		sharedHouseholds, err := db.selectSharedHouseholdsInTx(ctx, s, tx, criteria)
		if err != nil {
			return nil, xerrors.Errorf("failed to select shared households in tx: %w", err)
		}
		households = append(households, sharedHouseholds...)
	}

	return households, nil
}

func (db *DBClient) checkUserHouseholdExistInTx(ctx context.Context, s *table.Session, tx *table.Transaction, criteria HouseholdQueryCriteria) (bool, error) {
	households, err := db.getUserHouseholdsInTx(ctx, s, tx, criteria)
	if err != nil {
		return false, xerrors.Errorf("failed to check household %s existence for user %d: %w", criteria.HouseholdID, criteria.UserID, err)
	}
	return len(households) == 1, nil
}

func (db *DBClient) checkUserHouseholdExistTx(ctx context.Context, s *table.Session, tc *table.TransactionControl, criteria HouseholdQueryCriteria) (*table.Transaction, bool, error) {
	tx, households, err := db.getUserHouseholdsTx(ctx, s, tc, criteria)
	if err != nil {
		return nil, false, xerrors.Errorf("failed to check household %s existence for user %d: %w", criteria.HouseholdID, criteria.UserID, err)
	}
	return tx, len(households) == 1, nil
}

func (db *DBClient) selectUserHouseholds(ctx context.Context, criteria HouseholdQueryCriteria) ([]model.Household, error) {
	var households []model.Household
	selectFunc := func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (tx *table.Transaction, err error) {
		tx, households, err = db.getUserHouseholdsTx(ctx, s, txControl, criteria)
		if err != nil {
			return tx, err
		}
		return tx, nil
	}

	err := db.CallInTx(ctx, SerializableReadWrite, selectFunc)
	if err != nil {
		return households, xerrors.Errorf("database request has failed: %w", err)
	}

	return households, nil
}

func (db *DBClient) SelectUserHouseholds(ctx context.Context, userID uint64) (model.Households, error) {
	criteria := HouseholdQueryCriteria{UserID: userID, WithShared: true}
	return db.selectUserHouseholds(ctx, criteria)
}

func (db *DBClient) SelectUserHousehold(ctx context.Context, userID uint64, ID string) (model.Household, error) {
	criteria := HouseholdQueryCriteria{UserID: userID, HouseholdID: ID, WithShared: true}
	households, err := db.selectUserHouseholds(ctx, criteria)
	if err != nil {
		return model.Household{}, err
	}
	if len(households) == 0 {
		return model.Household{}, &model.UserHouseholdNotFoundError{}
	}
	if len(households) > 1 {
		return model.Household{}, fmt.Errorf("found more than one user household using criteria: %#v", criteria)
	}
	return households[0], err
}

func (db *DBClient) CreateUserHousehold(ctx context.Context, userID uint64, household model.Household) (householdID string, err error) {
	createFunc := func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
		criteria := HouseholdQueryCriteria{UserID: userID, WithShared: true}
		tx, userHouseholds, err := db.getUserHouseholdsTx(ctx, s, txControl, criteria)
		if err != nil {
			return tx, err
		}
		household.ID = ""
		if err := household.ValidateName(userHouseholds); err != nil {
			return tx, err
		}

		uid, err := uuid.NewV4()
		if err != nil {
			ctxlog.Warnf(ctx, db.Logger, "failed to generate UUID: %v", err)
		}
		householdID = uid.String()
		householdLocationParams := householdLocationToValueParams(household.Location)

		insertQuery := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $huid AS Uint64;
			DECLARE $user_id AS Uint64;
			DECLARE $id AS String;
			DECLARE $name AS String;
			DECLARE $latitude AS Optional<Double>;
			DECLARE $longitude AS Optional<Double>;
			DECLARE $address AS Optional<String>;
			DECLARE $short_address AS Optional<String>;
			DECLARE $timestamp AS Timestamp;

			UPSERT INTO
				Households (huid, user_id, id, name, latitude, longitude, address, short_address, created, archived)
			VALUES
				($huid, $user_id, $id, $name, $latitude, $longitude, $address, $short_address, $timestamp,false);`, db.Prefix)
		params := table.NewQueryParameters(
			table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
			table.ValueParam("$user_id", ydb.Uint64Value(userID)),
			table.ValueParam("$id", ydb.StringValue([]byte(householdID))),
			table.ValueParam("$name", ydb.StringValue([]byte(tools.StandardizeSpaces(household.Name)))),
			table.ValueParam("$timestamp", ydb.TimestampValue(db.CurrentTimestamp().YdbTimestamp())),
		)
		params.Add(householdLocationParams...)
		insertQueryStmt, err := s.Prepare(ctx, insertQuery)
		if err != nil {
			return tx, err
		}
		_, err = tx.ExecuteStatement(ctx, insertQueryStmt, params)
		if err != nil {
			return tx, err
		}

		return tx, nil
	}

	err = db.CallInTx(ctx, SerializableReadWrite, createFunc)
	if err != nil {
		return "", xerrors.Errorf("database request has failed: %w", err)
	}

	return householdID, nil
}

func (db *DBClient) DeleteUserHousehold(ctx context.Context, userID uint64, householdID string) error {
	deleteHouseholdFunc := func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
		//check if linked objects exist
		query := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $huid AS Uint64;
			DECLARE $household_id AS String;

			SELECT
				COUNT(*) AS cnt
			FROM
				Devices
			WHERE
				huid == $huid AND
				household_id == $household_id AND
				archived == false;

			SELECT
				current_household_id
			FROM
				Users
			WHERE
				hid == $huid;`, db.Prefix)
		params := table.NewQueryParameters(
			table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
			table.ValueParam("$household_id", ydb.StringValue([]byte(householdID))),
		)

		stmt, err := s.Prepare(ctx, query)
		if err != nil {
			return nil, err
		}

		tx, res, err := stmt.Execute(ctx, txControl, params)
		if err != nil {
			return tx, err
		}

		// Devices
		if !res.NextSet() {
			return tx, xerrors.New("can't count household devices: result set does not exist")
		}
		if !res.NextRow() {
			return tx, xerrors.New("can't read count of household devices: result set has no rows")
		}
		res.SeekItem("cnt")
		devicesCount := res.Uint64()
		if err := res.Err(); err != nil {
			return tx, xerrors.Errorf("can't read count household devices: %w", err)
		}

		// Users
		if !res.NextSet() {
			return tx, xerrors.New("can't read users: result set does not exist")
		}
		if !res.NextRow() {
			return tx, xerrors.New("can't read users: result set has no rows")
		}
		res.SeekItem("current_household_id")
		var currentHouseholdID string
		if !res.IsNull() {
			currentHouseholdID = string(res.OString())
		}

		households, err := db.getUserHouseholdsInTx(ctx, s, tx, HouseholdQueryCriteria{UserID: userID})
		if err != nil {
			return tx, xerrors.Errorf("failed to get user households in tx: %w", err)
		}

		switch {
		case devicesCount > 0:
			return tx, xerrors.Errorf("household contains linked devices: %w", &model.UserHouseholdContainsDevicesError{})
		case len(households) == 1:
			return tx, xerrors.Errorf("cannot delete the last household: %w", &model.UserHouseholdLastDeletionError{})
		case currentHouseholdID == householdID:
			households = households.DeleteByID(currentHouseholdID)
			sort.Sort(model.HouseholdsSorting(households))
			if err := db.setCurrentHouseholdForUserInTx(ctx, s, tx, userID, households[0].ID); err != nil {
				return tx, xerrors.Errorf("failed to reset current household as household %s: %w", householdID, err)
			}
		}

		deleteHouseholdQuery := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $huid AS Uint64;
			DECLARE $id AS String;

			UPDATE
				Households
			SET
				archived = true
			WHERE
				huid == $huid AND
				id == $id;

			UPDATE
				Rooms
			SET
				archived = true
			WHERE
				huid == $huid AND
				household_id == $id;

			UPDATE
				Groups
			SET
				archived = true
			WHERE
				huid == $huid AND
				household_id == $id;
			`, db.Prefix)
		params = table.NewQueryParameters(
			table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
			table.ValueParam("$id", ydb.StringValue([]byte(householdID))),
		)
		deleteHouseholdQueryStmt, err := s.Prepare(ctx, deleteHouseholdQuery)
		if err != nil {
			return tx, err
		}
		_, err = tx.ExecuteStatement(ctx, deleteHouseholdQueryStmt, params)
		if err != nil {
			return tx, err
		}
		ctxlog.Infof(ctx, db.Logger, "Deleted household %s for user %d", householdID, userID)
		return tx, nil
	}
	err := db.CallInTx(ctx, SerializableReadWrite, deleteHouseholdFunc)
	if err != nil {
		return xerrors.Errorf("database request has failed: %w", err)
	}
	return nil
}

func (db *DBClient) SelectCurrentHousehold(ctx context.Context, userID uint64) (model.Household, error) {
	var household model.Household
	selectFunc := func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (tx *table.Transaction, err error) {
		tx, household, err = db.selectCurrentHouseholdTx(ctx, s, txControl, CurrentHouseholdQueryCriteria{UserID: userID, WithShared: true})
		if err != nil {
			return tx, err
		}
		return tx, nil
	}

	err := db.CallInTx(ctx, SerializableReadWrite, selectFunc)
	if err != nil {
		return model.Household{}, xerrors.Errorf("database request has failed: %w", err)
	}

	return household, nil
}

func (db *DBClient) selectCurrentHouseholdTx(ctx context.Context, s *table.Session, tc *table.TransactionControl, criteria CurrentHouseholdQueryCriteria) (*table.Transaction, model.Household, error) {
	query := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $huid AS Uint64;

            SELECT
                current_household_id
            FROM
                Users
            WHERE
                hid == $huid;`, db.Prefix)
	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(criteria.UserID))),
	)

	stmt, err := s.Prepare(ctx, query)
	if err != nil {
		return nil, model.Household{}, err
	}

	tx, res, err := stmt.Execute(ctx, tc, params)
	defer func() {
		if err != nil {
			db.RollbackTransaction(ctx, tx, err)
		}
	}()
	if err != nil {
		return nil, model.Household{}, err
	}

	if !res.NextSet() || !res.NextRow() {
		return nil, model.Household{}, &model.UserHouseholdNotFoundError{}
	}
	var currentHouseholdID string
	if err := res.ScanWithDefaults(&currentHouseholdID); err != nil {
		return nil, model.Household{}, xerrors.Errorf("failed to scan ydb response: %w", err)
	}
	if res.Err() != nil {
		err := xerrors.Errorf("failed to parse YDB response row: %w", res.Err())
		ctxlog.Error(ctx, db.Logger, err.Error())
		return tx, model.Household{}, err
	}
	households, err := db.getUserHouseholdsInTx(ctx, s, tx, criteria.ToHouseholdQueryCriteria())
	if err != nil {
		return nil, model.Household{}, xerrors.Errorf("failed to get user households in tx: %w", err)
	}
	currentHousehold, ok := households.GetByID(currentHouseholdID)
	if !ok {
		return nil, model.Household{}, &model.UserHouseholdNotFoundError{}
	}
	return tx, currentHousehold, nil
}

func (db *DBClient) selectCurrentHouseholdInTx(ctx context.Context, s *table.Session, tx *table.Transaction, criteria CurrentHouseholdQueryCriteria) (model.Household, error) {
	query := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $huid AS Uint64;

            SELECT
                h.id as id,
                h.name as name,
                h.latitude as latitude,
                h.longitude as longitude,
                h.address as address,
				h.short_address as short_address
            FROM
                Users as u
            INNER JOIN
                (SELECT * from Households where huid == $huid and archived == false) as h
            ON h.id == u.current_household_id
            WHERE
                u.hid == $huid;`, db.Prefix)
	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(criteria.UserID))),
	)

	stmt, err := s.Prepare(ctx, query)
	if err != nil {
		return model.Household{}, err
	}

	res, err := tx.ExecuteStatement(ctx, stmt, params)
	if err != nil {
		return model.Household{}, err
	}

	if !res.NextSet() || !res.NextRow() {
		return model.Household{}, &model.UserHouseholdNotFoundError{}
	}

	household := db.parseHousehold(res)

	if res.Err() != nil {
		err := xerrors.Errorf("failed to parse YDB response row: %w", res.Err())
		ctxlog.Error(ctx, db.Logger, err.Error())
		return household, err
	}

	return household, nil
}

func (db *DBClient) SetCurrentHouseholdForUser(ctx context.Context, userID uint64, householdID string) error {
	updateFunc := func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
		criteria := HouseholdQueryCriteria{UserID: userID, HouseholdID: householdID, WithShared: true}
		tx, userHouseholds, err := db.getUserHouseholdsTx(ctx, s, txControl, criteria)
		if err != nil {
			return tx, err
		}

		if len(userHouseholds) == 0 {
			return tx, xerrors.Errorf("failed to set current household %s for user %d: %w", householdID, userID, &model.UserHouseholdNotFoundError{})
		}

		updateQuery := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $huid AS Uint64;
			DECLARE $household_id AS String;

			UPDATE
				Users
			SET
				current_household_id = $household_id
			WHERE
				hid == $huid;`, db.Prefix)
		params := table.NewQueryParameters(
			table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
			table.ValueParam("$household_id", ydb.StringValue([]byte(householdID))),
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

func (db *DBClient) setCurrentHouseholdForUserInTx(ctx context.Context, s *table.Session, tx *table.Transaction, userID uint64, householdID string) error {
	exist, err := db.checkUserHouseholdExistInTx(ctx, s, tx, HouseholdQueryCriteria{UserID: userID, HouseholdID: householdID, WithShared: true})
	if err != nil {
		return xerrors.Errorf("failed to check user %d household %s existence: %w", userID, householdID, err)
	}
	if !exist {
		return &model.UserHouseholdNotFoundError{}
	}
	query := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $huid AS Uint64;
			DECLARE $household_id AS String;

			UPDATE
				Users
			SET
				current_household_id = $household_id
			WHERE
				hid == $huid;`, db.Prefix)
	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
		table.ValueParam("$household_id", ydb.StringValue([]byte(householdID))),
	)
	queryStmt, err := s.Prepare(ctx, query)
	if err != nil {
		return err
	}
	_, err = tx.ExecuteStatement(ctx, queryStmt, params)
	if err != nil {
		return err
	}
	return nil
}

func (db *DBClient) UpdateUserHousehold(ctx context.Context, userID uint64, household model.Household) error {
	updateFunc := func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
		criteria := HouseholdQueryCriteria{UserID: userID, WithShared: true}
		tx, userHouseholds, err := db.getUserHouseholdsTx(ctx, s, txControl, criteria)
		if err != nil {
			return tx, err
		}
		if err := household.ValidateName(userHouseholds); err != nil {
			return tx, err
		}
		var householdExists bool
		for _, userHousehold := range userHouseholds {
			if userHousehold.ID == household.ID {
				householdExists = true
			}
		}
		if !householdExists {
			return tx, &model.UserHouseholdNotFoundError{}
		}
		householdLocationParams := householdLocationToValueParams(household.Location)
		insertQuery := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $huid AS Uint64;
			DECLARE $user_id AS Uint64;
			DECLARE $id AS String;
			DECLARE $name AS String;
			DECLARE $latitude AS Optional<Double>;
			DECLARE $longitude AS Optional<Double>;
			DECLARE $address AS Optional<String>;
			DECLARE $short_address AS Optional<String>;
			DECLARE $timestamp AS Timestamp;

			UPSERT INTO
				Households (huid, user_id, id, name, latitude, longitude, address, short_address)
			VALUES
				($huid, $user_id, $id, $name, $latitude, $longitude, $address, $short_address);`, db.Prefix)
		params := table.NewQueryParameters(
			table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
			table.ValueParam("$user_id", ydb.Uint64Value(userID)),
			table.ValueParam("$id", ydb.StringValue([]byte(household.ID))),
			table.ValueParam("$name", ydb.StringValue([]byte(tools.StandardizeSpaces(household.Name)))),
		)
		params.Add(householdLocationParams...)
		insertQueryStmt, err := s.Prepare(ctx, insertQuery)
		if err != nil {
			return tx, err
		}
		_, err = tx.ExecuteStatement(ctx, insertQueryStmt, params)
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

func (db *DBClient) MoveUserDevicesToHousehold(ctx context.Context, user model.User, deviceIDs []string, householdID string) error {
	deleteFunc := func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
		// Select user groups to start transaction and update groups types after devices moving
		criteria := GroupQueryCriteria{UserID: user.ID}
		tx, userGroups, err := db.getUserGroupsTx(ctx, s, txControl, criteria)
		if err != nil {
			return tx, err
		}

		userDevicesQuery := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $huid AS Uint64;

			SELECT
				*
			FROM
				Devices
			WHERE
				huid == $huid AND
				archived == false;`, db.Prefix)
		params := table.NewQueryParameters(
			table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(user.ID))),
		)

		stmt, err := s.Prepare(ctx, userDevicesQuery)
		if err != nil {
			return tx, err
		}

		res, err := tx.ExecuteStatement(ctx, stmt, params)
		if err != nil {
			return tx, err
		}

		var userDevices model.Devices
		if !res.NextResultSet(ctx, "id", "household_id", "room_id") {
			return tx, xerrors.Errorf("failed to get result set for devices: %w", res.Err())
		}

		for res.NextRow() {
			var device model.Device
			device.Room = &model.Room{}
			if err := res.ScanWithDefaults(&device.ID, &device.HouseholdID, &device.Room.ID); err != nil {
				return tx, xerrors.Errorf("failed to scan device values")
			}
			userDevices = append(userDevices, device)
		}

		changeSet := make([]deviceMoveToHouseholdChange, 0, len(deviceIDs))
		for _, deviceID := range deviceIDs {
			if device, exist := userDevices.GetDeviceByID(deviceID); exist {
				change := deviceMoveToHouseholdChange{ID: deviceID, UserID: user.ID, HouseholdID: householdID}
				if device.HouseholdID == householdID {
					change.RoomID = device.RoomID()
				}
				changeSet = append(changeSet, change)
			}
		}

		if len(changeSet) == 0 {
			ctxlog.Warn(ctx, db.Logger, "Move to household changeset is empty")
			return tx, nil
		}

		updateQuery := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $changeset AS List<Struct<
				id: String,
				huid: Uint64,
				household_id: String,
				room_id: Optional<String>
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
		deleteSet := make([]deviceGroupsDeleteChange, 0, len(userDevices))
		for _, deviceID := range deviceIDs {
			if _, exist := userDevices.GetDeviceByID(deviceID); !exist {
				continue
			}
			for _, group := range userGroups {
				if slices.Contains(group.Devices, deviceID) {
					deleteSet = append(deleteSet, deviceGroupsDeleteChange{
						UserID:   user.ID,
						DeviceID: deviceID,
						GroupID:  group.ID,
					})
				}
			}
		}

		if err := db.removeDevicesFromGroupsInTx(ctx, s, tx, user.ID, deleteSet); err != nil {
			return tx, xerrors.Errorf("failed to clean device groups: %w", err)
		}

		// Updating groups types after device has been moved from them
		groupsTypesChangeSet := make([]groupChangeSet, 0, len(userGroups))
		for _, group := range userGroups {
			if len(group.Devices) > 0 && tools.ContainsAll(deviceIDs, group.Devices) {
				groupsTypesChangeSet = append(groupsTypesChangeSet, groupChangeSet{ID: group.ID, Name: group.Name, Type: "", UserID: user.ID, Archived: false})
			}
		}

		if err := db.updateGroupsInTx(ctx, s, tx, groupsTypesChangeSet); err != nil {
			return tx, xerrors.Errorf("failed to update groups types: %w", err)
		}

		return tx, nil
	}

	if len(deviceIDs) == 0 {
		return nil
	}

	err := db.CallInTx(ctx, SerializableReadWrite, deleteFunc)
	if err != nil {
		return xerrors.Errorf("database request has failed: %w", err)
	}

	return nil
}

type deviceMoveToHouseholdChange struct {
	UserID      uint64
	ID          string
	HouseholdID string
	RoomID      string
}

func (d *deviceMoveToHouseholdChange) toStructValue() ydb.Value {
	var roomValue ydb.Value
	if d.RoomID == "" {
		roomValue = ydb.NullValue(ydb.TypeString)
	} else {
		roomValue = ydb.OptionalValue(ydb.StringValueFromString(d.RoomID))
	}
	return ydb.StructValue(
		ydb.StructFieldValue("id", ydb.StringValue([]byte(d.ID))),
		ydb.StructFieldValue("huid", ydb.Uint64Value(tools.Huidify(d.UserID))),
		ydb.StructFieldValue("household_id", ydb.StringValue([]byte(d.HouseholdID))),
		ydb.StructFieldValue("room_id", roomValue),
	)
}

func (db *DBClient) parseHousehold(res *table.Result) model.Household {
	var household model.Household
	res.SeekItem("id")
	household.ID = string(res.OString())
	res.NextItem()
	household.Name = string(res.OString())
	res.NextItem()
	var location model.HouseholdLocation
	var locationFilled bool
	if !res.IsNull() {
		locationFilled = true
		location.Latitude = res.ODouble()
	}
	res.NextItem()
	if !res.IsNull() {
		locationFilled = true
		location.Longitude = res.ODouble()
	}
	res.NextItem()
	if !res.IsNull() {
		locationFilled = true
		location.Address = string(res.OString())
	}
	res.NextItem()
	if !res.IsNull() {
		locationFilled = true
		location.ShortAddress = string(res.OString())
	}
	if locationFilled {
		household.Location = &location
	}
	return household
}

func householdLocationToValueParams(location *model.HouseholdLocation) []table.ParameterOption {
	var addressValue ydb.Value
	var shortAddressValue ydb.Value
	var latitudeValue ydb.Value
	var longitudeValue ydb.Value
	if location != nil {
		longitudeValue = ydb.OptionalValue(ydb.DoubleValue(location.Longitude))
		latitudeValue = ydb.OptionalValue(ydb.DoubleValue(location.Latitude))
		addressValue = ydb.OptionalValue(ydb.StringValue([]byte(location.Address)))
		shortAddressValue = ydb.OptionalValue(ydb.StringValue([]byte(location.ShortAddress)))
	} else {
		latitudeValue = ydb.NullValue(ydb.TypeDouble)
		longitudeValue = ydb.NullValue(ydb.TypeDouble)
		addressValue = ydb.NullValue(ydb.TypeString)
		shortAddressValue = ydb.NullValue(ydb.TypeString)
	}
	return []table.ParameterOption{
		table.ValueParam("$latitude", latitudeValue),
		table.ValueParam("$longitude", longitudeValue),
		table.ValueParam("$address", addressValue),
		table.ValueParam("$short_address", shortAddressValue),
	}
}
