import os

import vh
from alice.boltalka.generative.training.data.nn.filtered_twitter.mappers import FilterContextsByDicts, \
    FilterContextsByTokenNumber
from alice.boltalka.generative.training.data.nn.filtered_twitter.ops import build_dialogs_lazy, remove_emoticons_lazy, \
    twitter_text_normalizer_lazy, twitter_text_cleaner_lazy

from alice.boltalka.generative.training.data.nn.util.experimental.ops import map_fn, map_fn_lazy
from alice.boltalka.generative.training.data.nn.util.lib import tokenize_columns, mr_left_join_lazy, \
    filter_dialogs_by_occurrences, count_occurrences
from alice.boltalka.generative.training.data.nn.util.ops import get_yt_table, mr_merge, mr_map_exec_with_options, \
    yt_grep, mr_sort, yt_reduce, mr_left_join, get_yt_file, train_test_split_by_column, mr_shuffle, mr_mkdir, mr_copy, \
    mr_upload, to_tsv, svn_export_single_file
from alice.boltalka.generative.training.data.nn.util.preprocess import lower_and_punct_separator_lazy


def old_data_source_mapper(row):
    row['data_source'] = 'old'
    yield row


def twitter_to_contexts():
    table = mr_merge([
        get_yt_table('//home/misspell/deemonasd/alisa/boltalka/twitter/watson_v2/2019-05-15'),
        get_yt_table('//home/voice/krom/watson_2kkk_26042020/raw')
    ])
    arcadia_revision = 6747674
    table = mr_map_exec_with_options(
        input_table=table,
        executable={
            'arcadia_revision': arcadia_revision,
            'targets': 'alice/boltalka/scripts/twitter_dataset_build/watson/parse_raw_watson',
            'arts': 'alice/boltalka/scripts/twitter_dataset_build/watson/parse_raw_watson/parse_raw_watson'
        },
        options=['--tas-format'],
        max_ram=1000,
        max_disk=20000
    )
    build_dialogs = lambda table: build_dialogs_lazy(table, 9)
    table = build_dialogs(table)
    table = mr_map_exec_with_options(
        input_table=table,
        executable={
            'arcadia_revision': 6681944,
            'targets': 'alice/boltalka/scripts/twitter_dataset_build/watson/filter_watson_specificity',
            'arts': 'alice/boltalka/scripts/twitter_dataset_build/watson/filter_watson_specificity/filter_watson_specificity'
        },
        options=['--columns reply,context_0,context_1,context_2,context_3,context_4,context_5,context_6,context_7,context_8'],
        max_ram=1000,
        max_disk=20000
    )
    table = yt_grep(
        table,
        grep_expr='any(l == "ru" for l in row["langs"].split("\t"))',
        start_expr=None,
        preprocess_expr='row["data_source"] = "watson"'
    )

    table2 = build_dialogs(get_yt_table('//home/voice/dialog/twitter/twitter_all_sequences'))
    table2 = map_fn(table2, old_data_source_mapper)

    table = mr_sort(
        srcs=[table, table2],
        sort_by=['hashcode']
    )
    table = yt_reduce(
        table,
        reduce_by='hashcode',
        reduce_expr='if not result:\n\tresult.append(row)'
    )
    return table


def leave_only_contexts_reply_and_key_mapper(row):
    columns = ['context_{}'.format(i) for i in range(9)] + ['reply', 'key']
    to_yield = dict()
    for column in columns:
        to_yield[column] = row[column]
    yield to_yield


def none_to_empty_str_mapper(row):
    columns = ['context_{}'.format(i) for i in range(9)] + ['reply']
    for column in columns:
        row[column] = '' if row[column] is None else row[column]
    yield row


