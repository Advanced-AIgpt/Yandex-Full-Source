import re
import yt.wrapper as yt
import argparse
import random
from itertools import zip_longest

from alice.boltalka.tools.dssm_preprocessing.preprocessing.lib.main import TwitterCleaner, TextNormalizer


SEED = 57
MIN_CONTEXT_LEN = 6
MAX_CONTEXT_LEN = 9
NEG_NUM = 1


def reverse(context):
    return "reverse", list(reversed(context))


def dropout(context):
    dropout = random.randint(1, len(context) - 2)
    return f"dropout{dropout}", context[:dropout] + context[dropout + 1:]


def replace(context, neg_type, negatives):
    idx = random.randint(0, len(context) - 1)
    utterance = random.choice(negatives)
    context = context[:]
    context[idx] = utterance
    return f"replace{idx}{neg_type}", context


def swap(context):
    choices = list(range(len(context)))
    a = random.choice(choices)
    b = random.choice([i for i in choices if i != a])
    context = context[:]
    context[a], context[b] = context[b], context[a]
    return f"swap{a}w{b}", context


def sample_negative(context, tree_negatives, easy_negatives):
    if random.random() < 0.25:
        return reverse(context)
    if len(context) > MIN_CONTEXT_LEN and random.random() < 0.33:
        return dropout(context)
    if random.random() < 0.5:
        return swap(context)
    if tree_negatives and random.random() < 0.7:
        return replace(context, 'tree', tree_negatives)
    return replace(context, 'easy', easy_negatives)


class SessionFilter:
    def __init__(self):
        self.twitter_cleaner = TwitterCleaner(argparse.Namespace(
            skip_rt=True,
            skip_url=True,
            skip_hashtag=True,
            skip_new_post=True,
            strip_mention=True,
            skip_at=False,
            replace_html_entities=True
        ))
        self.normalizer = TextNormalizer(argparse.Namespace(
            lang='ru',
            strip_bad_chars=True,
            separate_punctuation=True,
            remove_punctuation=False,
            lowercase=True,
            uppercase_first=False,
            replace_yo=True,
            strip_ext=False,
            skip_140=False,
            skip_numeric_many=False
        ))

    def __call__(self, row):
        session = row['value']
        langs = row['langs']
        key = row['key']
        if langs:
            langs = langs.split('\t')
        cur_session = []
        prev_user = None

        def get_row(session):
            return dict(key=hash(key), session='\n'.join(session), head=session[0])

        for i, turn in enumerate(session.split('\t')):
            user, turn = turn.split(' ', 1)
            turn = self.twitter_cleaner(turn) or ''
            turn = self.normalizer(turn)
            bad_turn = not turn or (langs and langs[i] != 'ru')
            if user == prev_user or bad_turn:
                if len(cur_session) >= MIN_CONTEXT_LEN:
                    yield get_row(cur_session)
                cur_session = []
            if not bad_turn:
                cur_session.append(turn)
            prev_user = user
        if len(cur_session) >= MIN_CONTEXT_LEN:
            yield get_row(cur_session)


class PrefixSuppressor:
    def __call__(self, key, rows):
        # expexts reduce by first turn, sort by session
        prev_session = None
        prev_row = None
        for row in rows:
            cur_session = row['session'].split('\n')
            if prev_session is not None:
                for i in range(len(prev_session)):
                    if i >= len(cur_session) or cur_session[i] != prev_session[i]:
                        yield prev_row
                        break
            prev_session = cur_session
            prev_row = row
        if prev_session:
            yield prev_row


@yt.with_context
class PairIdAssigner(object):
    def __call__(self, row, context):
        row['batch_idx'] = int(context.row_index) // 2
        yield row


SENTENCE_REGEXP = re.compile(r'((?:[?!.]+\s*)+)')


def default_splitter(row):
    return row['session'].split('\n')


def megabart_splitter(row):
    session = row['text']
    if '[SEP]' in session:
        return [el.strip() for el in session.split('[SEP]')]
    else:
        session = session.lower()
        sentences = SENTENCE_REGEXP.split(session)
        return [a.strip() + b.strip() for a, b in zip_longest(sentences[::2], sentences[1::2], fillvalue='') if a]


SPLITTERS = dict(
    default=default_splitter,
    megabart=megabart_splitter
)


class NegativesSampler(object):
    def __init__(self, splitter, full_sessions):
        self.splitter = splitter
        self.full_sessions = full_sessions

    def process(self, session, easy_negatives, begin, end):
        context = session[begin:end]
        yield dict(Text=context, sample_type='pos', target=1)
        tree_negatives = session[:begin] + session[end:]
        for neg_idx in range(NEG_NUM):
            sample_type, neg_context = sample_negative(context, tree_negatives, easy_negatives)
            yield dict(Text=neg_context, sample_type=sample_type, target=0)

    def __call__(self, key, rows):
        pair_idx = key['batch_idx']
        rows = list(rows)
        if len(rows) != 2:
            return
        rows = [SPLITTERS[self.splitter](el) for el in rows]
        random.seed(SEED + pair_idx)
        for session, easy_negatives in [rows, reversed(rows)]:
            if self.full_sessions:
                yield from self.process(session, easy_negatives, 0, len(session))
            else:
                if len(session) < MIN_CONTEXT_LEN:
                    continue
                for end in range(MIN_CONTEXT_LEN, len(session)):
                    begin = max(0, end - MAX_CONTEXT_LEN)
                    yield from self.process(session, easy_negatives, begin, end)


def run(args):
    if args.mode == 'preprocess':
        yt.run_map_reduce(SessionFilter(), PrefixSuppressor(), args.src, args.dst, reduce_by='head', sort_by=('head', 'session'))
    elif args.mode == 'generate':
        yt.run_map_reduce(PairIdAssigner(), NegativesSampler(args.splitter, args.full_sessions), args.src, args.dst, reduce_by='batch_idx',
                          spec={'map_job_io': {'control_attributes': {'enable_row_index': True}}})


def main():
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('mode')
    parser.add_argument('--src', required=True)
    parser.add_argument('--dst', required=True)
    parser.add_argument('--splitter', default='default', choices=['default', 'megabart'])
    parser.add_argument('--full-sessions', action='store_true')
    args = parser.parse_args()
    run(args)
