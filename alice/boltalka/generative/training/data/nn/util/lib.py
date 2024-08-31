import os

from tokenizer import Tokenizer

import vh
import yt.wrapper as yt
from alice.boltalka.generative.tfnn.preprocess import preprocess_text_and_tokenize
from alice.boltalka.generative.training.data.nn.util import build_contexts_from_dialog


def filter_empty_dialogs(row):
    necessary_fields = ['reply', 'context_merged']
    for field in necessary_fields:
        if row[field] is None or row[field] == '':
            return
    yield row


def filter_empty_replies(row):
    if row['reply'] is None or row['reply'] == '':
        return
    yield row


class ContextsFromDialogsMapper:
    def __init__(self, n_contexts, strict_size=False):
        self.n_contexts = n_contexts
        self.strict_size = strict_size

    def __call__(self, row):
        all_contexts = build_contexts_from_dialog(row['value'], n_contexts=self.n_contexts,
                                                  strict_size=self.strict_size)

        for contexts in all_contexts:
            to_yield = dict(row)

            # dummy values
            for i in range(self.n_contexts):
                to_yield['context_{}'.format(i)] = ''

            # reversed contexts
            for i, ctx in enumerate(contexts):
                to_yield['context_{}'.format(len(contexts) - 1 - i)] = ctx
            yield to_yield


@vh.lazy.hardware_params(vh.HardwareParams(max_ram=1000))
@vh.lazy.from_annotations
def contexts_from_dialogs(input_table: vh.YTTable, n_contexts: int, strict_size: bool = False) -> vh.YTTable:
    out_table = yt.TablePath(yt.create_temp_table())
    yt.run_map(ContextsFromDialogsMapper(n_contexts=n_contexts, strict_size=strict_size), input_table, out_table)
    return out_table


class TokenizerMapper:
    def __init__(self, bpe_voc_path, columns_to_tokenize, skip_preprocessing=True):
        self.bpe_voc_path = bpe_voc_path
        self.columns_to_tokenize = columns_to_tokenize
        self.tokenizer = None
        self.skip_preprocessing = skip_preprocessing

    def start(self):
        self.tokenizer = Tokenizer(self.bpe_voc_path.encode('utf-8'))

    def __call__(self, row):
        for column in self.columns_to_tokenize:
            if row[column] is None:
                continue

            row[column] = preprocess_text_and_tokenize(
                row[column], self.tokenizer, skip_preprocessing=self.skip_preprocessing
            )

        yield row


@vh.lazy.hardware_params(vh.HardwareParams(max_ram=1000))
@vh.lazy.from_annotations
def tokenize_columns(input_table: vh.YTTable, bpe_voc: vh.File,
                     columns_to_tokenize: vh.mkoption(str, nargs='*'),
                     skip_preprocessing: vh.mkoption(bool, default=True),
                     output_table: vh.mkoutput(vh.YTTable)) -> vh.YTTable:
    yt.run_map(
        TokenizerMapper(
            bpe_voc_path=os.path.basename(bpe_voc), columns_to_tokenize=columns_to_tokenize,
            skip_preprocessing=skip_preprocessing
        ),
        input_table, output_table, local_files=[bpe_voc], ordered=True,
    )
    return output_table


class MergeContextsMapper:
    def __init__(self, separator_token, skip_empty_contexts=True):
        self.separator_token = separator_token
        self.skip_empty_contexts = skip_empty_contexts

    def __call__(self, row):
        reply = row['context_0']

        new_context = []
        i = 1
        while True:
            current_context = 'context_{}'.format(i)
            if current_context not in row:
                break

            if not self.skip_empty_contexts or (row[current_context] is not None and row[current_context] != ''):
                new_context.append(row[current_context])

            i += 1

        # reversing contexts so they are in chronological order
        new_context = list(reversed(new_context))

        row['reply'] = reply
        row['context_merged'] = (' {} '.format(self.separator_token)).join(new_context)
        yield row


@vh.lazy.hardware_params(vh.HardwareParams(max_ram=1000))
@vh.lazy.from_annotations
def merge_contexts_and_make_reply(input_table: vh.YTTable, separator_token: str) -> vh.YTTable:
    out_table = yt.TablePath(yt.create_temp_table())
    yt.run_map(MergeContextsMapper(separator_token), input_table, out_table)
    return out_table


class LeftJoinMapper:
    def __init__(self, column):
        self.column = column

    def __call__(self, row):
        row[b'__join_column'] = row[bytes(self.column, encoding='utf-8')]
        yield row