def basic_char_preprocessing(row):
    columns = ['context_{}'.format(i) for i in range(9)] + ['reply']
    for column in columns:
        value = row[column].decode('utf-8')
        for char_from, char_to in [
            (u'«', u'"'),
            (u'»', u'"'),
            (u'»', u'"'),
            (u'–', u'-'),
            (u'—', u'-'),
            (u'ё', u'е'),
            (u'Ё', u'Е'),
            (u'…', u'...')
        ]:
            value = value.replace(char_from, char_to)
        row[column] = value
    yield row


def remove_trailing_twitter_mentions_mapper(row):
    import re
    columns = ['context_{}'.format(i) for i in range(9)] + ['reply']
    for column in columns:
        value = row[column].decode('utf-8')
        value = re.sub(u'^@[A-Za-z_0-9]+ *', u'', value)
        row[column] = value
    yield row


def skip_twitter_mentions_mapper(row):
    def rule(value):
        return u'@' in value

    columns = ['reply'] + ['context_{}'.format(i) for i in range(9)]
    found_violation = False
    for column in columns:
        value = row[column].decode('utf-8')

        if value == '':
            continue

        if found_violation or rule(value):
            row[column] = ''
            found_violation = True

    # if sum([len(row[column]) for column in columns]) != 0:
    if sum([row[column] != '' for column in columns]) >= 1:
        yield row


def filter_urls_mapper(row):
    def rule(value):
        return 'http' in value or 'ftp' in value

    columns = ['reply'] + ['context_{}'.format(i) for i in range(9)]
    found_violation = False
    for column in columns:
        value = row[column].decode('utf-8')

        if value == '':
            continue

        if found_violation or rule(value):
            row[column] = ''
            found_violation = True

    # if sum([len(row[column]) for column in columns]) != 0:
    if sum([row[column] != '' for column in columns]) >= 1:
        yield row


def truncate_multiple_symbols_mapper(row):
    import re
    columns = ['reply'] + ['context_{}'.format(i) for i in range(9)]

    for column in columns:
        value = row[column].decode('utf-8')
        value = re.sub(u'(\\)+0*)+', ')', value)  # )))00)0)0))000 -> )
        value = re.sub(u'(\\(+9*)+', '(', value)  # (((9(((9(((999 -> (
        value = re.sub(u'(\\?!|!\\?)[!?]*', '?!', value)  # ?!!?!?!?!?!?!??!?! -> ?!
        row[column] = value
    yield row


def filter_short_and_long_mapper(row):
    def rule(value):
        return len(value) < 5 or len(value) > 2048

    columns = ['reply'] + ['context_{}'.format(i) for i in range(9)]
    found_violation = False
    for column in columns:
        value = row[column].decode('utf-8')

        if value == '':
            continue

        if found_violation or rule(value):
            row[column] = ''
            found_violation = True

    # if sum([len(row[column]) for column in columns]) != 0:
    if sum([row[column] != '' for column in columns]) >= 1:
        yield row


def filter_too_much_caps_mapper(row):
    def rule(value):
        import re
        n_capitalized = len(re.findall(u'[A-ZА-Я]', value))
        return float(n_capitalized) / len(value) > 0.2

    columns = ['reply'] + ['context_{}'.format(i) for i in range(9)]
    found_violation = False
    for column in columns:
        value = row[column].decode('utf-8')

        if value == '':
            continue

        if found_violation or rule(value):
            row[column] = ''
            found_violation = True

    # if sum([len(row[column]) for column in columns]) != 0:
    if sum([row[column] != '' for column in columns]) >= 1:
        yield row


def filter_not_punct_or_eng_and_rus_characters_mapper(row):
    def rule(value):
        import re
        n_allowed = len(re.findall(u'[а-яА-Яa-zA-Z!"#%(),\-.\\/:;? 0-9]', value))
        return float(n_allowed) != len(value)

    columns = ['reply'] + ['context_{}'.format(i) for i in range(9)]
    found_violation = False
    for column in columns:
        value = row[column].decode('utf-8')

        if value == '':
            continue

        if found_violation or rule(value):
            row[column] = ''
            found_violation = True

    # if sum([len(row[column]) for column in columns]) != 0:
    if sum([row[column] != '' for column in columns]) >= 1:
        yield row


