# coding: utf-8
# Импортирует поддержку UTF-8.
from __future__ import unicode_literals
import random

# Импортируем модули для работы с JSON и логами.
import json
import logging

# Импортируем подмодули Flask для запуска веб-сервиса.

from __builtin__ import str
from flask import Flask, request
app = Flask(__name__)


logging.basicConfig(level=logging.DEBUG)

# Хранилище данных о сессиях.
sessionStorage = {}

# Задаем параметры приложения Flask.
@app.route("/", methods=['POST'])

def main():
# Функция получает тело запроса и возвращает ответ.
    logging.info('Request: %r', request.json)

    response = {
        "version": request.json['version'],
        "session": request.json['session'],
        "response": {
            "end_session": False
        }
    }

    handle_dialog(request.json, response)

    logging.info('Response: %r', response)

    return json.dumps(
        response,
        ensure_ascii=False,
        indent=2
    )


def handle_dialog(req, res):
    user_id = req['session']['user_id']

    if req['session']['new']:
        # Это новый пользователь.
        # Инициализируем сессию и поприветствуем его.

        sessionStorage[user_id] = {
            'suggests': [
                "картинка",
                "картинки",
                "перенос",
                "без картинки"
                "смайлик",
                "🤔"

            ]
        }

        res['response']['text'] = 'Тестовый навык для проверки основного фугкционала на странице тестировния\n' \
                                  'Команда "картинка" - возвращает карточку с 1м изображением\n' \
                                  'Команды "перенос", перенос2, перенос3, перенос4 - возвращает текст в котором присутствует перенос\n' \
                                  'Команда "картинки" - возвращает галерею с картинками\n' \
                                  'Команда "без картинки" - возвращает  обычный ответ\n' \
                                  'Команда "смайлик" - возвращает смайлик\n'\
                                  'Команда 🤔 - отвечает смайликом в ответ'


        res['response']['buttons'] = get_suggests(user_id)
        return


