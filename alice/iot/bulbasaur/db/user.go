package db

import (
	"context"
	"fmt"

	"github.com/gofrs/uuid"

	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
)

func (db *DBClient) StoreExternalUser(ctx context.Context, externalID string, skillID string, user model.User) error {
	insertQuery := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $hskid AS Uint64;
			DECLARE $skill_id AS String;
			DECLARE $user_id AS Uint64;
			DECLARE $external_id AS String;

			UPSERT INTO
				ExternalUser (hskid, skill_id, user_id, external_id)
			VALUES
				($hskid, $skill_id, $user_id, $external_id);`, db.Prefix)
	params := table.NewQueryParameters(
		table.ValueParam("$hskid", ydb.Uint64Value(tools.HuidifyString(skillID))),
		table.ValueParam("$skill_id", ydb.StringValue([]byte(skillID))),
		table.ValueParam("$user_id", ydb.Uint64Value(user.ID)),
		table.ValueParam("$external_id", ydb.StringValue([]byte(externalID))),
	)
	return db.Write(ctx, insertQuery, params)
}

func (db *DBClient) SelectExternalUsers(ctx context.Context, externalID, skillID string) ([]model.User, error) {
	query := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $hskid AS Uint64;
			DECLARE $skill_id AS String;
			DECLARE $external_id AS String;

			SELECT
				user_id
			FROM
				ExternalUser VIEW external_id_index
			WHERE
				hskid == $hskid AND
				skill_id == $skill_id AND
				external_id == $external_id;`, db.Prefix)

	params := table.NewQueryParameters(
		table.ValueParam("$hskid", ydb.Uint64Value(tools.HuidifyString(skillID))),
		table.ValueParam("$skill_id", ydb.StringValue([]byte(skillID))),
		table.ValueParam("$external_id", ydb.StringValue([]byte(externalID))),
	)

	res, err := db.Read(ctx, query, params)
	if err != nil {
		return nil, err
	}

	if !res.NextSet() {
		return nil, xerrors.New("no result set found, query might be invalid")
	}

	users := make([]model.User, 0, res.SetRowCount())
	for res.NextRow() {
		res.SeekItem("user_id")
		user := model.User{ID: res.OUint64()}

		if res.Err() != nil {
			err := xerrors.Errorf("failed to parse YDB response row: %w", res.Err())
			ctxlog.Error(ctx, db.Logger, err.Error())
			return nil, err
		}
		if !user.IsEmpty() {
			users = append(users, user)
		} else {
			ctxlog.Warn(ctx, db.Logger, "One of the parsed external user rows held empty user")
		}
	}

	return users, nil
}

