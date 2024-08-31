import vh3
import typing
import library.python.resource as resource
from .resolve_slices import ResolveSlicesGraph
from .classifiers_training import ClassifierTrainingGraph
from .preclassifier_thresholds import PreclassifierThresholdsGraph
from .measure_quality_test import MeasureQualityTestGraph
from .operations import tsv_to_mr_table, build_arcadia_project, aggregate_classification_results, input_to_option, \
    merge_json_files, python_executor_with_script, options_to_json_7_key_values


# from vh3.lib.services.pulsar import add_instance, update_instance https://st.yandex-team.ru/NVSUP-3952
from vh3.lib.services.pulsar import PulsarOutput


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/d3410a47-373b-4665-91b9-6086a93c58ac")
def add_instance(
    *,
    pulsar_token: vh3.Secret = vh3.Factory(lambda: vh3.context.pulsar_token),
    model_name: vh3.String,
    dataset_name: vh3.String,
    model_version: vh3.String = None,
    model_options: vh3.String = None,
    dataset_info: vh3.String = None,
    tags: vh3.MultipleStrings = (),
    user_datetime: vh3.String = None,
    name: vh3.String = None,
    description: vh3.String = None,
    experiment_id: vh3.String = None,
    per_object_data_metainfo: vh3.String = None,
    diff_tool_config: vh3.String = None,
    save_tfevents: vh3.Boolean = False,
    tfevents_ttl: vh3.Integer = None,
    model_uid: vh3.String = None,
    model_scenario: vh3.String = None,
    produced_model_uid: vh3.String = None,
    timestamp: vh3.String = None,
    permissions: vh3.String = None,
    metrics: vh3.JSON = None,
    data: typing.Union[vh3.File, vh3.JSON, vh3.MRTable, vh3.TSV, vh3.Text] = None,
    dataset_path: typing.Union[vh3.MRDirectory, vh3.MRTable, vh3.MRFile] = None,
    report_url: vh3.JSON = None,
    tfevents: typing.Sequence[typing.Union[vh3.Binary, vh3.File, vh3.MRFile, vh3.MRTable, vh3.Text]] = (),
) -> PulsarOutput:
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/517861b4-a551-40d8-9b71-18a0a502acc4")
def update_instance(
    *,
    pulsar_token: vh3.Secret = vh3.Factory(lambda: vh3.context.pulsar_token),
    instance_id: vh3.String,
    tags: vh3.MultipleStrings = (),
    user_datetime: vh3.String = None,
    name: vh3.String = None,
    description: vh3.String = None,
    experiment_id: vh3.String = None,
    per_object_data_metainfo: vh3.String = None,
    diff_tool_config: vh3.String = None,
    save_tfevents: vh3.Boolean = False,
    tfevents_ttl: vh3.Integer = None,
    model_uid: vh3.String = None,
    model_scenario: vh3.String = None,
    produced_model_uid: vh3.String = None,
    permissions: vh3.String = None,
    metrics: vh3.JSON = None,
    data: typing.Union[vh3.File, vh3.JSON, vh3.MRTable, vh3.TSV, vh3.Text] = None,
    report_url: vh3.JSON = None,
    tfevents: typing.Sequence[typing.Union[vh3.Binary, vh3.File, vh3.MRFile, vh3.MRTable, vh3.Text]] = (),
) -> PulsarOutput:
    raise NotImplementedError("Write your local execution stub here")


