#!/usr/bin/python
# coding=utf-8
import os
import sys
import re
import codecs
import random
from collections import OrderedDict
from alice.boltalka.tools.reply_rewriter.base_replacer import BaseReplacer

random.seed(123)


def load_regexes(filename):
    dct = OrderedDict()
    with codecs.open(filename, 'r', 'utf-8') as inp:
        for line in inp:
            if not line.strip() or line.startswith('#'):
                continue
            type_, regex, sub = line.rstrip('\r\n').split('\t')
            if type_ == 'exact':
                regex = '^' + regex + '$'
            elif type_ == 'substring':
                regex = '^.*' + regex + '.*$'
            elif type_ == 'phrase':
                regex = '(^| )' + regex + '($| )'
                sub = r'\1' + sub + r'\2'
            else:
                print >> sys.stderr, 'unsupported match type: ' + type_
                sys.exit(1)
            dct[regex] = dct.get(regex, []) + [sub]
    return [(re.compile(regex_el), sub_el) for regex_el, sub_el in dct.iteritems()]


def get_random(list_):
    return list_[random.randint(0, len(list_) - 1)]


def substitute_reply(subs, reply):
    for regex, sub in subs:
        reply = regex.sub(get_random(sub), reply).strip()
        if not reply:
            break
    return reply


class SedReplacer(BaseReplacer):
    def __init__(self, args):
        self.regexes_file = args.sed_rewrites_file
        self.subs = None

    def start(self, local=False):
        if not local:
            self.regexes_file = os.path.basename(self.regexes_file)
        self.subs = load_regexes(self.regexes_file)

    def get_yt_extra_args(self):
        return dict(local_files=[self.regexes_file])

    def process(self, reply):
        return substitute_reply(self.subs, reply)

    @staticmethod
    def register_args(parser):
        parser.add_argument('--sed-rewrites-file', required=True)
