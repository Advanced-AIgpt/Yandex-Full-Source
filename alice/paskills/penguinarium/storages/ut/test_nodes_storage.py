import pytest
from asynctest.mock import patch, MagicMock
import json

from alice.paskills.penguinarium.storages.nodes import (
    BaseNodesStorage, YdbNodesStorage, NSRec
)
from alice.paskills.penguinarium.ml.index import SerializedModel


class FakeIndex:
    def serialize(self):
        return SerializedModel(
            meta=json.dumps({}),
            binary=b'12345'
        )


class MockedModelFactory(MagicMock):
    def load(self, serialized):
        return serialized.binary


@pytest.fixture
def ydb_nodes_storage():
    return YdbNodesStorage(
        ydb_manager=MagicMock(),
        model_factory=MockedModelFactory(),
        cache_size=3,
        ttl=7200,
        chunk_size=2,
        chunks_for_req=2
    )


async def fake_retriable_perform(ydb_manager, callee, *args, **kwargs):
    kwargs['commit_tx'] = False
    return await callee(MagicMock(), MagicMock(), *args, **kwargs)


@patch(
    'alice.paskills.penguinarium.storages.ydb_utils.YdbTable.perform_request',
)
@patch(
    'alice.paskills.penguinarium.storages.nodes.retriable_perform',
    new=fake_retriable_perform
)
@pytest.mark.asyncio
async def test_add_many_chunks(mock_perform_request, ydb_nodes_storage):
    await ydb_nodes_storage.add(NSRec(
        node_id='node_id_1',
        utterances=['a'],
        model=FakeIndex()
    ))

    requests, params_sets = zip(*[
        (call[-1]['request'], call[-1]['params'])
        for call in mock_perform_request.call_args_list
    ])

    assert len(requests) == 4
    assert len(params_sets) == 4

    assert params_sets[0]['$node_id'] == 'node_id_1'
    assert params_sets[1]['$records'][0]._asdict() == {
        'node_id': 'node_id_1',
        'utterances': json.dumps(['a']),
        'chunks_number': 3,
        'meta': json.dumps({}),
    }
    assert len(params_sets[2]['$records']) == 2
    assert len(params_sets[3]['$records']) == 1


@patch(
    'alice.paskills.penguinarium.storages.ydb_utils.YdbTable.perform_request',
    return_value=None
)
@patch(
    'alice.paskills.penguinarium.storages.nodes.retriable_perform',
    new=fake_retriable_perform
)
@pytest.mark.asyncio
async def test_get_empty(rpr_mock, ydb_nodes_storage):
    assert await ydb_nodes_storage.get('node_id_2') is None
    assert rpr_mock.await_count == 2


class FakeRec:
    def __init__(self, params):
        for k, v in params.items():
            setattr(self, k, v)


async def fake_perform_request(self, request, params, *args, **kwargs):
    if 'FROM [nodes]' in request:
        return [FakeRec({
            'node_id': 'node_id_1',
            'utterances': json.dumps(['a', 'b']),
            'chunks_number': 3,
            'meta': json.dumps({})
        })]
    elif 'FROM [chunks]' in request:
        return [
            FakeRec({
                'node_id': 'node_id_1',
                'chunk_id': 2,
                'data': b'2'
            }),
            FakeRec({
                'node_id': 'node_id_1',
                'chunk_id': 0,
                'data': b'0'
            }),
            FakeRec({
                'node_id': 'node_id_1',
                'chunk_id': 1,
                'data': b'1'
            }),
        ]


@patch(
    'alice.paskills.penguinarium.storages.ydb_utils.YdbTable.perform_request',
    new=fake_perform_request
)
@patch(
    'alice.paskills.penguinarium.storages.nodes.retriable_perform',
    new=fake_retriable_perform
)
@pytest.mark.asyncio
async def test_get_many_chunks(ydb_nodes_storage):
    storage_rec = await ydb_nodes_storage.get('node_id_1')
    assert storage_rec.model == b'012'


@pytest.mark.asyncio
async def test_base_nodes_storage():
    assert await BaseNodesStorage.add(None, None) is None
    assert await BaseNodesStorage.get(None, None) is None
    assert await BaseNodesStorage.rem(None, None) is None
