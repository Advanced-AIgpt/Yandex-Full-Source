#!/usr/bin/python
import sys
import codecs
sys.stdin = codecs.getreader('utf-8')(sys.stdin)
sys.stdout = codecs.getwriter('utf-8')(sys.stdout)

for line in sys.stdin:
    context, reply, context_speakers, reply_speakers = line.rstrip().split('\t')
    target_speaker = reply_speakers.split(' ', 1)[0]
    context_speakers = context_speakers.split()
    for i in xrange(len(context_speakers)):
        if context_speakers[i] != target_speaker:
            context_speakers[i] = '_USER_'
    print '\t'.join([context, reply, ' '.join(context_speakers), reply_speakers])
