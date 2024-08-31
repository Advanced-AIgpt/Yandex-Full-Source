#!/usr/bin/env python
# encoding: utf-8
from __future__ import unicode_literals
import re

from nile.api.v1 import clusters, Record

#
#
# LIKE_UTF8 = '\xf0\x9f\x91\x8d'  # 👍
# DISLIKE_UTF8 = '\xf0\x9f\x91\x8e'  # 👎
# SEARCH_UTF8 = '\xf0\x9f\x94\x8d'  # 🔍
#
# SPECIAL = [LIKE_UTF8, DISLIKE_UTF8, SEARCH_UTF8]

LIKE_UNICODE = u'\U0001f44d'  # 👍
DISLIKE_UNICODE = u'\U0001f44e'  # 👎
SEARCH_UNICODE = u'\U0001f50d'  # 🔍

SPECIAL = [LIKE_UNICODE, DISLIKE_UNICODE, SEARCH_UNICODE]


def edit_distance(s1, s2):
    # Source: http://rosettacode.org/wiki/Levenshtein_distance#Python
    if len(s1) > len(s2):
        s1, s2 = s2, s1
    distances = range(len(s1) + 1)
    for idx2, char2 in enumerate(s2):
        new_distances = [idx2 + 1]
        for idx1, char1 in enumerate(s1):
            if char1 == char2:
                new_distances.append(distances[idx1])
            else:
                new_distances.append(1 + min((distances[idx1],
                                             distances[idx1 + 1],
                                             new_distances[-1])))
        distances = new_distances
    return distances[-1]


def max_similar(phrase, suggests):
    best = min((edit_distance(phrase, normalize(s)), -len(s), s)
               for s in suggests)
    # sentence, similarity
    return best[2], max(float(len(phrase) - best[0]), 0.0) / len(phrase)


def normalize(sentence):
    return re.sub('[^a-я ]', '', sentence.lower(), flags=re.UNICODE).strip()


def is_not_special(sentence):
    for s in SPECIAL:
        if sentence.startswith(s):
            return False
    return True


def iter_session(session):
    suggests = None
    for replica in session:
        q = replica['_query']
        if q and suggests and replica["type"] == "voice":
            q = unicode(q, 'utf-8')
            sentence, similarity = max_similar(q, suggests)
            if similarity <= 0:
                sentence = None
            yield q, sentence, similarity, suggests
        cur = replica['suggests']
        if cur:
            cur = (unicode(s, 'utf-8') for s in cur)
            suggests = filter(is_not_special, cur)


# Join query with suggests

SEPARATOR = ' <[]> '

def join_suggest(records):
    for rec in records:
        last_suggests = None
        for replica in rec.session:
            q = replica['_query']
            if q and last_suggests and replica["type"] == "voice":
                q = normalize(unicode(q, 'utf-8'))
                yield Record(query='%s%s%s' % (q, SEPARATOR, last_suggests))
            cur_suggests = replica['suggests']
            if cur_suggests:
                cur_suggests = (unicode(s, 'utf-8') for s in cur_suggests)
                cur_suggests = filter(is_not_special, cur_suggests)
                last_suggests = SEPARATOR.join(map(normalize, cur_suggests))


def to_joined(date,
              session_root='//home/voice/dialog/sessions',
              out_root='//home/voice/yoschi/dialogs/suggests'):
    job = clusters.Hahn().job()
    inp = job.table('%s/%s' % (session_root, date))
    inp.project(
        b'session'
    ).map(
        join_suggest
    ).put('%s/%s' % (out_root, date), schema={
        'query': unicode,
    })
    job.run()


# Match with joined

def split_suggest(records):
    for rec in records:
        try:
            q, suggests = unicode(rec.query, 'utf-8').split(SEPARATOR, 1)
        except ValueError:
            continue
        suggests = suggests.split(SEPARATOR)
        sentence, similarity = max_similar(q, suggests)
        if similarity <= 0:
            sentence = None
        yield Record(query=q, suggested=sentence, similarity=similarity, others=suggests)


def to_splitted(inp_path, out_path=None):
    if out_path is None:
        out_path = inp_path.replace('_normalized', '') + '_sim'

    job = clusters.Hahn().job()
    inp = job.table(inp_path)
    inp.map(split_suggest).put(out_path, schema={
        'query': str,
        'suggested': str,
        'similarity': float,
        'others': dict,
    })
    job.run()


# YT interactions


def mapper(records):
    for r in records:
        for m in iter_session(r.session):
            yield Record(query=m[0], suggested=m[1], similarity=m[2], others=m[3])


def to_matchings(date, session_root='//home/voice/dialog/sessions', out_root='//home/voice/yoschi/dialogs/suggests'):
    job = clusters.Hahn().job()
    inp = job.table('%s/%s' % (session_root, date))
    inp.map(mapper).put('%s/%s' % (out_root, date), schema={
        'query': str,
        'suggested': str,
        'similarity': float,
        'others': dict,
    })
    job.run()



# TESTING

# phrase = "и мне очень приятно"
#
# suggests = [
#     "И мне тоже очень приятно",
#     "Взаимно, а ты откуда?",
#     "Мне тоже",
#     "И мне приятно",
#     "И мне тоже",
#     "Что ты умеешь?"
#     ]

#for s in suggests: print normalize(s)

#print max_similar(phrase, suggests)


#"Tuple<Int32,String>"

# import json
#
# session = json.load(open('session.json'))
#
# for p in iter_session(session):
#     print p[0], p[1], p[2], ' | '.join(p[3])


#to_matchings('2018-01-19')
