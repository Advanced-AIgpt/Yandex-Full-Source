import os

import yatest.common

from alice.matrix.library.testing.python.ydb import create_ydb_session

import ydb


MATRIX_WORKER_YDB_INIT_PATH = "alice/matrix/worker/tools/ydb_scripts/matrix_worker_init.ydb"


def init_matrix_worker_ydb(shard_count):
    with open(yatest.common.source_path(MATRIX_WORKER_YDB_INIT_PATH), "r") as f:
        query = f.read()

    ydb_session = create_ydb_session()
    ydb_session.execute_scheme(query)

    unlock_all_shard_locks_and_set_shard_count(ydb_session, shard_count)


def unlock_all_shard_locks_and_set_shard_count(ydb_session, shard_count):
    query_clear_shards = """
        PRAGMA TablePathPrefix("{}");

        DELETE FROM shard_locks;
    """.format(os.getenv("YDB_DATABASE"))

    query_add_shards = """
        PRAGMA TablePathPrefix("{}");

        DECLARE $shard_ids AS List<
            Struct<
                shard_id: Uint64,
            >
        >;

        UPSERT INTO shard_locks (
            SELECT
                shard_id AS shard_id,
                false AS locked,
                "none" AS last_lock_guid,
                "no one" AS last_locked_by,
                CAST(0 AS Timestamp) AS last_processing_start_at,
                CAST(0 AS Timestamp) AS last_heartbeat_at
            FROM AS_TABLE($shard_ids)
        );
    """.format(os.getenv("YDB_DATABASE"))

    prepared_query = ydb_session.prepare(query_clear_shards)
    parameters = {}
    ydb_session.transaction(ydb.SerializableReadWrite()).execute(
        prepared_query,
        parameters,
        commit_tx=True,
    )

    prepared_query = ydb_session.prepare(query_add_shards)
    parameters = {
        "$shard_ids": [
            {
                "shard_id": i,
            }
            for i in range(shard_count)
        ],
    }
    ydb_session.transaction(ydb.SerializableReadWrite()).execute(
        prepared_query,
        parameters,
        commit_tx=True,
    )


def get_shard_lock_rows(ydb_session):
    table_prefix = os.getenv("YDB_DATABASE")
    query = """
        PRAGMA TablePathPrefix("{}");

        SELECT * FROM shard_locks;
    """.format(table_prefix)

    prepared_query = ydb_session.prepare(query)
    parameters = {}
    res = ydb_session.transaction(ydb.OnlineReadOnly()).execute(
        prepared_query,
        parameters,
        commit_tx=True,
    )

    return [
        {
            "shard_id": row.shard_id,

            "locked": row.locked,
            "last_lock_guid": row.last_lock_guid.decode("utf-8"),
            "last_locked_by": row.last_locked_by.decode("utf-8"),

            "last_processing_start_at": row.last_processing_start_at,
            "last_heartbeat_at": row.last_heartbeat_at,
        }
        for row in res[0].rows
    ]


def get_action_rows_from_processing_queue(ydb_session, action_id):
    table_prefix = os.getenv("YDB_DATABASE")
    query = """
        PRAGMA TablePathPrefix("{}");

        DECLARE $action_id AS String;

        SELECT * FROM processing_queue
        WHERE
            action_id = $action_id
    """.format(table_prefix)

    parameters = {
        "$action_id": action_id.encode("utf-8"),
    }

    prepared_query = ydb_session.prepare(query)
    res = ydb_session.transaction(ydb.OnlineReadOnly()).execute(
        prepared_query,
        parameters,
        commit_tx=True,
    )

    return [
        {
            "shard_id": row.shard_id,
            "scheduled_at": row.scheduled_at,
            "action_id": row.action_id.decode("utf-8"),
            "action_guid": row.action_guid.decode("utf-8"),

            "added_to_incoming_queue_at": row.added_to_incoming_queue_at,

            "moved_from_incoming_to_processing_queue_by_sync_with_guid": row.moved_from_incoming_to_processing_queue_by_sync_with_guid.decode("utf-8"),
            "moved_from_incoming_to_processing_queue_at": row.moved_from_incoming_to_processing_queue_at,

            "last_reschedule_by_sync_with_guid": row.last_reschedule_by_sync_with_guid.decode("utf-8"),
            "last_reschedule_at": row.last_reschedule_at,
        }
        for row in res[0].rows
    ]
