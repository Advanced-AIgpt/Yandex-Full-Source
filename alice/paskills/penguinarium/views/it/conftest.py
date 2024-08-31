import pytest
from asynctest.mock import patch
from fakeredis import FakeStrictRedis
from aiohttp.test_utils import TestClient, TestServer

import asyncio
import os
import operator

from alice.paskills.penguinarium.init_app import init_app


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


@pytest.fixture(scope='module')
def fake_config(endpoint, database):
    return {
        'ydb': {
            'endpoint': endpoint,
            'database': database,
            'timeout': 5,
            'max_retries': 5,
            'root_folder': '',
            'connect_params': {
                'use_tvm': False,
                'auth_token_env': 'FAKE_YDB_TOKEN',
            }
        },
        'nodes_storage': {
            'cache_size': 256,
            'ttl': 600,
            'warm_up': {
                'nodes_idx': [],
                'sleep_time': 10000
            }
        },
        'dssm': {
            'path': 'qe_model',
            'input_name': 'query',
            'output_name': 'query_embedding',
            'empty_inputs': ['expansion'],
            'cache_size': 20000
        },
        'model': {
            'thresh': 0.65,
            'dist_thresh_rel': operator.lt,
            'metric': 'minkowski',
            'n_neighbors': 3,
            'p': 2
        },
        'metrics': {
            'http_requests': {'type': 'rate'},
            'http_response_time': {'type': 'hist', 'bins': [1, 10, 30, 50]},
            'http_responses': {'type': 'rate'},
        },
        'redis': {
            'address': '',
            'maxsize': 10
        }
    }


@pytest.fixture(scope='module')
def event_loop():
    loop = asyncio.get_event_loop()
    yield loop
    loop.close()


class MockedFakeStrictRedis(FakeStrictRedis):
    async def lrange(self, *args, **kwargs):
        return super().lrange(*args, **kwargs)

    async def iscan(self, *args, **kwargs):
        _, keys = super().scan(*args, **kwargs)
        for k in keys:
            yield k

    def pipeline(self):
        return self

    async def execute(self):
        pass

    async def wait_closed(self):
        pass


@pytest.fixture(scope='module')
def app(fake_config):
    auth_token_env = fake_config['ydb']['connect_params']['auth_token_env']
    os.environ[auth_token_env] = ''

    return init_app(fake_config)


@pytest.fixture(scope='module')
async def client(app):
    with patch('aioredis.create_redis_pool', return_value=MockedFakeStrictRedis()):
        client = TestClient(TestServer(app))
        await client.start_server()

    await app['nodes_storage']._nodes_table.create()
    await app['nodes_storage']._chunks_table.create()
    await app['graph_storage']._graph_table.create()

    try:
        yield client
    finally:
        await app['nodes_storage']._nodes_table.drop()
        await app['nodes_storage']._chunks_table.drop()
        app['graph_storage']._graph_table.drop()
        await client.close()


@pytest.fixture(scope='function')
def add_grph_pld():
    return {
        'id': 'some-skill-id',
        'welcome_node': 'for_one',
        'nodes': [
            {
                'id': 'for_one',
                'response': 'Ну что, поехали?',
                'fallback_response': 'Не понимаю вас!',
                'edges': [
                    {
                        'node_id': 'lack_of',
                        'utterances': ['да', 'ага']
                    }
                ]
            },
            {
                'id': 'lack_of',
                'response': 'Слушайте мой первый вопрос! Чего вечно не хватает?',
                'fallback_response': 'Такого варианта нет, к сожалению!',
                'edges': [
                    {
                        'node_id': 'lack_of',
                        'utterances': ['денег', 'денежек', 'зарплаты']
                    },
                    {
                        'node_id': 'for_one',
                        'utterances': 'fallback'
                    }
                ]
            }
        ],
        'named_utterances': {
            'fallback': [
                'прекрати',
                'стоп игра',
            ],
        },
        'model_params': {
            'thresh': 0.6
        }
    }


@pytest.fixture(scope='function')
def get_grph_intnt_pld(utt_tokens, is_new_session):
    return {
        'request': {
            'nlu': {
                'tokens': utt_tokens,
            }
        },
        'session': {
            'new': is_new_session,
            'message_id': 4,
            'session_id': 'some-session-id',
            'skill_id': 'some-skill-id',
            'user_id': 'some-user-id'
        },
        'state': {
            'session': {
                'node_id': 'for_one'
            }
        },
        'version': '1.0'
    }
