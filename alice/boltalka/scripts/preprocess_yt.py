# -*- coding: utf-8 -*-
import yt.wrapper as yt
import argparse, sys, string, re, codecs
import simplejson as json


yt.config.set_proxy("hahn.yt.yandex.net")


class Preprocessor(object):
    def __init__(self, args):
        self.lowercase = args.lowercase
        self.tokenize = args.tokenize
        self.tokenize_regex = re.compile(u'(['+string.punctuation+'])', re.UNICODE)

    def __call__(self, row):
        dialog = row['value']
        ids, turns = [], []
        for message in dialog.split('\t'):
            parts = message.split(' ')
            ids.append(parts[0])
            turns.append(' '.join(parts[1:]))
        text = '\t'.join(turns).decode('utf-8')
        if self.tokenize:
            text = self.tokenize_regex.sub(ur' \1 ', text)
        if self.lowercase:
            text = text.lower()


        text = re.sub('[^\S\t]+', ' ', text)

        turns = []
        for turn in text.split('\t'):
            turn = turn.strip(' ')
            if len(turn) == 0:
                return
            turns.append(turn)

        assert len(ids) == len(turns)

        dialog = '\t'.join([ids[i]+' '+turns[i] for i in xrange(len(ids))])

        row['value'] = dialog
        yield row


def main(args):
    yt.run_map(Preprocessor(args), args.src, args.dst)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', required=True)
    parser.add_argument('--dst', required=True)
    parser.add_argument('--lowercase', action='store_true')
    parser.add_argument('--tokenize', action='store_true')
    args = parser.parse_args()
    main(args)
