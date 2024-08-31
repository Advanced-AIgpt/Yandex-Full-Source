import vh3
from vh3.decorator import graph
from vh3.extras.json import dump

import functools
from typing import NamedTuple

from alice.beggins.internal.vh.flows.toloka_classification import toloka_classification
from alice.beggins.internal.vh.operations import ext

from alice.beggins.internal.vh.scripts.scripter import Scripter
from alice.beggins.internal.vh.scripts.python import json_conversion


SUB_GRAPHS_WORKFLOW_ID = 'https://nirvana.yandex-team.ru/flow/edd405d9-ed2d-48ac-8f30-9d75f0b3df60'


subgraph = functools.partial(graph, workflow_id=SUB_GRAPHS_WORKFLOW_ID)


def get_frequency_script() -> vh3.Expr:
    return vh3.Expr('''
PRAGMA yt.InferSchema = '99';

INSERT INTO {{output1}}
SELECT text AS text, COUNT(*) AS freq
FROM {{input1}}
GROUP BY text;
''')


def get_the_most_frequent_queries_script(samples_limit: vh3.Integer) -> vh3.Expr:
    return vh3.Expr(f'''
PRAGMA yt.InferSchema = '99';

$samples = (
    SELECT *
    FROM {{{{input1}}}}
    ORDER BY freq DESC
    LIMIT {samples_limit:expr}
);

INSERT INTO {{{{output1}}}} WITH TRUNCATE
SELECT * FROM $samples;
''')


def get_join_script() -> vh3.Expr:
    return vh3.Expr('''
PRAGMA yt.InferSchema = '99';

INSERT INTO {{output1}}
SELECT freq_table.text AS text,
       freq_table.freq AS freq,
       toloka.is_positive AS is_positive,
       toloka.is_positive_prob AS is_positive_prob
FROM {{input1}} AS toloka
     JOIN {{input2}} AS freq_table ON toloka.utterance = freq_table.text;
''')


def get_statistic_script(lower_bound_toloka_confidence: vh3.Number) -> vh3.Expr:
    return vh3.Expr(f'''
PRAGMA yt.InferSchema = '99';

$statistic = (
    SELECT SUM_IF(freq, is_positive == "N" AND is_positive_prob >= {lower_bound_toloka_confidence:expr}) AS negatives_number,
           SUM_IF(freq, is_positive == "Y" AND is_positive_prob >= {lower_bound_toloka_confidence:expr}) AS positives_number,
           SUM_IF(freq, is_positive_prob < {lower_bound_toloka_confidence:expr}) AS controversial_queries_number,
    FROM {{{{input1}}}}
);

INSERT INTO {{{{output1}}}}
SELECT negatives_number AS negatives_number,
       positives_number AS positives_number,
       controversial_queries_number AS controversial_queries_number,
       1.0 * positives_number / (positives_number + negatives_number) AS precision,
FROM $statistic;
''')


def get_negatives_script(lower_bound_toloka_confidence: vh3.Number) -> vh3.Expr:
    return vh3.Expr(f'''
PRAGMA yt.InferSchema = '99';

INSERT INTO {{{{output1}}}}
SELECT text AS text, freq AS freq,
FROM {{{{input1}}}}
WHERE is_positive == "N" AND is_positive_prob >= {lower_bound_toloka_confidence:expr}
ORDER BY freq DESC;
''')


def get_positives_script(lower_bound_toloka_confidence: vh3.Number) -> vh3.Expr:
    return vh3.Expr(f'''
PRAGMA yt.InferSchema = '99';

INSERT INTO {{{{output1}}}}
SELECT text AS text, freq AS freq,
FROM {{{{input1}}}}
WHERE is_positive == "Y" AND is_positive_prob >= {lower_bound_toloka_confidence:expr}
ORDER BY freq DESC;
''')


