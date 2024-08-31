import argparse
import hashlib

import vh
import yt.wrapper as yt
from alice.boltalka.tools.dssm_preprocessing.preprocessing.lib.main import EmoticonsRemover, TextNormalizer, \
    TwitterCleaner

yt.config.set_proxy("hahn.yt.yandex.net")


class Dialogues_builder(object):
    def __init__(self, context_length):
        self.context_length = context_length

    def __call__(self, row):
        turns = [''] * self.context_length + row['value'].split('\t') + ['']
        langs = [''] * self.context_length + row['langs'].split('\t') + [''] if 'langs' in row else None
        num_turns = len(turns) - self.context_length
        for i in range(num_turns):
            dialogue = '\t'.join(turns[i:i + self.context_length + 1])
            hashcode = hashlib.md5(dialogue.encode('utf-8')).hexdigest()
            row.update(
                {'value': dialogue, 'depth': i, 'height': num_turns - i - 2, 'inv_depth': -i, 'hashcode': hashcode})
            if langs is not None:
                row['langs'] = '\t'.join(langs[i:i + self.context_length + 1])
            yield row


def parse_dialogue(dialogue):
    user_ids, texts = [], []
    for turn in dialogue.split('\t'):
        if turn == '':
            user_ids.append(None)
            texts.append(None)
        else:
            parts = turn.split(' ')
            user_ids.append(parts[0])
            texts.append(' '.join(parts[1:]))
    dct = {'reply': texts[-1], 'user': user_ids[-1]}
    context_length = len(texts) - 1
    for i in range(context_length):
        j = context_length - i - 1
        dct['context_%d' % j] = texts[i]
        dct['user_%d' % j] = user_ids[i]
    return dct


def remove_duplicates(key, rows):
    row = next(rows)
    dct_upd = parse_dialogue(row['value'])
    del row['value'], row['inv_depth']
    row.update(dct_upd)
    yield row


class AddReduceKey(object):
    def __init__(self, columns):
        self.columns = columns

    def __call__(self, row):
        key = '\t'.join(str(row[k]) for k in self.columns)
        row['reduce_key'] = hashlib.md5(key.encode('utf-8')).hexdigest()
        yield row


def add_width(key, rows):
    rows = list(rows)
    if rows[0]['depth'] == -1:
        assert len(rows) == 1
        width = 0
    else:
        width = len(rows)
    for row in rows:
        row['width'] = width
        del row['reduce_key']
        yield row


def main(src, dst, context_length):
    yt.run_map_reduce(Dialogues_builder(context_length), remove_duplicates, src, dst,
                      reduce_by=['hashcode'], sort_by=['hashcode', 'inv_depth', 'key'])
    reduce_cols = ['key', 'depth']
    for i in range(context_length):
        reduce_cols.extend(['context_' + str(i), 'user_' + str(i)])
    yt.run_map_reduce(AddReduceKey(reduce_cols), add_width, dst, dst, reduce_by='reduce_key')
    yt.run_sort(dst, sort_by='key')


@vh.lazy.hardware_params(vh.HardwareParams(max_ram=1000))
@vh.lazy.from_annotations
def build_dialogs_lazy(
        input_table: vh.YTTable, context_length: int,
) -> vh.YTTable:
    out_table = yt.TablePath(yt.create_temp_table())
    main(str(input_table), str(out_table), context_length)
    return out_table


class EmoticonsRemoverMapper:
    def __init__(self, columns):
        self.remover = EmoticonsRemover()
        self.columns = columns

    def __call__(self, row):
        for column in self.columns:
            row[column] = self.remover(row[column])
        yield row


@vh.lazy.hardware_params(vh.HardwareParams(max_ram=1000))
@vh.lazy.from_annotations
def remove_emoticons_lazy(input_table: vh.YTTable, columns: vh.mkinput(str, nargs='*')) -> vh.YTTable:
    out_table = yt.TablePath(yt.create_temp_table())
    yt.run_map(EmoticonsRemoverMapper(columns), input_table, out_table)
    return out_table


class TwitterTextNormalizerMapper:
    def __init__(self, columns):
        parser = argparse.ArgumentParser()
        TextNormalizer.add_args(parser)
        args = parser.parse_args([
            '--strip-bad-chars',
            '--replace-yo',
            '--strip-ext',
            '--skip-140',
            '--skip-numeric-many'
        ])
        self.normalizer = TextNormalizer(args)
        self.columns = columns

    def __call__(self, row):
        for column in self.columns:
            processed = self.normalizer(row[column])
            if processed is None:
                processed = ''
            row[column] = processed
        yield row


@vh.lazy.hardware_params(vh.HardwareParams(max_ram=1000))
@vh.lazy.from_annotations
def twitter_text_normalizer_lazy(input_table: vh.YTTable, columns: vh.mkinput(str, nargs='*')) -> vh.YTTable:
    out_table = yt.TablePath(yt.create_temp_table())
    yt.run_map(TwitterTextNormalizerMapper(columns), input_table, out_table)
    return out_table


class TwitterTextCleanerMapper:
    def __init__(self, columns):
        parser = argparse.ArgumentParser()
        TwitterCleaner.add_args(parser)
        args = parser.parse_args([
            '--skip-rt',
            '--skip-url',
            '--skip-hashtag',
            '--skip-new-post',
            '--skip-at',
            '--replace-html-entities'
        ])
        self.cleaner = TwitterCleaner(args)
        self.columns = columns

    def __call__(self, row):
        for column in self.columns:
            processed = self.cleaner(row[column])
            if processed is None:
                processed = ''
            row[column] = processed
        yield row


@vh.lazy.hardware_params(vh.HardwareParams(max_ram=1000))
@vh.lazy.from_annotations
def twitter_text_cleaner_lazy(input_table: vh.YTTable, columns: vh.mkinput(str, nargs='*')) -> vh.YTTable:
    out_table = yt.TablePath(yt.create_temp_table())
    yt.run_map(TwitterTextCleanerMapper(columns), input_table, out_table)
    return out_table
