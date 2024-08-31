#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os
import logging
import re

from functools import wraps
from hashlib import sha256
from urllib.parse import urlparse, parse_qs

from emoji import emojize
from ptbcontrib.postgres_persistence import PostgresPersistence
from telegram.ext import Updater, CommandHandler, MessageHandler, CallbackQueryHandler, Filters
from telegram import InlineKeyboardButton, InlineKeyboardMarkup, ParseMode
from sqlalchemy import create_engine
from sqlalchemy.orm import sessionmaker, scoped_session

from bass import BassClient
from bulbasaur import BulbasaurClient
from passport import YandexOauthClient, YandexPassportClient
from util import group_by_room, get_chat_id, UnauthorizedError


MDB_PASSWORD = os.environ.get('MDB_PASSWORD')
if not MDB_PASSWORD:
    raise ValueError('missing MDB_PASSWORD envvar')

YA_OAUTH_CLIENT_ID = os.environ.get('YA_OAUTH_CLIENT_ID')
if not YA_OAUTH_CLIENT_ID:
    raise ValueError('missing YA_OAUTH_CLIENT_ID envvar')

YA_OAUTH_CLIENT_SECRET = os.environ.get('YA_OAUTH_CLIENT_SECRET')
if not YA_OAUTH_CLIENT_SECRET:
    raise ValueError('missing YA_OAUTH_CLIENT_SECRET envvar')

ALICE_SPEAKER_BOT_TOKEN = os.environ.get('ALICE_SPEAKER_BOT_TOKEN')
if not ALICE_SPEAKER_BOT_TOKEN:
    raise ValueError('missing ALICE_SPEAKER_BOT_TOKEN envvar')


logging.basicConfig(format='%(asctime)s - %(name)s - %(levelname)s - %(message)s', level=logging.INFO)
logger = logging.getLogger(__name__)

DB_HOSTS = "sas-hjcu0nu91yo9eiyl.db.yandex.net,vla-h6yqgqxas96p95fi.db.yandex.net"
DB_URI = "postgresql+psycopg2://asb:{password}@/asb?host={hosts}&port=6432&target_session_attrs=read-write".format(password=MDB_PASSWORD, hosts=DB_HOSTS)

YandexOauthClientURL = "https://oauth.yandex.ru"
YandexOauthClient = YandexOauthClient(YandexOauthClientURL, YA_OAUTH_CLIENT_ID, YA_OAUTH_CLIENT_SECRET)

YandexPassportClientURL = "https://login.yandex.ru"
YandexPassportClient = YandexPassportClient(YandexPassportClientURL)

BassClientURL = "http://bass-prod.yandex.net"
BassClient = BassClient(BassClientURL)

BulbasaurClientURL = "http://iot.quasar.yandex.net"
BulbasaurClient = BulbasaurClient(BulbasaurClientURL)

RedirectURL = 'https://clck.ru/RGnjh'  # https://oauth.yandex.ru/authorize?response_type=code&client_id=3f21544b38564ae9b8cc497de9c7d450&redirect_uri=https://ih1z8dv92f.execute-api.us-east-2.amazonaws.com/production
# RedirectURL = 'https://clck.ru/SuMCY'  # https://oauth.yandex.ru/authorize?response_type=code&client_id=3f21544b38564ae9b8cc497de9c7d450&redirect_uri=https://d5d0npr0buevr9f68rs1.apigw.yandexcloud.net

HALP = 'этот бот может помочь тебе управлять твоей колонкой с Алисой\n\n'
HALP += 'доступные команды:\n'
HALP += '/all - показать все твои колонки\n'
HALP += '/device - показать активную колонку\n\n'
HALP += '/say - сказать колонкой что-то\n'
HALP += '/youtube - запустить видео с ютуба\n'
HALP += '/menu - быстрое меню полезных команд\n'
HALP += '/remote - пульт управления мультимедиа\n'
HALP += '/scenario - твои сценарии\n\n'
HALP += '/info - служебная информация\n'
HALP += '/logout - выйти\n\n'
HALP += 'все команды имеют скоращения, например вместо /youtube можно написать /y\n'
HALP += 'а если просто послать сообщение, то колонка подумает, что ты в нее что-то сказал'

