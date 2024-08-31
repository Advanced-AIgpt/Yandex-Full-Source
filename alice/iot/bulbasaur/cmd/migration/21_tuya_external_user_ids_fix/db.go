package main

import (
	"context"
	"fmt"
	"path"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/alice/library/go/ydbclient"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type MigrationDBClient struct {
	tuyaDB      *ydbclient.YDBClient
	bulbasaurDB *ydbclient.YDBClient
}

type TuyaUser struct {
	ExternalID string
	YandexID   uint64
}

func (db *MigrationDBClient) SelectTuyaUsers(ctx context.Context) (map[uint64]TuyaUser, error) {
	userMap := make(map[uint64]TuyaUser)

	s, err := db.tuyaDB.SessionPool.Get(ctx)
	if err != nil {
		logger.Fatalf("Can't get session: %v", err)
		return nil, err
	}

	usersTablePath := path.Join(db.tuyaDB.Prefix, "Users")
	logger.Infof("Reading Users table from path %q", usersTablePath)

	res, err := s.StreamReadTable(ctx, usersTablePath,
		table.ReadColumn("id"),
		table.ReadColumn("tuya_uid"),
	)
	if err != nil {
		logger.Fatalf("Failed to read table: %v", err)
		return nil, err
	}
	defer func() {
		if err := res.Close(); err != nil {
			logger.Fatalf("Error while closing result set: %v", err)
		}
	}()

	var count int
	for res.NextStreamSet(ctx) {
		for res.NextRow() {
			count++

			var user TuyaUser
			res.NextItem()
			user.YandexID = res.OUint64()

			res.NextItem()
			user.ExternalID = string(res.OString())

			if err := res.Err(); err != nil {
				logger.Warnf("Error occurred while reading %s: %v", usersTablePath, err)
				return nil, err
			}

			userMap[user.YandexID] = user
		}
	}

	logger.Infof("Finished reading %d users from %s", count, usersTablePath)
	return userMap, nil
}

func (db *MigrationDBClient) SelectExternalUsers(ctx context.Context, skillID string) (map[uint64]struct{}, error) {
	userMap := make(map[uint64]struct{})

	s, err := db.bulbasaurDB.SessionPool.Get(ctx)
	if err != nil {
		logger.Fatalf("Can't get session: %v", err)
		return nil, err
	}

	usersTablePath := path.Join(db.bulbasaurDB.Prefix, "ExternalUsers")
	logger.Infof("Reading ExternalUsers table from path %q", usersTablePath)

	res, err := s.StreamReadTable(ctx, usersTablePath,
		table.ReadColumn("skill_id"),
		table.ReadColumn("user_id"),
	)
	if err != nil {
		logger.Fatalf("Failed to read table: %v", err)
		return nil, err
	}
	defer func() {
		if err := res.Close(); err != nil {
			logger.Fatalf("Error while closing result set: %v", err)
		}
	}()

	var count int
	for res.NextStreamSet(ctx) {
		for res.NextRow() {
			res.NextItem()
			userSkillID := string(res.OString())
			if userSkillID != skillID {
				continue
			}
			count++
			res.NextItem()
			userID := res.OUint64()

			if err := res.Err(); err != nil {
				logger.Warnf("Error occurred while reading %s: %v", usersTablePath, err)
				return nil, err
			}

			userMap[userID] = struct{}{}
		}
	}

	logger.Infof("Finished reading %d users from %s", count, usersTablePath)
	return userMap, nil
}

func (db *MigrationDBClient) FixBrokenTuyaUIDs(ctx context.Context, tuyaUsers []TuyaUser) error {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");

		DECLARE $values AS List<Struct<
			hskid: Uint64,
			skill_id: String,
			user_id: Uint64,
			external_id: String
		>>;

		UPSERT INTO
			ExternalUsers (hskid, skill_id, user_id, external_id)
		SELECT
			hskid, skill_id, user_id, external_id
		FROM AS_TABLE($values);
	`, db.bulbasaurDB.Prefix)

	values := make([]ydb.Value, 0, len(tuyaUsers))
	for _, user := range tuyaUsers {
		values = append(values, ydb.StructValue(
			ydb.StructFieldValue("hskid", ydb.Uint64Value(tools.HuidifyString(model.TUYA))),
			ydb.StructFieldValue("skill_id", ydb.StringValue([]byte(model.TUYA))),
			ydb.StructFieldValue("user_id", ydb.Uint64Value(user.YandexID)),
			ydb.StructFieldValue("external_id", ydb.StringValue([]byte(user.ExternalID))),
		))
	}
	params := table.NewQueryParameters(
		table.ValueParam("$values", ydb.ListValue(values...)),
	)
	if err := db.bulbasaurDB.Write(ctx, query, params); err != nil {
		return xerrors.Errorf("failed to fix broken tuya uids: %w", err)
	}

	return nil
}
