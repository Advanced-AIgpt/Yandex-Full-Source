#!/usr/bin/python
# coding=utf-8
import sys
import codecs
sys.stdin = codecs.getreader('utf-8')(sys.stdin)
sys.stdout = codecs.getwriter('utf-8')(sys.stdout)

def output_pairs(contexts, replies):
    for context in contexts:
        for reply in replies:
            print context + '\t' + reply

is_context = False
is_reply = False
contexts = []
replies = []
for line in sys.stdin:
    line = line.strip()
    if line == 'nlu:':
        if contexts:
            assert replies
            output_pairs(contexts, replies)
            contexts = []
            replies = []
        is_reply = False
        is_context = True
        continue
    if line == 'nlg:':
        assert is_context
        is_context = False
        is_reply = True
        continue
    if not line.strip().startswith('-'):
        continue
    line = line.lstrip('- ').strip(u"’' ")
    if not line:
        continue
    if is_context:
        contexts.append(line)
    if is_reply:
        replies.append(line)

if contexts:
    assert replies
    output_pairs(contexts, replies)
