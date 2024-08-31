import pytest
from alice.paskills.penguinarium.storages.graph import BaseGraphsStorage


@pytest.mark.asyncio
async def test_base():
    assert await BaseGraphsStorage.add(None, None) is None
    assert await BaseGraphsStorage.get(None, None, None) is None
    assert await BaseGraphsStorage.rem(None, None) is None
