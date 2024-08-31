import yatest.common

from alice.matrix.library.testing.python.ydb import create_ydb_session


NOTIFICATOR_YDB_INIT_PATH = "alice/matrix/notificator/tools/ydb_scripts/matrix_notificator_init.ydb"


def init_matrix_ydb():
    with open(yatest.common.source_path(NOTIFICATOR_YDB_INIT_PATH), "r") as f:
        query = f.read()

    ydb_session = create_ydb_session()
    ydb_session.execute_scheme(query)
