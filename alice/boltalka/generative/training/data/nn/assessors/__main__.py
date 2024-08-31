import os

import vh
from alice.boltalka.generative.training.data.nn.util.experimental.ops import map_fn
from alice.boltalka.generative.training.data.nn.util.lib import tokenize_columns, filter_dialogs_by_occurrences, \
    count_occurrences
from alice.boltalka.generative.training.data.nn.util.ops import get_yt_file, train_test_split_by_column, \
    mr_shuffle, mr_mkdir, mr_copy, mr_upload, to_tsv, get_merged_table_from_directory, mr_merge
from alice.boltalka.generative.training.data.nn.util.preprocess import lower_and_punct_separator_lazy


class FilterFrequentAssessors:
    def __init__(self, columns):
        self.columns = columns
        self.banned_phrases = {
            'о чем еще поболтаем ?'
            'вы о чем ?',
            'это не моя тема',
            'о чем поболтаем ?',
            'как скажете'
        }

    def __call__(self, row):
        for column in self.columns:
            if row[column] in self.banned_phrases:
                return
        yield row


def preprocess_assossors(table, columns):
    table = lower_and_punct_separator_lazy(table, columns=columns)
    # table = map_fn_lazy(table, FilterFrequentAssessors(columns=['reply']))
    return table


def merge_contexts_columns_with_separator(row):
    separator = '[SPECIAL_SEPARATOR_TOKEN]'
    context = [row[context_i] for context_i in reversed(['context_{}'.format(i) for i in range(3)]) if
               row[context_i] != '']
    yield {'reply': row['reply'], 'context_0': ' {} '.format(separator).join(context),
           'workerId': row['workerId']}  # TODO


def tokenize_twitter(table, target_folder='//home/voice/artemkorenev/boltalka/twitter_filtered'):
    columns = ['context_{}'.format(i) for i in range(3)] + ['reply']
    table = tokenize_columns(
        table,
        bpe_voc=get_yt_file('//home/voice/artemkorenev/boltalka/bart_lm/bpe.voc'),
        columns_to_tokenize=columns
    )

    table = map_fn(table, merge_contexts_columns_with_separator)

    mr_mkdir(target_folder)

    train, val = train_test_split_by_column(
        table, test_size=0.05, column='workerId'
    )
    train, val = mr_shuffle(train), mr_shuffle(val)
    mr_copy(train, path=os.path.join(target_folder, 'train'))
    mr_copy(val, path=os.path.join(target_folder, 'val'))
    mr_upload(to_tsv(val, columns=['context_0']), dst=os.path.join(target_folder, 'val_context_0.tsv'))
    mr_upload(to_tsv(val, columns=['reply']), dst=os.path.join(target_folder, 'val_reply.tsv'))


def dialog_empty_reply(row):
    if row['reply'] != '':
        yield row


def assessors_mark_mapping(row):
    mapping = {
        '3': 'good',
        '2': 'neutral',
        '1': 'bad'
    }
    mark = row['mark']
    if mark in mapping:
        row['mark'] = mapping[mark]
    yield row


def set_editors_source(row):
    row['source'] = 'editors'
    yield row


def set_assossors_source(row):
    row['source'] = 'assossors'
    yield row


def get_edited_replies(row):
    if row['result_edited']:
        row['reply'] = row['result_edited']
    else:
        row['reply'] = row['result']
    del row['result'], row['result_edited'], row['comment']

    row['rewritten_reply'] = row['reply']
    yield row


def filter_bad_replies(row):
    if row['mark'] != 'bad':
        yield row


def postprocess_assessors(row):
    for k in ['context_2', 'context_1', 'context_0', 'reply']:
        if row[k] == 'EMPTY' or row[k] is None:
            row[k] = ''
            continue
        idx = row[k].find(' [ Открывается ')
        if idx != -1:
            row[k] = row[k][:idx]
            continue
        for skill_list_response in ['Вот, что я могу:',
                                    'Вот, что я умею.',
                                    'Вот игры, в которые можно поиграть.']:
            if row[k].startswith(skill_list_response + '\n- '):
                row[k] = skill_list_response
                break
    yield row


def get_assessors():
    table_editors = get_merged_table_from_directory('//home/voice/dialog/toloka/sandbox/gc_check')
    table_editors = map_fn(table_editors, set_editors_source)

    table_assessors = get_merged_table_from_directory('//home/voice/dialog/yang/gc_check')
    table_assessors = map_fn(table_assessors, assessors_mark_mapping)
    table_assessors = map_fn(table_assessors, set_assossors_source)

    table = mr_merge([table_editors, table_assessors])
    table = map_fn(table, get_edited_replies)
    table = map_fn(table, filter_bad_replies)
    table = map_fn(table, postprocess_assessors)
    return table


if __name__ == '__main__':
    columns = ['context_{}'.format(i) for i in range(3)] + ['reply']
    with vh.Graph() as g:
        table = get_assessors()
        table = preprocess_assossors(table, columns)
        skip_higher_than = 100
        table = filter_dialogs_by_occurrences(
            table,
            occurrences_table=count_occurrences(table, columns=['reply'], skip_threshold=skip_higher_than),
            columns=['reply'], filter_higher_than=skip_higher_than
        )
        table = map_fn(table, dialog_empty_reply)
        tokenize_twitter(table, target_folder='//home/voice/artemkorenev/boltalka/assessors_new')

    info = vh.run(g, quota='dialogs', yt_token_secret='artemkorenev_yt_token', yt_proxy='hahn',
                  yt_pool='voice').get_workflow_info()
