package schema

import (
	"context"
	"fmt"
	"path"
	"time"

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
				table.WithColumn("device_id", ydb.Optional(ydb.TypeString)),
				table.WithColumn("ts", ydb.Optional(ydb.TypeTimestamp)),
				table.WithColumn("entity_type", ydb.Optional(ydb.TypeString)),
				table.WithColumn("type", ydb.Optional(ydb.TypeString)),
				table.WithColumn("instance", ydb.Optional(ydb.TypeString)),
				table.WithColumn("state", ydb.Optional(ydb.TypeJSON)),
				table.WithColumn("parameters", ydb.Optional(ydb.TypeJSON)),
				table.WithColumn("user_id", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("source", ydb.Optional(ydb.TypeString)),
				table.WithPrimaryKeyColumn("huid", "device_id", "entity_type", "type", "instance", "ts"),
				table.WithTimeToLiveSettings(
					table.TimeToLiveSettings{
						ColumnName:         "ts",
						ExpireAfterSeconds: uint32((7 * 24 * time.Hour).Seconds()),
					},
				),
				table.WithProfile(
					table.WithPartitioningPolicy(
						table.WithPartitioningPolicyUniformPartitions(64),
					),
				),
				table.WithColumnFamilies(table.ColumnFamily{
					Name:        "default",
					Compression: table.ColumnFamilyCompressionLZ4,
				}),
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
