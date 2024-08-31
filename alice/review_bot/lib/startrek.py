# coding: utf-8
import os
import re

from gevent.pool import Pool

from startrek_client import Startrek

from alice.review_bot.ybot.core.events import listener


__client = None
MODULE_NAME = 'alice.review_bot.lib.startrek'
ST_REGEXP = re.compile(r'(?:https?://st\.yandex-team\.ru\/)?([A-Z][A-Z0-9]+-\d+)/?')


def get_st_client(conf):
    global __client

    if __client is None:
        __client = Startrek(
            useragent='alice-review_bot',
            token=os.environ[conf['startrek_token_env']]
        )

    return __client


def format_message(message, issues, keys):
    url_tmplt = '<a href="{url}">{title}</a>'

    user = message.from_user
    res = '<b>{} {}</b> @{}\n\n'.format(user.first_name, user.last_name, user.username)
    start = 0

    for key, span in keys:
        iss = issues[key]
        begin, end = span

        # text formatting
        res += message.text[start:begin]
        res += url_tmplt.format(
            url='https://st.yandex-team.ru/' + iss.key,
            title='[{key}] {msg}'.format(key=iss.key, msg=iss.summary)
        )

        start = end
    return res + message.text[start:]


@listener('ybot.telegram.message')
def parse_standup(ctx, event_name, value):
    conf = ctx.config[MODULE_NAME]
    client = get_st_client(conf)

    STANDUP_HASHTAG = '#standup'
    text = value.message.text

    if STANDUP_HASHTAG not in text:
        return

    matches = ST_REGEXP.finditer(text)
    keys = [(m.group(1), m.span()) for m in matches]

    if keys:
        pool = Pool(5)
        issues = pool.imap(client.issues.get, set(i[0] for i in keys))
        issues = {i.key: i for i in issues}

        response = format_message(value.message, issues, keys)
        resp = value.send_message(text=response, parse_mode='HTML')
        value.bot.delete_message(
            chat_id=value.chat_id,
            message_id=value.message.message_id,
        )
        return resp
    else:
        return value.send_message(text='Пожалуйста, прилинкуй тикеты',
                                  reply_to_message_id=value.message.message_id)
