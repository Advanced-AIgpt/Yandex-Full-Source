#!/usr/bin/python
# coding=utf-8
import os
import sys
import codecs
import argparse
import yt.wrapper as yt
import re
import string

yt.config.set_proxy("hahn.yt.yandex.net")

PUNCT = u'([' + string.punctuation + ur'\\])'
PUNCT_REGEX = re.compile(u'([' + string.punctuation + ur'\\])')
REMOVED_PUNCT = u',.?-!…:;' + 'ur\\'
SAVED_PUNCT = set(PUNCT) - set(REMOVED_PUNCT)

def normalize(s):
    s = s.lower()
    s = re.sub(PUNCT_REGEX, ur' \1 ', s).strip()
    s = re.sub(ur'\s+', ' ', s, flags=re.U).strip()
    return s

def get_prev_word_ids(sent, token):
    last_word_id = -1
    prev_word_ids = []
    for x in sent.split(' '):
        if x == token:
            prev_word_ids.append(last_word_id)
        if x not in REMOVED_PUNCT:
            last_word_id += 1
    return prev_word_ids

def ends_with_ellipsis(s):
    period_cnt = 0
    for c in reversed(s):
        if c == ' ':
            continue
        if c != '.':
            break
        period_cnt += 1
    return period_cnt >= 2

def remove_trailing_period(s):
    if ends_with_ellipsis(s):
        return s
    if len(s) > 1 and s.endswith('.'):
        return s[:-1].strip()
    return s

class Selector(object):
    def __init__(self):
        self.dash_start_regex = re.compile(u' (кое|кой)$', flags=re.U)
        self.dash_end_regex = re.compile(u'^(то|либо|нибудь|ка|таки|это) ', flags=re.U)

    def _is_suitable_punctuation(self, reply, reply_corrected):
        if len(reply_corrected) == 0:
            return False

        reply = ' ' + normalize(reply) + ' '
        reply_corrected = ' ' + normalize(reply_corrected) + ' '

        reply_set = set(reply)
        reply_corrected_set = set(reply_corrected)

        if len(SAVED_PUNCT & reply_corrected_set) != 0:
            return False

        if not ((reply_corrected_set ^ reply_set) <= set(',.?-!')):
            return False

        if get_prev_word_ids(reply, '?') != get_prev_word_ids(reply_corrected, '?'):
            return False

        for a in list(re.finditer('-', reply_corrected)):
            if not (self.dash_end_regex.search(reply_corrected[a.end() + 1:]) or \
                    self.dash_start_regex.search(reply_corrected[:a.start() - 1])):
                return False

        # skip if there are neighboring numbers
        is_prev_number = False
        for token in reply.split(' '):
            if token in REMOVED_PUNCT:
                continue
            if token.isdigit():
                if is_prev_number:
                    return False
                is_prev_number = True
            else:
                is_prev_number = False

        return True

    def __call__(self, row):
        key = 'rewritten_reply' if 'rewritten_reply' in row else 'reply'
        reply = unicode(row[key], 'utf-8')

        reply_corrected = unicode(row['punctuated_reply'].strip(), 'utf-8')
        reply_corrected = re.sub(u'…', '. . .', reply_corrected)

        if self._is_suitable_punctuation(reply, reply_corrected):
            row['rewritten_reply'] = remove_trailing_period(reply_corrected)
        else:
            row['rewritten_reply'] = reply

        del row['punctuated_reply']
        yield row

def main(args):
    row_count = yt.get(args.src + '/@row_count')
    rows_per_job = 10000
    job_count = min((row_count + rows_per_job - 1) // rows_per_job, 1000)
    yt.run_map(Selector(), args.src, args.dst, spec={'job_count': job_count})

if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', required=True)
    parser.add_argument('--dst', required=True)
    args = parser.parse_args()
    main(args)
