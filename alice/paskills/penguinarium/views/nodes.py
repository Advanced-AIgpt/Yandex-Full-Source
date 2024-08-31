from aiohttp import web
from typing import Dict
from alice.paskills.penguinarium.views.base import JsonSchemaValidateHandler
from alice.paskills.penguinarium.storages.nodes import NSRec


class AddNodeHandler(JsonSchemaValidateHandler):
    def __init__(self) -> None:
        super().__init__()

    async def _handle(
        self,
        payload: Dict,
        app: web.Application
    ) -> web.Response:
        # ns = app['nodes_storage']
        # for t in (ns._nodes_table, ns._chunks_table):
        #     if await t.exists():
        #         await t.drop()
        #     await t.create()

        intents_ids, utterances = [], []
        for intent in payload['intents']:
            intent_id = intent['intent_id']
            for utterance in intent['utterances']:
                intents_ids.append(intent_id)
                utterances.append(utterance)

        model_params = None
        if 'threshold' in payload:
            model_params = {'thresh': payload['threshold']}
        model = app['intent_resolver'].build_model(
            utterances=utterances,
            intents=intents_ids,
            model_params=model_params
        )

        node_id = payload['node_id']
        await app['nodes_storage'].add(NSRec(
            node_id=node_id,
            utterances=utterances,
            model=model
        ))

        return web.json_response({'node_id': node_id})

    @property
    def request_schema(self) -> str:
        return {
            'type': 'object',
            'properties': {
                'node_id': {'type': 'string'},
                'intents': {
                    'type': 'array',
                    'items': {
                        'type': 'object',
                        'properties': {
                            'intent_id': {'type': 'string'},
                            'utterances': {
                                'type': 'array',
                                'items': {'type': 'string'},
                                'minItems': 1,
                            },
                        },
                        'additionalProperties': False,
                        'required': ['intent_id', 'utterances'],
                    },
                    'minItems': 1,
                },
                'threshold': {
                    'type': 'number',
                    'exclusiveMinimum': 0,
                    'maximum': 2
                }
            },
            'additionalProperties': False,
            'required': ['node_id', 'intents'],
        }


class RemNodeHandler(JsonSchemaValidateHandler):
    def __init__(self) -> None:
        super().__init__()

    async def _handle(
        self,
        payload: Dict,
        app: web.Application
    ) -> web.Response:
        node_id = payload['node_id']
        await app['nodes_storage'].rem(node_id=node_id)

        return web.json_response({'node_id': node_id})

    @property
    def request_schema(self) -> str:
        return {
            'type': 'object',
            'properties': {
                'node_id': {'type': 'string'},
            },
            'additionalProperties': False,
            'required': ['node_id'],
        }
