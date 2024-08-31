import pytest

from alice.beggins.cmd.manifestator.internal.model import DataEntry
from alice.beggins.cmd.manifestator.internal.dispatcher import EntriesLimiter


def new_data(n: int):
    return [DataEntry(text=f'{i}', target=i % 2) for i in range(n)]


@pytest.mark.parametrize('dispatcher, input_data, output_data', (
    (EntriesLimiter(limit=1), new_data(n=2), new_data(n=1)),
))
def test_dispatchers(dispatcher, input_data, output_data):
    assert output_data == list(dispatcher.dispatch((item for item in input_data)))
