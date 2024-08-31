package schema

import (
	"context"
	"fmt"
	"path"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
)

type description struct {
	Name    string
	Options []table.CreateTableOption
}

func CreateTables(ctx context.Context, sp *table.SessionPool, prefix string, nameSuffix string) error {
	tables := []description{
		{
			Name: "DeviceStates",
			Options: []table.CreateTableOption{
				table.WithColumn("huid", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("user_id", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("id", ydb.Optional(ydb.TypeString)),
				table.WithColumn("state", ydb.Optional(ydb.TypeString)),
				table.WithColumn("updated", ydb.Optional(ydb.TypeTimestamp)),
				table.WithPrimaryKeyColumn("huid"),
				table.WithProfile(
					table.WithPartitioningPolicy(
						table.WithPartitioningPolicyUniformPartitions(4),
					),
				),
			},
		},
		{
			Name: "ConnectingDeviceTypes",
			Options: []table.CreateTableOption{
				table.WithColumn("huid", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("user_id", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("id", ydb.Optional(ydb.TypeString)),
				table.WithColumn("device_type", ydb.Optional(ydb.TypeString)),
				table.WithPrimaryKeyColumn("huid", "id"),
				table.WithProfile(
					table.WithPartitioningPolicy(
						table.WithPartitioningPolicyUniformPartitions(4),
					),
				),
			},
		},
	}

	return table.Retry(ctx, sp, table.OperationFunc(func(ctx context.Context, s *table.Session) error {
		for _, td := range tables {
			err := s.CreateTable(ctx, path.Join(prefix, fmt.Sprintf("%s%s", td.Name, nameSuffix)), td.Options...)
			if err != nil {
				return err
			}
		}

		return nil
	}))
}
