import pytest

from alice.matrix.scheduler.tests.library.test_base import MatrixSchedulerTestBase


class TestSimple(MatrixSchedulerTestBase):

    @pytest.mark.asyncio
    async def test_ping(self, matrix_scheduler):
        assert "pong" == matrix_scheduler.perform_get_request("/admin?action=ping").text.strip()
