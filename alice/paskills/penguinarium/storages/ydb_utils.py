import asyncio
import logging
import os
from abc import ABC, abstractmethod
from collections import namedtuple
from itertools import repeat
from textwrap import dedent
from concurrent.futures import TimeoutError
from typing import Dict, Union, Callable, NamedTuple, List, Any

from jinja2 import Template
from kikimr.public.sdk.python import tvm
import ydb
from alice.paskills.penguinarium.util.metrics import sensors

logger = logging.getLogger(__name__)
ConnectionParams = Dict[str, Union[str, int, bool]]


class YdbManager:
    YDB_TVM_ID = 2002490

    def __init__(
        self,
        endpoint: str,
        database: str,
        root_folder: str,
        connect_params: ConnectionParams,
        timeout: int = None,
        max_retries: int = 5
    ) -> None:
        self._timeout = timeout
        self._max_retries = max_retries
        self._root_path = os.path.join(database, root_folder)

        credentials = self._create_credentials(connect_params)
        self._driver = self._setup_ydb_driver(endpoint, database, credentials)

        self._session_pool = ydb.SessionPool(self._driver)

    def _create_tvm_credentials(
            self,
            tvm_secret: str,
            self_tvm_id: int,
            ydb_tvm_id: int
    ) -> tvm.TvmCredentialsProvider:
        return tvm.TvmCredentialsProvider(
            self_client_id=int(self_tvm_id),
            self_secret=tvm_secret,
            destination_alias='ydb',
            dsts={'ydb': int(ydb_tvm_id)}
        )

    def _create_credentials(
            self,
            connect_params: ConnectionParams
    ) -> ydb.AuthTokenCredentials:
        if connect_params['use_tvm']:
            return self._create_tvm_credentials(
                tvm_secret=os.environ[connect_params['tvm_secret_env']],
                self_tvm_id=connect_params['self_tvm_id'],
                ydb_tvm_id=connect_params.get('ydb_tvm_id', self.YDB_TVM_ID),
            )

        return ydb.AuthTokenCredentials(
            os.environ[connect_params['auth_token_env']]
        )

    def _setup_ydb_driver(
            self,
            endpoint: str,
            database: str,
            credentials: Union[ydb.AbstractCredentials, ydb.Credentials]
    ) -> ydb.Driver:
        connection_params = ydb.DriverConfig(
            endpoint,
            database=database,
            credentials=credentials
        )
        try:
            driver = ydb.Driver(connection_params)
            driver.wait(timeout=5)
        except TimeoutError:
            logger.exception('Can\'t connect to YDB at _setup_ydb_driver')
            raise RuntimeError('Connect failed to YDB')

        return driver

    @property
    def driver(self) -> ydb.Driver:
        return self._driver

    @property
    def session_pool(self) -> ydb.SessionPool:
        return self._session_pool

    @property
    def root_path(self) -> str:
        return self._root_path

    @property
    def timeout(self) -> int:
        return self._timeout

    @property
    def max_retries(self) -> int:
        return self._max_retries


Column = namedtuple('Column', ['name', 'type'])


