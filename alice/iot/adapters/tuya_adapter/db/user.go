package db

import (
	"context"
	"fmt"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (db *DBClient) GetTuyaUserID(ctx context.Context, userID uint64, skillID string) (string, error) {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");
		DECLARE $hid AS Uint64;
		DECLARE $id AS Uint64;
		DECLARE $skill_id AS String;
		SELECT
			tuya_uid
		FROM
			GenericUsers
		WHERE
			hid == $hid AND
			id == $id AND
			skill_id = $skill_id`, db.Prefix)
	params := table.NewQueryParameters(
		table.ValueParam("$hid", ydb.Uint64Value(tools.Huidify(userID))),
		table.ValueParam("$id", ydb.Uint64Value(userID)),
		table.ValueParam("$skill_id", ydb.StringValue([]byte(skillID))),
	)

	res, err := db.Read(ctx, query, params)
	if err != nil {
		return "", xerrors.Errorf("failed to get tuya_uid from DB: %w", err)
	}

	if !res.NextSet() {
		return "", xerrors.Errorf("result set not found: %w", &model.UnknownUserError{})
	}
	if !res.NextRow() || res.SetRowCount() == 0 {
		return "", xerrors.Errorf("result set doesn't contain any rows: %w", &model.UnknownUserError{})
	}
	if res.SetRowCount() > 1 {
		return "", xerrors.Errorf("result set has incorrect row count: expected 1 row, got %d", res.SetRowCount())
	}
	if !res.SeekItem("tuya_uid") {
		return "", xerrors.Errorf(`result row has no field "tuya_uid"`)
	}

	tuyaUID := string(res.OString())
	if err := res.Err(); err != nil {
		return "", xerrors.Errorf("failed to parse result row: %w", err)
	}

	if len(tuyaUID) == 0 {
		return "", &model.UnknownUserError{}
	}
	return tuyaUID, nil
}

func (db *DBClient) CreateUser(ctx context.Context, userID uint64, skillID, login, tuyaUID string) error {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");
		DECLARE $hid AS Uint64;
		DECLARE $uid AS Uint64;
		DECLARE $skill_id AS String;
		DECLARE $login AS String;
		DECLARE $tuya_uid AS String;
		INSERT INTO
			GenericUsers (hid, id, skill_id, login, tuya_uid)
		VALUES
			($hid, $uid, $skill_id, $login, $tuya_uid);`, db.Prefix)
	params := table.NewQueryParameters(
		table.ValueParam("$hid", ydb.Uint64Value(tools.Huidify(userID))),
		table.ValueParam("$uid", ydb.Uint64Value(userID)),
		table.ValueParam("$skill_id", ydb.StringValue([]byte(skillID))),
		table.ValueParam("$login", ydb.StringValue([]byte(login))),
		table.ValueParam("$tuya_uid", ydb.StringValue([]byte(tuyaUID))),
	)

	err := db.Write(ctx, query, params)
	if err != nil {
		return xerrors.Errorf("failed to create user: %w", err)
	}
	return nil
}

func (db *DBClient) IsKnownUser(ctx context.Context, tuyaUID string) (bool, error) {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");
		DECLARE $tuya_uid AS String;
		SELECT
			COUNT(*) > 0 as known
		FROM
			GenericUsers VIEW tuya_uid_index
		WHERE
			tuya_uid = $tuya_uid;`, db.Prefix) // TODO: change : to view after v0 -> v1 sql syntax change
	params := table.NewQueryParameters(
		table.ValueParam("$tuya_uid", ydb.StringValue([]byte(tuyaUID))),
	)

	res, err := db.Read(ctx, query, params)
	if err != nil {
		return false, xerrors.Errorf("failed to check user from DB: %w", err)
	}

	if !res.NextSet() {
		return false, xerrors.New("result set not found")
	}
	if !res.NextRow() || res.SetRowCount() != 1 {
		return false, xerrors.Errorf("result set has incorrect row count: %d", res.SetRowCount())
	}
	if !res.SeekItem("known") {
		return false, xerrors.Errorf(`result row has no field "known"`)
	}

	isKnown := res.Bool()
	if err := res.Err(); err != nil {
		return false, xerrors.Errorf("failed to parse result row: %w", err)
	}
	return isKnown, nil
}

func (db *DBClient) GetTuyaUserSkillID(ctx context.Context, tuyaUID string) (string, error) {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");
		DECLARE $tuya_uid AS String;
		SELECT
			skill_id
		FROM
			GenericUsers VIEW tuya_uid_index
		WHERE
			tuya_uid = $tuya_uid`, db.Prefix)
	params := table.NewQueryParameters(
		table.ValueParam("$tuya_uid", ydb.StringValue([]byte(tuyaUID))),
	)

	res, err := db.Read(ctx, query, params)
	if err != nil {
		return "", xerrors.Errorf("failed to get skill_id from db: %w", err)
	}

	if !res.NextSet() {
		return "", xerrors.New("result set not found")
	}
	if !res.NextRow() || res.SetRowCount() == 0 {
		return "", xerrors.New("result set doesn't contain any rows")
	}
	if res.SetRowCount() > 1 {
		return "", xerrors.Errorf("result set has incorrect row count: expected 1 row, got %d", res.SetRowCount())
	}
	if !res.SeekItem("skill_id") {
		return "", xerrors.Errorf(`result row has no field "skill_id"`)
	}

	skillID := string(res.OString())
	if err := res.Err(); err != nil {
		return "", xerrors.Errorf("failed to parse result row: %w", err)
	}

	if len(skillID) == 0 {
		return "", xerrors.New("skill_id is not set")
	}
	return skillID, nil
}