class CollectPulsarWithMetricsGraph:
    def __init__(self,
                 resolve_slices_graph: ResolveSlicesGraph,
                 preclassifier_learning_graph: ClassifierTrainingGraph,
                 preclassifier_thresholds_graph: PreclassifierThresholdsGraph,
                 measure_quality_test_graph: MeasureQualityTestGraph):
        self.tsv_to_mr_table = tsv_to_mr_table(
            format='<columns=[id;winner_scenario;discarded_scenarios]>schemaful_dsv',
            tsv_data=measure_quality_test_graph.measure_mm_classifier_quality.requests_winners,
            **vh3.block_args(name='')
        )
        self.build_converter = build_arcadia_project(
            targets='alice/quality/train_graph/aggregate_table',
            arts='alice/quality/train_graph/aggregate_table/aggregate_table',
            arcadia_revision=vh3.context.arcadia_revision,
            strip_binaries=True,
            timestamp=vh3.context.timestamp_for_training,
            **vh3.block_args(name='Build converter')
        )
        self.aggregate_classification_results = aggregate_classification_results(
            executable=self.build_converter.arcadia_project,
            scenario_list=measure_quality_test_graph.measure_mm_classifier_quality.scenario_list,
            request_winner_scenarios=self.tsv_to_mr_table,
            request_ids=measure_quality_test_graph.sort_test,
            request_data_folder=vh3.context.basket_eval_folder,
            request_data_table_name=vh3.context.basket_eval_date,
            **vh3.block_args(name='')
        )
        self.input_to_option_1 = input_to_option(
            option_name='overall_quality',
            input=measure_quality_test_graph.measure_mm_classifier_quality.result,
            **vh3.block_args(name='')
        )
        self.input_to_option_2 = input_to_option(
            option_name='pre_quality',
            input=preclassifier_learning_graph.measure_mm_classifier_quality_1.result,
            **vh3.block_args(name='')
        )

        self.merge_json_files_1 = merge_json_files(
            input=[
                self.input_to_option_1,
                self.input_to_option_2,
            ],
            **vh3.block_args(name='')
        )
        self.python_executor_with_script_1 = python_executor_with_script(
            script=resource.find('collect_pulsar_with_metrics/thresholds_to_model_options_json.py').decode(),
            infiles=[preclassifier_thresholds_graph.measure_loss_1.out_json],
            **vh3.block_args(name='')
        )
        self.input_to_option_3 = input_to_option(
            option_name='model_options',
            input=self.python_executor_with_script_1.out_text,
            **vh3.block_args(name='')
        )
        self.python_executor_with_script_2 = python_executor_with_script(
            script=resource.find('collect_pulsar_with_metrics/slices_to_model_options_json.py').decode(),
            infiles=[resolve_slices_graph.mr_read_tsv_2],
            **vh3.block_args(name='')
        )
        self.options_to_json = options_to_json_7_key_values(
            key1='pre-zerod-factors',
            value1=vh3.context.force_zero_factors_pre,
            key2='post-zerod-factors',
            value2=vh3.context.force_zero_factors_post,
            key3='test-data-marks',
            value3=vh3.context.test_data_marks,
            key4='test-features',
            value4=vh3.context.test_features,
            key5='learn-features',
            value5=vh3.context.learn_features,
            **vh3.block_args(name='')
        )
        self.merge_json_files_2 = merge_json_files(
            input=[
                self.python_executor_with_script_2.out_json,
                self.options_to_json
            ],
            **vh3.block_args(name='')
        )
        self.python_executor_with_script_3 = python_executor_with_script(
            script=resource.find('collect_pulsar_with_metrics/thresholds_to_model_options_json.py').decode(),
            infiles=[self.merge_json_files_2],
            **vh3.block_args(name='')
        )
        self.input_to_option_4 = input_to_option(
            option_name='dataset_info',
            input=self.python_executor_with_script_3.out_text,
            **vh3.block_args(name='')
        )
        self.merge_json_files_3 = merge_json_files(
            input=[
                self.input_to_option_3,
                self.input_to_option_4
            ],
            **vh3.block_args(name='')
        )
        self.pulsar_add_instance = add_instance(
            model_name=vh3.context.model_name,
            dataset_name=vh3.context.dataset_name,
            model_version='0.0.2',
            dataset_info='{"feature_count": 9}',
            tags=vh3.context.tags,
            timestamp=vh3.context.timestamp_for_pulsar,
            **vh3.block_args(name='')
        )
        self.pulsar_update_instance = update_instance(
            instance_id='',
            user_datetime=vh3.Expr('${datetime.now}'),
            name=vh3.context.name,
            per_object_data_metainfo=resource.find('collect_pulsar_with_metrics/columns_result.json').decode(),
            metrics=self.merge_json_files_1,
            data=self.aggregate_classification_results,
            **vh3.block_args(name='', dynamic_options=[
                self.merge_json_files_3,
                self.pulsar_add_instance.info
            ])
        )