LOGIN = 'давай сначала залогинимся, а потом я смогу управлять твоими устройствами ;)'

speaker = emojize(":sound:", use_aliases=True)
bulb = emojize(":bulb:", use_aliases=True)
cross = emojize(":x:", use_aliases=True)
home = emojize(":house:", use_aliases=True)
volume_up = emojize(":loud_sound:", use_aliases=True)
volume_down = emojize(":speaker:", use_aliases=True)
music = emojize(":musical_note:", use_aliases=True)
arrow_up = emojize(":arrow_up:", use_aliases=True)
arrow_down = emojize(":arrow_down:", use_aliases=True)
arrow_right = emojize(":arrow_right:", use_aliases=True)
arrow_left = emojize(":arrow_left:", use_aliases=True)
fast_forward = emojize(":fast_forward:", use_aliases=True)
rewind = emojize(":rewind:", use_aliases=True)
pause = emojize(":pause_button:", use_aliases=True)
play = emojize(":play_button:", use_aliases=True)
stop = emojize(":stop_button:", use_aliases=True)
gear = emojize(":gear:", use_aliases=True)

COMMAND = 'command'

MENU = "menu"
MENU_PATTERN_STR = "^{}(?P<{}>.*)".format(MENU, COMMAND)
MENU_PATTERN = re.compile(MENU_PATTERN_STR)

CHOOSE = "choose"
CHOOSE_PATTERN_STR = "^{}(?P<{}>.*)".format(CHOOSE, COMMAND)
CHOOSE_PATTERN = re.compile(CHOOSE_PATTERN_STR)

REMOTE = "remote"
REMOTE_PATTERN_STR = "^{}(?P<{}>.*)".format(REMOTE, COMMAND)
REMOTE_PATTERN = re.compile(REMOTE_PATTERN_STR)

SCENARIO = "scenario"
SCENARIO_PATTERN_STR = "^{}(?P<{}>.*)".format(SCENARIO, COMMAND)
SCENARIO_PATTERN = re.compile(SCENARIO_PATTERN_STR)


# wrappers
def log(func):
    @wraps(func)
    def wrapped(update, context, *args, **kwargs):
        if getattr(update, 'message', None) is None:
            return
        username = update.message.from_user.username
        text = update.message.text
        logger.info("{}: {}".format(username, text))
        return func(update, context, *args, **kwargs)
    return wrapped


def log_callback(func):
    @wraps(func)
    def wrapped(update, context, *args, **kwargs):
        if getattr(update.callback_query, 'data', None) is None:
            return
        username = update.callback_query.from_user.username
        text = update.callback_query.data
        logger.info("{}: {}".format(username, text))
        return func(update, context, *args, **kwargs)
    return wrapped


def oauth(func):
    @wraps(func)
    def wrapped(update, context, *args, **kwargs):
        token = context.user_data.get('token')
        login_info = context.user_data.get('login_info')
        if token is None and login_info is None:
            return need_login(update, context)
        return func(update, context, *args, **kwargs)
    return wrapped


def device(func):
    @wraps(func)
    def wrapped(update, context, *args, **kwargs):
        device = context.user_data.get('device')
        if device is None:
            return need_device(update, context)
        return func(update, context, *args, **kwargs)
    return wrapped


def need_login(update, context):
    context.user_data.pop('token', None)
    context.user_data.pop('login_info', None)
    context.user_data.pop('device', None)

    chat_id = get_chat_id(update)
    context.bot.send_message(chat_id=chat_id, text=LOGIN)
    context.bot.send_message(chat_id=chat_id, text=RedirectURL)


def need_device(update, context):
    chat_id = get_chat_id(update)
    context.bot.send_message(chat_id=chat_id, text='сначала выбери колонку, которой хочешь управлять, посмотреть все свои колонки можно вот так: /all')


def hello(user_data):
    msg = 'привет {}!\n'.format(user_data.get('login_info').get_name())
    msg += '\n'
    msg += HALP
    return msg


def help_handler(update, _):
    update.message.reply_text(HALP)


