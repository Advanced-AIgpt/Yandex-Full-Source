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
			Name: "DeviceGroups",
			Options: []table.CreateTableOption{
				table.WithColumn("huid", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("user_id", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("device_id", ydb.Optional(ydb.TypeString)),
				table.WithColumn("group_id", ydb.Optional(ydb.TypeString)),
				table.WithPrimaryKeyColumn("huid", "device_id", "group_id"),
				table.WithProfile(
					table.WithPartitioningPolicy(
						table.WithPartitioningPolicyUniformPartitions(4),
					),
				),
				table.WithReadReplicasSettings(table.ReadReplicasSettings{
					Type:  table.ReadReplicasPerAzReadReplicas,
					Count: 1,
				}),
			},
		},

		{
			Name: "Devices",
			Options: []table.CreateTableOption{
				table.WithColumn("huid", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("user_id", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("id", ydb.Optional(ydb.TypeString)),
				table.WithColumn("external_id", ydb.Optional(ydb.TypeString)),
				table.WithColumn("name", ydb.Optional(ydb.TypeString)),
				table.WithColumn("aliases", ydb.Optional(ydb.TypeJSON)),
				table.WithColumn("external_name", ydb.Optional(ydb.TypeString)),
				table.WithColumn("type", ydb.Optional(ydb.TypeString)),
				table.WithColumn("original_type", ydb.Optional(ydb.TypeString)),
				table.WithColumn("skill_id", ydb.Optional(ydb.TypeString)),
				table.WithColumn("room_id", ydb.Optional(ydb.TypeString)),
				table.WithColumn("capabilities", ydb.Optional(ydb.TypeJSON)),
				table.WithColumn("properties", ydb.Optional(ydb.TypeJSON)),
				table.WithColumn("custom_data", ydb.Optional(ydb.TypeJSON)),
				table.WithColumn("device_info", ydb.Optional(ydb.TypeJSON)),
				table.WithColumn("created", ydb.Optional(ydb.TypeTimestamp)),
				table.WithColumn("updated", ydb.Optional(ydb.TypeTimestamp)),
				table.WithColumn("household_id", ydb.Optional(ydb.TypeString)),
				table.WithColumn("archived", ydb.Optional(ydb.TypeBool)),
				table.WithColumn("archived_at", ydb.Optional(ydb.TypeTimestamp)),
				table.WithColumn("status", ydb.Optional(ydb.TypeJSON)),
				table.WithColumn("internal_config", ydb.Optional(ydb.TypeJSON)),
				table.WithPrimaryKeyColumn("huid", "id"),
				table.WithIndex("devices_external_id_index",
					table.WithIndexType(table.GlobalIndex()),
					table.WithIndexColumns("external_id"),
				),
				table.WithProfile(
					table.WithPartitioningPolicy(
						table.WithPartitioningPolicyUniformPartitions(16),
					),
				),
				table.WithReadReplicasSettings(table.ReadReplicasSettings{
					Type:  table.ReadReplicasPerAzReadReplicas,
					Count: 1,
				}),
				table.WithTimeToLiveSettings(
					table.TimeToLiveSettings{
						ColumnName:         "archived_at",
						ExpireAfterSeconds: uint32((180 * 24 * time.Hour).Seconds()),
					},
				),
			},
		},

		{
			Name: "Groups",
			Options: []table.CreateTableOption{
				table.WithColumn("huid", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("user_id", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("id", ydb.Optional(ydb.TypeString)),
				table.WithColumn("name", ydb.Optional(ydb.TypeString)),
				table.WithColumn("aliases", ydb.Optional(ydb.TypeJSON)),
				table.WithColumn("type", ydb.Optional(ydb.TypeString)),
				table.WithColumn("created", ydb.Optional(ydb.TypeTimestamp)),
				table.WithColumn("household_id", ydb.Optional(ydb.TypeString)),
				table.WithColumn("archived", ydb.Optional(ydb.TypeBool)),
				table.WithPrimaryKeyColumn("huid", "id"),
				table.WithProfile(
					table.WithPartitioningPolicy(
						table.WithPartitioningPolicyUniformPartitions(4),
					),
				),
				table.WithReadReplicasSettings(table.ReadReplicasSettings{
					Type:  table.ReadReplicasPerAzReadReplicas,
					Count: 1,
				}),
			},
		},

		{
			Name: "Rooms",
			Options: []table.CreateTableOption{
				table.WithColumn("huid", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("user_id", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("id", ydb.Optional(ydb.TypeString)),
				table.WithColumn("name", ydb.Optional(ydb.TypeString)),
				table.WithColumn("created", ydb.Optional(ydb.TypeTimestamp)),
				table.WithColumn("household_id", ydb.Optional(ydb.TypeString)),
				table.WithColumn("archived", ydb.Optional(ydb.TypeBool)),
				table.WithPrimaryKeyColumn("huid", "id"),
				table.WithProfile(
					table.WithPartitioningPolicy(
						table.WithPartitioningPolicyUniformPartitions(4),
					),
				),
				table.WithReadReplicasSettings(table.ReadReplicasSettings{
					Type:  table.ReadReplicasPerAzReadReplicas,
					Count: 1,
				}),
			},
		},

		{
			Name: "Scenarios",
			Options: []table.CreateTableOption{
				table.WithColumn("huid", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("user_id", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("id", ydb.Optional(ydb.TypeString)),
				table.WithColumn("name", ydb.Optional(ydb.TypeString)),
				table.WithColumn("icon", ydb.Optional(ydb.TypeString)),
				table.WithColumn("triggers", ydb.Optional(ydb.TypeJSON)),
				table.WithColumn("devices", ydb.Optional(ydb.TypeJSON)),
				table.WithColumn("requested_speaker_capabilities", ydb.Optional(ydb.TypeJSON)),
				table.WithColumn("steps", ydb.Optional(ydb.TypeJSON)),
				table.WithColumn("created", ydb.Optional(ydb.TypeTimestamp)),
				table.WithColumn("is_active", ydb.Optional(ydb.TypeBool)),
				table.WithColumn("archived", ydb.Optional(ydb.TypeBool)),
				table.WithColumn("effective_time", ydb.Optional(ydb.TypeJSON)),
				table.WithColumn("push_on_invoke", ydb.Optional(ydb.TypeBool)),
				table.WithPrimaryKeyColumn("huid", "id"),
				table.WithProfile(
					table.WithPartitioningPolicy(
						table.WithPartitioningPolicyUniformPartitions(16),
					),
				),
				table.WithReadReplicasSettings(table.ReadReplicasSettings{
					Type:  table.ReadReplicasPerAzReadReplicas,
					Count: 1,
				}),
			},
		},

		{
			Name: "Stereopairs",
			Options: []table.CreateTableOption{
				table.WithColumn("huid", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("user_id", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("id", ydb.Optional(ydb.TypeString)),
				table.WithColumn("name", ydb.Optional(ydb.TypeString)),
				table.WithColumn("config", ydb.Optional(ydb.TypeJSON)),
				table.WithColumn("created", ydb.Optional(ydb.TypeTimestamp)),
				table.WithColumn("archived", ydb.Optional(ydb.TypeBool)),
				table.WithColumn("archived_at", ydb.Optional(ydb.TypeTimestamp)),
				table.WithPrimaryKeyColumn("huid", "id"),
				table.WithReadReplicasSettings(table.ReadReplicasSettings{
					Type:  table.ReadReplicasPerAzReadReplicas,
					Count: 1,
				}),
			},
		},

		{
			Name: "Users",
			Options: []table.CreateTableOption{
				table.WithColumn("hid", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("id", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("created", ydb.Optional(ydb.TypeTimestamp)),
				table.WithColumn("current_household_id", ydb.Optional(ydb.TypeString)),
				table.WithPrimaryKeyColumn("hid"),
				table.WithKeyBloomFilter(ydb.FeatureEnabled),
				table.WithProfile(
					table.WithPartitioningPolicy(
						table.WithPartitioningPolicyUniformPartitions(4),
					),
				),
				table.WithReadReplicasSettings(table.ReadReplicasSettings{
					Type:  table.ReadReplicasPerAzReadReplicas,
					Count: 1,
				}),
			},
		},

		{
			Name: "ExternalUser",
			Options: []table.CreateTableOption{
				table.WithColumn("hskid", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("skill_id", ydb.Optional(ydb.TypeString)),
				table.WithColumn("external_id", ydb.Optional(ydb.TypeString)),
				table.WithColumn("user_id", ydb.Optional(ydb.TypeUint64)),
				table.WithPrimaryKeyColumn("hskid", "skill_id", "user_id"),
				table.WithIndex("external_id_index",
					table.WithIndexType(table.GlobalIndex()),
					table.WithIndexColumns("hskid", "skill_id", "external_id"),
				),
				table.WithProfile(
					table.WithPartitioningPolicy(
						table.WithPartitioningPolicyUniformPartitions(16),
					),
				),
			},
		},

		{
			Name: "UserSkills",
			Options: []table.CreateTableOption{
				table.WithColumn("huid", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("user_id", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("skill_id", ydb.Optional(ydb.TypeString)),
				table.WithPrimaryKeyColumn("huid", "user_id", "skill_id"),
				table.WithProfile(
					table.WithPartitioningPolicy(
						table.WithPartitioningPolicyUniformPartitions(4),
					),
				),
			},
		},

		{
			Name: "UserNetworks",
			Options: []table.CreateTableOption{
				table.WithColumn("huid", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("ssid", ydb.Optional(ydb.TypeString)),
				table.WithColumn("user_id", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("password", ydb.Optional(ydb.TypeString)),
				table.WithColumn("archived", ydb.Optional(ydb.TypeBool)),
				table.WithColumn("updated", ydb.Optional(ydb.TypeTimestamp)),
				table.WithColumn("created", ydb.Optional(ydb.TypeTimestamp)),
				table.WithPrimaryKeyColumn("huid", "ssid"),
				table.WithProfile(
					table.WithPartitioningPolicy(
						table.WithPartitioningPolicyUniformPartitions(4),
					),
				),
			},
		},

		{
			Name: "Experiments",
			Options: []table.CreateTableOption{
				table.WithColumn("hname", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("name", ydb.Optional(ydb.TypeString)),
				table.WithColumn("created", ydb.Optional(ydb.TypeTimestamp)),
				table.WithColumn("is_enabled", ydb.Optional(ydb.TypeBool)),
				table.WithColumn("user_ids", ydb.Optional(ydb.TypeJSON)),
				table.WithColumn("allow_staff", ydb.Optional(ydb.TypeBool)),
				table.WithColumn("allow_all", ydb.Optional(ydb.TypeBool)),
				table.WithColumn("group_ids", ydb.Optional(ydb.TypeJSON)),
				table.WithColumn("description", ydb.Optional(ydb.TypeString)),
				table.WithPrimaryKeyColumn("hname"),
				table.WithProfile(
					table.WithPartitioningPolicy(
						table.WithPartitioningPolicyUniformPartitions(1),
					),
				),
			},
		},

		{
			Name: "ExperimentsUserGroup",
			Options: []table.CreateTableOption{
				table.WithColumn("group_id", ydb.Optional(ydb.TypeString)),
				table.WithColumn("user_ids", ydb.Optional(ydb.TypeJSON)),
				table.WithColumn("comment", ydb.Optional(ydb.TypeString)),
				table.WithPrimaryKeyColumn("group_id"),
			},
		},

		{
			Name: "ScenarioLaunches",
			Options: []table.CreateTableOption{
				table.WithColumn("huid", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("user_id", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("id", ydb.Optional(ydb.TypeString)),
				table.WithColumn("scenario_id", ydb.Optional(ydb.TypeString)),
				table.WithColumn("scenario_name", ydb.Optional(ydb.TypeString)),
				table.WithColumn("launch_trigger_id", ydb.Optional(ydb.TypeString)),
				table.WithColumn("launch_trigger_type", ydb.Optional(ydb.TypeString)),
				table.WithColumn("launch_trigger_value", ydb.Optional(ydb.TypeJSON)),
				table.WithColumn("icon", ydb.Optional(ydb.TypeString)),
				table.WithColumn("launch_data", ydb.Optional(ydb.TypeJSON)),
				table.WithColumn("steps", ydb.Optional(ydb.TypeJSON)),
				table.WithColumn("current_step_index", ydb.Optional(ydb.TypeInt64)),
				table.WithColumn("created", ydb.Optional(ydb.TypeTimestamp)),
				table.WithColumn("scheduled", ydb.Optional(ydb.TypeTimestamp)),
				table.WithColumn("finished", ydb.Optional(ydb.TypeTimestamp)),
				table.WithColumn("status", ydb.Optional(ydb.TypeString)),
				table.WithColumn("error", ydb.Optional(ydb.TypeString)),
				table.WithColumn("push_on_invoke", ydb.Optional(ydb.TypeBool)),
				table.WithPrimaryKeyColumn("huid", "id"),
				table.WithTimeToLiveSettings(
					table.TimeToLiveSettings{
						ColumnName:         "finished",
						ExpireAfterSeconds: uint32((31 * 24 * time.Hour).Seconds()),
					},
				),
				table.WithIndex("scenario_launches_huid_scenario_id_status",
					table.WithIndexType(table.GlobalIndex()),
					table.WithIndexColumns("huid", "scenario_id", "status"),
				),
				table.WithIndex("scenario_launches_huid_status",
					table.WithIndexType(table.GlobalIndex()),
					table.WithIndexColumns("huid", "status"),
				),
				table.WithProfile(
					table.WithPartitioningPolicy(
						table.WithPartitioningPolicyUniformPartitions(16),
					),
				),
			},
		},

		{
			Name: "Households",
			Options: []table.CreateTableOption{
				table.WithColumn("huid", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("user_id", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("id", ydb.Optional(ydb.TypeString)),
				table.WithColumn("name", ydb.Optional(ydb.TypeString)),
				table.WithColumn("address", ydb.Optional(ydb.TypeString)),
				table.WithColumn("short_address", ydb.Optional(ydb.TypeString)),
				table.WithColumn("latitude", ydb.Optional(ydb.TypeDouble)),
				table.WithColumn("longitude", ydb.Optional(ydb.TypeDouble)),
				table.WithColumn("archived", ydb.Optional(ydb.TypeBool)),
				table.WithColumn("created", ydb.Optional(ydb.TypeTimestamp)),
				table.WithPrimaryKeyColumn("huid", "id"),
				table.WithProfile(
					table.WithPartitioningPolicy(
						table.WithPartitioningPolicyUniformPartitions(4),
					),
				),
				table.WithReadReplicasSettings(table.ReadReplicasSettings{
					Type:  table.ReadReplicasPerAzReadReplicas,
					Count: 1,
				}),
			},
		},

		{
			Name: "DeviceTriggersIndex",
			Options: []table.CreateTableOption{
				table.WithColumn("huid", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("user_id", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("device_id", ydb.Optional(ydb.TypeString)),
				table.WithColumn("device_trigger_entity", ydb.Optional(ydb.TypeString)),
				table.WithColumn("type", ydb.Optional(ydb.TypeString)),
				table.WithColumn("instance", ydb.Optional(ydb.TypeString)),
				table.WithColumn("scenario_id", ydb.Optional(ydb.TypeString)),
				table.WithColumn("trigger", ydb.Optional(ydb.TypeJSON)),
				table.WithPrimaryKeyColumn("huid", "device_id", "device_trigger_entity", "type", "instance", "scenario_id"),
				table.WithProfile(
					table.WithPartitioningPolicy(
						table.WithPartitioningPolicyUniformPartitions(16),
					),
				),
			},
		},

		{
			Name: "Favorites",
			Options: []table.CreateTableOption{
				table.WithColumn("huid", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("target_id", ydb.Optional(ydb.TypeString)),
				table.WithColumn("type", ydb.Optional(ydb.TypeString)),
				table.WithColumn("key", ydb.Optional(ydb.TypeString)),
				table.WithColumn("parameters", ydb.Optional(ydb.TypeJSON)),
				table.WithColumn("user_id", ydb.Optional(ydb.TypeUint64)),
				table.WithPrimaryKeyColumn("huid", "target_id", "type", "key"),
				table.WithProfile(
					table.WithPartitioningPolicy(
						table.WithPartitioningPolicyUniformPartitions(8),
					),
				),
			},
		},

		{
			Name: "UserStorage",
			Options: []table.CreateTableOption{
				table.WithColumn("huid", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("user_id", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("config", ydb.Optional(ydb.TypeJSON)),
				table.WithPrimaryKeyColumn("huid"),
				table.WithProfile(
					table.WithPartitioningPolicy(
						table.WithPartitioningPolicyUniformPartitions(4),
					),
				),
			},
		},

		{
			Name: "IntentStates",
			Options: []table.CreateTableOption{
				table.WithColumn("huid", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("user_id", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("device_id", ydb.Optional(ydb.TypeString)),
				table.WithColumn("session_id", ydb.Optional(ydb.TypeString)),
				table.WithColumn("intent", ydb.Optional(ydb.TypeString)),
				table.WithColumn("state", ydb.Optional(ydb.TypeJSON)),
				table.WithColumn("ts", ydb.Optional(ydb.TypeTimestamp)),
				table.WithPrimaryKeyColumn("huid", "device_id", "session_id", "intent"),
				table.WithProfile(
					table.WithPartitioningPolicy(
						table.WithPartitioningPolicyUniformPartitions(4),
					),
				),
			},
		},

		{
			Name: "SharedHouseholds",
			Options: []table.CreateTableOption{
				table.WithColumn("guest_huid", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("guest_id", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("owner_huid", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("owner_id", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("household_id", ydb.Optional(ydb.TypeString)),
				table.WithColumn("household_name", ydb.Optional(ydb.TypeString)),
				table.WithPrimaryKeyColumn("guest_huid", "owner_huid", "household_id"),
				table.WithIndex("shared_households_owner_huid",
					table.WithIndexType(table.GlobalIndex()),
					table.WithIndexColumns("owner_huid"),
				),
				table.WithProfile(
					table.WithPartitioningPolicy(
						table.WithPartitioningPolicyUniformPartitions(4),
					),
				),
			},
		},
		{
			Name: "SharedHouseholdsLinks",
			Options: []table.CreateTableOption{
				table.WithColumn("sender_huid", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("sender_id", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("id", ydb.Optional(ydb.TypeString)),
				table.WithColumn("household_id", ydb.Optional(ydb.TypeString)),
				table.WithColumn("expire_at", ydb.Optional(ydb.TypeTimestamp)),
				table.WithPrimaryKeyColumn("sender_huid", "id", "household_id"),
				table.WithIndex("shared_households_links_id",
					table.WithIndexType(table.GlobalIndex()),
					table.WithIndexColumns("id"),
				),
				table.WithTimeToLiveSettings(
					table.TimeToLiveSettings{
						ColumnName:         "expire_at",
						ExpireAfterSeconds: 0,
					},
				),
				table.WithProfile(
					table.WithPartitioningPolicy(
						table.WithPartitioningPolicyUniformPartitions(4),
					),
				),
			},
		},
		{
			Name: "SharedHouseholdsInvitations",
			Options: []table.CreateTableOption{
				table.WithColumn("guest_huid", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("guest_id", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("sender_huid", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("sender_id", ydb.Optional(ydb.TypeUint64)),
				table.WithColumn("id", ydb.Optional(ydb.TypeString)),
				table.WithColumn("household_id", ydb.Optional(ydb.TypeString)),
				table.WithColumn("created", ydb.Optional(ydb.TypeTimestamp)),
				table.WithPrimaryKeyColumn("guest_huid", "household_id", "sender_huid"),
				table.WithIndex("shared_households_invitations_sender_huid",
					table.WithIndexType(table.GlobalIndex()),
					table.WithIndexColumns("sender_huid"),
				),
				table.WithIndex("shared_households_invitations_id",
					table.WithIndexType(table.GlobalIndex()),
					table.WithIndexColumns("id"),
				),
				table.WithTimeToLiveSettings(
					table.TimeToLiveSettings{
						ColumnName:         "created",
						ExpireAfterSeconds: uint32((31 * 24 * time.Hour).Seconds()),
					},
				),
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
