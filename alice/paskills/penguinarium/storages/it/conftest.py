import pytest
import os
from alice.paskills.penguinarium.storages.ydb_utils import YdbManager
from alice.paskills.penguinarium.storages.tables import (
    YdbNodesTable, YdbModelsChunksTable, YdbGraphTable
)


@pytest.fixture(scope='module')
def endpoint():
    with open('ydb_endpoint.txt', 'r') as r:
        ept = r.read().strip()
    return ept


@pytest.fixture(scope='module')
def database():
    with open('ydb_database.txt', 'r') as r:
        db = r.read().strip()
    return db


@pytest.fixture
def ydb_manager(endpoint, database):
    os.environ['YDB_TOKEN'] = ''
    return YdbManager(
        endpoint=endpoint,
        database=database,
        connect_params={
            'use_tvm': False,
            'auth_token_env': 'YDB_TOKEN'
        },
        root_folder=''
    )


@pytest.fixture
async def nodes_table(ydb_manager):
    _nodes_table = YdbNodesTable(ydb_manager)
    await _nodes_table.create()
    yield _nodes_table
    await _nodes_table.drop()


@pytest.fixture
async def chunks_table(ydb_manager):
    _chunks_table = YdbModelsChunksTable(ydb_manager)
    await _chunks_table.create()
    yield _chunks_table
    await _chunks_table.drop()


@pytest.fixture
async def graph_table(ydb_manager):
    _graph_table = YdbGraphTable(ydb_manager)
    await _graph_table.create()
    yield _graph_table
    await _graph_table.drop()
