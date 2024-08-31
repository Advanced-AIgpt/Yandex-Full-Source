from alice.boltalka.generative.training.data.nn.movies.lib import otzovik_to_dialogs, irecommend_to_dialogs, \
    build_movie_data, set_otzovik_source, set_irecommend_source, extract_movie_columns, merge_context_with_md
from alice.boltalka.generative.training.data.nn.util.experimental.ops import map_fn
from alice.boltalka.generative.training.data.nn.util.lib import contexts_from_dialogs, tokenize_columns, \
    merge_contexts_and_make_reply, filter_empty_dialogs
from alice.boltalka.generative.training.data.nn.util.ops import get_yt_table, \
    get_yt_file, merge_tables, train_test_split, mr_copy, to_tsv, mr_upload, dialog_preprocess
from alice.boltalka.generative.training.data.nn.util.preprocess import preprocess_contexts


def get_otzovik_data():
    table = get_yt_table(path='//home/voice/artemkorenev/boltalka/ugc/3rd_party/otzovik/data_raw_with_movie_data')
    table = otzovik_to_dialogs(table)
    return map_fn(table, set_otzovik_source)


def get_irecommend_data():
    table = get_yt_table(path='//home/voice/artemkorenev/boltalka/ugc/3rd_party/irecommend/data_raw_with_movie_data')
    table = irecommend_to_dialogs(table)
    return map_fn(table, set_irecommend_source)


def movies_pipeline():
    n_contexts = 10
    contexts = ['context_{}'.format(i) for i in range(n_contexts)]
    movie_data_columns = [
        'movie_title', 'movie_original_title', 'movie_director_name', 'movie_actor_name1', 'movie_actor_name2',
        'movie_actor_name3', 'movie_genre1', 'movie_genre2', 'movie_genre3'
    ]

    table = merge_tables([get_otzovik_data(), get_irecommend_data()])
    table = contexts_from_dialogs(table, n_contexts=n_contexts)
    table = map_fn(table, extract_movie_columns)
    table = dialog_preprocess(table, twitter_specificity_columns=contexts, normalize_nlg_columns=contexts)
    table = preprocess_contexts(table, columns=contexts + movie_data_columns)
    table = tokenize_columns(
        table,
        bpe_voc=get_yt_file('//home/voice/artemkorenev/boltalka/twitter/twitter_tokenized_bpe_dict'),
        columns_to_tokenize=contexts + movie_data_columns
    )

    for movie_data_fields, target_folder in zip(
            ([], ['movie_title'], ['movie_title', 'movie_director_name', 'movie_actor_name1', 'movie_genre1']),
            ('no_md', 'md_title_only', 'md_all')
    ):
        table_with_md = build_movie_data(table, movie_data_fields)
        table_with_md = merge_contexts_and_make_reply(table_with_md, separator_token='[SPECIAL_SEPARATOR_TOKEN]')
        table_with_md = map_fn(table_with_md, filter_empty_dialogs)
        table_with_md = map_fn(table_with_md, merge_context_with_md)
        train, val = train_test_split(table_with_md, test_size=0.05)
        mr_copy(train, '//home/voice/artemkorenev/boltalka/ugc/3rd_party/merged/{}/train'.format(target_folder))
        mr_copy(val, '//home/voice/artemkorenev/boltalka/ugc/3rd_party/merged/{}/val'.format(target_folder))
        mr_upload(
            to_tsv(val, ['context_merged']),
            '//home/voice/artemkorenev/boltalka/ugc/3rd_party/merged/{}/val_context.tsv'.format(target_folder)
        )
        mr_upload(
            to_tsv(val, ['reply']),
            '//home/voice/artemkorenev/boltalka/ugc/3rd_party/merged/{}/val_reply.tsv'.format(target_folder)
        )
