#!/usr/bin/env python
# encoding: utf-8


from nile.api.v1 import clusters, filters
import nile.api.v1.aggregators as aggr
from qb2.api.v1 import filters as qf
from nile.api.v1 import filters as nf
from nile.api.v1.record import Record

date = '2017-10-18'  # '2017-10-13'
prefix = 'c2-e3' #'a0-b1'

intent_path = '//home/voice/yoschi/dialogs/possible_intents'  # unused :o(
dialog_path = '//home/voice/yoschi/dialogs/%s-%s' % (date, prefix)
utterance_path = 'unknown_utterances.tsv'
possible_intents_path = 'possible_intents.tsv'

hahn = clusters.Hahn()

#job = hahn.job()

#dlg_table = job.table(dialog_path)

#intent_table = job.table(intent_path)


# In[5]:


with open(possible_intents_path) as f:
    intent_cache = set(l.split('\t', 1)[0] for l in f)
    #{rec.text for rec in hahn.read(intent_path)}  read from file...
    
del f


def write_unknown_utterances(out_path=utterance_path):
    with open(out_path, 'w') as out:
        for record in hahn.read(dialog_path):
            if record.form_name is None or 'external' not in record.form_name:
                continue

            text = record.utterance_text
            if text is None or text in intent_cache:
                continue

            try:
                out.write(text)
            except:
                print [type(text), text]
                raise
            out.write('\n')

            intent_cache.add(text)


write_unknown_utterances()

#hahn.write(intent_path, iter_intent_records(), append=True)
