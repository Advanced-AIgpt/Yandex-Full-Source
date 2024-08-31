#!/usr/bin/env python
# encoding: utf-8
from os.path import join

from nile.api.v1 import (
    Record,
    clusters,
    grouping as ng,
    extractors as ne,
    datetime as nd,
    filters as nf,
)


SCHEMA = {
    'intent_from': str,  # general_conversation only
    'intent_to': str,
    'src_query': str,
    'src_reply': str,
    'suggested': [str],
    'clicked_caption': str,
    'clicked_reply': str,
    'has_first_search': bool,
    'use_search': bool,
    # user info
    'uuid': str,
    'fielddate': str,
    'sample': str,  # control, experiment
}


CONTROL_ID = "74673"
EXP_ID = "74674"


LIKE_UTF8 = '\xf0\x9f\x91\x8d'  # 👍
DISLIKE_UTF8 = '\xf0\x9f\x91\x8e'  # 👎
FEEDBACK_SUGGESTS = [LIKE_UTF8, DISLIKE_UTF8]
SEARCH_UTF8 = '\xf0\x9f\x94\x8d'


def gen_transitions(records):
    for record in records:
        if CONTROL_ID in record['testids']:
            sample = 'control'
        elif EXP_ID in record['testids']:
            sample = 'experiment'
        else:
            continue

        user_fields = {'uuid': record['uuid'],
                       'fielddate': record['fielddate'],
                       'sample': sample}

        pending = None
        steps = 0
        for req in record['session']:
            cb = req.get('callback')
            if cb and cb.get('name') == 'on_suggest' and pending:
                caption = cb.get('caption')
                req_fields = {
                    'intent_to': req['intent'],
                    'clicked_caption': caption,
                    'clicked_reply': req['_reply'],
                    'use_search': caption and caption.startswith(SEARCH_UTF8),
                }
                req_fields.update(user_fields)

                if caption in pending['suggested']:
                    req_fields.update(pending)
                    yield Record(**req_fields)
                else:
                    pending.update(user_fields)
                    yield Record(**pending)

                    yield Record(**req_fields)

                pending = None
                steps = 0

            if 'general_conversation' in req['intent']:
                pending = {
                    'intent_from': req['intent'],
                    'suggested': req['suggests'],
                    'has_first_search': False,
                    'src_query': req['_query'],
                    'src_reply': req['_reply'],
                }
                steps = 0

                for s in pending['suggested']:
                    if s in FEEDBACK_SUGGESTS:
                        continue
                    elif s.startswith(SEARCH_UTF8):
                        pending['has_first_search'] = True
                    break
            elif pending:
                steps += 1
                if steps > 2:
                    pending.update(user_fields)
                    yield Record(**pending)
                    pending = None

        if pending:
            pending.update(user_fields)
            yield Record(**pending)


def make_transition_table(date='2018-04-17',
                          session_root='//home/voice/dialog/sessions',
                          out_root='//home/voice/yoschi/sessions'):
    job = clusters.Hahn().job()
    tbl = job.table(
        join(session_root, date)
    ).map(
        gen_transitions
    ).put(
        join(out_root, date),
        schema=SCHEMA,
    )

    with job.driver.transaction():
        job.run()


make_transition_table()
