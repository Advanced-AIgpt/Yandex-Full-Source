import asyncio
import logging
from abc import ABC, abstractmethod
from typing import List, Union, Callable
from collections import namedtuple
import json
from cachetools import TTLCache

from alice.paskills.penguinarium.storages.ydb_utils import (
    YdbManager, retriable_perform
)
from alice.paskills.penguinarium.storages.tables import (
    YdbNodesTable, YdbModelsChunksTable
)
from alice.paskills.penguinarium.ml.index import SerializedModel, ModelFactory
from alice.paskills.penguinarium.util.metrics import sensors

logger = logging.getLogger(__name__)
NSRec = namedtuple('NSRec', ['node_id', 'utterances', 'model'])


class BaseNodesStorage(ABC):
    @abstractmethod
    async def add(self, ns_rec: NSRec) -> str:
        pass

    @abstractmethod
    async def get(self, node_id: str) -> NSRec:
        pass

    @abstractmethod
    async def rem(self, node_id: str) -> str:
        pass


def chunked(data, chunk_size: int) -> List:
    return [
        data[i: i + chunk_size]
        for i in range(0, len(data), chunk_size)
    ]


class YdbNodesStorage(BaseNodesStorage):
    def __init__(
        self,
        ydb_manager: YdbManager,
        model_factory: ModelFactory,
        cache_size: int,
        ttl: int,
        chunk_size: int = 5 * 1024 * 1204,
        chunks_for_req: int = 5
    ) -> None:
        super().__init__()

        self._ydb_manager = ydb_manager
        self._nodes_table = YdbNodesTable(ydb_manager=self._ydb_manager)
        self._chunks_table = YdbModelsChunksTable(ydb_manager=self._ydb_manager)

        self._model_factory = model_factory

        self._cache = TTLCache(maxsize=cache_size, ttl=ttl)

        self._chunk_size = chunk_size
        self._chunks_for_req = chunks_for_req

    @sensors.with_timer('nodes_storage_add_time')
    async def add(
        self,
        ns_rec: NSRec,
        return_callee: bool = False
    ) -> Union[str, Callable]:
        serialized_model = ns_rec.model.serialize()
        chunks = chunked(serialized_model.binary, self._chunk_size)
        node_rec = self._nodes_table.rec(
            node_id=ns_rec.node_id,
            utterances=json.dumps(ns_rec.utterances),
            chunks_number=len(chunks),
            meta=serialized_model.meta
        )
        chunks_recs = [
            self._chunks_table.rec(
                node_id=ns_rec.node_id,
                chunk_id=i,
                data=data
            ) for i, data in enumerate(chunks)
        ]

        async def callee(session, transaction, commit_tx=False, clear_chunks=True):
            if clear_chunks:
                await self._chunks_table.delete(ns_rec.node_id, session, transaction)
            await self._nodes_table.insert([node_rec], session, transaction)

            for cr in chunked(chunks_recs, self._chunks_for_req):
                await self._chunks_table.insert(cr, session, transaction)

            if commit_tx:
                await asyncio.wrap_future(transaction.async_commit())

            return ns_rec.node_id

        if return_callee:
            return callee

        return await retriable_perform(
            ydb_manager=self._ydb_manager,
            callee=callee,
            commit_tx=True
        )

    @sensors.with_timer('nodes_storage_get_time')
    async def get(
        self,
        node_id: str,
        use_cache: bool = True,
        return_callee: bool = False
    ) -> Union[NSRec, Callable]:
        if not return_callee and use_cache and node_id in self._cache:
            return self._cache[node_id]

        async def callee(session, transaction, commit_tx=False):
            if use_cache and node_id in self._cache:
                return self._cache[node_id]

            async with sensors.timer('ydb_get_time'):
                logger.debug('Getting YDB recrods')
                nodes_rec = await self._nodes_table.get_one(
                    self._nodes_table.rec(node_id=node_id), session, transaction)
                chunks = await self._chunks_table.get(
                    node_id, session, transaction, commit_tx=commit_tx)
                logger.debug('Got YDB records')

            if nodes_rec is None:
                return None

            chunks = sorted(chunks, key=lambda c: c.chunk_id)
            serialized_model = SerializedModel(
                meta=nodes_rec.meta,
                binary=b''.join(chunk.data for chunk in chunks)
            )

            async with sensors.timer('deserialization_time'):
                logger.debug('Deserializing...')
                storage_rec = NSRec(
                    node_id=node_id,
                    utterances=json.loads(nodes_rec.utterances),
                    model=self._model_factory.load(serialized_model)
                )
                logger.debug('Deserialized!')

            self._cache[node_id] = storage_rec

            return storage_rec

        if return_callee:
            return callee

        return await retriable_perform(
            ydb_manager=self._ydb_manager,
            callee=callee,
            commit_tx=True
        )

    async def rem(
        self,
        node_id: str,
        return_callee: bool = False
    ) -> Union[str, Callable]:
        async def callee(session, transaction, commit_tx=False):
            await self._nodes_table.delete(
                self._nodes_table.rec(node_id=node_id), session, transaction)
            await self._chunks_table.delete(node_id, session, transaction)

            if commit_tx:
                await asyncio.wrap_future(transaction.async_commit())

            self._cache.pop(node_id, None)
            return node_id

        if return_callee:
            return callee

        return await retriable_perform(
            ydb_manager=self._ydb_manager,
            callee=callee,
            commit_tx=True
        )

    async def rem_multi(
        self,
        nodes_ids: List[str],
        return_callee: bool = False
    ) -> Union[List[str], Callable]:
        async def callee(session, transaction, commit_tx=False):
            await self._nodes_table.delete_multi(nodes_ids, session, transaction)
            await self._chunks_table.delete_multi(nodes_ids, session, transaction)

            if commit_tx:
                await asyncio.wrap_future(transaction.async_commit())

            for node_id in nodes_ids:
                self._cache.pop(node_id, None)
            return nodes_ids

        if return_callee:
            return callee

        return await retriable_perform(
            ydb_manager=self._ydb_manager,
            callee=callee,
            commit_tx=True
        )
