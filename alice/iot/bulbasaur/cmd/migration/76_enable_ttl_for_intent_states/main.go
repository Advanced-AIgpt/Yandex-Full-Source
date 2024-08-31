package main

import (
	"context"
	"path"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/cmd/migration/dbmigration"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
)

func do(env dbmigration.Environment) (err error) {
	tableName := "IntentStates"
	tableOptions := []table.AlterTableOption{
		table.WithSetTimeToLiveSettings(
			table.TimeToLiveSettings{
				ColumnName:         "ts",
				ExpireAfterSeconds: uint32((24 * time.Hour).Seconds()),
			},
		),
	}

	return table.Retry(env.Ctx, env.Mdb.SessionPool, table.OperationFunc(func(ctx context.Context, s *table.Session) error {
		desc, err := s.DescribeTable(ctx, path.Join(env.Mdb.Prefix, tableName))
		if err != nil {
			return err
		}

		var ttl float64
		if desc.TimeToLiveSettings == nil {
			ttl = -1
		} else {
			ttl = float64(desc.TimeToLiveSettings.ExpireAfterSeconds) / 60 / 60 / 24
		}
		env.Logger.Infof("TTL Before migration: %v (days, -1 mean no ttl)", ttl)

		if err = s.AlterTable(ctx, path.Join(env.Mdb.Prefix, tableName), tableOptions...); err != nil {
			return err
		}

		env.Logger.Infof("alter done\n")

		desc, err = s.DescribeTable(ctx, path.Join(env.Mdb.Prefix, tableName))
		if err != nil {
			return err
		}

		if desc.TimeToLiveSettings == nil {
			ttl = -1
		} else {
			ttl = float64(desc.TimeToLiveSettings.ExpireAfterSeconds) / 60 / 60 / 24
		}
		env.Logger.Infof("TTL After migration: %v (days, -1 mean no ttl)", ttl)

		return nil
	}))
}

func main() {
	env := dbmigration.AskMigration("Set 24 hours TTL for IntentStates by ts column")
	defer env.Close()

	if err := do(env); err != nil {
		env.Logger.Fatal(err.Error())
	}
}
