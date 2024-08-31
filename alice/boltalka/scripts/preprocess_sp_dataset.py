#!/usr/bin/python
import sys
import simplejson as json
import re
import codecs
sys.stdin = codecs.getreader('utf-8')(sys.stdin)
sys.stdout = codecs.getwriter('utf-8')(sys.stdout)

def get_dict_name(name):
    return '_' + name.rstrip(':').replace(' ', '_') + '_'

dialog = []
prev_empty = True # True to strip first empty line
for line in sys.stdin:
    d = json.loads(line.rstrip(',\n'))
    for speaker, reply in d['conversation']:
        if None in [speaker, reply]:
            continue
        speaker = speaker.strip()
        reply = re.sub('\[[^]]+\]', '', reply)
        reply = re.sub('\n|\t', ' ', reply)
        reply = reply.strip()
        if speaker == ':':
            speaker = 'Tom' # single case with missing Tom
        if ':' in speaker:
            speaker = speaker[:speaker.index(':')] # several cases like Casey:: or Tuong:"
        if speaker and not ':' in speaker:
            speaker += ':'  # several cases
        if '' in [speaker, reply]:
            if not prev_empty:
                prev_empty = True
                #print
                if dialog:
                    print '\t'.join(dialog)
                dialog = []
            continue
        prev_empty = False
        #print reply
        #print get_dict_name(speaker)
        #print speaker, reply

        dialog.append(get_dict_name(speaker) + ' ' + reply)

if dialog:
    print '\t'.join(dialog)
