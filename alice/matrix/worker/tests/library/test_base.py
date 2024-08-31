import pytest

from .matrix_notificator_mock import MatrixNotificatorMock
from .matrix_worker import MatrixWorker

from alice.matrix.scheduler.tests.library.test_base import MatrixSchedulerTestBase

from alice.matrix.library.testing.python.tornado_io_loop import TornadoIOLoop


class MatrixWorkerTestBase(MatrixSchedulerTestBase):
    worker_manual_sync_mode = False
    worker_ydb_init_shard_count = 3
    worker_main_loop_threads = None
    worker_default_min_loop_interval = None
    worker_default_max_loop_interval = None
    worker_min_loop_interval_for_skipped_sync = None
    worker_max_loop_interval_for_skipped_sync = None

    @pytest.fixture(scope="class")
    def matrix_notificator_mock(self):
        with MatrixNotificatorMock() as n:
            yield n

    @pytest.fixture(scope="function", autouse=True)
    def reset_notificator_mock(self, matrix_notificator_mock):
        # It is very expensive to restart notificator for each test
        # so, just reset it
        matrix_notificator_mock.reset()

    @pytest.fixture(scope="class", autouse=True)
    def tornado_io_loop(
        self,

        # Start tornado ioloop after adding all applications to it
        matrix_notificator_mock,
    ):
        with TornadoIOLoop() as l:
            yield l

    @pytest.fixture(scope="class")
    def matrix_worker(self, matrix_notificator_mock):
        with MatrixWorker(
            manual_sync_mode=self.worker_manual_sync_mode,
            ydb_init_shard_count=self.worker_ydb_init_shard_count,
            main_loop_threads=self.worker_main_loop_threads,
            default_min_loop_interval=self.worker_default_min_loop_interval,
            default_max_loop_interval=self.worker_default_max_loop_interval,
            min_loop_interval_for_skipped_sync=self.worker_min_loop_interval_for_skipped_sync,
            max_loop_interval_for_skipped_sync=self.worker_max_loop_interval_for_skipped_sync,
            matrix_notificator_port=matrix_notificator_mock.port,
        ) as m:
            yield m