def filter_too_low_russian_characters_rate_mapper(row):
    def rule(value):
        import re
        value = value.replace(' ', '')
        n_russian_chars = len(re.findall(u'[А-Яа-я]', value))
        return float(n_russian_chars) / len(value) < 0.7

    columns = ['reply'] + ['context_{}'.format(i) for i in range(9)]
    found_violation = False
    for column in columns:
        value = row[column].decode('utf-8')

        if value == '':
            continue

        if found_violation or rule(value):
            row[column] = ''
            found_violation = True

    # if sum([len(row[column]) for column in columns]) != 0:
    if sum([row[column] != '' for column in columns]) >= 1:
        yield row


def filter_starting_not_with_russian_character_mapper(row):
    def rule(value):
        import re
        return re.match(u'^[А-Яа-я].*', value) is None

    columns = ['reply'] + ['context_{}'.format(i) for i in range(9)]
    found_violation = False
    for column in columns:
        value = row[column].decode('utf-8')

        if value == '':
            continue

        if found_violation or rule(value):
            row[column] = ''
            found_violation = True

    # if sum([len(row[column]) for column in columns]) != 0:
    if sum([row[column] != '' for column in columns]) >= 1:
        yield row


# can there be the situation when same dialogs spawned? like (ctx0, reply) and  (ctx0, ctx1, reply) where ctx1 became empty during training? TODO
def truncate_dialogs_with_holes_mapper(row):
    columns = ['reply'] + ['context_{}'.format(i) for i in range(9)]
    found_hole = False
    for column in columns:
        if row[column] == '':
            found_hole = True

        if found_hole:
            row[column] = ''
    yield row


def dialog_minlen_filtering_mapper(row):
    if row['context_0'] != '' and row['reply'] != '':
        yield row


def remove_hashtags_mapper(row):
    import re
    columns = ['reply'] + ['context_{}'.format(i) for i in range(9)]
    for column in columns:
        value = row[column].decode('utf-8')
        value = re.sub(u'#.*', ' ', value)
        value = re.sub(u' {2,}', ' ', value)
        row[column] = value
    yield row


def preprocessing_mapper(row):
    def punct_separate(text):
        import string
        # space out punctuations
        for c in string.punctuation:
            text = text.replace(c, ' {} '.format(c))
        text = text.replace('  ', ' ')
        return text

    columns = ['context_{}'.format(i) for i in range(9)] + ['reply']
    for column in columns:
        text = row[column]
        text = text.lower()
        text = punct_separate(text)
        row[column] = text
    yield row


def merge_contexts_columns_with_separator(row):
    separator = '[SPECIAL_SEPARATOR_TOKEN]'
    context = [row[context_i] for context_i in reversed(['context_{}'.format(i) for i in range(9)]) if row[context_i] != '']
    yield {'reply': row['reply'], 'context_0': ' {} '.format(separator).join(context), 'key': row['key']}


def filtering_and_preprocess_twitter(table):
    columns = ['context_{}'.format(i) for i in range(9)] + ['reply']

    table = map_fn(table, leave_only_contexts_reply_and_key_mapper)
    table = map_fn(table, none_to_empty_str_mapper)
    table = map_fn(table, basic_char_preprocessing)
    table = map_fn(table, dialog_minlen_filtering_mapper)  # TODO
    table = map_fn(table, remove_trailing_twitter_mentions_mapper)
    # table = map_fn(table, skip_twitter_mentions_mapper)
    # table = map_fn(table, filter_urls_mapper)
    table = map_fn(table, truncate_multiple_symbols_mapper)
    table = twitter_text_normalizer_lazy(table, columns=columns)
    table = twitter_text_cleaner_lazy(table, columns=columns)
    table = remove_emoticons_lazy(table, columns=columns)
    table = map_fn(table, filter_short_and_long_mapper)
    table = map_fn(table, filter_too_much_caps_mapper)
    table = map_fn(table, filter_not_punct_or_eng_and_rus_characters_mapper)
    table = map_fn(table, filter_too_low_russian_characters_rate_mapper)
    table = map_fn(table, filter_starting_not_with_russian_character_mapper)

    table = map_fn(table, truncate_dialogs_with_holes_mapper)
    # table = map_fn(table, remove_hashtags_mapper)
    table = map_fn(table, dialog_minlen_filtering_mapper)
    # table = map_fn(table, preprocessing_mapper)
    table = lower_and_punct_separator_lazy(table, columns=columns)

    return table