# command handlers
def start_handler(update, context):
    # if we have token && login_info in the context we don't need to authorize user
    token = context.user_data.get('token')
    login_info = context.user_data.get('login_info')
    if token is not None and login_info is not None:
        update.message.reply_text(hello(context.user_data))
        return

    # authorization starts here
    text = update.message.text
    code = text.replace('/start', '').strip()
    if len(code) == 0:
        return need_login(update, context)

    token = YandexOauthClient.get_token(code)
    context.user_data['token'] = token

    login_info = YandexPassportClient.login(token)
    context.user_data['login_info'] = login_info

    # hello
    update.message.reply_text(hello(context.user_data))


@oauth
def choose_device_handler(update, context):
    current_device_id = context.user_data.get('device')
    if current_device_id is None:
        msg = 'пока у тебя нет активной колонки, выбери ее с помощью команды /all'
        update.message.reply_text(msg)
    else:
        update.message.reply_text('твоя активная колонка: <code>{}</code>'.format(current_device_id), parse_mode=ParseMode.HTML)
    return


@oauth
@device
def say_handler(update, context):
    say_payload = " ".join(context.args)
    if len(say_payload) == 0:
        msg = 'чтобы сказать что-то колонкой, напиши `/say текст`'
        update.message.reply_text(msg)
        return

    device_id = context.user_data.get('device')
    user_id = context.user_data.get('login_info').get_user_id()
    BassClient.say(say_payload, device_id, user_id)


@oauth
@device
def do_handler(update, context):
    text_payload = update.message.text.strip()
    if len(text_payload) == 0:
        return
    elif len(text_payload) > 1024:
        msg = 'слишком длинное сообщение хочешь отправить, не делай так'
        update.message.reply_text(msg)
        return

    device_id = context.user_data.get('device')
    user_id = context.user_data.get('login_info').get_user_id()
    BassClient.do(text_payload, device_id, user_id)


@oauth
@device
def youtube_handler(update, context):
    youtube_payload = " ".join(context.args)
    if len(youtube_payload) == 0:
        msg = 'чтобы колонка запустила видео с ютуба запусти команду вида `/youtube url`'
        msg += '\nнапример: /youtube https://www.youtube.com/watch?v=dQw4w9WgXcQ'
        update.message.reply_text(msg, disable_web_page_preview=True)
        return

    parsed_url = urlparse(youtube_payload)
    timestamp = '0'
    if parsed_url.netloc in ['www.youtube.com', 'youtube.com']:
        qs = parse_qs(parsed_url.query)
        video_id = qs.get('v')[0] if qs.get('v') is not None else ''
        timestamp = qs.get('s')[0] if qs.get('s') is not None else '0'
    elif parsed_url.netloc in ['youtu.be', 'www.youtu.be']:
        video_id = parsed_url.path[1:]
    else:
        return

    if video_id == '':
        return

    device_id = context.user_data.get('device')
    user_id = context.user_data.get('login_info').get_user_id()
    BassClient.youtube(video_id, timestamp, device_id, user_id)


def logout_handler(update, context):
    context.user_data.pop('token', None)
    context.user_data.pop('login_info', None)
    context.user_data.pop('device', None)
    update.message.reply_text('готово! забыла тебя')


@oauth
def all_devices_handler(update, context):
    token = context.user_data.get('token')
    chat_id = update.message.chat.id
    try:
        all_devices = BulbasaurClient.user_info(token).get_speakers()
    except UnauthorizedError:
        return need_login(update, context)

    grouped_by_room = group_by_room(all_devices)
    for room, devices in grouped_by_room.items():
        button_list = [[InlineKeyboardButton('{} {}'.format(speaker, d.get("name")),
                                             callback_data="{}{}".format(CHOOSE, d.get("id")))] for d in devices]
        reply_markup = InlineKeyboardMarkup(button_list)
        context.bot.send_message(chat_id=chat_id, text=room, reply_markup=reply_markup)


def choose_speaker_button(update, context):
    match = CHOOSE_PATTERN.search(update.callback_query.data)
    device_id = match.groupdict().get(COMMAND)
    if device_id:
        context.user_data['device'] = device_id
        chat_id = update.callback_query.message.chat.id
        context.bot.send_message(chat_id=chat_id, text='отлично! активная колонка теперь <code>{}</code>'.format(device_id), parse_mode=ParseMode.HTML)
    update.callback_query.answer()


