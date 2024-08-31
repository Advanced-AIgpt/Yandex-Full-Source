from aiohttp import web
from collections import namedtuple
import logging
import random
from typing import Tuple, List, Dict, Any

from alice.paskills.penguinarium.views.base import JsonSchemaValidateHandler
from alice.paskills.penguinarium.views.intent import IntentResolvingHandler
from alice.paskills.penguinarium.storages.nodes import NSRec
from alice.paskills.penguinarium.storages.graph import GSrec

logger = logging.getLogger(__name__)


class AddGraphHandler(JsonSchemaValidateHandler):
    def __init__(self) -> None:
        super().__init__()

    def _prep_intents(
        self,
        edges: List[Dict],
        named_utterances: Dict[str, List[str]]
    ) -> Tuple[List[str], List[str]]:
        intents_ids, utterances = [], []
        for edge in edges:
            intent_id = edge['node_id']

            edge_utts = edge['utterances']
            if isinstance(edge_utts, str):
                edge_utts = named_utterances[edge_utts]

            for utterance in edge_utts:
                intents_ids.append(intent_id)
                utterances.append(utterance)

        return intents_ids, utterances

    def _prep_ns_reqs(
        self,
        nodes: List[Dict],
        app: web.Application,
        model_params: Dict,
        named_utterances: Dict[str, List[str]]
    ) -> List[NSRec]:
        ns_recs = []
        for node in nodes:
            intents_ids, utterances = self._prep_intents(
                edges=node['edges'],
                named_utterances=named_utterances
            )
            model = app['intent_resolver'].build_model(
                utterances=utterances,
                intents=intents_ids,
                model_params=model_params
            )

            ns_recs.append(NSRec(
                node_id=node['id'],
                utterances=utterances,
                model=model
            ))

        return ns_recs

    async def _handle(
        self,
        payload: Dict,
        app: web.Application
    ) -> web.Response:
        ns_reqs = self._prep_ns_reqs(
            nodes=payload['nodes'],
            app=app,
            model_params=payload.get('model_params'),
            named_utterances=payload.get('named_utterances', {})
        )
        await app['graph_storage'].add(GSrec(
            graph_id=payload['id'],
            graph_descr=payload,
            ns_recs=ns_reqs
        ))

        return web.json_response({'skill_id': payload['id']})

    @property
    def request_schema(self) -> str:
        return {
            'type': 'object',
            'properties': {
                'id': {'type': 'string'},
                'welcome_node': {'type': 'string'},
                'nodes': {
                    'type': 'array',
                    'items': {
                        'type': 'object',
                        'properties': {
                            'id': {'type': 'string'},
                            'response': {
                                'anyOf': [
                                    {'type': 'string'},
                                    {'type': 'object'},
                                    {
                                        'type': 'array',
                                        'items': {
                                            'anyOf': [
                                                {'type': 'string'},
                                                {'type': 'object'},
                                            ]
                                        },
                                        'minItems': 1
                                    },
                                ]
                            },
                            'fallback_response': {
                                'anyOf': [
                                    {'type': 'string'},
                                    {'type': 'object'},
                                    {
                                        'type': 'array',
                                        'items': {
                                            'anyOf': [
                                                {'type': 'string'},
                                                {'type': 'object'},
                                            ]
                                        },
                                        'minItems': 1
                                    },
                                ]
                            },
                            'edges': {
                                'type': 'array',
                                'items': {
                                    'type': 'object',
                                    'properties': {
                                        'node_id': {'type': 'string'},
                                        'utterances': {
                                            'anyOf': [
                                                {
                                                    'type': 'string'
                                                },
                                                {
                                                    'type': 'array',
                                                    'items': {'type': 'string'},
                                                    'minItems': 1
                                                },
                                            ]
                                        },
                                    },
                                    'additionalProperties': False,
                                    'required': ['node_id', 'utterances']
                                },
                                'minItems': 0
                            }
                        },
                        'additionalProperties': False,
                        'required': ['id', 'response', 'fallback_response', 'edges']
                    },
                    'minItems': 1
                },
                'named_utterances': {
                    'type': 'object',
                    'patternProperties': {
                        '^[a-zA-Z0-9_]+$': {
                            'type': 'array',
                            'items': {'type': 'string'},
                            'minItems': 1
                        },
                    },
                    'additionalProperties': False,
                },
                'model_params': {'type': 'object'}
            },
            'additionalProperties': False,
            'required': ['id', 'welcome_node', 'nodes'],
        }

    def _validate_request(self, payload: Dict) -> Tuple[bool, str]:
        schema_valid, reason = super()._validate_request(payload)
        if not schema_valid:
            return False, reason

        given_nodes = set()
        edges_nodes = set()
        used_named_utterances = set()
        for node in payload['nodes']:
            given_nodes.add(node['id'])
            for edge in node['edges']:
                edges_nodes.add(edge['node_id'])
                if isinstance(edge['utterances'], str):
                    used_named_utterances.add(edge['utterances'])

        if payload['welcome_node'] not in given_nodes:
            return False, 'Welcome node id not found in nodes'

        if not edges_nodes <= given_nodes:
            ex_node = (edges_nodes - given_nodes).pop()
            return False, f'\"{ex_node}\" found in edge but not in nodes'

        named_utterances = payload.get('named_utterances', {}).keys()
        if not used_named_utterances.issubset(named_utterances):
            bad_named_utts = used_named_utterances - set(named_utterances)
            ex_named = bad_named_utts.pop()
            return False, f'\"{ex_named}\" named entity used but not given'

        return True, None


