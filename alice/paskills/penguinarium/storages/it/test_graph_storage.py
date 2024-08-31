import pytest
import operator
import numpy as np

from alice.paskills.penguinarium.ml.index import SklearnIndex
from alice.paskills.penguinarium.ml.index import ModelFactory
from alice.paskills.penguinarium.storages.nodes import YdbNodesStorage
from alice.paskills.penguinarium.storages.graph import YdbGraphsStorage, GSrec
from alice.paskills.penguinarium.storages.nodes import NSRec


@pytest.fixture
def factory():
    return ModelFactory(
        SklearnIndex,
        thresh=0.5,
        dist_thresh_rel=operator.lt,
        metric='minkowski',
        n_neighbors=1,
        p=2
    )


@pytest.fixture
def ydb_nodes_sotrage(ydb_manager, factory):
    return YdbNodesStorage(
        ydb_manager=ydb_manager,
        model_factory=factory,
        cache_size=3,
        ttl=7200,
        chunk_size=10,
        chunks_for_req=1
    )


@pytest.fixture
def ydb_graphs_storage(ydb_manager, ydb_nodes_sotrage, graph_table,
                       nodes_table, chunks_table):
    return YdbGraphsStorage(
        ydb_manager=ydb_manager,
        nodes_storage=ydb_nodes_sotrage
    )


@pytest.mark.asyncio
async def test_return_callee(ydb_graphs_storage):
    callee = await ydb_graphs_storage.add(GSrec(
        graph_id='graph_id',
        graph_descr={},
        ns_recs=[]
    ), return_callee=True)
    assert callable(callee)

    callee = await ydb_graphs_storage.get('graph_id', return_callee=True)
    assert callable(callee)

    callee = await ydb_graphs_storage.rem('graph_id', return_callee=True)
    assert callable(callee)


@pytest.mark.asyncio
async def test_return_nodes(ydb_graphs_storage, factory):
    intents = ['1', '2']
    model = factory.produce()
    model.build(np.array([[1., 2.], [3., 4.]]), intents)

    await ydb_graphs_storage.add(GSrec(
        graph_id='graph_id',
        graph_descr={
            'nodes': [{'id': '1'}]
        },
        ns_recs=[NSRec(
            node_id='1',
            utterances=['.', '..'],
            model=model
        )]
    ))

    gs_rec = await ydb_graphs_storage.get('graph_id', return_nodes=True)
    ns_recs = gs_rec.ns_recs

    assert len(ns_recs) == 1
    assert ns_recs[0].utterances == ['.', '..']