@oauth
def menu_handler(update, context):
    token = context.user_data.get('token')
    try:
        rooms = BulbasaurClient.user_info(token).get_rooms_with_lights()
    except UnauthorizedError:
        return need_login(update, context)

    button_list = [[InlineKeyboardButton('{} свет в комнате "{}"'.format(bulb, r.get("name")),
                                         callback_data="{}свет {}".format(MENU, r.get("name")))] for r in rooms]
    button_list.append([
        InlineKeyboardButton('{} моя музыка'.format(music), callback_data='{}включи мою музыку'.format(MENU)),
    ])
    button_list.append([
        InlineKeyboardButton('{} тише'.format(volume_down), callback_data='{}тише'.format(MENU)),
        InlineKeyboardButton('{} громче'.format(volume_up), callback_data='{}громче'.format(MENU))
    ])
    button_list.append([
        InlineKeyboardButton('{} назад'.format(arrow_left), callback_data='{}назад'.format(MENU)),
        InlineKeyboardButton('{} дальше'.format(arrow_right), callback_data='{}дальше'.format(MENU))
    ])
    button_list.append([
        InlineKeyboardButton('{} стоп'.format(cross), callback_data='{}хватит'.format(MENU)),
        InlineKeyboardButton('{} домой'.format(home), callback_data='{}домой'.format(MENU))
    ])
    reply_markup = InlineKeyboardMarkup(button_list)
    update.message.reply_text(text="вот, что я умею:", reply_markup=reply_markup)


@oauth
def menu_button(update, context):
    device_id = context.user_data.get('device')
    if device_id is None:
        return need_device(update, context)

    match = MENU_PATTERN.search(update.callback_query.data)
    text = match.groupdict().get(COMMAND)
    if text:
        login_info = context.user_data.get('login_info')
        BassClient.do(text, device_id, login_info.get_user_id())
    update.callback_query.answer()


@oauth
def remote_handler(update, context):
    button_list = list()
    button_list.append([
        InlineKeyboardButton('{} выше'.format(arrow_up), callback_data='{}выше'.format(REMOTE)),
        InlineKeyboardButton('{} ниже'.format(arrow_down), callback_data='{}ниже'.format(REMOTE))
    ])
    button_list.append([
        InlineKeyboardButton('{} назад'.format(arrow_left), callback_data='{}назад'.format(REMOTE)),
        InlineKeyboardButton('{} дальше'.format(arrow_right), callback_data='{}дальше'.format(REMOTE))
    ])
    button_list.append([
        InlineKeyboardButton('{} пауза'.format(pause), callback_data='{}пауза'.format(REMOTE)),
        InlineKeyboardButton('{} продолжи'.format(play), callback_data='{}продолжи'.format(REMOTE))
    ])
    button_list.append([
        InlineKeyboardButton('{} перемотать назад'.format(rewind), callback_data='{}перемотай на 10 секунд назад'.format(REMOTE)),
        InlineKeyboardButton('{} перемотать вперед'.format(fast_forward), callback_data='{}перемотай на 10 секунд вперед'.format(REMOTE))
    ])
    button_list.append([
        InlineKeyboardButton('{} тише'.format(volume_down), callback_data='{}тише'.format(REMOTE)),
        InlineKeyboardButton('{} громче'.format(volume_up), callback_data='{}громче'.format(REMOTE))
    ])
    button_list.append([
        InlineKeyboardButton('{} стоп'.format(cross), callback_data='{}хватит'.format(REMOTE)),
        InlineKeyboardButton('{} домой'.format(home), callback_data='{}домой'.format(REMOTE))
    ])
    reply_markup = InlineKeyboardMarkup(button_list)
    update.message.reply_text(text="вот, что я умею:", reply_markup=reply_markup)


@oauth
def remote_button(update, context):
    device_id = context.user_data.get('device')
    if device_id is None:
        return need_device(update, context)

    match = REMOTE_PATTERN.search(update.callback_query.data)
    text = match.groupdict().get(COMMAND)
    if text:
        login_info = context.user_data.get('login_info')
        BassClient.do(text, device_id, login_info.get_user_id())
    update.callback_query.answer()