def tokenize_twitter(table, target_folder='//home/voice/artemkorenev/boltalka/twitter_filtered', min_tokens=None):
    columns = ['context_{}'.format(i) for i in range(9)] + ['reply']
    table = tokenize_columns(
        table,
        bpe_voc=get_yt_file('//home/voice/artemkorenev/boltalka/bart_lm/bpe.voc'),
        columns_to_tokenize=columns
    )

    if min_tokens is not None:
        table = map_fn_lazy(table, FilterContextsByTokenNumber(columns, min_n_tokens=2))
        table = map_fn(table, truncate_dialogs_with_holes_mapper)
        table = map_fn(table, dialog_minlen_filtering_mapper)

    table = map_fn(table, merge_contexts_columns_with_separator)

    mr_mkdir(target_folder)

    train, val = train_test_split_by_column(
        table, test_size=0.0003, column='key'
    )
    train, val = mr_shuffle(train), mr_shuffle(val)
    mr_copy(train, path=os.path.join(target_folder, 'train'))
    mr_copy(val, path=os.path.join(target_folder, 'val'))
    mr_upload(to_tsv(val, columns=['context_0']), dst=os.path.join(target_folder, 'val_context_0.tsv'))
    mr_upload(to_tsv(val, columns=['reply']), dst=os.path.join(target_folder, 'val_reply.tsv'))


def create_allowed_dict():
    return vh.op(id='d63ba405-96d1-4ec8-b7c8-70a7a5c01d8c', name='Build NLG Rus Lister Dict')(
        bad_dict=svn_export_single_file('arcadia/alice/boltalka/filters/bad_dict.txt', return_type='tsv'),
        bad_rus=svn_export_single_file('arcadia/alice/boltalka/filters/bad_rus.txt', return_type='tsv'),
        rus_lister_dict=vh.data(id='b9c9e8d9-9f22-43f9-b9db-fc7fa24b0425'),
        build_rus_lister_regex=svn_export_single_file('arcadia/alice/boltalka/scripts/normalize_and_filter/build-rus-lister-dict.sh', return_type='text'),
        max_ram=2000
    )['output']


if __name__ == '__main__':
    columns = ['context_{}'.format(i) for i in range(9)] + ['reply']
    with vh.Graph() as g:
        table = twitter_to_contexts()
        table = filtering_and_preprocess_twitter(table)
        table = map_fn_lazy(
            table,
            FilterContextsByDicts(
                columns=columns, truncate_non_empty_len=2, dict_filepaths=['file_0'], whitelist_filtering=True,
                preprocessing_regex=r'[^а-яА-Я ]'

            ),
            aux_files=[create_allowed_dict()],
            memory_limit_mb=1024
        )
        skip_higher_than = 50
        table = filter_dialogs_by_occurrences(
            table,
            occurrences_table=count_occurrences(table, columns=columns, skip_threshold=skip_higher_than),
            columns=columns, filter_higher_than=skip_higher_than
        )
        table = map_fn(table, truncate_dialogs_with_holes_mapper)
        table = map_fn(table, dialog_minlen_filtering_mapper)
        tokenize_twitter(table, target_folder='//home/voice/artemkorenev/boltalka/twitter_filtered_whitelist')

    info = vh.run(g, quota='dialogs', yt_token_secret='artemkorenev_yt_token', yt_proxy='hahn', yt_pool='voice').get_workflow_info()
