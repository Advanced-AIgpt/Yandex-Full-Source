import pytest
import uuid

from .matrix_scheduler import MatrixScheduler

from alice.matrix.library.testing.python.ydb import create_ydb_session


class MatrixSchedulerTestBase:

    @pytest.fixture(scope="class")
    def matrix_scheduler(self):
        with MatrixScheduler() as m:
            yield m

    @pytest.fixture(scope="class")
    def ydb_session(self):
        return create_ydb_session()

    @pytest.fixture(scope="function")
    def action_id(self):
        return f"action_{uuid.uuid4()}"

    @pytest.fixture(scope="function")
    def puid(self):
        return f"puid_{uuid.uuid4()}"

    @pytest.fixture(scope="function")
    def device_id(self):
        return f"device_id_{uuid.uuid4()}"
