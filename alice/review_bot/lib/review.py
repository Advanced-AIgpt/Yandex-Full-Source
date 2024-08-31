# coding: utf-8

import json
import logging
import os

from alice.review_bot.ybot.core.events import listener
from alice.review_bot.ybot.modules.telegram import get_bot
from alice.review_bot.lib.helpers import update_state, ReviewInfo, format_review_list


REVIEWS_KEY = 'reviews'
MODULE_NAME = 'alice.review_bot.lib.review'
logger = logging.getLogger(MODULE_NAME)


def get_state(storage, chat_id=None):
    if chat_id is None:
        data = storage.get(REVIEWS_KEY, '[]')
    else:
        data = storage.get(chat_id, REVIEWS_KEY, '[]')

    return json.loads(data)


def set_state(storage, new_value, chat_id=None):
    data = json.dumps(new_value)
    if chat_id is None:
        storage.set(REVIEWS_KEY, data)
    else:
        storage.set(chat_id, REVIEWS_KEY, data)


def get_reviews(storage, chat_id=None):
    cur_state = get_state(storage, chat_id)
    return map(ReviewInfo.from_dict, cur_state)


def set_reviews(storage, reviews, chat_id=None):
    new_state = [x.to_dict() for x in reviews]
    set_state(storage, new_state, chat_id)


def update_reviews_state(storage, token, chat_id=None, urls=None, mentions=None):
    if urls is None:
        urls = []

    if mentions is None:
        mentions = []

    state = get_state(storage, chat_id=chat_id)
    new_state, new_reviews = update_state(state, urls, mentions, token)

    logger.debug('Update reviewers state %s', new_state)
    set_state(storage, new_state, chat_id=chat_id)
    return new_state, new_reviews


@listener('ybot.telegram.message')
def parse_review_links(ctx, event_name, value):
    conf = ctx.config[MODULE_NAME]
    token = os.environ[conf['arcanum_token_env']]
    msg = value.message

    url_entities = msg.parse_entities(['url'])
    mention_entities = msg.parse_entities(['mention'])

    if not url_entities:
        return

    new_state, new_reviews = update_reviews_state(
        value.state,
        token,
        urls=url_entities.values(),
        mentions=mention_entities.values(),
    )

    if new_reviews:
        reviews_formatted = format_review_list(new_reviews)
        value.send_message(text=u'Запомнил:\n' + reviews_formatted, parse_mode='HTML')


@listener('ybot.telegram.command')
def commands(ctx, name, value):
    msg = None

    if '/list' in value.message.text:
        msg = format_review_list(get_reviews(value.state))
        if msg == '':
            msg = u'Нет ни одного актуального ревью.'
    elif '/help' in value.message.text:
        msg = (u'Бот помогает отслеживать актуальные ревью.'
               u' Добавляй бота в чат группы или кидай ему в личку ссылки на арканум'
               u' и получай список актуальных ревью командой /list или в ежедневной рассылке.')

    if msg is not None:
        value.send_message(text=msg, parse_mode='HTML')


@listener('alice.review_bot.lib.review.whats_new')
def whats_new(ctx, name, value):
    for chat_id in ctx.state.get_chat_ids():
        reviews = list(get_reviews(ctx.state, chat_id=chat_id))

        if not reviews:
            return

        msg = format_review_list(reviews)

        msg = u'Актуальные ревью:\n\n{}'.format(msg)
        get_bot().send_message(chat_id=chat_id, text=msg, parse_mode='HTML')


@listener('alice.review_bot.lib.review.update')
def update_reviews(ctx, name, value):
    conf = ctx.config[MODULE_NAME]
    token = os.environ[conf['arcanum_token_env']]

    for chat_id in ctx.state.get_chat_ids():
        update_reviews_state(ctx.state, token, chat_id)
