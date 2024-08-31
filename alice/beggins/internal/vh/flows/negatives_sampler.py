from typing import Literal

import vh3

from vh3.decorator import graph
from vh3.extras.json import dump

from alice.beggins.internal.vh.operations import ext
from alice.beggins.internal.vh.flows.toloka_classification import toloka_classification


def get_random_queries_script(samples_limit: vh3.Integer) -> vh3.Expr:
    return vh3.Expr(f'''
PRAGMA yt.InferSchema = '99';

INSERT INTO {{{{output1}}}}
SELECT * FROM {{{{input1}}}}
ORDER BY Random(TableRow())
LIMIT {samples_limit:expr};
''')


def get_strong_negatives_extraction_script(toloka_confidence_lower_bound: vh3.Number) -> vh3.Expr:
    return vh3.Expr(f'''
PRAGMA yt.InferSchema = '99';

INSERT INTO {{{{output1}}}}
SELECT utterance AS text,
CAST(is_positive == "Y" as INT64) as target
FROM {{{{input1}}}}
WHERE is_positive_prob >= {toloka_confidence_lower_bound:expr}
''')


@graph(workflow_id='https://nirvana.yandex-team.ru/flow/d2143f77-9620-4035-9acc-62a4a3529548')
def negatives_sampler(queries: vh3.MRTable,
                      samples_limit: vh3.Integer = 3000,
                      toloka_confidence_lower_bound: vh3.Number = 0.8,
                      sampler_type: vh3.Enum[Literal['random', 'alice logs miner']] = 'random') -> vh3.MRTable:
    """
    Looking for negatives among the specified queries using Toloka Classification
    :param queries: table with queries, column name with queries is 'text'
    :param samples_limit: queries number that will be sent to Toloka Classification
    :param toloka_confidence_lower_bound: only queries with at least the specified confidence will be selected
    :param sampler_type: type of subsampling extraction, by default, it is random
    :return:
    """
    result = ext.light_groovy_json_filter_if(
        filter=vh3.Expr(f'"{sampler_type:expr}" == "random"'),
        input=dump({}),
        **vh3.block_args(name='if (sampler_type == random)'),
    )

    random_subsample = ext.yql_2(
        input1=(queries, ),
        request=get_random_queries_script(samples_limit),
        **vh3.block_args(name='get random queries', dynamic_options=result.output_true),
    ).output1

    alice_logs_miner_subsample = ext.alice_logs_miner(
        data=queries,
        max_unique_utterances=samples_limit,
        **vh3.block_args(name='alice logs miner', dynamic_options=result.output_false),
    )

    subsample = ext.yql_2(
        input1=(random_subsample, alice_logs_miner_subsample),
        request='INSERT INTO {{output1}} SELECT * FROM {{input1}};',
        **vh3.block_args(name='concatenate'),
    ).output1

    toloka_scores = toloka_classification(
        queries=subsample,
        **vh3.block_args(name='toloka classification'),
    )
    false_positives = ext.yql_2(
        input1=(toloka_scores, ),
        request=get_strong_negatives_extraction_script(toloka_confidence_lower_bound),
        **vh3.block_args(name='extract false positives'),
    ).output1
    return false_positives
