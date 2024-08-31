import os

import yatest.common
import yatest.common.network

import ydb

from alice.cachalot.tests.local_cachalot import LocalCachalot, LocalCachalotStorageConfig
import alice.cachalot.tests.ydb_tables as ydb_tables


class CachalotFixture(LocalCachalot):
    def __init__(self, env={}):
        bin_path = yatest.common.binary_path("alice/cachalot/bin")
        work_path = yatest.common.output_path()

        storage_config = (
            LocalCachalotStorageConfig()
            .with_ydb_database(os.getenv("YDB_DATABASE"))
            .with_ydb_endpoint(os.getenv("YDB_ENDPOINT"))
            .add_fake(("GDPR", "Ydb"))
            .add_fake(("Location", "Ydb"))
            .add_fake(("VinsContext", "Ydb"))
        )

        super().__init__(
            bin_dir=bin_path,
            work_dir=work_path,
            port_manager=yatest.common.network.PortManager(),
            storage_config=storage_config,
            env=env,
        )

    def _run_command(self, command):

        class Args:
            def __init__(self, local_cachalot):
                self.force = False
                self.ydb_database = local_cachalot.storage_config.ydb_database
                self.ydb_endpoint = local_cachalot.storage_config.ydb_endpoint

        ydb_tables.create_tables(Args(self))

        return yatest.common.execute(command, wait=False, env=self.env)


class YdbFixture:
    def __init__(self):
        self.driver = ydb.Driver(
            ydb.DriverConfig(
                os.getenv('YDB_ENDPOINT'),
                os.getenv('YDB_DATABASE')
            )
        )
        self.driver.wait()
        self.session = ydb.retry_operation_sync(lambda: self.driver.table_client.session().create())

    def __enter__(self):
        return self.session

    def __exit__(self, exc_type, exc_val, exc_tb):
        pass
