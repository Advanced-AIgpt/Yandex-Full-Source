# coding: utf-8

you = '(ты|Алиса)'
good = '(хорошо|замечательно|прекрасно|великолепно|классно|отлично)'
compliment = ('({you} [(очень|так*|сегодня|просто|моя)] '
              '(красивый|молодец|классный|прекрасный|красав*|супер|красотк*|подруж*|детк*|сексуальный|секси|сексе|милый'
              '|милаха|прелесть|{good} выглядишь|солнышк*|зайк*|зая|отличный|отпадный|замечательный|крутой|крутая|'
              'потрясающий|потрясный|потрясная|обворожителый|несравненный|головокружител*|клевый|бесподобный|'
              'бесподобная|прикольный|прикольная|симпатичный|хорошенький|хорошенькая|добрый|ласковый|милаш|милашка|'
              'привлекательный|ахуенный|очаровательный|великолепный|ослепительный|сногшибательный|шикарный)|'
              'платье * огонь|поздравляю|молодец*|молодц*)'.format(you=you, good=good))

common = {
    'agree': '(ага|угу|давай|да|хорошо|конечно)',
    'disagree': '(не надо|не хочу|неа|нет|не)',
    'notNow': '(не [могу] сейчас|мне (некогда|не до *того)|(в другой|не в этот) раз |'
              '[давай] (*позже|потом|не сегодня|завтра|не сейчас|пока не надо|в (следующий|другой) раз) [поговорим]|'
              'нет времени|я (занят|занята)|не готов|не готова) [{compliment}]'
              ''.format(compliment=compliment),
    'startGame': '(начин*|начн*|попытаюсь|попробую|поехали|го|гоу|погнали|продолж*|готов*|давай|валяй|реди|рэди|камон'
                 '|сыграем|сыграю|играем|играю)',
    'you': you,
    'good': good,
    'compliment': compliment,
    'maybe': '(кажется|наверное|можеть быть|вероятно|)',
    'think': '[я] (думаю|считаю|знаю|верю)',
    'stopGame': 'хватит|стоп|надоел*|останови*|стой|прекрати|завязывай*|закончили'
}


general__game_start = (
    '* [давай|может] * (сыграем|игра*|поигра*|игру|сыгра*|включи|активируй|~начать) * [в] (угад*|отгад*) (зоо*|животн*|звер*) * |'  # noqa
    '* (давай|хочу|хочется|включи|активируй|~начать) * (угад*|отгад*) (зоо*|животн*|звер*) * |'
    ''  # empty string request
)
game_start__game_yes = (
    '* ({agree}|{startGame}|начин*|начн*|помогу|стараться|попытаюсь|попробую|давай) [начин*|начн*|поигра*|сыгра*|игра*|попробую|попробуем] * |'  # noqa
    '* [{agree}] ({agree}|{startGame}|начин*|начн*|помогу|постараюсь|стараться|попытаюсь|попробую|давай|включай|включи|включаем|врубай|вруби|врубаем|поехали|погнали) *'  # noqa
    .format(**common)
)
game_start__game_no = '* ({disagree}|{notNow}) *'.format(**common)


answer_processing__repeat_sound = (
    '* [повтори*] [звук] (еще|еще раз|снова|опять) *|'
    '* повтори* [звук] [(еще|еще раз|снова|опять)] *|'
    '* не (расслышал*|понял|услышал) *'
)
answer_processing__user_cant_hear = '* не слыш* *'
answer_processing__user_valid_answer = (
    '* [{maybe}|{think}|давай|как насчет|конечно] [это] $Animal [{maybe}|{think}|что ли|давай] *'
    .format(**common)
)
answer_processing__user_invalid_answer = '* не $Animal *'
answer_processing__score = (
    '* счет *|'
    '* сколько (я|у меня) * (отгадал*|правильн*) *'
)
answer_processing__skip_question = (
    '* ({agree}|{disagree}|не хочу|не знаю|не буду|не надо|подскажи|помоги|правильный|уже спрашивал|повторяешься|(скажи|какой|назови) * (ответ|сам*|ты)|((сам*|ты) (ответь|отвечай))) * |'  # noqa
    '* [{agree}] (пропус*|дальше|след*|ещё|еще|игра*|задавай|другой) [дальше|след*|ещё|еще|игра*|задавай|другой] [вопрос*] * |'  # noqa
    '* {agree} (дальше|след*|ещё|еще|игра*|задавай|другой) [дальше|след*|ещё|еще|игра*|задавай|другой] вопрос* * |'
    '* (хз|без понятия|беспонятия|понятия не имею|[даже] не представляю) *'
    .format(**common)
)


