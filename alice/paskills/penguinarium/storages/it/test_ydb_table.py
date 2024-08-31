import pytest
import asyncio
import json

import ydb
from alice.paskills.penguinarium.storages.tables import YdbNodesTable, YdbModelsChunksTable


@pytest.mark.asyncio
async def test_nodes_table(ydb_manager):
    ynt = YdbNodesTable(ydb_manager)

    await ynt.create()
    assert await ynt.exists()

    node_id = 'node_id_1'
    rec = ynt.rec(
        node_id=node_id,
        utterances=json.dumps(['.', '..', '...']),
        chunks_number=2,
        meta=json.dumps({'some_key': 'some_value'})
    )

    with ydb_manager.session_pool.async_checkout() as async_session:
        session = await asyncio.wrap_future(async_session)
        transaction = session.transaction(ydb.SerializableReadWrite())

        await ynt.insert([rec], session, transaction, commit_tx=True)
        rec_from_db = await ynt.get_one(
            ynt.rec(node_id=node_id), session, transaction, commit_tx=True)
        assert rec == rec_from_db

        await ynt.delete(
            ynt.rec(node_id=node_id), session, transaction, commit_tx=True)
        rec_from_db = await ynt.get_one(
            ynt.rec(node_id=node_id), session, transaction, commit_tx=True)
        assert rec_from_db is None

    await ynt.drop()
    assert not await ynt.exists()


@pytest.mark.asyncio
async def test_chunks_table(ydb_manager):
    yct = YdbModelsChunksTable(ydb_manager)

    await yct.create()
    assert await yct.exists()

    node_id = 'node_id_1'
    chunks_ids = list(range(5))

    with ydb_manager.session_pool.async_checkout() as async_session:
        session = await asyncio.wrap_future(async_session)
        transaction = session.transaction(ydb.SerializableReadWrite())

        recs = [
            yct.rec(node_id=node_id, chunk_id=chunk_id, data=bytes(i))
            for i, chunk_id in enumerate(chunks_ids)
        ]
        await yct.insert(recs[::-1], session, transaction, commit_tx=True)

        recs_from_db = await yct.get(node_id, session, transaction, commit_tx=True)
        assert recs == recs_from_db

        await yct.delete(node_id, session, transaction, commit_tx=True)
        recs_from_db = await yct.get(node_id, session, transaction, commit_tx=True)
        assert len(recs_from_db) == 0

    await yct.drop()
    assert not await yct.exists()
