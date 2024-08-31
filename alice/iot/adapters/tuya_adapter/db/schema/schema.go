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
		// Users table will be deprecated
		{
			Name: "Users",
			Options: []table.CreateTableOption{
				table.WithColumn("hid", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("id", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("login", ydb.Optional(ydb.TypeString)),
				table.WithColumn("tuya_uid", ydb.Optional(ydb.TypeString)),
				table.WithPrimaryKeyColumn("hid", "id"),
				table.WithIndex("tuya_uid_index",
					table.WithIndexType(table.GlobalIndex()),
					table.WithIndexColumns("tuya_uid"),
				),
				table.WithProfile(
					table.WithPartitioningPolicy(
						table.WithPartitioningPolicyUniformPartitions(4),
					),
				),
			},
		},
		{
			Name: "GenericUsers",
			Options: []table.CreateTableOption{
				table.WithColumn("hid", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("id", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("skill_id", ydb.Optional(ydb.TypeString)),
				table.WithColumn("login", ydb.Optional(ydb.TypeString)),
				table.WithColumn("tuya_uid", ydb.Optional(ydb.TypeString)),
				table.WithPrimaryKeyColumn("hid", "id", "skill_id"),
				table.WithIndex("tuya_uid_index",
					table.WithIndexType(table.GlobalIndex()),
					table.WithIndexColumns("tuya_uid"),
				),
				table.WithProfile(
					table.WithPartitioningPolicy(
						table.WithPartitioningPolicyUniformPartitions(4),
					),
				),
			},
		},
		{
			Name: "DeviceOwnerCache",
			Options: []table.CreateTableOption{
				table.WithColumn("hid", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("id", ydb.Optional(ydb.TypeString)),
				table.WithColumn("tuya_uid", ydb.Optional(ydb.TypeString)),
				table.WithColumn("skill_id", ydb.Optional(ydb.TypeString)),
				table.WithColumn("updated", ydb.Optional(ydb.TypeTimestamp)),
				table.WithPrimaryKeyColumn("hid", "id"),
				table.WithProfile(
					table.WithPartitioningPolicy(
						table.WithPartitioningPolicyUniformPartitions(4),
					),
				),
			},
		},
		{
			Name: "CustomControls",
			Options: []table.CreateTableOption{
				table.WithColumn("huid", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("id", ydb.Optional(ydb.TypeString)),
				table.WithColumn("user_id", ydb.Optional(ydb.TypeString)),
				table.WithColumn("name", ydb.Optional(ydb.TypeString)),
				table.WithColumn("device_type", ydb.Optional(ydb.TypeString)),
				table.WithColumn("buttons", ydb.Optional(ydb.TypeJSON)),
				table.WithColumn("archived", ydb.Optional(ydb.TypeBool)),
				table.WithColumn("created", ydb.Optional(ydb.TypeTimestamp)),
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
