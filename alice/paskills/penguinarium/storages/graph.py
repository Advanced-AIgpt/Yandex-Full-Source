from abc import ABC, abstractmethod
import asyncio
from collections import namedtuple
import json
from typing import Union, Callable

from alice.paskills.penguinarium.storages.nodes import BaseNodesStorage
from alice.paskills.penguinarium.storages.tables import YdbGraphTable
from alice.paskills.penguinarium.storages.ydb_utils import retriable_perform, YdbManager


GSrec = namedtuple('GSrec', ['graph_id', 'graph_descr', 'ns_recs'])


class BaseGraphsStorage(ABC):
    @abstractmethod
    async def add(self, gs_rec: GSrec) -> str:
        pass

    @abstractmethod
    async def get(self, graph_id: str, return_nodes: bool = False) -> GSrec:
        pass

    @abstractmethod
    async def rem(self, graph_id: str) -> str:
        pass


class YdbGraphsStorage(BaseGraphsStorage):
    def __init__(
        self,
        ydb_manager: YdbManager,
        nodes_storage: BaseNodesStorage
    ) -> None:
        self._ydb_manager = ydb_manager
        self._nodes_storage = nodes_storage
        self._graph_table = YdbGraphTable(self._ydb_manager)

    async def add(
        self,
        gs_rec: GSrec,
        return_callee: bool = False
    ) -> Union[str, Callable]:
        add_callees = [
            await self._nodes_storage.add(ns_rec, return_callee=True)
            for ns_rec in gs_rec.ns_recs
        ]

        async def callee(session, transaction, commit_tx=False):
            ygt_rec = await self._graph_table.get_one(
                self._graph_table.rec(id=gs_rec.graph_id), session, transaction)
            if ygt_rec is not None:
                rem_callee = await self._nodes_storage.rem_multi(
                    [node['id'] for node in json.loads(ygt_rec.descr)['nodes']],
                    return_callee=True
                )
                await rem_callee(session, transaction)

            for add_callee in add_callees:
                await add_callee(session, transaction, clear_chunks=False)

            await self._graph_table.insert([
                self._graph_table.rec(
                    id=gs_rec.graph_id,
                    descr=json.dumps(gs_rec.graph_descr)
                )
            ], session, transaction, commit_tx=commit_tx)

            return gs_rec.graph_id

        if return_callee:
            return callee

        return await retriable_perform(
            ydb_manager=self._ydb_manager,
            callee=callee,
            commit_tx=True
        )

    async def get(
        self,
        graph_id: str,
        return_nodes: bool = False,
        return_callee: bool = False
    ) -> Union[GSrec, Callable]:
        async def callee(session, transaction, commit_tx=False):
            ygt_rec = await self._graph_table.get_one(
                self._graph_table.rec(id=graph_id), session, transaction)
            if ygt_rec is None:
                return None

            descr = json.loads(ygt_rec.descr)
            ns_recs = None

            if return_nodes:
                ns_recs = []
                for node in descr['nodes']:
                    get_callee = await self._nodes_storage.get(
                        node['id'], return_callee=True)
                    ns_recs.append(await get_callee(session, transaction))

            if commit_tx:
                await asyncio.wrap_future(transaction.async_commit())

            return GSrec(
                graph_id=ygt_rec.id,
                graph_descr=descr,
                ns_recs=ns_recs
            )

        if return_callee:
            return callee

        return await retriable_perform(
            ydb_manager=self._ydb_manager,
            callee=callee,
            commit_tx=True
        )

    async def rem(
        self,
        graph_id: str,
        return_callee: bool = False
    ) -> Union[str, Callable]:
        async def callee(session, transaction, commit_tx=False):
            ygt_rec = await self._graph_table.get_one(
                self._graph_table.rec(id=graph_id), session, transaction)
            if ygt_rec is None:
                return graph_id

            rem_callee = await self._nodes_storage.rem_multi(
                [node['id'] for node in json.loads(ygt_rec.descr)['nodes']],
                return_callee=True
            )
            await rem_callee(session, transaction)

            await self._graph_table.delete(
                self._graph_table.rec(id=graph_id),
                session=session,
                transaction=transaction,
                commit_tx=commit_tx
            )

            return graph_id

        if return_callee:
            return callee

        return await retriable_perform(
            ydb_manager=self._ydb_manager,
            callee=callee,
            commit_tx=True
        )
