#!/usr/bin/python
import argparse
import yt.wrapper as yt
import re
import string

from normalize_twitter_text import TwitNormalizer, separate_punctuation, BAD_CHAR_REGEX
from remove_emoticons import EmoticonsRemover

yt.config.set_proxy("hahn.yt.yandex.net")

_RT_REGEX = re.compile(ur'(?:^|[^a-z])rt[^a-z]|[^a-z]rt(?:$|[^a-z])')
_140_REGEX = re.compile(ur'(?:^|[^0-9])140[^0-9]|[^0-9]140(?:$|[^0-9])')
_BAD_CHAR_REGEX = BAD_CHAR_REGEX
#_normalizer = TwitNormalizer(strip_nonstandard_chars=True)
_emoji_remover = EmoticonsRemover(strip_nonstandard_chars=True)

def is_valid(context):
    non_empty_suffix = 0
    was_empty = False
    for i in xrange(len(context)):
        id_, turn = context[-i - 1].split(' ', 1)
        if not turn:
            was_empty = True
            continue
        if not was_empty:
            non_empty_suffix += 1
        else:
            return False # do not allow empty turns in the middle
    return non_empty_suffix > 1 # at least (context_0, reply) should not be empty

class GenerateDialoguesMapper(object):
    def __init__(self, context_length):
        self.context_length = context_length

    def start(self):
        pass

    def __call__(self, row):
        dialog = unicode(row['value'], 'utf-8')

        if _RT_REGEX.search(dialog.lower()):
            return

        context = [' '] * (self.context_length + 1)
        for line in dialog.split('\t'):
            id_, original_turn = line.strip().split(' ', 1)
            #turn = _normalizer(original_turn)
            turn = _emoji_remover(original_turn, iterative=True)
            turn = separate_punctuation(turn).lower()
            context.pop(0)
            context.append(id_ + ' ' + turn)

            if _BAD_CHAR_REGEX.search(original_turn):
                continue
            if _140_REGEX.search(turn):
                continue
            if len(re.findall('[0-9]', turn)) > 3:
                continue
            if not is_valid(context):
                continue

            yield {
                'key': row['key'],
                'value': '\t'.join(context),
            }

        """
        turns = ['']*2 + [_normalizer(s) for s in dialog.split('\t')]
        for i in xrange(len(turns)-3):
            if not turns[i + 3] or _140_REGEX.search(turns[i + 3]):
                continue
            yield {'key': row['key'], 'value': '\t'.join(turns[i:i+4])}
        """

class MakeUniqueReducer(object):
    def __init__(self, context_length):
        self.context_length = context_length

    def start(self):
        pass

    def __call__(self, key, row):
        for dialog in {row['value'] for row in rows}:
            ids, turns = [], []
            context_keys = []
            id_keys = []
            for i, message in enumerate(dialog.split('\t')):
                id_, turn = message.split(' ', 1)
                ids.append(id_)
                turns.append(turn)
                id_keys.append('user' if i == self.context_length else 'user_' + str(self.context_length - i - 1))
                context_keys.append('reply' if i == self.context_length else 'context_' + str(self.context_length - i - 1))
            yield dict(zip(['key'] + context_keys + id_keys, [str(key['key'])] + turns + ids))

def main(args):
    yt.run_map_reduce(GenerateDialoguesMapper(args.context_length), MakeUniqueReducer(args.context_length), args.src, args.dst, reduce_by=['key'], spec={'data_size_per_map_job': 30 * 2**20})


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', required=True)
    parser.add_argument('--dst', required=True)
    parser.add_argument('--context-length', type=int, default=3)
    args = parser.parse_args()
    main(args)
