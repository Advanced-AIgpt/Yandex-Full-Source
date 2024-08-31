import yatest.common


IBPE_TOOL_PATH = yatest.common.binary_path("alice/boltalka/tools/train_ibpe/train_ibpe")


def local_canonical_file(*args, **kwargs):
    return yatest.common.canonical_file(*args, local=True, **kwargs)


def test_word_grams():
    output_tokens_path = yatest.common.test_output_path('tokens.tsv')
    output_ibpe_dict_path = yatest.common.test_output_path('ibpe_dict.tsv')
    cmd = (
        IBPE_TOOL_PATH,
        '-d', yatest.common.source_path('alice/boltalka/tools/train_ibpe/ibpe_test/data/sentences_data.tsv'),
        '-n', '10',
        '-a', '10',
        '-v', output_tokens_path,
        '-m', output_ibpe_dict_path,
    )
    yatest.common.execute(cmd)
    return [local_canonical_file(output_tokens_path), local_canonical_file(output_ibpe_dict_path)]


def test_word_grams_skip_unknown():
    output_tokens_path = yatest.common.test_output_path('tokens.tsv')
    output_ibpe_dict_path = yatest.common.test_output_path('ibpe_dict.tsv')
    cmd = (
        IBPE_TOOL_PATH,
        '-d', yatest.common.source_path('alice/boltalka/tools/train_ibpe/ibpe_test/data/sentences_data.tsv'),
        '-n', '10',
        '-a', '10',
        '-v', output_tokens_path,
        '-m', output_ibpe_dict_path,
        '--skip-unknown',
    )
    yatest.common.execute(cmd)
    return [local_canonical_file(output_tokens_path), local_canonical_file(output_ibpe_dict_path)]


def test_char_grams():
    output_tokens_path = yatest.common.test_output_path('tokens.tsv')
    output_ibpe_dict_path = yatest.common.test_output_path('ibpe_dict.tsv')
    cmd = (
        IBPE_TOOL_PATH,
        '-d', yatest.common.source_path('alice/boltalka/tools/train_ibpe/ibpe_test/data/char_data.tsv'),
        '-n', '10',
        '-a', '10',
        '-v', output_tokens_path,
        '-m', output_ibpe_dict_path,
    )
    yatest.common.execute(cmd)
    return [local_canonical_file(output_tokens_path), local_canonical_file(output_ibpe_dict_path)]
