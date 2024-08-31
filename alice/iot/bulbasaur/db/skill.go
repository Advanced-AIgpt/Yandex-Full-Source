package db

import (
	"context"
	"fmt"

	"a.yandex-team.ru/library/go/core/log/ctxlog"

	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"golang.org/x/xerrors"
)

func (db *DBClient) SelectUserSkills(ctx context.Context, userID uint64) ([]string, error) {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");
		DECLARE $huid AS Uint64;
		DECLARE $user_id AS Uint64;
		SELECT
			skill_id
		FROM
			UserSkills
		WHERE
			huid == $huid AND
			user_id == $user_id`, db.Prefix)

	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
		table.ValueParam("$user_id", ydb.Uint64Value(userID)),
	)

	res, err := db.Read(ctx, query, params)
	if err != nil {
		return nil, err
	}

	if !res.NextSet() {
		return nil, xerrors.New("result set not found")
	}

	skills := make([]string, 0, res.SetRowCount())
	for res.NextRow() {
		res.NextItem()
		skills = append(skills, string(res.OString()))

		if res.Err() != nil {
			err := xerrors.Errorf("failed to parse YDB response row: %w", res.Err())
			ctxlog.Error(ctx, db.Logger, err.Error())
			return nil, err
		}
	}

	return skills, nil
}

func (db *DBClient) CheckUserSkillExist(ctx context.Context, userID uint64, skillID string) (bool, error) {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");
		DECLARE $huid AS Uint64;
		DECLARE $user_id AS Uint64;
		DECLARE $skill_id AS String;

		SELECT
			skill_id
		FROM
			UserSkills
		WHERE
			huid == $huid AND
			user_id == $user_id AND
			skill_id == $skill_id;`, db.Prefix)

	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
		table.ValueParam("$user_id", ydb.Uint64Value(userID)),
		table.ValueParam("$skill_id", ydb.StringValue([]byte(skillID))),
	)

	res, err := db.Read(ctx, query, params)
	if err != nil {
		return false, err
	}

	if !res.NextSet() {
		return false, xerrors.New("result set not found")
	}

	return res.SetRowCount() > 0, nil
}

func (db *DBClient) StoreUserSkill(ctx context.Context, userID uint64, skillID string) error {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");
		DECLARE $huid AS Uint64;
		DECLARE $user_id AS Uint64;
		DECLARE $skill_id AS String;
		UPSERT INTO
			UserSkills (huid, user_id, skill_id)
		VALUES
			($huid, $user_id, $skill_id)`, db.Prefix)
	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
		table.ValueParam("$user_id", ydb.Uint64Value(userID)),
		table.ValueParam("$skill_id", ydb.StringValue([]byte(skillID))),
	)
	return db.Write(ctx, query, params)
}

func (db *DBClient) DeleteUserSkill(ctx context.Context, userID uint64, skillID string) error {
	deleteQuery := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $huid AS Uint64;
			DECLARE $user_id AS Uint64;
			DECLARE $skill_id AS String;

			DELETE FROM
				UserSkills
			WHERE
			  huid == $huid AND
			  user_id == $user_id AND
			  skill_id == $skill_id;`, db.Prefix)
	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
		table.ValueParam("$user_id", ydb.Uint64Value(userID)),
		table.ValueParam("$skill_id", ydb.StringValue([]byte(skillID))),
	)
	return db.Write(ctx, deleteQuery, params)
}