def get_controversial_queries_script(lower_bound_toloka_confidence: vh3.Number) -> vh3.Expr:
    return vh3.Expr(f'''
PRAGMA yt.InferSchema = '99';

INSERT INTO {{{{output1}}}}
SELECT text AS text, freq AS freq, is_positive AS is_positive, is_positive_prob AS is_positive_prob,
FROM {{{{input1}}}}
WHERE is_positive_prob < {lower_bound_toloka_confidence:expr}
ORDER BY freq DESC;
''')


class TolokaStatisticsResult(NamedTuple):
    metrics: vh3.JSON
    false_positives: vh3.MRTable
    true_positives: vh3.MRTable
    controversial_queries: vh3.MRTable


@graph(workflow_id='https://nirvana.yandex-team.ru/flow/df9d551f-f34e-4f8a-8c60-6b517c760177')
def toloka_statistics(data: vh3.MRTable,
                      samples_limit: vh3.Integer = 3000,
                      toloka_confidence_lower_bound: vh3.Number = 0.8) -> TolokaStatisticsResult:
    """
    Calculates statistics of queries using Toloka Classification
    :param data: table with queries, column name with queries is 'text'
    :param samples_limit: queries number that will be sent to Toloka Classification,
                          the most frequent queries will have a higher priority
    :param toloka_confidence_lower_bound: only queries with at least the specified confidence will be selected
    :return:
    """
    frequency_table = ext.yql_2(
        input1=[data],
        request=get_frequency_script(),
        **vh3.block_args(name='build frequency table'),
    ).output1

    frequent_queries = ext.yql_2(
        input1=[frequency_table],
        request=get_the_most_frequent_queries_script(samples_limit),
        **vh3.block_args(name='the most frequent queries'),
    ).output1

    toloka_response = toloka_classification(frequent_queries)

    text_statistic = ext.yql_2(
        input1=[toloka_response],
        input2=[frequency_table],
        request=get_join_script(),
        **vh3.block_args(name='join'),
    ).output1

    mr_table_metrics = ext.yql_2(
        input1=[text_statistic],
        request=get_statistic_script(toloka_confidence_lower_bound),
        **vh3.block_args(name='calculate results'),
    ).output1
    json_metrics = ext.python3_json_process(
        code=Scripter.to_string(json_conversion),
        mr=[mr_table_metrics],
        token1=vh3.context.yt_token,
        **vh3.block_args(name='convert to json'),
    ).out1

    negatives = ext.yql_2(
        input1=[text_statistic],
        request=get_negatives_script(toloka_confidence_lower_bound),
        **vh3.block_args(name='get negatives'),
    ).output1
    positives = ext.yql_2(
        input1=[text_statistic],
        request=get_positives_script(toloka_confidence_lower_bound),
        **vh3.block_args(name='get positives'),
    ).output1
    controversial_queries = ext.yql_2(
        input1=[text_statistic],
        request=get_controversial_queries_script(toloka_confidence_lower_bound),
        **vh3.block_args(name='get controversial queries'),
    ).output1

    return TolokaStatisticsResult(
        metrics=json_metrics,
        false_positives=negatives,
        true_positives=positives,
        controversial_queries=controversial_queries,
    )


@subgraph()
def toloka_statistics_with_condition(data: vh3.MRTable,
                                     samples_limit: vh3.Integer = 3000,
                                     toloka_confidence_lower_bound: vh3.Number = 0.8,
                                     need_toloka_classification: vh3.Boolean = False) -> TolokaStatisticsResult:
    need_toloka_classification = ext.light_groovy_json_filter_if(
        filter=vh3.Expr(f'"{need_toloka_classification:expr}" == "true"'),
        input=dump({}),
        **vh3.block_args(name='if need_toloka_classification'),
    )
    return toloka_statistics(
        data=data,
        samples_limit=samples_limit,
        toloka_confidence_lower_bound=toloka_confidence_lower_bound,
        **vh3.block_args(dynamic_options=need_toloka_classification.output_true),
    )
