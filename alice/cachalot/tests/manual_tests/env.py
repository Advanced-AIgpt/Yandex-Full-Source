from alice.cachalot.tests.local_cachalot import LocalCachalot, LocalCachalotStorageConfig

from alice.cuttlefish.tests.common import SimpleApphostServantPortManager, ApphostServantProcess


class CachalotService(LocalCachalot):
    def __init__(
        self,
        bin_dir,
        work_dir,
        http_port,
        grpc_port,
        ydb_database,
        env={},
    ):
        storage_config = (
            LocalCachalotStorageConfig()
            .with_ydb_database(ydb_database)
            .with_ydb_endpoint("ydb-ru.yandex.net:2135")
            .add_fake(("GDPR", "Ydb"))
            .add_fake(("LocationDb",))
            .add_fake(("VinsContextDb",))
            .add_fake(("YabioContextDb",))
        )

        super().__init__(
            bin_dir=bin_dir,
            work_dir=work_dir,
            port_manager=SimpleApphostServantPortManager(http_port, grpc_port),
            storage_config=storage_config,
            env=env,
        )

    def _run_command(self, command):
        return ApphostServantProcess(command, env=self.env)
