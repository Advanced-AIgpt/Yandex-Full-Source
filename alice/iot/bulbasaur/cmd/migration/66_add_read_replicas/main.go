package main

import (
	"context"
	"fmt"
	"path"

	"a.yandex-team.ru/alice/iot/bulbasaur/cmd/migration/dbmigration"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
)

var tableNames = []string{"Devices", "DeviceGroups", "Groups", "Rooms", "Scenarios", "Households", "Users", "Stereopairs"}

func do(env dbmigration.Environment) (err error) {
	for _, tableName := range tableNames {
		err := env.Mdb.Retry(context.Background(), func(ctx context.Context, s *table.Session) (err error) {
			tablePath := path.Join(env.Mdb.Prefix, tableName)
			desc, err := s.DescribeTable(ctx, tablePath)
			if err != nil {
				return err
			}
			env.Logger.Infof("before migration read replica settings for table: %s: %+v", tableName, desc.ReadReplicaSettings)

			err = s.AlterTable(ctx, tablePath, table.WithAlterReadReplicasSettings(table.ReadReplicasSettings{
				Type:  table.ReadReplicasPerAzReadReplicas,
				Count: 2,
			}))
			if err == nil {
				env.Logger.Infof("alter table ok")
			}
			desc, err = s.DescribeTable(ctx, tablePath)
			if err != nil {
				return err
			}
			env.Logger.Infof("after migration read replica settings for table: %s: %+v", tableName, desc.ReadReplicaSettings)
			return nil
		})
		if err != nil {
			return err
		}
	}
	return nil
}

func main() {
	env := dbmigration.AskMigration(
		fmt.Sprintf("Add read replicas for tables: %v", tableNames),
	)
	defer env.Close()

	if err := do(env); err != nil {
		env.Logger.Fatal(err.Error())
	}
}
