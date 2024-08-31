# -*- coding: utf-8 -*-
import sys
import argparse
import yt.wrapper as yt
import simplejson as json
import regex as re

import string, codecs
from nltk.tokenize.casual import _replace_html_entities


yt.config.set_proxy("hahn.yt.yandex.net")
yt.config["pickling"]["additional_files_to_archive"] = [('/usr/local/lib/python2.7/dist-packages/nltk/internals.pyc', 'nltk/internals.pyc')]


class TwitsPreprocessor(object):
    def __init__(self, args):
        self.replace_html_entities = args.replace_html_entities
        self.skip_rt = args.skip_rt
        self.rt_regex = r'(?<=(^|\s))RT(?=\s)'
        self.replace_yo = args.replace_yo
        self.yo_small_regex = re.compile(u'[ёë]', re.UNICODE)
        self.yo_big_regex = re.compile(u'[ЁË]', re.UNICODE)
        """
        self.strip_nonstandard_chars = args.strip_nonstandard_chars
        self.nonstandard_chars_regex = re.compile(u'[^0-9a-zA-Zа-яА-ЯёëЁË \t'+string.punctuation+']', re.UNICODE)
        self.skip_nonstandard_langs = args.skip_nonstandard_langs
        if self.skip_nonstandard_langs:
            with codecs.open(args.nonstandard_langs_chars_path, 'r', 'utf-8') as f:
                nonstandard_langs_chars = f.read().strip('\n')
            self.nonstandard_langs_regex = re.compile(u'['+nonstandard_langs_chars+']', re.UNICODE)
        self.strip_comments = args.strip_comments
        self.comments_regex = re.compile(ur'[*\\][\w]+[*\\]', re.UNICODE)
        """

    def __call__(self, row):
        dialog = row['value']
        ids, turns = [], []
        for message in dialog.split('\t'):
            parts = message.split(' ')
            ids.append(parts[0])
            turns.append(' '.join(parts[1:]))
        text = '\t'.join(turns)
        """
        if self.skip_nonstandard_langs and self.nonstandard_langs_regex.search(text):
            return
        """
        if self.skip_rt and re.search(self.rt_regex, text):
            return
        if self.replace_html_entities:
            text = _replace_html_entities(text)
        if self.replace_yo:
            text = self.yo_big_regex.sub(u'Е', self.yo_small_regex.sub(u'е', text))
        """
        if self.strip_nonstandard_chars:
            text = self.nonstandard_chars_regex.sub('', text)
        """
        text = re.sub('[^\S\t]+', ' ', text)
        turns = [turn.strip(' ') for turn in text.split('\t')]

        assert len(ids) == len(turns)
        for turn in turns:
            if len(turn) == 0:
                return

        dialog = '\t'.join([ids[i]+' '+turns[i] for i in xrange(len(ids))])
        yield {'key': row['key'], 'subkey': row['subkey'], 'value': dialog}


def main(args):
    yt.run_map(TwitsPreprocessor(args), args.src, args.dst)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', required=True)
    parser.add_argument('--dst', required=True)
    parser.add_argument('--replace-html-entities', action='store_true')
    parser.add_argument('--skip-rt', action='store_true')
    parser.add_argument('--replace-yo', action='store_true')
    """
    parser.add_argument('--skip-nonstandard-langs', action='store_true')
    parser.add_argument('--nonstandard-langs-chars-path')
    parser.add_argument('--strip-nonstandard-chars', action='store_true')
    parser.add_argument('--strip-comments', action='store_true')
    """
    args = parser.parse_args()
    main(args)
