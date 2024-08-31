import pytest
import operator
import numpy as np
from numpy.testing import assert_array_equal

from alice.paskills.penguinarium.ml.index import SklearnIndex
from alice.paskills.penguinarium.storages.nodes import YdbNodesStorage, NSRec
from alice.paskills.penguinarium.ml.index import ModelFactory


@pytest.mark.parametrize(
    'chunk_size,chunks_for_req',
    [(5 * 1024 * 1024, 5), (5, 1)],
    ids=['big-chunks', 'small-chunks']
)
@pytest.mark.asyncio
async def test_add_get_rem(ydb_manager, nodes_table, chunks_table,
                           chunk_size, chunks_for_req):
    factory = ModelFactory(
        SklearnIndex,
        thresh=0.5,
        dist_thresh_rel=operator.lt,
        metric='minkowski',
        n_neighbors=1,
        p=2
    )
    yns = YdbNodesStorage(
        ydb_manager=ydb_manager,
        model_factory=factory,
        cache_size=3,
        ttl=7200,
        chunk_size=chunk_size,
        chunks_for_req=chunks_for_req
    )

    intents = ['1', '2']
    model = factory.produce()
    model.build(np.array([[1., 2.], [3., 4.]]), intents)

    await yns.add(NSRec(
        node_id='1',
        utterances=['.', '..'],
        model=model
    ))

    storage_rec = await yns.get('1')
    assert_array_equal(storage_rec.model._intents, intents)
    assert '1' in yns._cache

    await yns.rem('1')
    assert '1' not in yns._cache

    storage_rec = await yns.get('1')
    assert storage_rec is None


@pytest.fixture
async def factory():
    return ModelFactory(
        SklearnIndex,
        thresh=0.5,
        dist_thresh_rel=operator.lt,
        metric='minkowski',
        n_neighbors=1,
        p=2
    )


@pytest.fixture
async def ydb_nodes_storage(ydb_manager, nodes_table, chunks_table, factory):
    return YdbNodesStorage(
        ydb_manager=ydb_manager,
        model_factory=factory,
        cache_size=3,
        ttl=7200,
        chunk_size=10,
        chunks_for_req=1
    )


@pytest.fixture
async def model(factory):
    intents = ['1', '2']
    model = factory.produce()
    model.build(np.array([[1., 2.], [3., 4.]]), intents)

    return model


@pytest.mark.asyncio
async def test_return_callee(ydb_nodes_storage, model):
    callee = await ydb_nodes_storage.add(NSRec(
        node_id='1',
        utterances=['.', '..'],
        model=model
    ), return_callee=True)
    assert callable(callee)

    callee = await ydb_nodes_storage.get('1', return_callee=True)
    assert callable(callee)

    callee = await ydb_nodes_storage.rem('1', return_callee=True)
    assert callable(callee)


@pytest.mark.asyncio
async def test_rem_multi(ydb_nodes_storage, model):
    node_ids = ['1', '2', '3']
    for node_id in node_ids:
        await ydb_nodes_storage.add(NSRec(
            node_id=node_id,
            utterances=['.', '..'],
            model=model
        ))

    await ydb_nodes_storage.rem_multi(node_ids)

    for node_id in node_ids:
        storage_rec = await ydb_nodes_storage.get(node_id)
        assert storage_rec is None


@pytest.mark.asyncio
async def test_cache(ydb_nodes_storage, model):
    ns_rec = NSRec(
        node_id='1',
        utterances=['.', '..'],
        model=model
    )
    await ydb_nodes_storage.add(ns_rec)

    ns_rec_from_storage = await ydb_nodes_storage.get('1')
    assert ns_rec_from_storage.utterances == ns_rec.utterances
    assert '1' in ydb_nodes_storage._cache

    ydb_nodes_storage._cache['1'] = 'plug'
    assert await ydb_nodes_storage.get('1') == 'plug'
