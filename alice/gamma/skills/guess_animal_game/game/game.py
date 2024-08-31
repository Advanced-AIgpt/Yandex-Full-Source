# coding: utf-8
import random

from gamma_sdk.sdk import sdk

from . import data
from . import resources


class State:
    def __init__(self, num_questions=0, attempts=0, used_animals=None, last_record='', last_sound='',
                 right_answers=0, invalid_answers=0, max_attempts=0, state=''):
        self.num_questions = num_questions
        self.attempts = attempts
        self.used_animals = used_animals or []
        self.last_record = last_record
        self.last_sound = last_sound
        self.right_answers = right_answers
        self.invalid_answers = invalid_answers
        self.max_attempts = max_attempts
        self.state = state

    def to_dict(self):
        return{
            'num_questions': self.num_questions,
            'attempts': self.attempts,
            'used_animals': self.used_animals,
            'last_record': self.last_record,
            'last_sound': self.last_sound,
            'right_answers': self.right_answers,
            'invalid_answers': self.invalid_answers,
            'max_attempts': self.max_attempts,
            'state': self.state
        }

    @classmethod
    def from_dict(cls, dct):
        return cls(**dct)


class GuessAnimalSkill(sdk.Skill):
    entities = {
        'Animal': {
            key: value['entity'] for key, value in data.animals.items()
        },
    }

    @staticmethod
    def text_for_yandex_station(meta):
        if not meta.interfaces.screen or meta.client_id.startswith('ru.yandex.iosdk.elariwatch'):
            return 'Если надоест, скажите: «Алиса, хватит».'
        return ''

    @staticmethod
    def add_spaces(text):
        return text + ' ' if text else ''

    @staticmethod
    def get_random_animal(animals, used_animals):
        return random.choice([animal for animal in animals if animal not in used_animals])

    @staticmethod
    def say(context, text, voice=None):
        voice = voice or text
        context.text.append(text)
        context.voice.append(voice)

    def send_question(self, logger, context, request, meta):
        num_questions = context.state.num_questions
        used_animals = set(context.state.used_animals)
        if num_questions == self.max_questions:
            return self.round_ending(context, request, meta)
        elif len(used_animals) < len(data.animals):
            context.state.num_questions += 1
            animal = self.get_random_animal(data.animals, used_animals)
            used_animals.add(animal)
            context.state.used_animals = list(used_animals)
            context.state.last_record = animal
            logger.debug('animal: %s', animal)
            sound = random.choice(data.animals[animal]['dict_tts'])
            context.state.last_sound = sound
            context.buttons.extend((
                sdk.Button(title='Повторить звук', hide=True),
                sdk.Button(title='Пропустить вопрос', hide=True),
                sdk.Button(title='Я не слышу', hide=True),
            ))
            text = random.choice((
                'Что это за животное?',
                'Кто так звучит?',
                'Это кто?',
                'Кто это?',
            ))
            self.say(context, text=text, voice=text + ' ' + sound)
            return resources.STATE_ANSWER_PROCESSING
        else:
            return self.round_ending(context, request, meta)

    def skip_question(self, logger, context, request, meta):
        context.state.attempts = 0
        animal = context.state.last_record
        self.say(
            context,
            text='Правильный ответ: «{}».'.format(data.animals[animal]['name']),
            voice='Правильный ответ: «{}».'.format(data.animals[animal]['name_tts'])
        )
        num_questions = context.state.num_questions
        used_animals = context.state.used_animals
        unused_animals = len(data.animals) - len(used_animals)
        logger.debug('num_questions %s, unused_animals %s', num_questions, unused_animals)
        if num_questions < unused_animals and num_questions < self.max_questions:
            self.say(context, 'Перехожу к следующему вопросу.')
        return self.send_question(logger, context, request, meta)

    def right_answer(self, logger, context, request, meta):
        text, voice = random.choice((
            ('Ура! Это правильный ответ.', '<speaker audio="Alisa_quiz_win.opus"> Ура! Это правильный ответ.'),
            ('Угадали! Это правильный ответ.', '<speaker audio="Alisa_quiz_win.opus"> Угад+али! Это правильный ответ.'),
            ('Точно!', '<speaker audio="Alisa_quiz_win.opus"> Точно!'),
            ('Ответ правильный. Вы молодец!', '<speaker audio="Alisa_quiz_win.opus"> Ответ правильный. Вы молодец!'),
            ('Верно!', '<speaker audio="Alisa_quiz_win.opus"> Верно!'),
        ))
        self.say(context, text, voice)
        logger.debug('right_answers %s', context.state.right_answers)
        context.state.right_answers += 1
        context.state.attempts = 0
        context.state.invalid_answers = 0
        return self.send_question(logger, context, request, meta)

    def wrong_answer(self, logger, context, request, meta):
        logger.debug('attempts %s', context.state.attempts)
        context.state.attempts += 1
        context.state.max_attempts = 2
        if context.state.attempts >= context.state.max_attempts:
            if context.state.invalid_answers >= context.state.max_attempts:
                context.state.invalid_answers = 0
                # session.invalid_answer = true
                self.say(context, random.choice((
                    'Судя по всему,',
                    'Видимо,',
                    'Похоже,'
                )) + ' вы не знаете правильный ответ.')
                return self.skip_question(logger, context, request, meta)
            else:
                text, voice = random.choice((
                    (
                        'К сожалению, это неправильный ответ.',
                        '<speaker audio="Alisa_quiz_loose.opus"> К сожалению, это неправильный ответ.'
                    ),
                    ('Нет, увы!', '<speaker audio="Alisa_quiz_loose.opus"> Нет, увы!'),
                    (
                        'Не получилось. Ответ неправильный.',
                        '<speaker audio="Alisa_quiz_loose.opus"> Не получилось. Ответ неправильный.'
                    ),
                    ('Неправильно.', '<speaker audio="Alisa_quiz_loose.opus"> Неправильно.'),
                    ('Неверно.', '<speaker audio="Alisa_quiz_loose.opus"> Неверно.'),
                ))
                self.say(context, text, voice)
                animal = context.state.last_record
                self.say(
                    context,
                    text='Правильный ответ: «{}».'.format(data.animals[animal]['name']),
                    voice='Правильный ответ: «{}».'.format(data.animals[animal]['name_tts'])

                )
                context.state.attempts = 0
                context.state.invalid_answers = 0
                # session.invalid_answer = false
                return self.send_question(logger, context, request, meta)
        else:
            sound = context.state.last_sound
            self.say(context, text='Дам вам ещё попытку.', voice='Дам вам ещё попытку. {}'.format(sound))
            context.buttons.extend((
                sdk.Button(title='Я не слышу', hide=True),
                sdk.Button(title='Повторить звук', hide=True),
                sdk.Button(title='Я не слышу', hide=True),
            ))
            return resources.STATE_ANSWER_PROCESSING

    def round_ending(self, context, request, meta):
        num_questions = context.state.num_questions
        right_answers = context.state.right_answers
        if num_questions == self.max_questions:
            if right_answers > ((self.max_questions + 1) / 2):
                self.say(
                    context,
                    random.choice((
                        'Отлично! В этом раунде вы угадали {right_answers} животных из {num_questions}.',
                        'Замечательно! В этом раунде вы угадали {right_answers} животных из {num_questions}.',
                        'Вот это да! В этом раунде вы угадали {right_answers} животных из {num_questions}.',
                        'Класс! В этом раунде вы угадали {right_answers} животных из {num_questions}.',
                    )).format(right_answers=right_answers, num_questions=num_questions)
                )
            elif right_answers > 1:
                self.say(context, random.choice((
                    'Хорошо! В этом раунде вы угадали {right_answers} животных из {num_questions}.',
                    'Неплохой результат! В этом раунде вы угадали {right_answers} животных из {num_questions}.',
                )).format(right_answers=right_answers, num_questions=num_questions))
            elif right_answers == 1:
                self.say(
                    context,
                    text='В этом раунде вы угадали 1 животное из {num_questions}. '
                         'Наверное, лучше потренироваться ещё.'.format(num_questions=num_questions),
                    voice='В этом раунде вы угадали одно животное из {num_questions}. '
                          'Наверное, лучше потренироваться ещё.'.format(num_questions=num_questions)
                )
            elif right_answers == 0:
                self.say(
                    context,
                    'В этом раунде вам не удалось угадать ни одно животное. Наверное, вам лучше потренироваться ещё.'
                )
            else:
                self.say(context, 'Раунд закончен.')
        else:
            self.say(context, 'В этом раунде вы угадали {right_answers} животных из {num_questions}.'.format(
                right_answers=right_answers,
                num_questions=num_questions,
            ))
        return self.want_to_play(context, request, meta)

    def want_to_play(self, context, request, meta):
        used_animals = set(context.state.used_animals)
        if len(used_animals) < len(data.animals):
            self.say(context, 'Хотите сыграть ещё раунд?')
            context.buttons.append(
                sdk.Button(title='Да', hide=True),
            )
            return resources.STATE_NEW_ROUND
        self.say(context, 'у меня закончились животные, которых вы еще не угадывали. Хотите начать игру заново?')
        context.buttons.extend((
            sdk.Button(title='Да', hide=True),
            sdk.Button(title='Нет', hide=True),
        ))
        return resources.STATE_NEW_GAME

    def game_stop(self, context, request, meta):
        self.say(context, random.choice((
            'Давайте закончим!',
            'Хорошо, давайте закончим!',
            'Тогда заканчиваем.',
        )))
        return self.exit(context, request, meta)

    def exit(self, context, request, meta):
        context.end_session = True
        return resources.STATE_EXIT

    def fallback(self, context, request, meta):
        self.say(context, 'Извините, я вас не поняла.')

    def state__general(self, logger, context, request, meta):
        self.say(context, (
            random.choice((
                'Давайте.',
                'Сыграем в игру «Угадай животное».',
                'Отлично!',
                'Хорошо.',
            )) + ' Я буду ' +
            random.choice((
                'давать вам послушать',
                'проигрывать',
                'ставить',
            )) + ' звук, который издает животное, а вам нужно угадать — кто это. ' +
            self.add_spaces(self.text_for_yandex_station(meta)) +
            random.choice((
                'Готовы?',
                'Начинаем?',
                'Давайте начнем?',
                'Поехали?',
                'Если вы готовы, скажите «да».',
            ))
        ))
        context.buttons.append(sdk.Button(title='Да', hide=True))
        return resources.STATE_GAME_START

    def state__game_start(self, logger, context, request, meta):
        hypothesis, _ = next(context.match(request, self.extractor, resources.patterns.get(resources.STATE_GAME_START)))
        if hypothesis == resources.intents__start_game_yes:
            self.say(context, 'Отлично!')
            return self.send_question(logger, context, request, meta)
        if hypothesis == resources.intents__start_game_no:
            self.say(context, 'Ну нет так нет.')
            return self.exit(context, request, meta)

        # todo: transition? payload={'transition': resources.STATE_GAME_START, 'mode': 'game yes'}
        context.buttons.append(sdk.Button(title='Да', hide=True))
        self.say(context, (
            'Извините, я вас не поняла. ' +
            random.choice((
                'Готовы?',
                'Начинаем?',
                'Давайте начнем?',
                'Поехали?',
                'Если вы готовы, скажите «да».',
            ))
        ))
        return resources.STATE_GAME_START

    def state__answer_processing(self, logger, context, request, meta):
        hypothesis, variables = next(context.match(request, self.extractor, resources.patterns.get(resources.STATE_ANSWER_PROCESSING)))
        if hypothesis == resources.intents__repeat_sound:
            sound = context.state.last_sound
            text = random.choice((
                'Хорошо.',
                'Ладно.',
                'Повторяю.',
                'Вот.',
                'Ну хорошо.',
            ))
            voice = text + ' ' + sound
            self.say(context, text, voice)
            context.buttons.extend((
                sdk.Button(title='Повторить звук', hide=True),
                sdk.Button(title='Пропустить вопрос', hide=True),
                sdk.Button(title='Я не слышу', hide=True),
            ))
            return resources.STATE_ANSWER_PROCESSING
        if hypothesis == resources.intents__user_cant_hear:
            context.buttons.extend((
                sdk.Button(title='Повторить звук', hide=True),
                sdk.Button(title='Пропустить вопрос', hide=True),
            ))
            self.say(context, random.choice((
                'Чтобы услышать звук, произнесите «Повтори звук». '
                'Это голосовой навык, поэтому звук проигрывается, только когда мы общаемся голосом.',
                'Чтобы услышать звук, произнесите «Повтори звук».',
            )))
            return resources.STATE_ANSWER_PROCESSING
        if hypothesis == resources.intents__user_valid_answer:
            animal = context.state.last_record
            guess = variables.get('Animal')[0]
            logger.debug('User guess: %s', guess)
            if guess == animal:
                return self.right_answer(logger, context, request, meta)
            return self.wrong_answer(logger, context, request, meta)
        if hypothesis == resources.intents__user_invalid_answer:
            return self.wrong_answer(logger, context, request, meta)
        if hypothesis == resources.intents__score:
            right_answers = context.state.right_answers
            num_questions = context.state.num_questions
            if right_answers == 0:
                self.say(context, 'Пока вы не угадали ни одно животное в этом раунде.')
            elif right_answers == 1:
                self.say(
                    context,
                    text='В этом раунде вы угадали 1 животное из {}.'.format(num_questions),
                    voice='В этом раунде вы угадали одно животное из {}.'.format(num_questions)
                )
            else:
                text = 'В этом раунде вы угадали {right_answers} животных из {num_questions}.'.format(
                    right_answers=right_answers,
                    num_questions=num_questions,
                )
                self.say(context, text)
            return resources.STATE_ANSWER_PROCESSING
        if hypothesis == resources.intents__skip_question:
            return self.skip_question(logger, context, request, meta)

        return self.wrong_answer(logger, context, request, meta)

    def state__new_game(self, logger, context, request, meta):
        hypothesis, _ = next(context.match(request, self.extractor, resources.patterns.get(resources.STATE_NEW_GAME)))
        if hypothesis == resources.intents__new_game_yes:
            self.say(context, random.choice((
                'Хорошо. Начнём игру заново.',
                'Отлично! Начнём игру заново.',
            )))
            self.new_game(context)
            return self.send_question(logger, context, request, meta)
        if hypothesis == resources.intents__new_game_no:
            return self.game_stop(context, request, meta)
        self.fallback(context, request, meta)
        return resources.STATE_NEW_GAME

    def state__new_round(self, logger, context, request, meta):
        hypothesis, _ = next(context.match(request, self.extractor, resources.patterns.get(resources.STATE_NEW_ROUND)))
        if hypothesis == resources.intents__new_round_yes:
            self.say(context, random.choice((
                'Хорошо! Сыграем ещё.',
                'Отлично! Начнём новый раунд.',
            )))
            self.new_round(context)
            state = self.send_question(logger, context, request, meta)
            return state
        if hypothesis == resources.intents__new_round_no:
            return self.game_stop(context, request, meta)
        self.fallback(context, request, meta)
        return resources.STATE_NEW_ROUND

    def state__exit(self, logger, context, request, meta):
        self.fallback(context, request, meta)
        return self.exit(context, request, meta)

    def __init__(self, max_questions=5):
        self.states = {
            resources.STATE_GENERAL: self.state__general,
            resources.STATE_GAME_START: self.state__game_start,
            resources.STATE_ANSWER_PROCESSING: self.state__answer_processing,
            resources.STATE_NEW_GAME: self.state__new_game,
            resources.STATE_NEW_ROUND: self.state__new_round,
            resources.STATE_EXIT: self.state__exit,
        }
        self.max_questions = max_questions
        self.state_cls = State
        super().__init__()

    @staticmethod
    def new_round(context):
        context.state.invalid_answers = 0
        context.state.last_sound = ''
        context.state.right_answers = 0
        context.state.attempts = 0
        context.state.num_questions = 0
        context.state.last_record = ''

    def new_game(self, context):
        self.new_round(context)
        context.state.used_animals = []

    def new_session(self, context):
        context.state.state = resources.STATE_GENERAL
        self.new_game(context)

    def play(self, logger, context, request, meta):
        context.text = []
        context.voice = []
        context.buttons = []
        context.end_session = False

        if context.is_new_session():
            state = self.state__general(logger, context, request, meta)
        else:
            hypothesis, _ = next(context.match(request, self.extractor, resources.patterns.get(resources.STATE_GENERAL)))
            if hypothesis == resources.intents__stop_game:
                state = self.game_stop(context, request, meta)
            else:
                state = context.state.state
                state = self.states[state](logger, context, request, meta)
        logger.debug('New state: %s', state)
        context.state.state = state
        return state

    def handle(self, logger, context, request, meta):
        self.play(logger, context, request, meta)

        text = '\n'.join(context.text)
        voice = '\n'.join(context.voice)
        return sdk.Response(text=text, tts=voice, buttons=context.buttons, end_session=context.end_session)
