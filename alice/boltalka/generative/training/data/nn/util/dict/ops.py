import subprocess

import vh


@vh.lazy.from_annotations
def convert_dict_with_bpe(dicts_archive: vh.File) -> (vh.File, vh.File):
    out_file_bpe = str(vh.File('bpe.voc'))
    out_file_tid = str(vh.File('token_to_id_raw.voc'))

    subprocess.call(
        'mkdir dictionaries && tar xvfz {} -C dictionaries'.format(str(dicts_archive)),
        shell=True
    )

    loaded_voc = dict()

    # loading dict
    with open('dictionaries/dictionary0.txt') as f:
        for i, line in enumerate(f):
            if i < 2:  # skipping unnecessary dictionary info
                continue

            line = line.rstrip('\n')
            id, freq, token = line.split('\t')

            if token == '':  # beginning of the word
                token = '^'
            elif token == ' ':  # ending of the word
                token = '$'
            elif token in ['$', '^', '\\']:
                token = '\{}'.format(token)  # escaping these symbols since they are used in BPE formatting

            loaded_voc[int(id)] = token

    standard_voc_size = len(loaded_voc) + 2  # +2 since there are 2 tokens skipped between simple dict and bpe

    voc_offset = 3  # in tfnn first tokens are _BOS_ _EOS_ _UNK_

    skipped_tokens = set()
    out_tokens = []
    with open('dictionaries/bpe0.txt') as f:
        for i, line in enumerate(f):
            line = line.rstrip('\n')

            id1, id2, freq, token = line.split('\t')
            id1, id2 = int(id1), int(id2)

            current_token_id = i + standard_voc_size

            if '_UNK_' in token or id1 in skipped_tokens or id2 in skipped_tokens:  # invalid token or consists of invalid tokens
                skipped_tokens.add(current_token_id)
                continue

            loaded_voc[current_token_id] = loaded_voc[id1] + loaded_voc[id2]

            out_tokens.append((loaded_voc[id1], loaded_voc[id2], str(i + voc_offset - len(skipped_tokens))))

    with open(out_file_bpe, 'w') as f:
        for token1, token2, id in out_tokens:
            f.write('\t'.join([token1, token2, id]))
            f.write('\n')

    with open(out_file_tid, 'w') as f:
        for token1, token2, id in out_tokens:
            if token1.startswith('^'):
                new_token = token1.lstrip('^') + token2.rstrip('$')
            else:
                new_token = '`' + token1 + token2.rstrip('$')

            f.write(('\t'.join([new_token, id])))
            f.write('\n')

    return out_file_bpe, out_file_tid


@vh.lazy.from_annotations
def convert_dict(dicts_archive: vh.File) -> vh.File:
    out_file_tid = str(vh.File('token_to_id.voc'))

    subprocess.call(
        'mkdir dictionaries && tar xvfz {} -C dictionaries'.format(str(dicts_archive)),
        shell=True
    )

    voc_offset = 3  # in tfnn first tokens are _BOS_ _EOS_ _UNK_
    # loading dict
    with open('dictionaries/dictionary0.txt') as f, open(out_file_tid, 'w') as out_f:
        for i, line in enumerate(f):
            if i < 2:  # skipping unnecessary dictionary info
                continue

            line = line.rstrip('\n')
            id, freq, token = line.split('\t')
            out_f.write('{}\t{}\n'.format(token, int(id) + voc_offset))

    return out_file_tid


def generate_dicts(table, columns_for_dict, delimeter, token_level_type, occurrence_lower_bound, is_bpe, bpe_size=None):
    if is_bpe:
        assert bpe_size is not None

    tokenizer_options = vh.op(id='214f832c-0b67-48fd-b511-bf5ffda2a933', name='Text: Tokenizer Options')(
        lowercasing=True,
        lemmatizing=False,
        number_process_policy='LeaveAsIs',
        number_token='🔢',
        separator_type='ByDelimiter',
        delimiter=delimeter,
        split_by_set=False,
        skip_empty=True,
        token_types=['Word', 'Number', 'Punctuation'],
        sub_tokens_policy='SingleToken'
    )

    return vh.op(id='477e6e41-53a3-4f60-ad4d-5ed4e4e16af3', name='Text: Create Dictionary')(
        input=table,
        tokenizer_options=tokenizer_options,

        use_tokenizer=True,
        occurrence_lower_bounds=[occurrence_lower_bound],
        token_level_types=[token_level_type],
        num_bpe_units=[bpe_size] if is_bpe else None,
        dictionary_types=['Bpe'] if is_bpe else None,
        columns_to_process=columns_for_dict,
        max_ram=32768,
        ttl=3600,
        yt_token=vh.get_yt_token_secret()
    )
