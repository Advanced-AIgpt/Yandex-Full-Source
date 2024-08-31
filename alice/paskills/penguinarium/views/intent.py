import logging
from aiohttp import web
from typing import Dict

from alice.paskills.penguinarium.views.base import JsonSchemaValidateHandler
from alice.paskills.penguinarium.ml.intent_resolver import IResolveRes

logger = logging.getLogger(__name__)


class IntentResolvingHandler(JsonSchemaValidateHandler):
    def __init__(self):
        super().__init__()

    async def _resolve_intents(
        self,
        app: web.Application,
        node_id: str,
        utterance: str
    ) -> IResolveRes:
        ns_rec = await app['nodes_storage'].get(node_id=node_id)

        if ns_rec is None:
            logger.error('Unknown node id %s', node_id)
            raise web.HTTPBadRequest(reason=f'Unknown node id {node_id}')

        intents = await app['intent_resolver'].resolve_intents(
            utterance=utterance,
            model=ns_rec.model,
        )

        return intents


class GetIntentsHandler(IntentResolvingHandler):
    def __init__(self):
        super().__init__()

    async def _handle(
        self,
        payload: Dict,
        app: web.Application
    ) -> web.Response:
        distances, intents = await self._resolve_intents(
            app=app,
            node_id=payload['node_id'],
            utterance=payload['utterance'],
        )

        return web.json_response({
            'distances': distances,
            'intents': intents,
        })

    @property
    def request_schema(self) -> str:
        return {
            'type': 'object',
            'properties': {
                'node_id': {'type': 'string'},
                'utterance': {'type': 'string'},
            },
            'additionalProperties': False,
            'required': ['node_id', 'utterance'],
        }