@yt.with_context
class LeftJoinReducer:
    def __init__(self, prefix, except_column):
        self.prefix = prefix
        self.except_column = except_column

    def __call__(self, key, rows, context):
        to_join = None
        for row in rows:
            if context.table_index == 0:
                to_join = row
                del to_join[bytes(self.except_column, encoding='utf-8')]
            else:
                if to_join is not None:
                    for key in to_join:
                        row[bytes(self.prefix, encoding='utf-8') + key] = to_join[key]
                del row[b'__join_column']
                yield row


@vh.lazy.hardware_params(vh.HardwareParams(max_ram=1000))
@vh.lazy.from_annotations
def mr_left_join_lazy(main_table: vh.YTTable, other_table: vh.YTTable,
                      join_by_main: vh.mkinput(str, nargs='+'),
                      join_by_other: vh.mkinput(str, nargs='+'),
                      join_prefixes: vh.mkinput(str, nargs='+')) -> vh.YTTable:
    assert len(join_by_main) == len(join_by_other) == len(join_prefixes)

    out_table = yt.TablePath(yt.create_temp_table())

    tmp_main_table = yt.TablePath(yt.create_temp_table())
    tmp_other_table = yt.TablePath(yt.create_temp_table())

    for main_column, other_column, prefix in zip(join_by_main, join_by_other, join_prefixes):
        tracker = yt.OperationsTracker()
        tracker.add(yt.run_map(
            LeftJoinMapper(main_column), main_table, tmp_main_table, sync=False, format=yt.YsonFormat(encoding=None)
        ))
        tracker.add(yt.run_map(
            LeftJoinMapper(other_column), other_table, tmp_other_table, sync=False, format=yt.YsonFormat(encoding=None)
        ))
        tracker.wait_all()

        tracker = yt.OperationsTracker()
        tracker.add(yt.run_sort(tmp_main_table, sort_by=['__join_column'], sync=False))
        tracker.add(yt.run_sort(tmp_other_table, sort_by=['__join_column'], sync=False))
        tracker.wait_all()

        yt.run_reduce(
            LeftJoinReducer(prefix, except_column=other_column), [tmp_other_table, tmp_main_table], destination_table=out_table,
            reduce_by=['__join_column'], format=yt.YsonFormat(encoding=None)
        )

        main_table = out_table

    return out_table


class FlattenizeMapper:
    def __init__(self, columns_from, column_to, skip_nones=False):
        self.columns_from = columns_from
        self.column_to = column_to
        self.skip_nones = skip_nones

    def __call__(self, row):
        yield from ({self.column_to: row[column]} for column in self.columns_from
                    if not self.skip_nones or row[column] is not None)


@vh.lazy.hardware_params(vh.HardwareParams(max_ram=1000))
@vh.lazy.from_annotations
def count_occurrences(input_table: vh.YTTable, columns: vh.mkoption(str, nargs='+'),
                      skip_nones: vh.mkoption(bool, default=False),
                      skip_threshold: vh.mkoption(int, default=-1)) -> vh.YTTable:
    out_table = yt.TablePath(yt.create_temp_table())
    column_to_count = 'key'

    def count_reducer(key, rows):
        to_yield = dict(key)
        to_yield['occurrences'] = sum(1 for _ in rows)

        if skip_threshold == -1 or to_yield['occurrences'] > skip_threshold:
            yield to_yield

    yt.run_map_reduce(
        mapper=FlattenizeMapper(columns_from=columns, column_to=column_to_count, skip_nones=skip_nones),
        reducer=count_reducer,
        reduce_by=[column_to_count],
        source_table=input_table,
        destination_table=out_table
    )
    return out_table


@vh.lazy.hardware_params(vh.HardwareParams(max_ram=1000))
@vh.lazy.from_annotations
def filter_dialogs_by_occurrences(input_table: vh.YTTable,
                                  occurrences_table: vh.YTTable,
                                  columns: vh.mkoption(str, nargs='+'),
                                  filter_higher_than: vh.mkoption(int)) -> vh.YTTable:
    out_table = yt.TablePath(yt.create_temp_table())

    occurrence_map = dict()
    for row in yt.read_table(occurrences_table):
        occurrence_map[row['key']] = row['occurrences']

    def mapper(row):
        for column in columns:
            if row[column] in occurrence_map:
                row[column] = ''
        yield row

    yt.run_map(mapper, input_table, out_table)
    return out_table
