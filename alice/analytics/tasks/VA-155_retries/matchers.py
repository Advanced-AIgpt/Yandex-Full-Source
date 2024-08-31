#!/usr/bin/env python
# encoding: utf-8
from __future__ import unicode_literals
import re

from nile.api.v1 import clusters, Record
from nile.api.v1 import extractors as ne


def is_extended_from(ext, src):  # сравнивать ещё длины
    pattern = u'.*'.join(map(re.escape, src))
    if re.search(pattern, ext, flags=re.U | re.I | re.M):
        return float(len(src)) / len(ext)
    else:
        return 0.0


def set_match(text1, text2):
    set1 = set(text1.split())
    set2 = set(text2.split())
    intersect = set1 & set2
    # len(intersect) / len(union)
    if intersect:
        return float(len(intersect)) / (len(set1) + len(set2) - len(intersect))
    else:
        return 0.0


def levenshtein_match(s1, s2):
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

    dist = distances[-1]
    return float(len(s2) - dist) / len(s2)


def test_it():
    assert is_extended_from(u'смотреть фильм', u'фильм') > 0
    assert is_extended_from(u'фильм', u'смотреть фильм') == 0.0
    assert is_extended_from(u'не фильм', u'смотреть фильм') == 0.0
    assert levenshtein_match('а', 'b') == 0.0
    assert levenshtein_match('абвд', 'гбв') == 0.5
    assert levenshtein_match('фыва', 'фыва') == 1.0

    return "OK"


# YT interaction


def iter_session(session):
    prev_query, prev_intent = None, None
    for replica in session:
        query = replica['_query'] and unicode(replica['_query'], 'utf-8')
        intent = replica['intent'] and unicode(replica['intent'], 'utf-8')
        if query:
            if prev_query:
                yield query, prev_query, intent, prev_intent
            prev_query = query
            prev_intent = intent


def mapper(records):
    for r in records:
        for query, prev_query, intent, prev_intent in iter_session(r.session):
            conf_ext = is_extended_from(query, prev_query)
            conf_set = set_match(query, prev_query)
            conf_lvns = levenshtein_match(query, prev_query)
            # TODO: Другие типы совпадений
            if conf_ext > 0.3 or conf_set > 0.3 or conf_lvns > 0.3:
                yield Record(query=query, prev_query=prev_query,
                             intent=intent, prev_intent=prev_intent,
                             conf_ext=conf_ext, conf_set=conf_set, conf_lvns=conf_lvns)


def to_matchings(date, session_root='//home/voice/dialog/sessions', out_root='//home/voice/yoschi/dialogs/retries'):
    job = clusters.Hahn().job()
    inp = job.table('%s/%s' % (session_root, date)).project(b'session').random(fraction=0.0002)
    inp.map(mapper).put('%s/%s' % (out_root, date), schema={
        'query': str,
        'prev_query': str,
        'intent': str,
        'prev_intent': str,
        'conf_ext': float,
        'conf_set': float,
        'conf_lvns': float,
    })
    job.run()


if __name__ == '__main__':
    to_matchings('2018-05-04')
    #print test_it()
