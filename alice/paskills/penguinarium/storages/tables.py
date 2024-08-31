from textwrap import dedent
from jinja2 import Template
from typing import List, NamedTuple

import ydb
from alice.paskills.penguinarium.storages.ydb_utils import (
    YdbManager, YdbTable, Column
)


class YdbNodesTable(YdbTable):
    def __init__(
        self,
        ydb_manager: YdbManager,
        table_rel_path: str = 'nodes'
    ) -> None:
        super().__init__(
            ydb_manager=ydb_manager,
            table_rel_path=table_rel_path
        )
        self.delete_multi_request = self.render_delete_multi_request()

    @property
    def columns(self) -> List[Column]:
        return [
            Column('node_id', ydb.DataType.Utf8),
            Column('utterances', ydb.DataType.Json),
            Column('chunks_number', ydb.DataType.Uint32),
            Column('meta', ydb.DataType.Json),
        ]

    @property
    def primary_keys_num(self) -> int:
        return 1

    def render_delete_multi_request(self) -> str:
        template = Template(dedent("""\
        PRAGMA TablePathPrefix("{{ root_path }}");

        DECLARE $nodes_ids AS "List<Utf8>";

        DELETE FROM [{{ table_rel_path }}]
        WHERE node_id in $nodes_ids"""))

        return template.render(
            root_path=self._root_path,
            table_rel_path=self._table_rel_path,
            del_key=self.columns[0]
        )

    async def delete_multi(
        self,
        nodes_ids: List[str],
        session: ydb.Session,
        transaction: ydb.TxContext,
        commit_tx: bool = False
    ) -> List[NamedTuple]:
        return await self.perform_request(
            request=self.delete_multi_request,
            params={'$nodes_ids': nodes_ids},
            session=session,
            transaction=transaction,
            commit_tx=commit_tx
        )


class YdbModelsChunksTable(YdbTable):
    def __init__(
        self,
        ydb_manager: YdbManager,
        table_rel_path: str = 'chunks'
    ) -> None:
        super().__init__(
            ydb_manager=ydb_manager,
            table_rel_path=table_rel_path
        )
        self.delete_multi_request = self.render_delete_multi_request()

    @property
    def columns(self) -> List[Column]:
        return [
            Column('node_id', ydb.DataType.Utf8),
            Column('chunk_id', ydb.DataType.Uint32),
            Column('data', ydb.DataType.String),
        ]

    @property
    def primary_keys_num(self) -> int:
        return 2

    def render_get_request(self) -> str:
        template = Template(dedent("""\
        PRAGMA TablePathPrefix("{{ root_path }}");

        DECLARE ${{ key_col.name }} AS {{ key_col.type }};

        SELECT
        {%- for column in columns %}
            {{ column.name }}
            {%- if not loop.last -%}, {%- endif -%}
        {%- endfor %}
        FROM [{{ table_rel_path }}]
        WHERE {{ key_col.name }} = ${{ key_col.name }}
        ORDER BY {{ order_col.name }};"""))

        return template.render(
            root_path=self._root_path,
            table_rel_path=self._table_rel_path,
            columns=self.columns,
            key_col=self.columns[0],
            order_col=self.columns[1]
        )

    async def get(
        self,
        node_id: str,
        session: ydb.Session,
        transaction: ydb.TxContext,
        commit_tx: bool = False
    ) -> List[NamedTuple]:
        return await self.perform_request(
            request=self._get_request,
            params={'$node_id': node_id},
            session=session,
            transaction=transaction,
            commit_tx=commit_tx
        )

    def render_delete_request(self) -> str:
        template = Template(dedent("""\
        PRAGMA TablePathPrefix("{{ root_path }}");

        DECLARE ${{ del_key.name }} AS {{ del_key.type }};

        DELETE FROM [{{ table_rel_path }}]
        WHERE {{ del_key.name }} = ${{ del_key.name }}"""))

        return template.render(
            root_path=self._root_path,
            table_rel_path=self._table_rel_path,
            del_key=self.columns[0]
        )

    async def delete(
        self,
        node_id: str,
        session: ydb.Session,
        transaction: ydb.TxContext,
        commit_tx: bool = False
    ) -> List[NamedTuple]:
        return await self.perform_request(
            request=self.delete_request,
            params={'$node_id': node_id},
            session=session,
            transaction=transaction,
            commit_tx=commit_tx
        )

    def render_delete_multi_request(self) -> str:
        template = Template(dedent("""\
        PRAGMA TablePathPrefix("{{ root_path }}");

        DECLARE $nodes_ids AS "List<Utf8>";

        DELETE FROM [{{ table_rel_path }}]
        WHERE node_id in $nodes_ids"""))

        return template.render(
            root_path=self._root_path,
            table_rel_path=self._table_rel_path,
            del_key=self.columns[0]
        )

    async def delete_multi(
        self,
        nodes_ids: List[str],
        session: ydb.Session,
        transaction: ydb.TxContext,
        commit_tx: bool = False
    ) -> List[NamedTuple]:
        return await self.perform_request(
            request=self.delete_multi_request,
            params={'$nodes_ids': nodes_ids},
            session=session,
            transaction=transaction,
            commit_tx=commit_tx
        )


class YdbGraphTable(YdbTable):
    def __init__(
        self,
        ydb_manager: YdbManager,
        table_rel_path: str = 'graph'
    ) -> None:
        super().__init__(
            ydb_manager=ydb_manager,
            table_rel_path=table_rel_path
        )

    @property
    def columns(self) -> List[Column]:
        return [
            Column('id', ydb.DataType.Utf8),
            Column('descr', ydb.DataType.Json),
        ]

    @property
    def primary_keys_num(self) -> int:
        return 1
