# coding: utf-8
from gamma_sdk.sdk import sdk
import gamma_sdk.client as client


class State:
    def __init__(self, counter=0):
        self.counter = counter

    def to_dict(self):
        return{
            "counter": self.counter
        }

    @classmethod
    def from_dict(cls, dct):
        return cls(**dct)


class EchoSkill(sdk.Skill):
    state_cls = State

    def handle(self, logger, context, request, meta):

        if not context.is_new_session():
            text = '{}: {}'.format(context.state.counter, request.original_utterance)
            response = sdk.Response(text=text, tts=text)
        else:
            context.state.counter = 0
            response = sdk.Response(text='Ηχω', tts='Эхо')

        logger.debug('User utterance: %s; Stored value: %d', request.original_utterance, context.state.counter)
        context.state.counter += 1

        return response


def main():
    client.start_skill(EchoSkill())


if __name__ == '__main__':
    main()
