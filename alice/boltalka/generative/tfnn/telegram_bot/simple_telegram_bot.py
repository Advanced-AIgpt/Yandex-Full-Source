import argparse
import json
import logging
import re
import time
import threading
from copy import copy

import requests
import urllib

import telegram
from telegram import Update, ReplyKeyboardMarkup, ReplyKeyboardRemove
from telegram.ext import Updater, CommandHandler, MessageHandler, Filters, CallbackContext, ConversationHandler


logging.basicConfig(
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s', level=logging.INFO
)
logger = logging.getLogger(__name__)


DEFAULT, SET_MODEL = 0, 1

class ApiClient:
    def __init__(self, url):
        self.url = url

    def request(self, text):
        response = requests.get(self.url + '?' + urllib.parse.urlencode({'text': text}))
        return response.text


class TransaliceBotHandler:
    def __init__(self, args):
        self.args = args
        self.models = {
            'zeliboba1.4': 'http://gpt-bot-1.sas.yp-c.yandex.net/ask_bot'
        }

    def check_username(self, username):
        if self.args.no_auth:
            return True

        try:
            if username is None:
                return False

            result = requests.get(
                url='https://staff-api.yandex-team.ru/v3/persons',
                headers={
                    'Authorization': 'OAuth {}'.format(self.args.staff_token),
                },
                params={
                    '_one': 1,
                    '_query': 'accounts==match({{"type":"telegram","value_lower":"{}"}})and(official.is_dismissed==False)'.format(
                        username.lower())
                }
            ).json()

            return 'official' in result and not result['official'].get('is_dismissed', True)
        except Exception:
            return False

    def __call__(self, update, context):
        try:
            if not self.check_username(update.message.from_user.username):
                message = 'System: кажется, ты не можешь мне писать :( обратись к тому, кто дал тебе бота'
            else:
                message = self.do_call(update, context)
            update.message.reply_text(message, parse_mode=telegram.ParseMode.HTML)
        except Exception as ex:
            update.message.reply_text('System: что-то пошло не так, попробуй повторить запрос или напиши khr2@')
            raise ex

    def maybe_init_chat_data(self, user_data):
        default_options = {
            'model': self.models[list(self.models.keys())[0]]
        }

        for key in default_options:
            if key not in user_data:
                user_data[key] = default_options[key]

    def postprocess_reply(self, reply):
        return reply.replace("[NL]", "\n")

    def _format_user_params(self, dict):
        return '\n'.join('{}: <b>{}</b>'.format(key, dict[key]) for key in sorted(dict.keys()))

    def set_model(self, update, context):
        self.maybe_init_chat_data(context.user_data)

        reply_keyboard = [list(self.models)]

        update.message.reply_text(
            f'System: текущая модель: \n{context.user_data["model"]}\n' \
            f'Выберите другую или введите путь до своей',
            reply_markup=ReplyKeyboardMarkup(reply_keyboard, one_time_keyboard=True),
        )

        return SET_MODEL

    def set_choosen_model(self, update, context):
        user = update.message.from_user
        if update.message.text in self.models:
            context.user_data['model'] = self.models[update.message.text]
        else:
            context.user_data['model'] = update.message.text
        update.message.reply_text(
            f'System: новая модель: \n{context.user_data["model"]}',
            reply_markup=ReplyKeyboardRemove(remove_keyboard = True)
        )
        return DEFAULT

    def start(self, update, context):
        return self.help(update, context)

    def help(self, update, context):
        update.message.reply_text('System: бот для общения с болталкой с генеративной моделью. ' \
                   '\nПросто пиши реплику и моделька постарается ответить на нее. ' \
                   '\nВ боте может быть загружено несколько моделей, для их просмотра и ' \
                   'изменения используй /set_model')

    def do_call(self, update, context):
        self.maybe_init_chat_data(context.user_data)
        client = ApiClient(context.user_data['model'])
        print(f"call: {update.message.text}")
        response = self.postprocess_reply(client.request(update.message.text))
        print(f"response: {response}")
        return response



def main():
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--telegram-token', type=str, default=None)
    parser.add_argument('--staff-token', type=str, default=None)
    parser.add_argument('--no-auth', type=int, default=0)
    args = parser.parse_args()

    global_handler = TransaliceBotHandler(args)
    updater = Updater(token=vars(args)['telegram_token'])
    dispatcher = updater.dispatcher

    conv_handler = ConversationHandler(
        entry_points=[
            MessageHandler(Filters.text & ~Filters.command, global_handler),
            CommandHandler('help', global_handler.help),
            CommandHandler('set_model', global_handler.set_model),
        ],
        states={
            SET_MODEL: [MessageHandler(Filters.regex(f'^.+$'), global_handler.set_choosen_model)],
            DEFAULT: [
                MessageHandler(Filters.text & ~Filters.command, global_handler),
                CommandHandler('help', global_handler.help),
                CommandHandler('set_model', global_handler.set_model),
            ],
        },
        fallbacks=[],
    )
    dispatcher.add_handler(conv_handler)
    updater.start_polling()
    updater.idle()

if __name__ == '__main__':
    main()