class YdbTable(ABC):
    def __init__(self, ydb_manager: YdbManager, table_rel_path: str) -> None:
        super().__init__()

        self._ydb_manager = ydb_manager
        self._root_path = self._ydb_manager.root_path
        self._table_rel_path = table_rel_path

        self._insert_request = self.render_insert_request()
        self._get_request = self.render_get_request()
        self._delete_request = self.render_delete_request()

        self.Record = namedtuple(type(self).__name__ + 'Record', [
            column.name for column in self.columns
        ], defaults=repeat(None, len(self.columns) - len(self.primary_keys)))

    @property
    @abstractmethod
    def columns(self) -> List[Column]:
        pass

    @property
    @abstractmethod
    def primary_keys_num(self) -> int:
        pass

    @property
    def primary_keys(self) -> List[Column]:
        return self.columns[:self.primary_keys_num]

    def rec(self, *args, **kwargs):
        return self.Record(*args, **kwargs)

    @property
    def table_path(self) -> str:
        return os.path.join(self._root_path, self._table_rel_path)

    async def create(self) -> None:
        td = ydb.TableDescription()
        for column in self.columns:
            td = td.with_column(ydb.Column(column.name, ydb.OptionalType(column.type)))
        for column in self.primary_keys:
            td = td.with_primary_key(column.name)

        with self._ydb_manager.session_pool.checkout() as session:
            return await asyncio.wrap_future(session.async_create_table(
                self.table_path, td
            ))

    async def perform_request(
        self,
        request: str,
        params: Dict,
        session: ydb.Session,
        transaction: ydb.TxContext,
        commit_tx: bool = False
    ) -> List[NamedTuple]:
        prepare_f = session.async_prepare(request)
        prepared_query = await asyncio.wrap_future(prepare_f)
        f = transaction.async_execute(
            prepared_query,
            params,
            commit_tx=commit_tx
        )
        result_sets = await asyncio.wait_for(
            asyncio.wrap_future(f),
            timeout=self._ydb_manager.timeout
        )

        if not result_sets:
            return None
        return [self.rec(**row) for row in result_sets[0].rows]

    def render_insert_request(self) -> str:
        template = Template(dedent("""\
        PRAGMA TablePathPrefix("{{ root_path }}");

        DECLARE $records AS "List<Struct<
        {%- for column in columns %}
            {{ column.name }}: {{ column.type }}
            {%- if not loop.last -%}, {%- endif -%}
        {%- endfor %}
        >>";

        REPLACE INTO [{{ table_rel_path }}]
        SELECT
        {%- for column in columns %}
            {{ column.name }}
            {%- if not loop.last -%}, {%- endif -%}
        {%- endfor %}
        FROM AS_TABLE($records);"""))

        return template.render(
            root_path=self._root_path,
            table_rel_path=self._table_rel_path,
            columns=self.columns
        )

    @property
    def insert_request(self) -> str:
        return self._insert_request

    async def insert(
        self,
        records: List[NamedTuple],
        session: ydb.Session,
        transaction: ydb.TxContext,
        commit_tx: bool = False
    ) -> List[NamedTuple]:
        return await self.perform_request(
            request=self.insert_request,
            params={'$records': records},
            session=session,
            transaction=transaction,
            commit_tx=commit_tx
        )

    def render_get_request(self) -> str:
        template = Template(dedent("""\
        PRAGMA TablePathPrefix("{{ root_path }}");
        {% for column in primary_keys %}
        DECLARE ${{ column.name }} AS {{ column.type }};
        {%- endfor %}

        SELECT
        {%- for column in columns %}
            {{ column.name }}
            {%- if not loop.last -%}, {%- endif -%}
        {%- endfor %}
        FROM [{{ table_rel_path }}]
        WHERE {% for column in primary_keys -%}
        {{ column.name }} = ${{ column.name }}
        {%- if not loop.last %} and {% endif -%}
        {%- endfor %};"""))

        return template.render(
            root_path=self._root_path,
            table_rel_path=self._table_rel_path,
            columns=self.columns,
            primary_keys=self.primary_keys
        )

    @property
    def get_request(self) -> str:
        return self._get_request

    def record_to_pk_params(self, record: NamedTuple) -> Dict[str, str]:
        return {
            f'${col.name}': getattr(record, col.name)
            for col in self.primary_keys
        }

    async def get(
        self,
        record: NamedTuple,
        session: ydb.Session,
        transaction: ydb.TxContext,
        commit_tx: bool = False
    ) -> List[NamedTuple]:
        return await self.perform_request(
            request=self.get_request,
            params=self.record_to_pk_params(record),
            session=session,
            transaction=transaction,
            commit_tx=commit_tx
        )

    async def get_one(self, *args, **kwargs) -> NamedTuple:
        rows: List[NamedTuple] = await self.get(*args, **kwargs)
        if not rows:
            return None
        return rows[0]

    async def drop(self) -> None:
        with self._ydb_manager.session_pool.checkout() as session:
            await asyncio.wrap_future(
                session.async_drop_table(self.table_path)
            )

    async def exists(self) -> bool:
        try:
            with self._ydb_manager.session_pool.checkout() as session:
                await asyncio.wrap_future(
                    session.async_describe_table(self.table_path)
                )
        except ydb.SchemeError:
            return False

        return True

    def render_delete_request(self) -> str:
        template = Template(dedent("""\
        PRAGMA TablePathPrefix("{{ root_path }}");
        {% for column in primary_keys %}
        DECLARE ${{ column.name }} AS {{ column.type }};
        {%- endfor %}

        DELETE FROM [{{ table_rel_path }}]
        WHERE {% for column in primary_keys -%}
        {{ column.name }} = ${{ column.name }}
        {%- if not loop.last %} and {% endif -%}
        {%- endfor %}"""))

        return template.render(
            root_path=self._root_path,
            table_rel_path=self._table_rel_path,
            primary_keys=self.primary_keys
        )

    @property
    def delete_request(self) -> str:
        return self._delete_request

    async def delete(
        self,
        record: NamedTuple,
        session: ydb.Session,
        transaction: ydb.TxContext,
        commit_tx: bool = False
    ) -> List[NamedTuple]:
        return await self.perform_request(
            request=self.delete_request,
            params=self.record_to_pk_params(record=record),
            session=session,
            transaction=transaction,
            commit_tx=commit_tx
        )


async def retriable_perform(
    ydb_manager: YdbManager,
    callee: Callable,
    *args,
    **kwargs
) -> Any:
    retries = ydb_manager.max_retries
    for attempt in range(retries + 1):
        try:
            with ydb_manager.session_pool.async_checkout() as async_session:
                session = await asyncio.wrap_future(async_session)
                transaction = session.transaction(ydb.SerializableReadWrite())

                return await callee(session, transaction, *args, **kwargs)

        except (ydb.issues.BadSession, asyncio.TimeoutError):
            await sensors.inc_counter('ydb_errors')
            if attempt == retries:
                raise
            logger.exception(
                'Exception at retriable_perform. Retrying %d/%d',
                attempt + 1,
                retries + 1
            )
