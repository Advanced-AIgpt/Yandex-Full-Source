# coding:utf-8
import copy

import alice.gamma.sdk.api.api_pb2 as api
import gamma_sdk.sdk.sdk as sdk


def new_session():
    return api.Session(
        new=True,
        messageId=0,
        sessionId='7357f00d-1337c0de-d34dc0d3-f33dbac6',
        userId='31337D34D7357C0D32',
        skillId='test',
    )


def new_state():
    return {}


def new_context(session, state, matches_dict=None):
    return TestingContext(session=session, state=state, matches_dict=matches_dict)


def next_message(context):
    session = copy.copy(context._SkillContext__session)
    session.messageId += 1
    session.new = False
    context._SkillContext__session = session


def text_request(text):
    return sdk.Request(command=text, original_utterance=text, type='SimpleUtterance')


DEFAULT_META = sdk.Meta(locale='ru-Ru', timezone='UTC', client_id='tests/1.0', interfaces=sdk.Interfaces(screen=False))


class TestingContext(sdk.SkillContext):
    def __init__(self, session, state, matches_dict):
        super().__init__(logger=None, ctx=None, stub=None, session=session, state=state, timeout=0)
        self.matches_dict = matches_dict

    def match(self, request, extractor, patterns):
        for (name, variables) in self.matches_dict.get(request.command, []):
            yield name, variables
        yield None, None
