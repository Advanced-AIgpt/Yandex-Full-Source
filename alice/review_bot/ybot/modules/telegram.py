# coding: utf-8

"""
Sample config:

ybot.modules.telegram:
    token_env: "YBOT_TELEGRAM_TOKEN"
    connection_pool: 8

"""

from __future__ import absolute_import
import logging
import re
import os

import attr
import telegram
from telegram.utils.request import Request

from ..core.events import emitter, splitter
from ..core.state import ChatStateWrapper


MODULE_NAME = 'ybot.modules.telegram'

__bot = None
logger = logging.getLogger(__name__)


def get_bot():
    if __bot is None:
        raise RuntimeError
    return __bot


def set_bot(bot):
    global __bot
    __bot = bot


@attr.s
class Message(object):
    chat_id = attr.ib()
    state = attr.ib()
    message = attr.ib()
    bot = attr.ib()

    def send_message(self, **kwargs):
        self.bot.send_message(self.chat_id, **kwargs)


@emitter('ybot.telegram.message')
def updates(ctx):
    conf = ctx.config[MODULE_NAME]
    request = Request(
        con_pool_size=conf.get('connection_pool', 8),
        connect_timeout=conf.get('connection_timeout', 1),
        read_timeout=conf.get('read_timeout', 15),
    )

    bot = telegram.Bot(token=os.environ[conf['token_env']], request=request)
    set_bot(bot)

    timeout = conf.get('polling_timeout', 10)
    offset = None

    while True:
        try:
            updates = bot.get_updates(offset=offset, timeout=timeout) or []
        except telegram.TelegramError:
            logger.exception('Telegram error')
            continue

        for update in updates:
            offset = update.update_id + 1
            if update.message:
                chat = update.effective_chat
                logger.debug('Got update from chat %s', chat.id)
                yield Message(
                    chat_id=chat.id,
                    state=ChatStateWrapper(chat.id, ctx.state),
                    message=update.message,
                    bot=bot,
                )


@splitter(
    ['ybot.telegram.message'],
    [
        'ybot.telegram.command',
        'ybot.telegram.new_chat_participant',
        'ybot.telegram.left_chat_participant',
    ],
)
def command_splitter(ctx, name, value):
    conf = ctx.config[MODULE_NAME]
    msg = value.message

    bot_name = conf.get('bot_name', r'\w{1,32}')
    pattern = r'^/\w+(:?@{name})?( .*)?$'.format(name=bot_name)

    if re.match(pattern, msg.text):
        yield ('ybot.telegram.command', value)

    if msg.new_chat_members:
        yield ('ybot.telegram.new_chat_members', value)
    elif msg.left_chat_member:
        yield ('ybot.telegram.left_chat_member', value)
