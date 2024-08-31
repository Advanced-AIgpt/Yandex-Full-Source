import functools
from typing import NamedTuple, Literal

from vh3 import (
    Binary, MRTable, JSON, HTML, block_args, Number, Expr, Secret, Factory, context, Integer, Enum, context as ctx
)
from vh3.decorator import graph

from alice.beggins.internal.vh.operations import ext
from alice.beggins.internal.vh.flows.common import (
    static_name, CacheContext, YqlContext, ArcContext, eval_catboost_model, SoyContext, EmbedderType
)

import alice.beggins.internal.vh.flows.yql as yql
from alice.beggins.internal.vh.flows.scrapper import scrape_queries_with_cache

from alice.beggins.internal.vh.scripts.scripter import Scripter
from alice.beggins.internal.vh.scripts.python import (
    find_thresholds_script,
    eval_metrics_script,
    extract_threshold_script
)


SUB_GRAPHS_WORKFLOW_ID = 'https://nirvana.yandex-team.ru/flow/edd405d9-ed2d-48ac-8f30-9d75f0b3df60'

subgraph = functools.partial(graph, workflow_id=SUB_GRAPHS_WORKFLOW_ID)


ThresholdSelectionStrategyType = Enum[
    Literal[
        'max_f1_no_conditions',
        'max_precision_condition_recall',
        'max_f1_condition_recall'
    ]
]


class Context(CacheContext, YqlContext, ArcContext, SoyContext):
    ...


@static_name('unpack model')
@subgraph()
def unpack_model(model_package: Binary) -> Binary:
    model_path_meta = ext.run_bash_command(
        command=r'echo {\"path\": \"$(tar -tf $in1 | grep cbm)\"} > $out1',
        in1=model_package,
        **block_args(name='find model path'),
    ).out1
    model_path_meta = ext.convert_any_to_json(
        file=model_path_meta,
        **block_args(name='cast as json'),
    )
    return ext.extract_from_tar(
        path='',
        out_type='binary',
        archive=model_package,
        **block_args(name='extract model', dynamic_options=[model_path_meta]),
    ).binary_file


class FindThresholdsResult(NamedTuple):
    threshold_meta: JSON
    plot: HTML
    threshold_param: JSON


@graph(workflow_id='https://nirvana.yandex-team.ru/flow/755ad1f6-0317-425b-8b33-91df1f7624a6')
def find_thresholds(scores: MRTable, recall_threshold: Number = 0.8, beta_logging_threshold: Number = 2.0,
                    yt_token: Secret = Factory(lambda: context.yt_token),
                    threshold_selection_strategy: ThresholdSelectionStrategyType = Factory(lambda: context.threshold_selection_strategy)) -> FindThresholdsResult:
    """
    {
        "threshold": 3.3645065917612555,
        "logging_threshold": 1.210563116130288,
        "threshold_metrics": {
            "precision": 1.0,
            "recall": 0.8015267175572519,
            "f1": 0.8898305084745763
        },
        "logging_threshold_metrics": {
            "precision": 1.0,
            "recall": 1.0,
            "f1": 1.0
        },
        "params": {
            "recall_threshold": 0.8,
            "beta_logging_threshold": 2.0
            "threshold_selection_strategy": 'default'
        }
    }
    """
    params = ext.single_option_to_json_output(
        Expr(f'{{'
             f'"recall_threshold": {recall_threshold:expr}, '
             f'"beta_logging_threshold": {beta_logging_threshold:expr},'
             f'"threshold_selection_strategy": "{threshold_selection_strategy:expr}"'
             f'}}'),
        **block_args(name='get params'),
    )
    result = ext.python3_json_process(
        code=Scripter.to_string(find_thresholds_script),
        in1=[params],
        mr=[scores],
        token1=yt_token,
        **block_args(name='find thresholds impl'),
    )
    threshold_meta = result.out1
    threshold = ext.python3_json_process(
        in1=[threshold_meta],
        code=Scripter.to_string(extract_threshold_script),
        **block_args(name='extract threshold')
    ).out1
    return FindThresholdsResult(threshold_meta, result.html_report, threshold)


class EvalMetricsResult(NamedTuple):
    metrics_meta: JSON
    confusion_matrix: HTML
    false_predictions: MRTable


@graph(workflow_id='https://nirvana.yandex-team.ru/flow/1a566e37-8d61-4675-86f4-cbccd8b32dfc')
def eval_metrics(scores: MRTable, threshold: Number = 0.5,
                 yt_token: Secret = Factory(lambda: context.yt_token)) -> EvalMetricsResult:
    params = ext.single_option_to_json_output(Expr(f"""{{
      "threshold": {threshold:expr}
    }}"""))
    result = ext.python3_json_process(
        code=Scripter.to_string(eval_metrics_script),
        in1=[params],
        mr=[scores],
        token1=yt_token,
        **block_args(name='eval metrics impl'),
    )
    false_predictions = ext.yql_2(
        request=Expr(f"""
INSERT INTO {{{{output1}}}}
SELECT * FROM {{{{input1}}}}
WHERE target != IF(score >= {threshold:expr}, 1, 0);
        """),
        input1=[scores],
        yt_token=yt_token,
        **block_args(name='get false predictions'),
    ).output1
    return EvalMetricsResult(result.out1, result.html_report, false_predictions)


@static_name('add embeddings')
@subgraph()
def add_embeddings(dataset: MRTable, embeddings: MRTable) -> MRTable:
    return ext.yql_2(
        input1=[dataset],
        input2=[embeddings],
        request=yql.ADD_EMBEDDINGS_SCRIPT
    ).output1


@subgraph()
def evaluate_catboost_model(model: Binary, dataset: MRTable, threshold: Number = 0.5,
                            max_rps: Integer = 100,
                            embedder: Enum[Literal['Beggins', 'Zeliboba']] = Factory(lambda: ctx.embedder)) -> EvalMetricsResult:
    embeddings = scrape_queries_with_cache(dataset, max_rps=max_rps, embedder=embedder)
    dataset = add_embeddings(dataset, embeddings)
    scores = eval_catboost_model(model, dataset)
    return eval_metrics(
        scores,
        threshold=threshold,
        **block_args(name='eval metrics'),
    )


@graph(workflow_id='https://nirvana.yandex-team.ru/flow/334f4be7-d8ce-4534-b725-7c5e327d5c6d')
def evaluate_catboost_classifier(model_package: Binary, dataset: MRTable, threshold: Number = 0.5,
                                 max_rps: Integer = 100,
                                 embedder: EmbedderType = Factory(lambda: ctx.embedder)) -> EvalMetricsResult:
    model = unpack_model(model_package)
    return evaluate_catboost_model(
        model=model, dataset=dataset, threshold=threshold, max_rps=max_rps, embedder=embedder,
    )
