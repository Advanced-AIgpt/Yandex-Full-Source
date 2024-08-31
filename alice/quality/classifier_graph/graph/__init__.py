import vh3
import typing
from .data_preparation import DataPreparationGraph
from .enums import ClientTypeEnum, FormulaResourceOptionsEnum
from .resolve_slices import ResolveSlicesGraph
from .classifiers_training import ClassifierTrainingGraph
from .preclassifier_thresholds import PreclassifierThresholdsGraph
from .mm_preclassifier_emulation import MMPreclassificationEmulators
from .postclassifier_learning_filter_factors import PostclassifierLearningFilterFactorsGraph
from .measure_quality_test import MeasureQualityTestGraph
from .collect_resources_with_formulas import CollectResourcesWithFormulasGraph
from .collect_pulsar_with_metrics import CollectPulsarWithMetricsGraph
from .eval_feature import EvalFeatureGraph
from .operations import build_arcadia_project


class ClassifierGraphContext(vh3.DefaultContext):
    mr_output_ttl: vh3.Integer

    timestamp_for_pulsar: vh3.String
    timestamp_for_training: vh3.String
    timestamp_for_data_preparation: vh3.String

    arcadia_revision: vh3.Integer
    build_target_revision: vh3.Integer

    test_data_requests: vh3.String
    basket_eval_folder: vh3.String
    basket_eval_date: vh3.String
    test_data_marks: vh3.String
    train_data_marks: vh3.String
    learn_features: vh3.String
    test_features: vh3.String
    slices: vh3.String

    name: vh3.String
    tags: vh3.MultipleStrings
    force_zero_factors_pre: vh3.String
    force_zero_factors_post: vh3.String
    iterations: vh3.Integer

    model_name: vh3.String
    dataset_name: vh3.String

    client_type: ClientTypeEnum
    create_resource: FormulaResourceOptionsEnum

    build_target_patch: typing.Optional[vh3.String]
    build_target_additional_flags: typing.Optional[vh3.String]

    eval_features: typing.Optional[vh3.String] = None


all_scenarios = [
    "music",
    "video",
    "vins",
    "search",
    "gc"
]


def build_graph(
        scenarios,
        eval_features,
        scenarios_confident,
        scenarios_recall_precision,
        build_target_options,
        train_full_factors_postclassifier
):
    data_preparation_graph = DataPreparationGraph(
        scenarios=scenarios,
        all_scenarios=all_scenarios,
        build_target_options=build_target_options
    )
    resolve_slices_graph = ResolveSlicesGraph(
        data_preparation_graph=data_preparation_graph
    )

    preclassifier_learning_graph = ClassifierTrainingGraph(
        scenarios=scenarios,
        all_scenarios=all_scenarios,
        train_data=data_preparation_graph.yql_1_blocks[0].output1,
        val_data=data_preparation_graph.yql_1_blocks[1].output1,
        test_data=data_preparation_graph.yql_1_blocks[2].output1,
        ignored_features=resolve_slices_graph.process_python3_1.output1,
        slices=resolve_slices_graph.python3_any_to_json_1
    )
    if train_full_factors_postclassifier:
        postclassifier_learning_graph = ClassifierTrainingGraph(
            scenarios=scenarios,
            all_scenarios=all_scenarios,
            train_data=data_preparation_graph.yql_1_blocks[3].output1,
            val_data=data_preparation_graph.yql_1_blocks[4].output1,
            test_data=data_preparation_graph.yql_1_blocks[2].output1,
            ignored_features=resolve_slices_graph.process_python3_2.output1,
            slices=resolve_slices_graph.python3_any_to_json_1
        )
    else:
        postclassifier_learning_graph = None

    preclassifier_thresholds_graph = PreclassifierThresholdsGraph(
        scenarios=scenarios,
        all_scenarios=all_scenarios,
        scenarios_confident=scenarios_confident,
        scenarios_recall_precision=scenarios_recall_precision,
        data_preparation_graph=data_preparation_graph,
        preclassifier_learning_graph=preclassifier_learning_graph,
        postclassifier_learning_graph=postclassifier_learning_graph
    )
    mm_preclassifiers_emulators = MMPreclassificationEmulators(
        scenarios=scenarios,
        post_data_learn=data_preparation_graph.yql_1_blocks[3].output1,
        post_data_test=data_preparation_graph.yql_1_blocks[4].output1,
        slices_tsv=resolve_slices_graph.mr_read_tsv_2,
        preclassifiers=preclassifier_learning_graph.cat_boost_trains,
        preclassifier_thresholds=preclassifier_thresholds_graph.measure_loss_1.out_json,
    )
    if eval_features is None:
        measure_quality = build_arcadia_project(
            targets='alice/quality/train_graph/measure_quality',
            arts='alice/quality/train_graph/measure_quality/measure_quality',
            arcadia_revision=vh3.context.arcadia_revision,
            timestamp=vh3.context.timestamp_for_training,
            **vh3.block_args(name='')
        )
        postclassifier_learning_filter_factors_graph = PostclassifierLearningFilterFactorsGraph(
            scenarios=scenarios,
            measure_quality_executable=measure_quality.arcadia_project,
            learn_mm_emulator=mm_preclassifiers_emulators.mm_preclassifier_emulator_catboost_1,
            test_mm_emulator=mm_preclassifiers_emulators.mm_preclassifier_emulator_catboost_2,
            cd=mm_preclassifiers_emulators.single_option_to_text_output_1,
            ignored_features=resolve_slices_graph.process_python3_2.output1,
            slices=resolve_slices_graph.python3_any_to_json_1
        )
        measure_quality_test_graph = MeasureQualityTestGraph(
            scenarios=scenarios,
            all_scenarios=all_scenarios,
            measure_quality_executable=measure_quality.arcadia_project,
            data_preparation_graph=data_preparation_graph,
            resolve_slices_graph=resolve_slices_graph,
            preclassifier_learning_graph=preclassifier_learning_graph,
            preclassifier_thresholds_graph=preclassifier_thresholds_graph,
            post_classifiers=postclassifier_learning_filter_factors_graph.cat_boost_trains,
            cd=mm_preclassifiers_emulators.single_option_to_text_output_1
        )
        CollectResourcesWithFormulasGraph(
            scenarios=scenarios,
            resolve_slices_graph=resolve_slices_graph,
            preclassifier_learning_graph=preclassifier_learning_graph,
            preclassifier_thresholds_graph=preclassifier_thresholds_graph,
            post_classifiers=postclassifier_learning_filter_factors_graph.cat_boost_trains
        )
        CollectPulsarWithMetricsGraph(
            resolve_slices_graph=resolve_slices_graph,
            preclassifier_learning_graph=preclassifier_learning_graph,
            preclassifier_thresholds_graph=preclassifier_thresholds_graph,
            measure_quality_test_graph=measure_quality_test_graph
        )
    else:
        EvalFeatureGraph(
            scenarios=scenarios,
            pre_features=data_preparation_graph.yql_1_blocks[0].output1,
            pre_columns_descriptions=preclassifier_learning_graph.single_option_to_text_outputs,
            learn_mm_emulator=mm_preclassifiers_emulators.mm_preclassifier_emulator_catboost_1,
            ignored_features_pre=resolve_slices_graph.process_python3_1.output1,
            ignored_features_post=resolve_slices_graph.process_python3_2.output1,
        )
