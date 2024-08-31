#!/usr/bin/env python
# encoding: utf-8
import json
from urllib2 import urlopen, URLError
from urllib import urlencode
from time import time, sleep
from itertools import izip, cycle
import threading


intent_url = 'http://vins-int.dev.voicetech.yandex.net/qa/nlu/pa/?{}'.format
small_talk_categories = ['general_conversation', 'handcrafted', 'feedback']


def fill_record(fields, response):
    for x in response:
        intent_name = x['intent_name']
        if any((p in intent_name) for p in small_talk_categories):
            continue
        elif 'search' in intent_name:
            if fields['search_name']:  # незачем перезаписывать менее вероятным ответом
                continue
            fields['search_name'] = intent_name
            fields['search_confidence'] = x['confidence']
            fields['search_score'] = x['tagger_score']
            if fields['intent_name']:
                return fields  # всё что нужно заполнено
        else:
            if fields['intent_name']:  # незачем перезаписывать менее вероятным ответом
                continue
            fields['intent_name'] = intent_name
            fields['intent_confidence'] = x['confidence']
            fields['intent_score'] = x['tagger_score']
            if fields['search_name']:
                return fields # всё что нужно заполнено
    return fields


def get_intent(text, retries=5):
    fields = {'intent_name': '',
              'intent_confidence': 0.0,
              'intent_score': 0.0,
              'search_name': '',
              'search_confidence': 0.0,
              'search_score': 0.0}

    if not text:
        return fields

    url = intent_url(urlencode({'utterance': text}))
    for x in xrange(retries):
        try:
            response = json.load(urlopen(url))
        except URLError, err:
            print 'intermediate', err
            continue
        else:
            return fill_record(fields, response)
    print 'overall', err
    return fields


def write_intents(inp_path='unknown_utterances.tsv',
                  out_path='possible_intents.tsv'):
    fmt = '{text}\t{intent_name}\t{intent_confidence}\t{intent_score}\t{search_name}\t{search_confidence}\t{search_score}\n'

    with open(inp_path) as inp:
        with open(out_path, 'a') as out:
            st = time()
            for n, line in enumerate(inp):
                if n % 200 == 0:
                    print n, out_path, (time() - st) / 60
                line = line.strip()
                intent = get_intent(line)
                out.write(fmt.format(text=line, **intent))


#write_intents()


def split_unknown(inp_path='unknown_utterances.tsv', parts_cnt=8, out_path_fmt='unknown_{}.tsv'):
    files = [open(out_path_fmt.format(x), 'w')
             for x in xrange(parts_cnt)]

    with open(inp_path) as f:
        for n, line in izip(cycle(range(8)), f):
            files[n].write(line)

    for f in files:
        f.close()

#split_unknown()


def threaded_write_intent(inp_path_fmt='unknown_{}.tsv', parts_cnt=8, out_path_fmt='intents_{}.tsv'):
    threads = []
    for x in xrange(parts_cnt):
        t = threading.Thread(target=write_intents, args=(inp_path_fmt.format(x),
                                                         out_path_fmt.format(x)))
        t.start()
        threads.append(t)

    for t in threads:
        t.join()


#threaded_write_intent(parts_cnt=8)


def merge_intents(out_path='possible_intents.tsv', parts_cnt=8, inp_path_fmt='intents_{}.tsv'):
    with open(out_path, 'a') as out:
       for x in xrange(parts_cnt):
           with open(inp_path_fmt.format(x)) as inp:
               out.write(inp.read())


merge_intents()




#hahn.write(intent_path, iter_intent_records(), append=True)

#print get_intent('столица малайзии')
#print 'the end'


#print get_intent(u'зонт сегодня нужен')
#print get_intent(u'окей ещё')
#print get_intent(u'чёртичо')
#print get_intent(u'столица малайзии')

