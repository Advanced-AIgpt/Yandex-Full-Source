import json
import vh3
import yaml

from typing import Sequence, Union, Optional
from library.python.resource import resfs_read
from vh3.lib.util.json import light_json_flat_merge_files

from alice.beggins.internal.vh.flows.evaluation import ThresholdSelectionStrategyType
from alice.beggins.internal.vh.flows.classification import (
    train_binary_classifier, Context as ClassificationContext, TrainBinaryClassifierResult
)
from alice.beggins.internal.vh.flows.common import (
    ArcContext, SandboxContext, CacheContext, YqlContext, PulsarContext, SoyContext,
    static_name, EmbedderType
)
from alice.beggins.internal.vh.flows.repeatable_training import mine_samples_by_pre_trained_classifier
from alice.beggins.internal.vh.meta.basket_generators.custom_generator import custom_generator
from alice.beggins.internal.vh.meta.basket_generators.dc_miner import dc_miner
from alice.beggins.internal.vh.meta.basket_generators.tom_with_cache import tom_with_cache
from alice.beggins.internal.vh.meta.basket_generators.dc_tom import dc_tom
from alice.beggins.internal.vh.meta.basket_generators.stable_generator import stable_generator
from alice.beggins.internal.vh.meta.graph.config import (
    load_meta_config, get_by_key_path
)
from alice.beggins.internal.vh.meta.graph.scripts import (
    get_training_config, join_single_intent_metrics, join_metrics_and_model_name,
    aggregate_metrics, format_metrics_table, get_toloka_parameters_from_config
)
from alice.beggins.internal.vh.operations import ext
from alice.beggins.internal.vh.scripts.scripter import get_function_body


class MetaEvaluationContext(ArcContext, SandboxContext, CacheContext, YqlContext, PulsarContext, SoyContext):
    mr_account: vh3.String = "alice-dev"
    week: vh3.String = "one_year_logs_v2_1"
    st_ticket: vh3.String = None
    threshold_selection_strategy: ThresholdSelectionStrategyType = 'max_precision_condition_recall'
    abc_service: vh3.String = "1459"


@vh3.decorator.graph()
def _train_binary_classifier_wrap(
    model_name: vh3.String = '',
    embedder: EmbedderType = 'Zeliboba',
    catboost_params: vh3.String = '',
    manifest: Union[vh3.Binary, vh3.File, vh3.JSON] = None,
    train: Sequence[vh3.MRTable] = (),
    accept: Sequence[vh3.MRTable] = (),
    custom_basket: Optional[vh3.MRTable] = None,
) -> TrainBinaryClassifierResult:
    with vh3.context(
        ClassificationContext,
        model_name=model_name,
        embedder=embedder,
        catboost_params=catboost_params,
    ):
        return train_binary_classifier(
            manifest=manifest,
            train=train,
            accept=accept,
            custom_basket=custom_basket,
        )


@vh3.decorator.graph()
def _mine_samples_by_pre_trained_classifier_wrap(
    config: vh3.JSON,
    model_name: vh3.String = '',
    embedder: EmbedderType = 'Zeliboba',
    catboost_params: vh3.String = '',
    manifest: Union[vh3.Binary, vh3.File, vh3.JSON] = None,
    train: Sequence[vh3.MRTable] = (),
    accept: Sequence[vh3.MRTable] = (),
    classification_task_question: Optional[vh3.String] = None,
    classification_project_instructions: Optional[vh3.String] = None,
    positive_honeypots: Optional[vh3.String] = None,
    negative_honeypots: Optional[vh3.String] = None,
) -> vh3.MRTable:
    toloka_parameters = ext.python3_any_to_json(
        input_type='json-mem',
        body=get_function_body(get_toloka_parameters_from_config),
        input=config,
        **vh3.block_args(name='Get toloka parameters')
    )
    with vh3.context(
        ClassificationContext,
        model_name=model_name,
        embedder=embedder,
        catboost_params=catboost_params,
        classification_task_question=classification_task_question,
        classification_project_instructions=classification_project_instructions,
        positive_honeypots=positive_honeypots,
        negative_honeypots=negative_honeypots,
    ):
        return mine_samples_by_pre_trained_classifier(
            manifest=manifest,
            train=train,
            accept=accept,
            toloka_parameters=toloka_parameters,
        ).mined_dataset


