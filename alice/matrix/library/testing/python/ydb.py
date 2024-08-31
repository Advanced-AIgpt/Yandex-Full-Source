import os
import ydb


def create_ydb_session():
    driver = ydb.Driver(ydb.DriverConfig(os.getenv("YDB_ENDPOINT"), os.getenv("YDB_DATABASE")))
    driver.wait()

    return ydb.retry_operation_sync(lambda: driver.table_client.session().create())
