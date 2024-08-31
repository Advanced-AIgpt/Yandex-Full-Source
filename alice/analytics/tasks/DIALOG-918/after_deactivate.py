#!/usr/bin/env python
# encoding: utf-8
import json
from collections import Counter

fn = 'sessions-2017-10-13.json'
#fn = 'sessions-2017-10-18.json'


with open(fn) as _inp:
    SESSIONS = map(json.loads, _inp)
del _inp

#print len(SESSIONS)
#print SESSIONS[0]['actions'][0]['request_text']


# ====== Включают - выключают ===========
def zero_request():
    for s in SESSIONS:
        if s['phrase_cnt'] == 0:
            print s['actions'], '\n'


# ================= Разовые запросы ====================
def one_request():
    cnt = Counter()
    for s in SESSIONS:
        if s['phrase_cnt'] == 1:
            for a in s['actions']:
                cnt[a['form_name']] += 1
            #    print a['form_name'], a['request_text']
            #print '\n'

    for l in cnt.most_common():
        print l


def what_after():
    cnt = Counter()
    for s in SESSIONS:
        if s['explicit_deactivate'] and s['is_first']:
            cnt['total'] += 1
            it = iter(s['actions'])
            deactivated = False
            for a in it:
                if a['form_name'] and a['form_name'].endswith('deactivate'):
                    deactivated = True
                    continue
                elif deactivated:
                    cnt[a['form_name']] += 1
                    break
            else:
                cnt['finish'] += 1

    for k, v in cnt.most_common():
        print '%s %s (%d%%)' % (k, v, 100.0 * v / cnt['total'])

what_after()
