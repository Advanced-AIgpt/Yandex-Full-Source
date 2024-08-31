package main

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/db/dao"

	"a.yandex-team.ru/library/go/core/xerrors"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"

	"a.yandex-team.ru/alice/iot/bulbasaur/cmd/migration/dbmigration"
)

func main() {
	env := dbmigration.AskMigration("null zero values in ScenarioLaunches")
	defer env.Close()

	migrateSource := env.Mdb.PragmaPrefix(`--!syntax_v1
		SELECT
			*
		FROM
			ScenarioLaunches
		WHERE
			finished = Timestamp('1970-01-01T00:00:00.000000Z');
`)

	chunker := dbmigration.NewHuidChunker(migrateSource, func() dao.HuidRow {
		return &dao.ScenarioLaunch{}
	})

	err := dbmigration.MigrateDatabase(env.Ctx, env.Logger, env.Mdb, chunker, func(ctx context.Context, mdb *dbmigration.Client, chunk dbmigration.Chunk) error {
		return mdb.CallInTx(ctx, db.SerializableReadWrite, func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
			query := `--!syntax_v1
				DECLARE $records AS List<Struct<huid: UInt64, id: String>>;

				UPSERT INTO
					ScenarioLaunches
				SELECT
					huid, id, NULL AS finished
				FROM AS_TABLE($records);
`

			records := make([]ydb.Value, 0, len(chunk.Rows))
			for _, row := range chunk.Rows {
				row := row.(*dao.ScenarioLaunch)
				records = append(records, ydb.StructValue(
					ydb.StructFieldValue("huid", ydb.Uint64Value(row.Huid)),
					ydb.StructFieldValue("id", ydb.StringValueFromString(row.ID)),
				))
			}

			params := table.NewQueryParameters(
				table.ValueParam("$records", ydb.ListValue(records...)),
			)
			tx, _, err := s.Execute(ctx, txControl, env.Mdb.PragmaPrefix(query), params, table.WithQueryCachePolicy(table.WithQueryCachePolicyKeepInCache()))
			if err != nil {
				err = xerrors.Errorf("failed to update launches: %w", err)
			}
			return tx, err
		})
	})

	if err == nil {
		env.Logger.Infof("migration finished OK")
	} else {
		env.Logger.Fatalf("migration error: %+v", err)
	}
}
