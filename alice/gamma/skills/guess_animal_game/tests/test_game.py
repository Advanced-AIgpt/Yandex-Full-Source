# coding: utf-8
import random

import attr
import logging
import pytest

from gamma_sdk.testing import sdk as testing

from alice.gamma.skills.guess_animal_game.game.game import GuessAnimalSkill
from alice.gamma.skills.guess_animal_game.game.game import State
from alice.gamma.skills.guess_animal_game.game import resources


logging.basicConfig(level=logging.DEBUG)


@pytest.fixture
def skill():
    return GuessAnimalSkill(max_questions=2)


@pytest.fixture(scope='function', autouse=True)
def freeze_random():
    random.seed(42)


@pytest.fixture(scope='function')
def session():
    return testing.new_session()


@pytest.fixture(scope='function')
def state():
    return State()


@pytest.fixture(scope='function')
def context(session, state):
    matches_dict = {
        'я устал': [
            (resources.intents__stop_game, {})
        ],
        'да': [
            (resources.intents__start_game_yes, {})
        ],
        'давай': [
            (resources.intents__new_round_yes, {})
        ],
        'коровка': [
            (resources.intents__user_valid_answer, {'Animal': ['cow']})
        ],
        'повтори звук': [
            (resources.intents__repeat_sound, {})
        ],
        'наверное это петух': [
            (resources.intents__user_valid_answer, {'Animal': ['rooster']})
        ],
        'какой счет?': [
            (resources.intents__score, {})
        ],
        'курица?': [
            (resources.intents__user_valid_answer, {'Animal': ['chicken']})
        ],
        'петух': [
            (resources.intents__user_valid_answer, {'Animal': ['rooster']})
        ],
        'лось?': [
            (resources.intents__user_valid_answer, {'Animal': ['moose']})
        ],
        'не лошадки': [
            (resources.intents__user_invalid_answer, {'Animal': ['horse']})
        ],
        'точно лошадка': [
            (resources.intents__user_valid_answer, {'Animal': ['horse']})
        ],
    }
    return testing.new_context(session, state, matches_dict)


@pytest.fixture
def meta():
    return testing.DEFAULT_META


@attr.s(frozen=True)
class Response:
    state = attr.ib(type=str)
    text = attr.ib(type=list, default=None)
    context = attr.ib(type=dict, factory=dict)
    end_session = attr.ib(type=bool, default=None)


@attr.s(frozen=True)
class Command:
    request = attr.ib(type=str)
    response = attr.ib(type=Response)


def format_error(i, command, key, value):
    return 'command {}, request {}, key {}, expected: {}, actual: {}\n'.format(
        i, command.request, key, value, getattr(context.state, key)
    )


@pytest.mark.parametrize(
    'commands',
    [[
        Command(
            'привет. давай сыграем в угадай животное',
            Response(
                state=resources.STATE_GAME_START
            )
        ),
        Command(
            'да',
            Response(
                state=resources.STATE_ANSWER_PROCESSING,
                context={
                    'last_record': 'rooster',
                    'num_questions': 1,
                }
            )
        ),
        Command(
            'коровка',
            Response(
                state=resources.STATE_ANSWER_PROCESSING,
                context={
                    'attempts': 1,
                }
            )
        ),
        Command(
            'повтори звук',
            Response(
                state=resources.STATE_ANSWER_PROCESSING,
                context={
                    'attempts': 1,
                },
                text=['Хорошо.'],
            )
        ),
        Command(
            'наверное это петух',
            Response(
                state=resources.STATE_ANSWER_PROCESSING,
                context={
                    'attempts': 0,
                    'right_answers': 1,
                    'num_questions': 2,
                    'last_record': 'chicken',
                },
            )
        ),
        Command(
            'какой счет?',
            Response(
                state=resources.STATE_ANSWER_PROCESSING,
                context={
                    'attempts': 0,
                    'right_answers': 1,
                    'num_questions': 2,
                    'last_record': 'chicken',
                },
                text=['В этом раунде вы угадали 1 животное из 2.'],
            )
        ),
        Command(
            'курица?',
            Response(
                state=resources.STATE_NEW_ROUND,
                context={
                    'attempts': 0,
                    'right_answers': 2,
                    'num_questions': 2,
                    'last_record': 'chicken',
                },
                text=[
                    'Ура! Это правильный ответ.',
                    'Отлично! В этом раунде вы угадали 2 животных из 2.',
                    'Хотите сыграть ещё раунд?',
                ],
            )
        ),
        Command(
            'я устал',
            Response(
                state=resources.STATE_EXIT,
                text=['Давайте закончим!'],
                end_session=True,
            )
        ),
    ], [
        Command(
            'привет. давай сыграем в угадай животное',
            Response(state=resources.STATE_GAME_START)
        ),
        Command(
            'да',
            Response(state=resources.STATE_ANSWER_PROCESSING)
        ),
        Command(
            'петух',
            Response(
                state=resources.STATE_ANSWER_PROCESSING,
                context={
                    'attempts': 0,
                    'right_answers': 1,
                    'num_questions': 2,
                }
            )
        ),
        Command(
            'лось?',
            Response(
                state=resources.STATE_NEW_ROUND,
                context={
                    'attempts': 0,
                    'right_answers': 2,
                    'num_questions': 2,
                },
            )
        ),
        Command(
            'давай',
            Response(
                state=resources.STATE_ANSWER_PROCESSING,
                context={
                    'attempts': 0,
                    'right_answers': 0,
                    'num_questions': 1,
                    'used_animals': ['horse', 'moose', 'rooster'],
                    'last_record': 'horse'
                }
            )
        ),
        Command(
            'не лошадки',
            Response(
                state=resources.STATE_ANSWER_PROCESSING,
                context={
                    'attempts': 1,
                    'right_answers': 0,
                    'num_questions': 1,
                    'used_animals': ['horse', 'moose', 'rooster'],
                    'last_record': 'horse'
                }
            )
        ),
        Command(
            'точно лошадка',
            Response(
                state=resources.STATE_ANSWER_PROCESSING,
                context={
                    'attempts': 0,
                    'right_answers': 1,
                    'num_questions': 2,
                    'used_animals': ['horse', 'moose', 'rooster', 'sheep'],
                    'last_record': 'sheep'
                }
            )
        ),
    ]]
)
def test_simple_call(context, skill, meta, commands):
    for i, command in enumerate(commands):
        new_state = skill.play(
            logging.getLogger('tests'),
            context,
            testing.text_request(command.request),
            meta
        )
        fail_message = 'command {}, request {}'.format(i, command.request)
        assert new_state == command.response.state, fail_message

        if command.response.text is not None:
            assert context.text == command.response.text, fail_message
        if command.response.end_session is not None:
            assert context.end_session == command.response.end_session, fail_message
        for key, value in command.response.context.items():
            if key == 'used_animals':
                assert set(getattr(context.state, key)) == set(value), format_error(i, command, key, value)
            else:
                assert getattr(context.state, key) == value, format_error(i, command, key, value)
        testing.next_message(context)
