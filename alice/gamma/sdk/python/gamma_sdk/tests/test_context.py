# coding: utf-8

import grpc
import pytest
import logging
from concurrent import futures

import alice.gamma.sdk.api.api_pb2 as gamma_api
import alice.gamma.sdk.api.commands_pb2 as api
import alice.gamma.sdk.api.commands_pb2_grpc as grpc_api
from gamma_sdk import client
from gamma_sdk.sdk import sdk
from gamma_sdk.testing.sdk import new_session, new_state, text_request


class SdkServerMock(grpc_api.SdkServicer):
    def Match(self, request, context):
        if request.input == 'Алиса ты котик':
            return api.MatchResponse(matches=[
                api.MatchResponse.Match(
                    name='any',
                    variables={'animal': api.MatchResponse.Values(values=[b'"cat"'])}
                )
            ])
        return api.MatchResponse(matches=[])


@pytest.fixture
def sdk_server():
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=1))
    grpc_api.add_SdkServicer_to_server(
        SdkServerMock(), server
    )
    port = server.add_insecure_port('[::]:0')
    server.start()
    return port


@pytest.fixture
def sdk_client(sdk_server):
    return client._SdkClient('localhost:' + str(sdk_server))


@pytest.fixture
def sdk_stub(sdk_client):
    return sdk_client.connect()


def test_match(sdk_stub):
    context = sdk.SkillContext(
        logger=logging.getLogger('tests'),
        ctx=None,
        stub=sdk_stub,
        session=new_session(),
        state=new_state()
    )
    extractor = sdk.EntityExtractor(
        {
            'animal': {
                'cat': 'котик'
            }
        }
    )
    result = context.match(text_request('Алиса ты котик'), extractor, {
        'any': '* ты $animal',
        'none': 'нет ты',
    })
    assert list(result) == [('any', {'animal': ['cat']}), (None, None)]


class State:
    def __init__(self, used_animals=None, **kwargs):
        self.used_animals = used_animals or []
        self.d = kwargs

    def to_dict(self):
        d = {'used_animals': self.used_animals}
        d.update(self.d)
        return d

    @classmethod
    def from_dict(cls, d):
        return cls(**d)


class Skill(sdk.Skill):
    state_cls = State

    def handle(self, logger, context, request, meta):
        context.state.used_animals.append('dinosaur')
        return sdk.Response(text='', tts='')


def test_state(sdk_client):
    logger = logging.getLogger(__name__)
    handler = client._SkillHandler(skill=Skill(), sdk_client=sdk_client, logger=logger, sdk_timeout=0.1)
    request = gamma_api.SkillRequest(
        request=gamma_api.RequestBody(
            command='test'
        ),
        session=gamma_api.Session(),
        state=gamma_api.State(
            storage=b"{"
                    b"  \"state\": \"answer processing\","
                    b"  \"last_sound\": \"<speaker audio=\\\"Alisa_animal_quiz_chicken_1.opus\\\">\","
                    b"  \"attempts\": 0, \"num_questions\": 3,"
                    b"  \"used_animals\": [\"crow\", \"chicken\", \"frog\"],"
                    b"  \"last_record\": \"chicken\""
                    b"}"
        )
    )
    response = handler.Handle(request, None)
    assert response.state.storage == (
        b"{"
        b"\"used_animals\": [\"crow\", \"chicken\", \"frog\", \"dinosaur\"], "
        b"\"state\": \"answer processing\", "
        b"\"last_sound\": \"<speaker audio=\\\"Alisa_animal_quiz_chicken_1.opus\\\">\", "
        b"\"attempts\": 0, \"num_questions\": 3, "
        b"\"last_record\": \"chicken\""
        b"}"
    )


def test_bad_state(sdk_client):
    logger = logging.getLogger(__name__)
    handler = client._SkillHandler(skill=Skill(), sdk_client=sdk_client, logger=logger, sdk_timeout=0.1)
    request = gamma_api.SkillRequest(
        request=gamma_api.RequestBody(
            command='test'
        ),
        session=gamma_api.Session(),
        state=gamma_api.State(storage=b"some data here")
    )
    response = handler.Handle(request, None)
    assert response.state.storage == (b"{\"used_animals\": [\"dinosaur\"]}")
