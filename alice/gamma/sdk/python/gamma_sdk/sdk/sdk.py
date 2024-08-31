import attr
import json

import alice.gamma.sdk.api.api_pb2 as api
import alice.gamma.sdk.api.commands_pb2 as commands
from gamma_sdk.inner.matcher import Entity, EntityExtractor
from .button import Button
from .card import Card


class Skill:
    entities = {}

    def __init__(self):
        self.extractor = EntityExtractor(self.entities)

    def handle(self, logger, context, request, meta):
        raise NotImplementedError()


class SkillContext:
    def __init__(self, logger, ctx, stub, session, state, timeout=0.25):
        self.__logger = logger
        self.__ctx = ctx
        self.__stub = stub
        self.state = state
        self.__session = session
        self.__timeout = timeout

    def is_new_session(self):
        return self.__session.new

    def match(self, request, extractor, patterns):
        entities = extractor.get_entities_as_list(request.command)
        proto_entities = [
            api.Entity(
                start=entity.begin,
                end=entity.end,
                type=entity.type,
                value=json.dumps(entity.value, ensure_ascii=False).encode('utf-8')
            ) for entity in entities
        ]
        proto_patterns = [
            commands.MatchRequest.Pattern(
                name=name,
                pattern=pattern
            ) for name, pattern in patterns.items()
        ]
        request = commands.MatchRequest(
            input=request.command,
            entities=proto_entities,
            patterns=proto_patterns
        )
        response = self.__stub.Match(request, self.__timeout)
        for match_ in response.matches:
            variables = _variables_from_proto(match_.variables)
            self.__logger.debug('Matched: %s (%s)', match_.name, variables)
            yield match_.name, variables
        yield None, None


def _variables_from_proto(proto):
    result = {}
    for name, values in proto.items():
        result[name] = [json.loads(value) for value in values.values]
    return result


@attr.s(frozen=True)
class Nlu:
    tokens = attr.ib(factory=list)
    entities = attr.ib(factory=dict)

    @classmethod
    def from_proto(cls, proto):
        entities = {}
        if proto.entities:
            for entity in proto.entities:
                if entity.type not in entities:
                    entities[entity.type] = []
                value = entity.protoValue
                entities[entity.type].append(
                    Entity(
                        type=entity.type,
                        begin=entity.start,
                        end=entity.end,
                        value=value
                    )
                )
        return cls(
            tokens=[token for token in proto.tokens] if proto.tokens else [],
            entities=entities
        )


@attr.s(frozen=True, str=False)
class Request:
    command = attr.ib(type=str)
    original_utterance = attr.ib(type=str)
    type = attr.ib(type=str)
    payload = attr.ib(default=None)
    nlu = attr.ib(type=Nlu, default=Nlu())

    @classmethod
    def from_proto(cls, proto):
        return cls(
            command=proto.command,
            original_utterance=proto.originalUtterance,
            payload=json.loads(proto.payload) if proto.payload else None,
            type=proto.type,
            nlu=Nlu.from_proto(proto.nlu) if proto.nlu else Nlu(),
        )

    def __str__(self):
        return 'Request(command={command}, utterance={utterance}, type={type}, payload={payload}, nlu={nlu})'.format(
            command=self.command, utterance=self.original_utterance, type=self.type, payload=self.payload, nlu=self.nlu
        )


@attr.s(frozen=True)
class Interfaces:
    screen = attr.ib(type=bool)


@attr.s(frozen=True)
class Meta:
    locale = attr.ib(type=str)
    timezone = attr.ib(type=str)
    client_id = attr.ib(type=str)
    interfaces = attr.ib(type=Interfaces)

    @classmethod
    def from_proto(cls, proto):
        return cls(
            locale=proto.locale,
            timezone=proto.timezone,
            client_id=proto.clientId,
            interfaces=Interfaces(proto.interfaces.screen if proto.interfaces else False),
        )


@attr.s(frozen=True)
class Response:
    text = attr.ib(type=str)
    tts = attr.ib(type=str)
    card = attr.ib(type=Card, default=None)
    buttons = attr.ib(type=list, default=list())
    end_session = attr.ib(type=bool, default=False)

    def add_buttons(self, *buttons):
        self.buttons.extend(buttons)

    def to_proto(self):
        return api.ResponseBody(
            text=self.text,
            tts=self.tts,
            buttons=Button.buttons_to_proto(self.buttons) if self.buttons else None,
            card=self.card.to_proto() if self.card else None,
            endSession=self.end_session,
        )


# deprecated
def match(matcher, extractor, context, request):
    entities = extractor.get_entities(request.command)
    for match_ in matcher.match(request.command, entities):
        yield match_