GraphDescr = namedtuple('GraphDescr', ['id', 'welcome_node', 'id2node'])


class GetGraphIntent(IntentResolvingHandler):
    def __init__(self) -> None:
        super().__init__()

    async def _get_graph_descr(
        self,
        app: web.Application,
        skill_id: str
    ) -> GraphDescr:
        gs_rec = await app['graph_storage'].get(graph_id=skill_id)

        if gs_rec is None:
            logger.error('Unknown graph id %s', skill_id)
            raise web.HTTPBadRequest(reason=f'Unknown graph id {skill_id}')

        return GraphDescr(
            id=gs_rec.graph_id,
            welcome_node=gs_rec.graph_descr['welcome_node'],
            id2node={
                node['id']: node
                for node in gs_rec.graph_descr['nodes']
            }
        )

    async def _get_response(
        self,
        app: web.Application,
        node_id: str,
        id2node: Dict[str, Any],
        utterance: str
    ) -> Tuple[str, str]:
        _, intents = await self._resolve_intents(
            app=app,
            node_id=node_id,
            utterance=utterance,
        )

        if not intents:
            return node_id, id2node[node_id]['fallback_response']

        best_intent = intents[0]
        return best_intent, id2node[best_intent]['response']

    async def _handle(
        self,
        payload: Dict,
        app: web.Application
    ) -> web.Response:
        graph_descr = await self._get_graph_descr(
            app=app,
            skill_id=payload['session']['skill_id']
        )
        session = payload['session']

        if session['new']:
            new_intent = graph_descr.welcome_node
            response = graph_descr.id2node[new_intent]['response']
        else:
            session_state = payload.get('state', {}).get('session', {})
            node_id = session_state.get('node_id', graph_descr.welcome_node)

            tokens = payload['request']['nlu']['tokens']
            utterance = ' '.join(tokens)

            new_intent, response = await self._get_response(
                app=app,
                node_id=node_id,
                id2node=graph_descr.id2node,
                utterance=utterance
            )

        if isinstance(response, list):
            response = random.choice(response)
        if isinstance(response, str):
            response = {'text': response}
        return web.json_response({
            'response': response,
            'session': {
                'message_id': session['message_id'],
                'session_id': session['session_id'],
                'user_id': session['user_id'],
            },
            'session_state': {
                'node_id': new_intent
            },
            'version': payload['version'],
        })

    @property
    def request_schema(self) -> str:
        return {
            'type': 'object',
            'properties': {
                'request': {
                    'type': 'object',
                    'properties': {
                        'nlu': {
                            'type': 'object',
                            'properties': {
                                'tokens': {
                                    'type': 'array',
                                    'items': {'type': 'string'},
                                    'minItems': 0,
                                },
                            },
                            'additionalProperties': True,
                            'required': ['tokens'],
                        }
                    },
                    'additionalProperties': True,
                    'required': ['nlu'],
                },
                'session': {
                    'type': 'object',
                    'properties': {
                        'message_id': {'type': 'integer'},
                        'session_id': {'type': 'string'},
                        'user_id': {'type': 'string'},
                    },
                    'additionalProperties': True,
                    'required': ['message_id', 'session_id', 'user_id'],
                },
                'version': {'type': 'string'},
            },
            'additionalProperties': True,
            'required': ['request', 'session', 'version'],
        }

    @property
    def response_schema(self) -> str:
        return {
            'type': 'object',
            'properties': {
                'response': {
                    'type': 'object',
                    'properties': {
                        'text': {'type': 'string'},
                        'tts': {'type': 'string'},
                    },
                    'additionalProperties': True,
                    'required': ['text']
                },
                'session': {
                    'type': 'object',
                    'properties': {
                        'message_id': {'type': 'integer'},
                        'session_id': {'type': 'string'},
                        'user_id': {'type': 'string'},
                    },
                    'additionalProperties': True,
                    'required': ['message_id', 'session_id', 'user_id'],
                },
                'session_state': {'type': 'object'},
                'version': {'type': 'string'},
            },
            'additionalProperties': False,
            'required': ['response', 'session', 'session_state', 'version'],
        }


class RemGraphHandler(JsonSchemaValidateHandler):
    def __init__(self) -> None:
        super().__init__()

    async def _handle(
        self,
        payload: Dict,
        app: web.Application
    ) -> web.Response:
        graph_id = payload['graph_id']
        await app['graph_storage'].rem(graph_id=graph_id)

        return web.json_response({'graph_id': graph_id})

    @property
    def request_schema(self) -> str:
        return {
            'type': 'object',
            'properties': {
                'graph_id': {'type': 'string'},
            },
            'additionalProperties': False,
            'required': ['graph_id'],
        }
