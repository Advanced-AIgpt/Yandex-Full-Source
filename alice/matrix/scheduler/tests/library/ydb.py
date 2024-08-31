import os

import yatest.common

from alice.protos.api.matrix.scheduled_action_pb2 import (
    TScheduledActionMeta,
    TScheduledActionSpec,
    TScheduledActionStatus,
)

from alice.matrix.library.testing.python.ydb import create_ydb_session

from cityhash import hash64 as CityHash64

import ydb


MATRIX_SCHEDULER_YDB_INIT_PATH = "alice/matrix/scheduler/tools/ydb_scripts/matrix_scheduler_init.ydb"


def init_matrix_scheduler_ydb():
    with open(yatest.common.source_path(MATRIX_SCHEDULER_YDB_INIT_PATH), "r") as f:
        query = f.read()

    ydb_session = create_ydb_session()
    ydb_session.execute_scheme(query)


def get_action_rows_from_incoming_queue(ydb_session, action_id):
    table_prefix = os.getenv("YDB_DATABASE")
    query = """
        PRAGMA TablePathPrefix("{}");

        DECLARE $action_id AS String;

        SELECT * FROM incoming_queue
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
            "created_at": row.created_at,
            "action_id": row.action_id.decode("utf-8"),
            "action_guid": row.action_guid.decode("utf-8"),

            "scheduled_at": row.scheduled_at,
        }
        for row in res[0].rows
    ]


def get_action_data(ydb_session, action_id):
    table_prefix = os.getenv("YDB_DATABASE")
    query = """
        PRAGMA TablePathPrefix("{}");

        DECLARE $action_id_hash AS Uint64;
        DECLARE $action_id AS String;

        SELECT * FROM scheduled_actions
        WHERE
            meta_action_id_hash = $action_id_hash AND
            meta_action_id = $action_id
    """.format(table_prefix)

    parameters = {
        "$action_id_hash": CityHash64(action_id.encode("utf-8")),
        "$action_id": action_id.encode("utf-8"),
    }

    prepared_query = ydb_session.prepare(query)
    res = ydb_session.transaction(ydb.OnlineReadOnly()).execute(
        prepared_query,
        parameters,
        commit_tx=True,
    )

    def parse_proto_column(proto_type, raw_data):
        proto = proto_type()
        proto.ParseFromString(raw_data)
        return proto

    if len(res[0].rows) == 0:
        return None

    assert len(res[0].rows) == 1

    row = res[0].rows[0]
    return {
        "meta_action_id_hash": row.meta_action_id_hash,
        "meta_action_id": row.meta_action_id.decode("utf-8"),

        "meta": parse_proto_column(TScheduledActionMeta, row.meta),
        "meta_action_guid": row.meta_action_guid.decode("utf-8"),

        "spec": parse_proto_column(TScheduledActionSpec, row.spec),

        "status": parse_proto_column(TScheduledActionStatus, row.status),
        "status_scheduled_at": row.status_scheduled_at,
    }