# Обрабатываем ответ пользователя.

        # utt = req['request']['original_utterance'].lower()
    utt = req['request']['original_utterance'].lower() if 'original_utterance' in req['request'] else ''


    if utt in [
        'перенос'
    ]:
        res['response'] = {
             "text": " В МИДе прокомментировали учения НАТО Trident Junction. \nИнтересно?",
             "tts": "В МИДе прокомментировали учения НАТО Trident Junction. - Интересно?",
                "buttons": [
                    {
                        "title": "да",
                        #"payload": {},
                        #"text": 'да',
                        "hide": True
                    }
                ],
        }
        return

    if utt in [
        'перенос2'
    ]:
        res['response'] = {
             "text": " В МИДе прокомментировали учения НАТО Trident Junction. \\nИнтересно?",
             "tts": "В МИДе прокомментировали учения НАТО Trident Junction. - Интересно?",
                "buttons": [
                    {
                        "title": "да",
                         #"payload": {},
                        #"text": 'да',
                        "hide": True
                    }
                ],
        }
        return

    if utt in [
        'перенос3'
    ]:
        res['response'] = {
            "text": " В МИДе прокомментировали учения НАТО Trident Junction. \\\nИнтересно?",
            "tts": "В МИДе прокомментировали учения НАТО Trident Junction. - Интересно?",
            "buttons": [
                {
                    "title": "да",
                    #"payload": {},
                    #"text": 'да',
                    "hide": True
                }
            ],
        }
        return

    if utt in [
        'перенос4'
    ]:
        res['response'] = {
            "text": " В МИДе прокомментировали учения НАТО Trident Junction. \nИнте\nресно?",
            "tts": "В МИДе прокомментировали учения НАТО Trident Junction. - Интересно?",
            "buttons": [
                {
                    "title": "да",
                    #"payload": {},
                    #"text": 'да',
                    "hide": True
                }
            ],
        }
        return


    if utt in [
        'смайлик'
    ]:
        res['response'] = {
            "text": " Получи смайлик 🤔🤔",
            "tts": "Получи смайлик",
            "buttons": [
                {
                    "title": "да",
                    #"payload": {},
                    #"text": 'да',
                    "hide": True
                }
            ],
        }
        return


    if utt in [
        '🤔'
    ]:
        res['response'] = {
            "text": " Получи смайлик 🤔🤔",
            "tts": "Получи смайлик",
            "buttons": [
                {
                    "title": "да",
                    #"payload": {},
                    #"text": 'да',
                    "hide": True
                }
            ],
        }
        return


    if utt in [
        'да'
    ]:
        res['response'] = {
            "text": "Британский журналист Том Оу в статье для The Telegraph рассказал о первом посещении русской бани в Лондоне. "
                    "Оу решил пойти в одну из самых знаменитых бань британской столицы, «Баню № 1», которую, как он отметил, "
                    "любят посещать Кейт Мосс, Джастин Бибер, Эмилия Кларк и другие знаменитости. "
                    "Во время массажа березовым веником, который журналист окрестил «оружием войны», "
                    "Оу казалось, что его «бьет большое дерево во время ливня».\n«Расслабление — это любопытная вещь, размышлял я, поглядывая из-под белой шляпы "
                    "в стиле головного убора Смурфиков, когда бородатый бодибилдер хлестал мою обнаженную "
                    "спину распаренным березовым веником», — вспоминает репортер. "
                    "Однако после купания в ледяной воде смущение журналиста ушло на второй план, "
                    "и он почувствовал себя «немного посвежевшим и в целом более расслабленным»",
            "tts": "Британский журналист Том Оу в статье для The Telegraph рассказал о первом посещении русской бани в Лондоне. "
                   "Оу решил пойти в одну из самых знаменитых бань британской столицы, «Баню № 1», "
                   "которую, как он отметил, любят посещать Кейт Мосс, \nДжастин Бибер, Эмилия Кларк и другие знаменитости. "
                   "Во время массажа березовым веником, который журналист окрестил «оружием войны», "
                   "Оу казалось, что его «бьет большое дерево во время ливня». "
                   "«Расслабление — это любопытная вещь, размышлял я, поглядывая из-под белой шляпы в стиле головного убора Смурфиков, "
                   "когда бородатый бодибилдер хлестал мою обнаженную спину распаренным березовым веником», — вспоминает репортер. "
                   "Однако после купания в ледяной воде смущение журналиста ушло на второй план, "
                   "и он почувствовал себя «немного посвежевшим и в целом более расслабленным».",
            "buttons": [
                {
                    "title": "да",
                    #"payload": {},
                    #"text": 'да',
                    "hide": True
                }
            ],
        }
        return


    if utt in [
        'картинка'
    ]:
        res['response']= {
            "text": "Привет меня зовут пепе",
            "tts": "Привет меня зовут п+епе",
            "card": {
                "type": "BigImage",
                "image_id": "965417/74523f4ec543400f93d5",
                "title": "pepe is live",
                "description": "pepe is live",
                "button": {
                    "text": "pepe mode on",
                    "url": "https://www.reddit.com/r/pepe/",
                    "payload": {}
                }
            },
            "buttons": [
                {
                    "title": "pepe mode on",
                    "payload": {},
                    "url": "https://yandex.ru",
                    "hide": True
                }
            ],
        }
        return

    if utt in [
        'картинки'
    ]:
        res['response'] = {
            "text": "Посмотри коллекцию рарных пепе",
            "tts": "Посмотри коллекцию рарных пепе.",
            "card": {
                "type": "ItemsList",
                "header": {
                    "text": "make pepe great again18",
                },
                "items": [
                    {
                        "image_id": "937455/c88344a17b4872ad61d0",
                        "title": "make pepe great again14",
                        "description": "make pepe great again13",
                        "button": {
                            "text": "make pepe great again12",
                            "url": "https://www.reddit.com/r/pepe/",
                            "payload": {},
                            "hide": True
                        }
                    },

                    {
                        "image_id": "937455/7e58ff315f3fa752122e",
                        "title": "make pepe great again7",
                        "description": "make pepe great again9",
                        "button": {
                            "text": "make pepe great again1",
                            "url": "https://www.reddit.com/r/pepe/",
                            "payload": {},
                            "hide": True
                        }
                    },


                    {
                        "image_id": "213044/20799e131767645b7641",
                        "title": "make pepe great again7",
                        "description": "make pepe great again11",
                        "button": {
                            "text": "make pepe great again2",
                            "url": "https://www.reddit.com/r/pepe/",
                            "payload": {},
                            "hide": True,
                        }
                    }


                ],
                "footer": {
                    "text": "make pepe great again5",
                    "button": {
                        "text": "make pepe great again3",
                        "url": "https://www.reddit.com/r/pepe/",
                        "payload": {},
                        "hide": True
                    }
                }
            },
            "buttons": [
                {
                    "title": "make pepe great again4",
                    "payload": {},
                    "url": "https://www.reddit.com/r/pepe/",
                    "hide": True
                }
            ],

        }
        return


    if utt in [
        'без картинки'
    ]:
        res['response'] = {

                "text": "Здравствуйте! Это мы, хороводоведы.",
                "tts": "Здравствуйте! Это мы, хоров+одо в+еды.",
                "buttons": [
                    {
                        "title": "да",
                        "payload": {},
                        "url": "https://yandex.ru",
                        "hide": True
                    }
                ],
        }

    res['response']['text'] = 'Ты меня сломал'


# Функция возвращает две подсказки для ответа.
def get_suggests(user_id):
    session = sessionStorage[user_id]

    # Выбираем четыре первые подсказки из массива.
    suggests = [
        {'title': suggest, 'hide': True}
        for suggest in session['suggests'][:4]
    ]

    # Убираем первую подсказку, чтобы подсказки менялись каждый раз.
    session['suggests'] = session['suggests'][1:]
    sessionStorage[user_id] = session

    # Если осталась только одна подсказка, j
    if len(suggests) < 2:
        suggests.append({
            "title": "сыграем в днд ?",
            "url": "https://ru.wikihow.com/играть-в-D%26D",
            "hide": True
        })

    return suggests


if __name__ == '__main__':
    import os
    app.run(host='::', port=int(os.environ.get('QLOUD_HTTP_PORT', 5000)))




