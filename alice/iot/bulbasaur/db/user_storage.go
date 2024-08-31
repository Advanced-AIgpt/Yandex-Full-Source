package db

import (
	"context"
	"encoding/json"
	"fmt"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (db *DBClient) StoreUserStorageConfig(ctx context.Context, user model.User, config model.UserStorageConfig) error {
	storeFunc := func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
		storedConfig, err := db.SelectUserStorageConfig(ctx, user)
		if err != nil {
			return nil, xerrors.Errorf("failed to select user %d storage config: %w", user.ID, err)
		}
		mergedConfig, err := storedConfig.MergeConfig(config)
		if err != nil {
			return nil, xerrors.Errorf("failed to merge user %d storage configs: %w", user.ID, err)
		}
		if err := db.upsertUserStorageConfig(ctx, user, mergedConfig); err != nil {
			return nil, xerrors.Errorf("failed to store user %d storage config: %w", user.ID, err)
		}
		return nil, nil
	}
	return db.CallInTx(ctx, SerializableReadWrite, storeFunc)
}

func (db *DBClient) SelectUserStorageConfig(ctx context.Context, user model.User) (model.UserStorageConfig, error) {
	ctx = db.ContextWithTransactionType(ctx, db.StaleReadTransactionType)

	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");
		DECLARE $huid AS Uint64;

		SELECT
			config
		FROM
			UserStorage
		WHERE
			huid == $huid;`, db.Prefix)

	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(user.ID))),
	)

	res, err := db.Read(ctx, query, params)
	if err != nil {
		return nil, err
	}

	if !res.NextSet() {
		return nil, xerrors.New("result set not found")
	}
	userStorageConfig := make(model.UserStorageConfig)
	if !res.NextRow() {
		// no values saved yet
		return userStorageConfig, nil
	}
	res.NextItem()
	userStorageConfigRaw := res.OJSON()
	if err := json.Unmarshal([]byte(userStorageConfigRaw), &userStorageConfig); err != nil {
		return nil, xerrors.Errorf("failed to unmarshal user storage config: %w", err)
	}
	if res.Err() != nil {
		err := xerrors.Errorf("failed to parse YDB response row: %w", res.Err())
		ctxlog.Warnf(ctx, db.Logger, err.Error())
		return nil, err
	}
	return userStorageConfig, nil
}

func (db *DBClient) DeleteUserStorageConfig(ctx context.Context, user model.User) error {
	deleteQuery := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $huid AS Uint64;

			DELETE
			FROM
				UserStorage
			WHERE
				huid == $huid;`, db.Prefix)
	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(user.ID))),
	)
	return db.Write(ctx, deleteQuery, params)
}

func (db *DBClient) upsertUserStorageConfig(ctx context.Context, user model.User, config model.UserStorageConfig) error {
	query := fmt.Sprintf(`
				--!syntax_v1
				PRAGMA TablePathPrefix("%s");
				DECLARE $huid AS Uint64;
				DECLARE $user_id AS Uint64;
				DECLARE $config AS Json;

				UPSERT INTO
					UserStorage (huid, user_id, config)
				VALUES
					($huid, $user_id, $config)`, db.Prefix)
	userConfigB, err := json.Marshal(config)
	if err != nil {
		return xerrors.Errorf("failed to marshal user %d config: %w", user.ID, err)
	}
	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(user.ID))),
		table.ValueParam("$user_id", ydb.Uint64Value(user.ID)),
		table.ValueParam("$config", ydb.JSONValue(string(userConfigB))),
	)
	return db.Write(ctx, query, params)
}
