# coding: utf-8

import logging
import vh

from operator import attrgetter

from global_options import nirvana_global_options

logger = logging.getLogger(__name__)


_PREPARE_TABLE_IN_BOLTALKA_FORMAT_YQL = '''
    PRAGMA yt.InferSchema = '1';

    INSERT INTO {{output1}}
    SELECT
        utterance_text as context_0,
        "" as context_1,
        "" as context_2,
        utterance_text as reply
    FROM (
        SELECT DISTINCT utterance_text
        FROM {{input1}}
    )
'''

_JOIN_EMBEDDINGS_YQL = '''
    PRAGMA yt.InferSchema = '1';

    INSERT INTO {{output1}} WITH TRUNCATE
    SELECT
        lhs.*,
        rhs.%s as %s
    FROM {{input1}} as lhs
    INNER JOIN {{input2}} as rhs
    ON lhs.utterance_text == rhs.context_0
'''


def _vh_op_with_globals(global_options, **kwargs):
    global_options = {
        option: attrgetter(option.replace('-', '_'))(nirvana_global_options)
        for option in global_options
    }

    return vh.op(**kwargs).partial(_options=global_options)


def _yql_op(**kwargs):
    return _vh_op_with_globals(
        global_options=['mr-account', 'yt-token', 'yql-token', 'yt-pool'],
        **kwargs
    ).partial(_options={
        'syntax_version': 'v1'
    })


def _load_boltalka_embedder():
    get_mr_file_op = _vh_op_with_globals(
        name='Get MR File',
        owner='robot-nirvana',
        global_options=['yt-token']
    )
    download_mr_file_op = _vh_op_with_globals(
        name='MR Download File as Binary Data',
        owner='robot-nirvana',
        global_options=['yt-token']
    )

    mr_file = get_mr_file_op(
        _options={
            'path': '//home/voice/artemkorenev/nlu_search_dssm_model',
            'cluster': 'hahn',
        }
    )
    binary = download_mr_file_op(
        file=mr_file,
        max_disk=16384
    )['content']

    return binary


def build_boltalka_embeddings_collection_subgraph(data_table, embedding_name):
    yql_1_op = _yql_op(name='YQL 1', owner='robot-mlmarines')
    yql_2_op = _yql_op(name='YQL 2', owner='robot-mlmarines')
    apply_boltalka_dssm_op = _vh_op_with_globals(
        name='Apply Multiple NLG DSSM Models',
        id='47d68e0c-1182-4b13-8730-e3f086419607',
        global_options=['yt-token', 'mr-account', 'yt-pool']
    )

    requests_table = yql_1_op(
        _inputs={
            'input1': data_table
        },
        _options={
            'request': _PREPARE_TABLE_IN_BOLTALKA_FORMAT_YQL
        }
    )['output1']

    embedder = _load_boltalka_embedder()

    embeddings_table = apply_boltalka_dssm_op(
        _inputs={
            'input': requests_table,
            'model': embedder
        },
        _options={
            'max-ram': 10000,
            'job-count': 300,
            'batch-size': 100
        }
    )['output']

    data_table = yql_2_op(
        _inputs={
            'input1': data_table,
            'input2': embeddings_table
        },
        _options={
            'request': _JOIN_EMBEDDINGS_YQL % (embedding_name, embedding_name)
        }
    )['output1']

    return data_table