@oauth
def scenario_handler(update, context):
    token = context.user_data.get('token')
    try:
        scenarios = BulbasaurClient.user_info(token).get_scenarios()
    except UnauthorizedError:
        return need_login(update, context)

    if len(scenarios) == 0:
        update.message.reply_text(text='ого, да у тебя совсем нет сценариев. создай их в приложении яндекса, в разделе "устройства"')
        return

    button_list = [[InlineKeyboardButton('{} {}'.format(gear, s.get("name")),
                                         callback_data="{}{}".format(SCENARIO, s.get("id")))] for s in scenarios]
    reply_markup = InlineKeyboardMarkup(button_list)
    update.message.reply_text(text="вот такие сценарии у тебя есть:", reply_markup=reply_markup)


@oauth
def scenario_button(update, context):
    match = SCENARIO_PATTERN.search(update.callback_query.data)
    text = match.groupdict().get(COMMAND)
    if text:
        try:
            token = context.user_data.get('token')
            BulbasaurClient.scenario_run(token, text)
        except Exception:
            pass  # how come?

    update.callback_query.answer()


@oauth
def info_handler(update, context):
    token = context.user_data.get('token')
    try:
        all_devices = BulbasaurClient.user_info(token).get_speakers()
    except UnauthorizedError:
        return need_login(update, context)

    grouped_by_room = group_by_room(all_devices)
    msg = ''
    for room, devices in grouped_by_room.items():
        msg += '{}:\n'.format(room)
        for d in devices:
            msg += '    {} (<code>{}</code>)\n'.format(d.get('name'), d.get('id'))
    if len(msg) == 0:
        msg += 'не похоже, что ты колоночку то купил, эх'
    update.message.reply_text(msg, parse_mode=ParseMode.HTML)


def main():
    # SQLAlchemy session maker
    def start_session() -> scoped_session:
        engine = create_engine(DB_URI, client_encoding="utf8")
        return scoped_session(sessionmaker(bind=engine, autoflush=False))

    # start the session
    session = start_session()

    updater = Updater(ALICE_SPEAKER_BOT_TOKEN, persistence=PostgresPersistence(session), use_context=True)

    dp = updater.dispatcher

    dp.add_handler(CommandHandler(["start"], log(start_handler)))
    dp.add_handler(CommandHandler(["h", "help"], log(help_handler)))
    dp.add_handler(CommandHandler(["d", "device"], log(choose_device_handler)))
    dp.add_handler(CommandHandler(["a", "all"], log(all_devices_handler)))
    dp.add_handler(CommandHandler(["l", "logout"], log(logout_handler)))
    dp.add_handler(CommandHandler(["i", "info"], log(info_handler)))

    dp.add_handler(CommandHandler(["s", "say"], log(say_handler)))
    dp.add_handler(CommandHandler(["y", "youtube"], log(youtube_handler)))
    dp.add_handler(CommandHandler(["m", "menu"], log(menu_handler)))
    dp.add_handler(CommandHandler(["r", "remote"], log(remote_handler)))
    dp.add_handler(CommandHandler(["sc", "scenario"], log(scenario_handler)))

    dp.add_handler(CallbackQueryHandler(log_callback(choose_speaker_button), pattern=CHOOSE_PATTERN))
    dp.add_handler(CallbackQueryHandler(log_callback(menu_button), pattern=MENU_PATTERN))
    dp.add_handler(CallbackQueryHandler(log_callback(remote_button), pattern=REMOTE_PATTERN))
    dp.add_handler(CallbackQueryHandler(log_callback(scenario_button), pattern=SCENARIO_PATTERN))

    dp.add_handler(MessageHandler(Filters.text & ~Filters.command, log(do_handler)))

    updater.start_webhook(listen='::',
                          port=8080,
                          url_path=ALICE_SPEAKER_BOT_TOKEN,
                          webhook_url='https://asb.iot.yandex.net/' + str(sha256(ALICE_SPEAKER_BOT_TOKEN)))
    updater.idle()


if __name__ == '__main__':
    main()
