package main

import (
	"context"
	"fmt"
	"path"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/ptr"
)

type MigrationDBClient struct {
	*db.DBClient
}

type Object struct {
	UserID      uint64
	ID          string
	HouseholdID *string
}

func (db *MigrationDBClient) StreamObjects(ctx context.Context, tableName string) <-chan Object {
	objectsChannel := make(chan Object)

	go func() {
		defer close(objectsChannel)

		s, err := db.SessionPool.Get(ctx)
		if err != nil {
			logger.Fatalf("Can't get session: %v", err)
		}

		tablePath := path.Join(db.Prefix, tableName)
		logger.Infof("Reading %s table from path %q", tableName, tablePath)

		res, err := s.StreamReadTable(ctx, tablePath,
			table.ReadColumn("id"),
			table.ReadColumn("user_id"),
			table.ReadColumn("household_id"),
		)
		if err != nil {
			logger.Fatalf("Failed to read table: %v", err)
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

				var object Object

				res.SeekItem("id")
				object.ID = string(res.OString())

				res.SeekItem("user_id")
				object.UserID = res.OUint64()

				res.SeekItem("household_id")
				if !res.IsNull() {
					object.HouseholdID = ptr.String(string(res.OString()))
				}

				if err := res.Err(); err != nil {
					logger.Warnf("Error occurred while reading %s: %v", tablePath, err)
					continue
				}

				if object.HouseholdID == nil {
					objectsChannel <- object
				}
			}
		}

		logger.Infof("Finished reading %d objects from %s", count, tablePath)
	}()

	return objectsChannel
}

func (db *MigrationDBClient) FillHouseholdID(ctx context.Context, objects []Object, tableName string) error {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");

		DECLARE $values AS List<Struct<
			huid: Uint64,
			id: String
		>>;

		UPSERT INTO
			%s (huid, id, household_id)
        SELECT
            t.huid as huid, t.id as id, u.current_household_id as household_id
        FROM AS_TABLE($values) as t
        INNER JOIN Users as u
        ON u.hid == t.huid;
	`, db.Prefix, tableName)

	values := make([]ydb.Value, 0, len(objects))
	for _, object := range objects {
		values = append(values, ydb.StructValue(
			ydb.StructFieldValue("huid", ydb.Uint64Value(tools.Huidify(object.UserID))),
			ydb.StructFieldValue("id", ydb.StringValue([]byte(object.ID))),
		))
	}
	params := table.NewQueryParameters(
		table.ValueParam("$values", ydb.ListValue(values...)),
	)
	if err := db.Write(ctx, query, params); err != nil {
		return xerrors.Errorf("failed to fill household_id column on %s: %w", tableName, err)
	}

	return nil
}
