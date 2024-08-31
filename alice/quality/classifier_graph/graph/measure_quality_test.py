import vh3
import library.python.resource as resource
from vh3.yt import get_mr_table
from vh3.lib.services.yql.ml_mariners.yql_ops import yql_4
from .data_preparation import DataPreparationGraph
from .resolve_slices import ResolveSlicesGraph
from .classifiers_training import ClassifierTrainingGraph
from .preclassifier_thresholds import PreclassifierThresholdsGraph
from .operations import build_mm_target, mr_sort, mm_preclassifier_emulator_catboost, cat_boost_apply, \
    cut_tsv_header, python3_any_to_tsv, measure_mm_classifier_quality
from .queries import measure_quality_test_cast_scenarios_query


class MeasureQualityTestGraph:
    def __init__(self,
                 scenarios: list,
                 all_scenarios: list,
                 measure_quality_executable: vh3.Executable,
                 data_preparation_graph: DataPreparationGraph,
                 resolve_slices_graph: ResolveSlicesGraph,
                 preclassifier_learning_graph: ClassifierTrainingGraph,
                 preclassifier_thresholds_graph: PreclassifierThresholdsGraph,
                 post_classifiers: dict,
                 cd: vh3.TSV):
        self.get_mr_table_features_filtered = get_mr_table(
            cluster='hahn',
            creation_mode='CHECK_EXISTS',
            table=vh3.context.test_features,
            **vh3.block_args(name='')
        )
        self.get_mr_table_marks = get_mr_table(
            cluster='hahn',
            creation_mode='CHECK_EXISTS',
            table=vh3.context.test_data_marks,
            **vh3.block_args(name='')
        )
        self.yql_2_1 = yql_4(
            request=resource.find('data_preparation/join_marks_features.yql').decode(),
            timestamp=vh3.context.timestamp_for_data_preparation,
            input1=[self.get_mr_table_features_filtered],
            input2=[self.get_mr_table_marks],
            **vh3.block_args(name='')
        )
        self.build_mm_target_1 = build_mm_target(
            executable=data_preparation_graph.build_target_builder.arcadia_project,
            input_table=self.yql_2_1.output1,
            max_ram=200,
            ignore_gc=False,
            raw_scores=True,
            cache_sync=vh3.context.timestamp_for_data_preparation,
            scenarios_list=','.join(scenarios),
            client=vh3.context.client_type,
            additional_flags=vh3.context.build_target_additional_flags,
            **vh3.block_args(name='')
        )
        self.yql_2_2 = yql_4(
            request=resource.find('measure_quality_test/deduplication.yql').decode(),
            input1=[self.build_mm_target_1],
            **vh3.block_args(name='TERRIBLE DEDUP CRUTCH')
        )
        self.sort_test = mr_sort(
            sort_by=['text', 'reqid'],
            srcs=[self.yql_2_2.output1],
            timestamp=vh3.context.timestamp_for_training,
            **vh3.block_args(name='Sort test')
        )
        self.yql_1_1 = yql_4(
            request=measure_quality_test_cast_scenarios_query(scenarios, all_scenarios),
            input1=[self.yql_2_2.output1],
            **vh3.block_args(name='')
        )
        pre_formulas = dict()
        for scenario in scenarios:
            pre_formulas[f'{scenario}_pre_formula'] = preclassifier_learning_graph.cat_boost_trains[scenario].model_bin
        self.mm_preclassifier_emulator_catboost = mm_preclassifier_emulator_catboost(
            data=self.yql_1_1.output1,
            slices=resolve_slices_graph.mr_read_tsv_2,
            thresholds=preclassifier_thresholds_graph.measure_loss_1.out_json,
            timestamp=vh3.context.timestamp_for_training,
            arcadia_revision=vh3.context.arcadia_revision,
            max_ram=1000,
            **pre_formulas,
            **vh3.block_args(name='')
        )
        self.cat_boost_applies = dict()
        self.cut_tsv_headers = dict()
        self.python3_any_to_tsvs = dict()
        for scenario in scenarios:
            self.cat_boost_applies[scenario] = cat_boost_apply(
                pool=getattr(self.mm_preclassifier_emulator_catboost, f'{scenario}_output'),
                model_bin=post_classifiers[scenario].model_bin,
                output_columns=['SampleId', 'RawFormulaVal', 'Label', 'Timestamp'],
                cd=cd,
                **vh3.block_args(name='')
            )
            self.cut_tsv_headers[scenario] = cut_tsv_header(
                input=self.cat_boost_applies[scenario].result,
                **vh3.block_args(name='')
            )
            self.python3_any_to_tsvs[scenario] = python3_any_to_tsv(
                input_type='tsv-mem',
                max_ram=8024,
                body=resource.find('measure_quality_test/reformat_from_catboost.py').decode(),
                input=self.cut_tsv_headers[scenario],
                **vh3.block_args(name='')
            )
        inputs = dict()
        for scenario in scenarios:
            key = scenario if scenario != 'vins' else 'other'
            inputs[key] = self.python3_any_to_tsvs[scenario]
        self.measure_mm_classifier_quality = measure_mm_classifier_quality(
            executable=measure_quality_executable,
            **inputs,
            **vh3.block_args(name='')
        )
