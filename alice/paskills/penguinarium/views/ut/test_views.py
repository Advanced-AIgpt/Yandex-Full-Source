import pytest
from unittest.mock import patch
import operator
import numpy as np
import json

from alice.paskills.penguinarium.storages.nodes import BaseNodesStorage
from alice.paskills.penguinarium.ml.intent_resolver import DssmKnnIntentResolver
from alice.paskills.penguinarium.ml.embedder import BaseEmbedder
from alice.paskills.penguinarium.ml.index import BaseIndex, SearchRes, ModelFactory
from alice.paskills.penguinarium.views.nodes import AddNodeHandler, RemNodeHandler
from alice.paskills.penguinarium.views.intent import GetIntentsHandler
from alice.paskills.penguinarium.views.misc import solomon_handler
from alice.paskills.penguinarium.views.graph import AddGraphHandler


class FakeIndex(BaseIndex):
    def __init__(self, thresh):
        super().__init__(
            thresh=thresh,
            dist_thresh_rel=operator.lt,
            n_neighbors=1
        )

    def _build(self, embeddings, n_neighbors):
        self._embeddings = embeddings

    def _search(self, embedding):
        for i, embed in enumerate(self._embeddings):
            if embed == embedding:
                return SearchRes(preds=[i], distances=[0.])

        return SearchRes(preds=[0], distances=[2.])

    def _serialize(self):
        """
        >>> fi = FakeIndex(thresh=0)
        >>> fi._serialize()
        """
        pass

    @classmethod
    def load(cls, serialized):
        """
        >>> fi = FakeIndex(thresh=0)
        >>> fi.load(b'')
        """
        pass


class FakeEmbedder(BaseEmbedder):
    def embed(self, text):
        return [len(text)]


class FakeNodesStorage(BaseNodesStorage):
    def __init__(self):
        self._storage = {}

    async def add(self, ns_rec):
        self._storage[ns_rec.node_id] = ns_rec

    async def get(self, node_id):
        return self._storage.get(node_id)

    async def rem(self, node_id):
        self._storage.pop(node_id, None)


class FakeRequest:
    def __init__(self, payload, storage=None):
        self._payload = payload
        self.app = {
            'intent_resolver': DssmKnnIntentResolver(
                embedder=FakeEmbedder(),
                model_factory=ModelFactory(FakeIndex, thresh=1.)
            ),
            'nodes_storage': storage or FakeNodesStorage()
        }

    async def json(self):
        return self._payload


@pytest.mark.parametrize(
    'handler,req,resp_content',
    [
        (
            AddNodeHandler(),
            {
                'node_id': '1',
                'intents': [{'intent_id': '1', 'utterances': ['.']}]
            },
            {'node_id': '1'}
        ),
        (
            RemNodeHandler(),
            {'node_id': '1'},
            {'node_id': '1'}
        ),
    ],
    ids=[
        'AddNodeHandler',
        'RemNodeHandler',
    ]
)
@pytest.mark.asyncio
async def test_handler_answer_empty_storage(handler, req, resp_content):
    resp = await handler.handle(FakeRequest(req))
    assert json.loads(resp.text) == resp_content


@pytest.mark.parametrize(
    'handler,req,resp_content',
    [
        (
            GetIntentsHandler(),
            {'node_id': 'tinkoff', 'utterance': 'Хоп хей лалалей'},
            {'intents': [], 'distances': []}
        ),
        (
            GetIntentsHandler(),
            {'node_id': 'tinkoff', 'utterance': 'Хоп'},
            {'intents': ['1'], 'distances': [0.]}
        )
    ],
    ids=[
        'GetIntentsHandler-unknown',
        'GetIntentsHandler-tinkoff'
    ]
)
@pytest.mark.asyncio
async def test_handler_answer(handler, req, resp_content):
    storage = FakeNodesStorage()

    add_req = FakeRequest({
        'node_id': 'tinkoff',
        'intents': [{'intent_id': '1', 'utterances': ['...']}]
    }, storage)
    await AddNodeHandler().handle(add_req)

    request = FakeRequest(req, storage)
    resp = await handler.handle(request)
    assert json.loads(resp.text) == resp_content


@pytest.mark.asyncio
async def test_storage_add_rem():
    storage = FakeNodesStorage()

    add_req = FakeRequest({
        'node_id': '1',
        'intents': [{'intent_id': '1', 'utterances': ['...']}]
    }, storage)
    await AddNodeHandler().handle(add_req)

    embeddings = storage._storage['1'].model._embeddings
    assert embeddings == np.array([[3]], dtype=np.float32)

    rem_req = FakeRequest({'node_id': '1'}, storage)
    await RemNodeHandler().handle(rem_req)

    assert len(storage._storage) == 0


@pytest.mark.asyncio
async def test_storage_add_with_thresh():
    storage = FakeNodesStorage()

    add_req = FakeRequest({
        'node_id': '1',
        'intents': [{'intent_id': '1', 'utterances': ['...']}],
        'threshold': 0.5
    }, storage)
    await AddNodeHandler().handle(add_req)

    assert storage._storage['1'].model._thresh == 0.5


@pytest.mark.asyncio
async def test_solomon_handler():
    smn = await solomon_handler(FakeRequest({}))
    assert json.loads(smn.text) == {'sensors': []}


def test_add_graph_validate_request():
    agh = AddGraphHandler()

    m2patch = 'alice.paskills.penguinarium.views.base.' \
              'JsonSchemaValidateHandler._validate_request'
    with patch(m2patch, return_value=(True, None)):
        assert agh._validate_request({
            'welcome_node': 'n1',
            'nodes': [{
                'id': 'n1',
                'edges': []
            }]
        }) == (True, None)

        assert agh._validate_request({
            'welcome_node': 'n2',
            'nodes': [{
                'id': 'n1',
                'edges': []
            }]
        }) == (False, 'Welcome node id not found in nodes')

        assert agh._validate_request({
            'welcome_node': 'n1',
            'nodes': [{
                'id': 'n1',
                'edges': [{
                    'node_id': 'n2',
                    'utterances': ['привет']
                }]
            }]
        }) == (False, '\"n2\" found in edge but not in nodes')

        assert agh._validate_request({
            'welcome_node': 'n1',
            'nodes': [{
                'id': 'n1',
                'edges': [{
                    'node_id': 'n1',
                    'utterances': 'named'
                }]
            }]
        }) == (False, '\"named\" named entity used but not given')