user_knew_right_answer = '* знал* прав* ~ответ *'


new_game__yes = (
    '* ({agree}|{startGame}|начин*|начн*|помогу|стараться|попытаюсь|попробую|давай) [начин*|начн*|поигра*|сыгра*|игра*|попробую|попробуем] * |'  # noqa
    '* [{agree}] ({agree}|{startGame}|начин*|начн*|помогу|постараюсь|стараться|попытаюсь|попробую|давай) *'
    .format(**common)
)
new_game__no = (
    '* ({disagree}|{notNow}) *'
    .format(**common)
)


new_round__yes = (
    '* ({agree}|{startGame}|начин*|начн*|помогу|стараться|попытаюсь|попробую|давай) [начин*|начн*|поигра*|сыгра*|игра*|попробую|попробуем] * |'  # noqa
    '* [{agree}] ({agree}|{startGame}|начин*|начн*|помогу|постараюсь|стараться|попытаюсь|попробую|давай) *'
    .format(**common)
)
new_round__no = (
    '* последн* [раунд*] * |'
    '* ({disagree}|{notNow}) *'
    .format(**common)
)


stop_game = (
    '{stopGame} |'
    '* [давай] * перерыв * |'
    '[я] устал* |'
    '* (сегодня (хватит|достаточно|все) [на] [хватит] [достаточно]) * |'
    'Алиса, хватит.'
    .format(**common)
)

intents__stop_game = 'stop game'
intents__start_game_yes = 'start game yes'
intents__start_game_no = 'start game no'
intents__repeat_sound = 'repeat sound'
intents__user_cant_hear = 'user cant hear'
intents__user_invalid_answer = 'user invalid answer'
intents__user_valid_answer = 'user valid answer'
intents__score = 'score'
intents__skip_question = 'skip question'
intents__new_round_yes = 'new round yes'
intents__new_round_no = 'new round no'
intents__new_game_yes = 'new game yes'
intents__new_game_no = 'new game no'

STATE_GENERAL = 'general'
STATE_GAME_START = 'game start'
STATE_ANSWER_PROCESSING = 'answer processing'
STATE_NEW_GAME = 'new game'
STATE_NEW_ROUND = 'new round'
STATE_EXIT = 'exit'

patterns = {
    STATE_GENERAL: {
        intents__stop_game: stop_game,
    },
    STATE_GAME_START: {
        intents__start_game_yes: game_start__game_yes,
        intents__start_game_no: game_start__game_no
    },
    STATE_ANSWER_PROCESSING: {
        intents__repeat_sound: answer_processing__repeat_sound,
        intents__user_cant_hear: answer_processing__user_cant_hear,
        intents__user_valid_answer: answer_processing__user_valid_answer,
        intents__user_invalid_answer: answer_processing__user_invalid_answer,
        intents__score: answer_processing__score,
        intents__skip_question: answer_processing__skip_question
    },
    STATE_NEW_ROUND: {
        intents__new_round_yes: new_round__yes,
        intents__new_round_no: new_round__no,
    },
    STATE_NEW_GAME: {
        intents__new_game_yes: new_game__yes,
        intents__new_game_no: new_game__no,
    }
}
