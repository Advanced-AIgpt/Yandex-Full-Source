import os

import vh

from alice.boltalka.generative.training.data.nn.bart.ops import bart_preprocessing
from alice.boltalka.generative.training.data.nn.util.dict.ops import generate_dicts, convert_dict_with_bpe, convert_dict
from alice.boltalka.generative.training.data.nn.util.experimental.ops import map_fn
from alice.boltalka.generative.training.data.nn.util.lib import tokenize_columns
from alice.boltalka.generative.training.data.nn.util.ops import get_yt_table, to_tsv, mr_upload, mr_copy, \
    mr_mkdir, train_test_split_by_column, mr_merge, mr_shuffle


def convert_sep(row):
    row['text'] = row['text'].replace(' [sep] ', '\n - ')
    yield row


def unbpe_text_column_map(row):
    row['text'] = row['text'].replace(' `', '')
    yield row


def preprocess_anekdot(row):
    import string
    import re
    text = row['text'].decode('utf-8')
    text = text.lower()
    text = text.replace('``', '"')
    text = text.replace('--', '-')

    # space out punctuations
    for c in string.punctuation:
        text = text.replace(c, ' {} '.format(c))

    text = re.sub(' +', ' ', text)

    yield {'text': text, 'source': 'anekdot'}


def escape_new_line(row):
    import re
    text = row['text']
    text = text.replace('\n', ' \\n ')
    text = re.sub(' +', ' ', text)
    row['text'] = text
    yield row


if __name__ == '__main__':
    with vh.Graph() as g:
        dataset_folder = '//home/voice/dan-anastasev/LM/dataset'

        # getting data
        train, val = get_yt_table(os.path.join(dataset_folder, 'train')), \
                     get_yt_table(os.path.join(dataset_folder, 'valid'))

        # untokenizing tables, converting <sep>
        train, val = [map_fn(map_fn(x, convert_sep), unbpe_text_column_map) for x in [train, val]]
        train_anekdot, val_anekdot = train_test_split_by_column(
            map_fn(get_yt_table('//home/voice/krom/anekdotru/parsed'), preprocess_anekdot), test_size=0.005, column='text'
        )
        train, val = mr_merge([train, train_anekdot]), mr_merge([val, val_anekdot])
        train, val = [map_fn(x, escape_new_line) for x in [train, val]]

        # generating bpe dict
        dicts = generate_dicts(
            train, columns_for_dict=['text'], delimeter='', token_level_type='Letter',
            occurrence_lower_bound=1000,
            is_bpe=True, bpe_size=32000
        )
        bpe_voc, raw_token_to_id_voc = convert_dict_with_bpe(dicts)

        # vocs_dir = '//home/voice/artemkorenev/boltalka/twitter'
        # bpe_voc = get_yt_file(os.path.join(vocs_dir, 'twitter_tokenized_bpe_dict'))
        # token_to_id_voc = get_yt_file(os.path.join(vocs_dir, 'twitter_tokenized_id_to_token_dict'))

        # tokenizing tables again
        train, val = [
            tokenize_columns(
                x,
                bpe_voc=bpe_voc,
                columns_to_tokenize=['text']
            )
            for x in [train, val]
        ]

        # generating token to id voc
        dicts = generate_dicts(
            train, columns_for_dict=['text'], delimeter=' ', token_level_type='Word',
            occurrence_lower_bound=2000, is_bpe=False
        )
        token_to_id_voc = convert_dict(dicts)

        target_folder = '//home/voice/artemkorenev/boltalka/bart_lm'
        mr_mkdir(target_folder)

        # preprocess val
        val = bart_preprocessing(
            input_table=val,
            voc=token_to_id_voc,
            columns=['text'],
            processed_prefix='bart_processed_'
        )

        # shuffling data
        train, val = mr_shuffle(train), mr_shuffle(val)

        # save all data
        mr_copy(train, path=os.path.join(target_folder, 'train'))
        mr_copy(val, path=os.path.join(target_folder, 'val'))
        mr_upload(to_tsv(val, columns=['bart_processed_text']), dst=os.path.join(target_folder, 'val_input.tsv'))
        mr_upload(to_tsv(val, columns=['text']), dst=os.path.join(target_folder, 'val_output.tsv'))

        mr_upload(token_to_id_voc, dst=os.path.join(target_folder, 'token_to_id.voc'))
        mr_upload(raw_token_to_id_voc, dst=os.path.join(target_folder, 'token_to_id_raw.voc'))
        mr_upload(bpe_voc, dst=os.path.join(target_folder, 'bpe.voc'))

    info = vh.run(g, quota='dialogs', yt_token_secret='artemkorenev_yt_token', yt_proxy='hahn').get_workflow_info()
    print('https://nirvana.yandex-team.ru/flow/{}'.format(info.workflow_id))
