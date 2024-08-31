#!/usr/bin/python
# coding=utf-8
import re
import random
import time
from alice.boltalka.tools.reply_rewriter.base_replacer import BaseReplacer

BAD_REPLIES = [
    u'сладенький ответ', u'знаешь чей ответ', u'сам знаешь кого ответ',
    u'сама знаешь кого ответ', u'гомофоба ответ', u'голубя ответ',
    u'алкаша ответ', u'ты знаешь чей это ответ', u'ты знаешь чей ответ',
    u'дурочки ответ', u'прокурора ответ', u'еврейский ответ', u'еврея ответ',
    u'кассира макдональдса ответ', u'ну ты знаешь чей ответ',
    u'знаешь кого ответ', u'гуманитария ответ', u'дурака ответ',
    u'либерала ответ', u'кого ответ', u'чей ответ', u'жадины ответ',
    u'медика ответ', u'школьника ответ', u'знаешь кого ответ', u'рэпера ответ',
    u'репера ответ', u'историка ответ', u'женщины ответ', u'шоколадки ответ',
    u'юли ответ', u'кати ответ', u'светы ответ', u'дат', u'скотины ответ',
    u'ответ от бога', u'эрика ответ'
]

BAD_REPLIES = [' ' + s.strip() + ' ' for s in BAD_REPLIES]

REPLACE_REPLIES = [
    u'красноречивый ответ', u'содержательный ответ', u'лаконичный ответ',
    u'очень развернутый ответ', u'какой содержательный ответ',
    u'я предсказала ваш ответ', u'котика ответ', u'информативный ответ',
    u'отрицательный ответ', u'это ваш окончательный ответ ?',
    u'исчерпывающий ответ', u'не могу придумать смешной ответ',
    u'на нет и суда нет', u'аргументированный ответ', u'сухой ответ'
]


def get_random(list_):
    return list_[random.randint(0, len(list_) - 1)]


def substitute_reply(bad, replace, reply):
    original_reply = reply
    reply = ' '.join(re.sub(ur'[^а-я\s]', ' ',
                            reply.strip()).split(' ')).strip()
    reply = ' ' + reply + ' '
    for b in bad:
        if b in reply:
            return get_random(replace)
    return original_reply


class SubstituteReplacer(BaseReplacer):
    def start(self, local=False):
        random.seed(int(time.time() * 1e6))

    def process(self, reply):
        return substitute_reply(BAD_REPLIES, REPLACE_REPLIES, reply)
