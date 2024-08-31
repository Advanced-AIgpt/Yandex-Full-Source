import pytest

from alice.matrix.worker.tests.library.test_base import MatrixWorkerTestBase


class TestSimple(MatrixWorkerTestBase):

    @pytest.mark.asyncio
    async def test_ping(
        self,
        matrix_scheduler,
        matrix_worker,
    ):
        assert "pong" == matrix_worker.perform_get_request("/admin?action=ping").text.strip()
