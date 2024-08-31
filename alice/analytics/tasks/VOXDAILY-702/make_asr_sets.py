#!/usr/bin/env python
# encoding: utf-8
"""
Подготовка dev/train сетов для asr.
В готовую разметку добавляется контекст из диалога.
Выгружается по-умолчанию в //home/voice/testsets/asr_context.

Выгрузка по каждой дате идёт в отдельную табличку и параллельно, но нужно помнить об ограничениях на количество джобов в YT.

Так же стоит иметь в виду, что разметка есть не за все дни, а параметры start_date, end_date должны задавать непрерывный диапазон.
"""
import re
from collections import deque
from functools import partial
from threading import Thread

from nile.api.v1 import clusters, filters as nf
from nile.api.v1 import extractors as ne
from nile.api.v1.record import Record
from nile.api.v1.datetime import date_range

from qb2.api.v1 import filters as qf

from nirvana.job_context import context


ANNOTATIONS_PATH = '//home/voice/toloka/ru-RU/daily/assistant/'
SPEECHBASE_PATH = '//home/voice-speechbase/uniproxy/logs_v2/'
QLOUD_PATH = '//home/logfeller/logs/qloud-runtime-log/1d/'
DIALOGS_PATH = '//home/voice/vins/logs/dialogs/'

ANN_FIELDS = ['mark', 'mds_key', 'text', 'url', 'uuid']
SPEECH_FIELDS = ['mds_key', 'requestId']
DLG_FIELDS = ['utterance_text', 'uuid', 'server_time', 'callback_name', 'form_name', 'request', 'response']
SESSION_FIELDS = ['mark', 'text', 'url', 'requestId', 'uuid']


VINS_REQ_SEARCH = re.compile(r'(?<=\"request_id\"=\")[^"]+').search
USER_ID_SEARCH = re.compile(r'(?<=\"uuid\"=\")[^"]+').search
UNIPROXY_REQ_SEARCH = re.compile(r'(?<=\"ForEvent\"=\")[^"]+').search


def msg_exractor(searcher):
    def extractor(msg):
        if msg is None:
            return None
        m = searcher(msg)
        if m is None:
            return None
        return m.group()

    return ne.custom(extractor, 'message')


def to_action(act_record):
    fields = {'callback_name': act_record.get('callback_name'),
              'form_name': act_record.get('form_name'),
              'request_text': act_record.get('utterance_text'),
              'request_id': act_record.request['request_id']}

    try:
        fields['response_text'] = act_record.response['cards'][0]['text']
    except (KeyError, IndexError):
        fields['response_text'] = ''

    return fields


def reduce_to_sessions(groups, size=3):
    for key, records in groups:
        actions = deque(maxlen=size)

        for act_rec in records:
            try:
                actions.append(to_action(act_rec))
            except AttributeError:
                continue  # skip records without request

            if act_rec.get('vins_req_id') == act_rec.request['request_id']:
                yield Record(actions=list(actions), **{f: act_rec[f] for f in SESSION_FIELDS})
                list(deque(records, maxlen=0))  # Быстрый способ "доесть" итератор


def get_annotations(job, date, context_size=3):
    uuid_fmt = 'uu/{}'.format
    ann = job.table(ANNOTATIONS_PATH + date).project(*ANN_FIELDS, user_id=ne.custom(uuid_fmt, 'uuid'))
    speechbase = job.table(SPEECHBASE_PATH + date).project(*SPEECH_FIELDS)

    qloud = job.table(QLOUD_PATH + date).filter(qf.search('message', '"type\\"=\\"VinsRequest\\"'),
                                                nf.equals('qloud_application', 'uniproxy'))
    qloud = qloud.project(vins_req_id=msg_exractor(VINS_REQ_SEARCH),
                          #user_id=msg_exractor(USER_ID_SEARCH),  # не пригодилось
                          uniproxy_req_id=msg_exractor(UNIPROXY_REQ_SEARCH))

    dialogs = job.table(DIALOGS_PATH + date).project(*DLG_FIELDS)

    #ann = ann.random(count=100)  # for debug
    ann = ann.join(speechbase, by='mds_key', assume_unique=True, type='left')
    ann = ann.join(qloud, by_left='requestId', by_right='uniproxy_req_id', assume_unique=True, type='left')
    ann = ann.join(dialogs, by_left='user_id', by_right='uuid', assume_unique_left=True, type='left')

    ann = ann.groupby("uuid").sort('server_time').reduce(partial(reduce_to_sessions, size=context_size))
    return ann


def make_set(date, context_size, out_prefix):
    job = clusters.Hahn().job('make_asr_set_%s' % date)
    ann = get_annotations(job, date, context_size)
    ann.put('%s/%s' % (out_prefix, date))
    job.run()


def main():
    ctx = context()
    params = ctx.get_parameters()

    start_date = params['start_date']
    end_date = params['end_date']
    context_size = params['context_size']  # Количество пар вопрос-ответ, включая аннотированный, default: 3
    out_prefix = params['out_prefix'].rstrip('/')    # default: '//home/voice/testsets/asr_context'

    if start_date == end_date:
        make_set(start_date, context_size, out_prefix)
    else:
        threads = []
        for date in date_range(start_date, end_date):
            t = Thread(target=make_set, args=(date, context_size, out_prefix))
            t.start()
            threads.append(t)

        for t in threads:
            t.join()


if __name__ == '__main__':
    main()