func (db *DBClient) DeleteExternalUser(ctx context.Context, skillID string, user model.User) error {
	deleteQuery := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $hskid AS Uint64;
			DECLARE $skill_id AS String;
			DECLARE $user_id AS Uint64;

			DELETE
			FROM
				ExternalUser
			WHERE
				hskid == $hskid AND
				skill_id == $skill_id AND
				user_id == $user_id;`, db.Prefix)
	params := table.NewQueryParameters(
		table.ValueParam("$hskid", ydb.Uint64Value(tools.HuidifyString(skillID))),
		table.ValueParam("$skill_id", ydb.StringValue([]byte(skillID))),
		table.ValueParam("$user_id", ydb.Uint64Value(user.ID)),
	)
	return db.Write(ctx, deleteQuery, params)
}

func (db *DBClient) SelectUser(ctx context.Context, userID uint64) (model.User, error) {
	var user model.User
	ctx = db.ContextWithTransactionType(ctx, db.StaleReadTransactionType)
	rawUser, err := db.getUser(ctx, userID)
	if err != nil {
		return user, xerrors.Errorf("failed to get user with id %d from DB: %w", userID, err)
	}
	user = rawUser.toModelUser()
	if user.IsEmpty() {
		return user, &model.UnknownUserError{}
	}
	return user, nil
}

type rawUser struct {
	ID                 uint64
	Login              string
	CurrentHouseholdID string
}

func (u rawUser) toModelUser() model.User {
	return model.User{ID: u.ID, Login: u.Login}
}

func (db *DBClient) getUser(ctx context.Context, userID uint64) (rawUser, error) {
	var user rawUser

	hid := tools.Huidify(userID)
	res, err := db.Read(ctx, db.PragmaPrefix(`
		DECLARE $hid AS UInt64;
		SELECT
			id, current_household_id
		FROM
			Users
		WHERE
			hid=$hid
	`), table.NewQueryParameters(table.ValueParam("$hid", ydb.Uint64Value(hid))))
	if err != nil {
		return user, xerrors.Errorf("failed to select current household id: %w", err)
	}

	if !res.NextResultSet(ctx, "id", "current_household_id") {
		return user, xerrors.Errorf("failed to get result set for user: %w", err)
	}
	if !res.NextRow() {
		return user, xerrors.Errorf("failed to get user row: empty select result: %w", &model.UnknownUserError{})
	}
	if err := res.ScanWithDefaults(&user.ID, &user.CurrentHouseholdID); err != nil {
		return user, xerrors.Errorf("failed to scan result for user: %w", err)
	}
	return user, nil
}

func (db *DBClient) StoreUser(ctx context.Context, user model.User) error {
	_, err := db.StoreUserWithHousehold(ctx, user, model.GetDefaultHousehold())
	return err
}

func (db *DBClient) StoreUserWithHousehold(ctx context.Context, user model.User, household model.Household) (string, error) {
	createFunc := func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
		tx, u, err := db.getUserTx(ctx, s, txControl, user.ID)
		if err != nil {
			return tx, err
		}

		if !u.IsEmpty() {
			return tx, nil
		}

		if err := household.ValidateName(nil); err != nil {
			return nil, xerrors.Errorf("failed to validate household name: %w", err)
		}

		household.ID = uuid.Must(uuid.NewV4()).String()

		householdLocationParams := householdLocationToValueParams(household.Location)

		storeUserWithHouseholdQuery := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $huid AS Uint64;
			DECLARE $user_id AS Uint64;
			DECLARE $household_id AS String;
			DECLARE $household_name AS String;
			DECLARE $latitude AS Optional<Double>;
			DECLARE $longitude AS Optional<Double>;
			DECLARE $address AS Optional<String>;
			DECLARE $short_address AS Optional<String>;
			DECLARE $timestamp AS Timestamp;

			UPSERT INTO
				Households (huid, user_id, id, name, latitude, longitude, address, short_address, created, archived)
			VALUES
				($huid, $user_id, $household_id, $household_name, $latitude, $longitude, $address, $short_address, $timestamp,false);

			UPSERT INTO
				Users (id, hid, current_household_id, created)
			VALUES
				($user_id, $huid, $household_id, $timestamp);`, db.Prefix)
		params := table.NewQueryParameters(
			table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(user.ID))),
			table.ValueParam("$user_id", ydb.Uint64Value(user.ID)),
			table.ValueParam("$household_id", ydb.StringValue([]byte(household.ID))),
			table.ValueParam("$household_name", ydb.StringValue([]byte(household.Name))),
			table.ValueParam("$timestamp", ydb.TimestampValue(db.CurrentTimestamp().YdbTimestamp())),
		)
		params.Add(householdLocationParams...)
		storeUserWithHouseholdQueryStmt, err := s.Prepare(ctx, storeUserWithHouseholdQuery)
		if err != nil {
			return tx, err
		}
		_, err = tx.ExecuteStatement(ctx, storeUserWithHouseholdQueryStmt, params)
		return tx, err
	}

	err := db.CallInTx(ctx, SerializableReadWrite, createFunc)
	if err != nil {
		return household.ID, err
	}

	return household.ID, err
}

// todo(galecore): murder getUserTx and refactor StoreUser to use db.CreateHousehold instead of code duplication
func (db *DBClient) getUserTx(ctx context.Context, s *table.Session, tc *table.TransactionControl, userID uint64) (tx *table.Transaction, u model.User, err error) {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");
		DECLARE $hid AS Uint64;

		SELECT
			id
		FROM
			Users
		WHERE
			hid == $hid`, db.Prefix)
	params := table.NewQueryParameters(
		table.ValueParam("$hid", ydb.Uint64Value(tools.Huidify(userID))),
	)

	stmt, err := s.Prepare(ctx, query)
	if err != nil {
		return nil, u, err
	}

	tx, res, err := stmt.Execute(ctx, tc, params)
	defer func() {
		if err != nil {
			db.RollbackTransaction(ctx, tx, err)
		}
	}()
	if err != nil {
		return nil, u, err
	}

	for res.NextSet() {
		for res.NextRow() {
			res.SeekItem("id")
			u.ID = res.OUint64()

			if res.Err() != nil {
				err := xerrors.Errorf("failed to parse YDB response row: %w", res.Err())
				ctxlog.Error(ctx, db.Logger, err.Error())
				return tx, u, err
			}
		}
	}

	return tx, u, nil
}