def _make_train_and_evaluate_by_config(enable_repeatable_training):
    @static_name('Train and evaluate' + (' repeatable' if enable_repeatable_training else ''))
    @vh3.decorator.graph()
    def _train_and_evaluate_by_config(
        config: vh3.JSON,
        manifest: Union[vh3.Binary, vh3.File, vh3.JSON] = None,
        train: Sequence[vh3.MRTable] = (),
        accept: Sequence[vh3.MRTable] = (),
    ) -> vh3.JSON:
        meta_manifest = ext.python3_any_to_json(
            input_type='json-mem',
            body="return v['meta_evaluation']['data']['accept']",
            input=config,
            **vh3.block_args(name='Get meta evaluation dataset manifest')
        )
        meta_dataset = ext.process_dataset_manifest(manifest=meta_manifest)

        training_config = ext.python3_any_to_json(
            input_type='json-mem',
            body=get_function_body(get_training_config),
            input=config,
            **vh3.block_args(name='Get training config')
        )

        if enable_repeatable_training:
            mined_dataset = _mine_samples_by_pre_trained_classifier_wrap(
                config=config,
                manifest=manifest,
                train=train,
                accept=accept,
                **vh3.block_args(dynamic_options=training_config),
            )
            train = (train, mined_dataset)

        training_result = _train_binary_classifier_wrap(
            manifest=manifest,
            train=train,
            accept=accept,
            custom_basket=meta_dataset,
            **vh3.block_args(dynamic_options=training_config),
        )

        metrics = ext.python3_any_any_any_to_json(
            input0_type='json-mem',
            input1_type='json-mem',
            input2_type='json-mem',
            body=get_function_body(join_single_intent_metrics),
            input0=training_result.accept_basket_metrics,
            input1=training_result.custom_basket_metrics,
            **vh3.block_args(name='Join metrics')
        )

        return ext.python3_any_any_any_to_json(
            input0_type='json-mem',
            input1_type='json-mem',
            input2_type='json-mem',
            body=get_function_body(join_metrics_and_model_name),
            input0=config,
            input1=metrics,
            input2=training_result.year_logs_matches_stats,
            **vh3.block_args(name='Join metrics and model name')
        )
    return _train_and_evaluate_by_config


def _check_is_intent_enabled(flow_config, intent_config):
    russian_name = intent_config.get('russian_name')
    for path in flow_config.get('required_data', []):
        if not get_by_key_path(intent_config, path):
            print(f'  - {russian_name} (no {path})')
            return False
    print(f'  + {russian_name}')
    return True


def _load_yaml_from_resource(path):
    return yaml.full_load(resfs_read(path).decode("utf-8"))


def _build_meta_evaluation_graph(flow_name, evaluated_graph):
    print(f'Build meta evaluation graph for {flow_name}')

    meta_config = load_meta_config('alice/beggins/data/meta/config.yaml', _load_yaml_from_resource)
    flow_config = meta_config['flows'][flow_name]

    metrics_list = []
    for intent_config in flow_config['intents']:
        if not _check_is_intent_enabled(flow_config, intent_config):
            continue

        config_output = ext.single_option_to_json_output(
            input=json.dumps(intent_config, indent=2, ensure_ascii=False, separators=(',', ': ')),
            **vh3.block_args(name=intent_config['russian_name'])
        )
        datasets = evaluated_graph(
            config=config_output,
            yt_token=vh3.context.yt_token,
            yql_token=vh3.context.yql_token,
            mr_account=vh3.context.mr_account,
            mr_default_cluster=vh3.context.mr_default_cluster,
            week=vh3.context.week,
            st_ticket=vh3.context.st_ticket,
            abc_service=vh3.context.abc_service,
            **vh3.block_args(name=flow_name.replace('_', ' '))
        )
        metrics = _make_train_and_evaluate_by_config(flow_config.get('repeatable_training', False))(
            config=config_output,
            train=datasets.dev,
            accept=datasets.accept
        )
        metrics_list.append(metrics)

    merged_metrics = light_json_flat_merge_files(
        metrics_list,
        **vh3.block_args(name='Merge metrics')
    )
    ext.python3_any_to_json(
        input_type='json-mem',
        body=get_function_body(aggregate_metrics),
        input=merged_metrics,
        **vh3.block_args(name='Aggregate metrics')
    )
    ext.python3_any_to_txt(
        input_type='json-mem',
        body=get_function_body(format_metrics_table),
        input=merged_metrics,
        **vh3.block_args(name='Metrics table')
    )


@vh3.decorator.graph()
def meta_evaluate_stable_generator() -> None:
    _build_meta_evaluation_graph('stable_generator', stable_generator)


@vh3.decorator.graph()
def meta_evaluate_stable_generator_repeatable() -> None:
    _build_meta_evaluation_graph('stable_generator_repeatable', stable_generator)


@vh3.decorator.graph()
def meta_evaluate_custom_generator() -> None:
    _build_meta_evaluation_graph('custom_generator', custom_generator)


@vh3.decorator.graph()
def meta_evaluate_dc_miner() -> None:
    _build_meta_evaluation_graph('dc_miner', dc_miner)


@vh3.decorator.graph()
def meta_evaluate_dc_miner_old_dates_regex_only() -> None:
    _build_meta_evaluation_graph('dc_miner_old_dates_regex_only', dc_miner)


@vh3.decorator.graph()
def meta_evaluate_dc_miner_old_dates_similar_only() -> None:
    _build_meta_evaluation_graph('dc_miner_old_dates_similar_only', dc_miner)


@vh3.decorator.graph()
def meta_evaluate_dc_miner_old_dates_regex_and_similar() -> None:
    _build_meta_evaluation_graph('dc_miner_old_dates_regex_and_similar', dc_miner)


@vh3.decorator.graph()
def meta_evaluate_dc_miner_new_dates() -> None:
    _build_meta_evaluation_graph('dc_miner_new_dates', dc_miner)


@vh3.decorator.graph()
def meta_evaluate_dc_tom_no_logs() -> None:
    _build_meta_evaluation_graph('dc_tom_no_logs', dc_tom)


@vh3.decorator.graph()
def meta_evaluate_tom_with_cache_old_dates() -> None:
    _build_meta_evaluation_graph('tom_with_cache_old_dates', tom_with_cache)


@vh3.decorator.graph()
def meta_evaluate_tom_with_cache_old_dates_repeatable() -> None:
    _build_meta_evaluation_graph('tom_with_cache_old_dates_repeatable', tom_with_cache)
