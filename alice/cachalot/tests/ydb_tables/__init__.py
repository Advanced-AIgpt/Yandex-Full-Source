import ydb

import os


def table_exists(session, path):
    try:
        session.describe_table(path)
        return True
    except Exception as exc:
        print("table_exists Exception:", str(type(exc)), str(exc))
        return False


def ensure_table_absent(args, session, path):
    if table_exists(session, path):
        if args.force:
            session.drop_table(path)
        else:
            raise Exception(f"Table {path} already exists. Use --force to drop it.")


def create_table_impl(args, session, table_name, table_description):
    table_path = os.path.join(args.ydb_database, table_name)
    ensure_table_absent(args, session, table_path)
    session.create_table(table_path, table_description)


def create_tables(args):
    driver_config = ydb.DriverConfig(
        args.ydb_endpoint, args.ydb_database,
        credentials=ydb.credentials.AuthTokenCredentials(os.getenv("YDB_TOKEN"))
    )

    with ydb.Driver(driver_config) as driver:
        try:
            driver.wait(timeout=5)
        except TimeoutError:
            print("Connect failed to YDB")
            print("Last reported errors by discovery:")
            print(driver.discovery_debug_details())
            exit(1)

        with ydb.SessionPool(driver, size=2) as session_pool:
            def callee(session):
                for table_name in ("ttsaudio", "ttsaudio_beta"):
                    create_table_impl(
                        args, session, table_name,
                        ydb.TableDescription()
                        .with_column(ydb.Column("ShardId", ydb.OptionalType(ydb.PrimitiveType.Uint64)))
                        .with_column(ydb.Column("Key", ydb.OptionalType(ydb.PrimitiveType.Utf8)))
                        .with_column(ydb.Column("Audio", ydb.OptionalType(ydb.PrimitiveType.String)))
                        .with_column(ydb.Column("Deadline", ydb.OptionalType(ydb.PrimitiveType.Timestamp)))
                        .with_primary_keys("ShardId", "Key")
                        .with_ttl(ydb.TtlSettings().with_date_type_column("Deadline"))
                    )

                create_table_impl(
                    args, session, "data2",
                    ydb.TableDescription()
                    .with_column(ydb.Column("shard_key", ydb.OptionalType(ydb.PrimitiveType.Uint64)))
                    .with_column(ydb.Column("key", ydb.OptionalType(ydb.PrimitiveType.String)))
                    .with_column(ydb.Column("data", ydb.OptionalType(ydb.PrimitiveType.String)))
                    .with_column(ydb.Column("updated_at", ydb.OptionalType(ydb.PrimitiveType.Uint64)))
                    .with_column(ydb.Column("ttl", ydb.OptionalType(ydb.PrimitiveType.Datetime)))
                    .with_column(ydb.Column("puid", ydb.OptionalType(ydb.PrimitiveType.String)))
                    .with_primary_keys("shard_key", "key")
                    .with_ttl(ydb.TtlSettings().with_date_type_column("ttl", expire_after_seconds=14400))
                )

                create_table_impl(
                    args, session, "v2/activation_announcements",
                    ydb.TableDescription()
                    .with_column(ydb.Column("UserIdHash", ydb.OptionalType(ydb.PrimitiveType.Uint64)))
                    .with_column(ydb.Column("UserId", ydb.OptionalType(ydb.PrimitiveType.String)))
                    .with_column(ydb.Column("DeviceId", ydb.OptionalType(ydb.PrimitiveType.String)))
                    .with_column(ydb.Column("ActivationAttemptTime", ydb.OptionalType(ydb.PrimitiveType.Timestamp)))
                    .with_column(ydb.Column("AvgRMS", ydb.OptionalType(ydb.PrimitiveType.Float)))
                    .with_column(ydb.Column("SpotterValidated", ydb.OptionalType(ydb.PrimitiveType.Bool)))
                    .with_column(ydb.Column("Deadline", ydb.OptionalType(ydb.PrimitiveType.Timestamp)))
                    .with_primary_keys("UserId", "DeviceId")
                    .with_ttl(ydb.TtlSettings().with_date_type_column("Deadline"))
                )

                create_table_impl(
                    args, session, "v2/activation_leaders",
                    ydb.TableDescription()
                    .with_column(ydb.Column("UserIdHash", ydb.OptionalType(ydb.PrimitiveType.Uint64)))
                    .with_column(ydb.Column("UserId", ydb.OptionalType(ydb.PrimitiveType.String)))
                    .with_column(ydb.Column("DeviceId", ydb.OptionalType(ydb.PrimitiveType.String)))
                    .with_column(ydb.Column("ActivationAttemptTime", ydb.OptionalType(ydb.PrimitiveType.Timestamp)))
                    .with_column(ydb.Column("AvgRMS", ydb.OptionalType(ydb.PrimitiveType.Float)))
                    .with_column(ydb.Column("Deadline", ydb.OptionalType(ydb.PrimitiveType.Timestamp)))
                    .with_primary_keys("UserId")
                    .with_ttl(ydb.TtlSettings().with_date_type_column("Deadline"))
                )

                create_table_impl(
                    args, session, "takeout_results",
                    ydb.TableDescription()
                    .with_column(ydb.Column("job_id", ydb.OptionalType(ydb.PrimitiveType.String)))
                    .with_column(ydb.Column("shard_key", ydb.OptionalType(ydb.PrimitiveType.Uint64)))
                    .with_column(ydb.Column("chunk_id", ydb.OptionalType(ydb.PrimitiveType.String)))
                    .with_column(ydb.Column("created_at", ydb.OptionalType(ydb.PrimitiveType.Timestamp)))
                    .with_column(ydb.Column("puid", ydb.OptionalType(ydb.PrimitiveType.String)))
                    .with_column(ydb.Column("texts", ydb.OptionalType(ydb.PrimitiveType.Json)))
                    .with_primary_keys("job_id", "chunk_id", "shard_key")
                    .with_ttl(ydb.TtlSettings().with_date_type_column("created_at"))
                )

                create_table_impl(
                    args, session, "yabio_storage_restyled",
                    ydb.TableDescription()
                    .with_column(ydb.Column("shard_key", ydb.OptionalType(ydb.PrimitiveType.Uint64)))
                    .with_column(ydb.Column("group_id", ydb.OptionalType(ydb.PrimitiveType.String)))
                    .with_column(ydb.Column("dev_model", ydb.OptionalType(ydb.PrimitiveType.String)))
                    .with_column(ydb.Column("dev_manuf", ydb.OptionalType(ydb.PrimitiveType.String)))
                    .with_column(ydb.Column("context", ydb.OptionalType(ydb.PrimitiveType.String)))
                    .with_column(ydb.Column("updated_at", ydb.OptionalType(ydb.PrimitiveType.Uint64)))
                    .with_column(ydb.Column("version", ydb.OptionalType(ydb.PrimitiveType.Uint32)))
                    .with_primary_keys("shard_key", "group_id", "dev_model", "dev_manuf")
                )

            return session_pool.retry_operation_sync(callee)
