#!/usr/bin/env python
# encoding: utf-8
from itertools import izip
from nile.api.v1 import clusters, filters
import nile.api.v1.aggregators as aggr
from nile.api.v1.utils import with_hints
from qb2.api.v1 import filters as qf
from nile.api.v1 import filters as nf
from nile.api.v1.record import Record

#sample_path = '//home/voice/yoschi/tmp/sample-13-abc'
#session_path = '//home/voice/yoschi/tmp/sessions-13-abc'

# 13-bc, 14-ef, 18-de, 20-cd

date, prefix = '2017-10-18', 'c2-e3'
#date, prefix = '2017-10-18', 'de'
#date, prefix = '2017-10-13', 'a0-b1'

sample_path = '//home/voice/yoschi/dialogs/%s-%s' % (date, prefix)
session_path = '//home/voice/yoschi/dialogs/sessions-%s-%s' % (date, prefix)

lifetime_path = '//home/voice/yoschi/dialogs/users_lifetime'


explicit_timeout = 30  # Если session_start случился раньше, чем через этот таймаут,
                       # считаем продолжением разговора
implicit_timeout = 1800  # Если пользователь явным образом не выходил,
                         # но промолчал столько времени - считаем началом нового разговора

possible_intents_path = 'possible_intents.tsv'
intent_fields = [('intent_name', str), ('intent_confidence', float), ('intent_score', float),
                 ('search_name', str), ('search_confidence', float), ('search_score', float)]
small_talk_categories = ['general_conversation', 'handcrafted', 'feedback']


hahn = clusters.Hahn()

job = hahn.job()

sample_table = job.table(sample_path)
lifetime_table = job.table(lifetime_path)


# In[58]:


import yt.wrapper as yt


client = yt.YtClient(proxy='hahn', config={"tabular_data_format": "tsv"})


schema = [{'name': 'user_id', "type" : "string"},
          {'name': 'actions', "type" : "any"},
          {'name': 'start_time', "type" : "uint64"},
          {'name': 'end_time', "type" : "uint64"},
          {'name': 'is_first', "type" : "boolean"},
          {'name': 'phrase_cnt', "type" : "uint64"},
          {'name': 'external_cnt', "type" : "uint64"},
          {'name': 'maybe_search_cnt', "type" : "uint64"},
          {'name': 'maybe_intent_cnt', "type" : "uint64"},
          {'name': 'for_small_talk', "type" : "boolean"},
          {'name': 'explicit_deactivate', "type" : "uint64"}]


def make_table(tname, schema):
    if client.exists(tname):
        pass #client.alter_table(tname, schema=schema)
    else:
        client.create_table(tname, attributes={'schema': schema})


make_table(session_path, schema)


INTENTS = {}
with open(possible_intents_path) as f:
    for line in f:
        parts = line.strip().split('\t')
        text = parts[0]
        INTENTS[text] = parts[1:]

del f


class SessionRecord(Record):
    def __init__(self, first_act_record):
        super(SessionRecord, self).__init__(
            user_id=first_act_record.uuid,
            actions=[],
            start_time=first_act_record.server_time,
            end_time=first_act_record.server_time,
            is_first=(first_act_record.server_time == first_act_record.first_time),
            phrase_cnt=0,
            external_cnt=0,
            maybe_search_cnt=0,
            maybe_intent_cnt=0,
            for_small_talk=None,
            explicit_deactivate=0)
        self.update_with_next_act(first_act_record)

    def to_action(self, act_record):
        fields = {'callback_name': act_record.callback_name,
                  'form_name': act_record.form_name,
                  'request_text': act_record.utterance_text}

        form_name = act_record.form_name or ''

        if self.for_small_talk is None:
            if 'external' in form_name:
                self.for_small_talk = True
            elif all((p not in form_name) for p in small_talk_categories):
                self.for_small_talk = False

        if act_record.utterance_text:
            self.phrase_cnt += 1

        if form_name.endswith('external_skill__deactivate'):
            try:
                for meta_rec in act_record.response['meta']:
                    if meta_rec['type'] == 'error':
                        break
                else:
                    self.explicit_deactivate += 1
            except (KeyError, IndexError), err:
                #print err
                pass

        if 'external' in form_name:
            self.external_cnt += 1
            try:
                intent_estimate = INTENTS[act_record.utterance_text]
                possible = {f: conv(val)
                            for (f, conv), val in izip(intent_fields, intent_estimate)}
                fields['possible'] = possible
                if possible['search_score'] > 0.8:
                    self.maybe_search_cnt += 1
                if possible['intent_score'] > 0.8:
                    self.maybe_intent_cnt += 1

            except KeyError:
                pass

            try:
                fields['response_text'] = act_record.response['cards'][0]['text']
            except (KeyError, IndexError):
                fields['response_text'] = ''

        return fields

    def too_late_to_continue(self, next_act):
        if (next_act.form_name is not None and
            ('session_start' in next_act.form_name) and
            (next_act.server_time - self.end_time) > explicit_timeout):
            return True

        if (next_act.server_time - self.end_time) > implicit_timeout:
            return True

        return False

    def update_with_next_act(self, next_act):
        self.end_time = next_act.server_time
        self.actions.append(self.to_action(next_act))


def reduce_to_sessions(groups):
    for key, records in groups:
        act_rec = next(records)
        session_rec = SessionRecord(act_rec)

        for act_rec in records:
            if session_rec.too_late_to_continue(act_rec):
                yield session_rec
                session_rec = SessionRecord(act_rec)
            else:
                session_rec.update_with_next_act(act_rec)

        yield session_rec


print 'session_path', session_path

session_records = sample_table.join(lifetime_table,
                                    type='left',
                                    by='uuid',
                                    assume_unique_right=True)
session_records = session_records.groupby("uuid").sort('server_time').reduce(reduce_to_sessions)

out = session_records.put(session_path)

job.run()

