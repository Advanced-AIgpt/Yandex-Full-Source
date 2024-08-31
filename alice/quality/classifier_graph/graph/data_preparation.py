import vh3
import library.python.resource as resource
from vh3.yt import get_mr_table
from vh3.lib.services.yql.ml_mariners.yql_ops import yql_4
from .operations import build_arcadia_project, build_mm_target
from .queries import data_preparation_cast_scenarios_query


class DataPreparationGraph:
    def __init__(self, scenarios: list, all_scenarios: list, build_target_options: dict):
        self.get_mr_table_learn_features = get_mr_table(
            cluster='hahn',
            creation_mode='CHECK_EXISTS',
            table=vh3.context.learn_features,
            **vh3.block_args(name='Get learn features')
        )
        self.get_mr_table_train_marks = get_mr_table(
            cluster='hahn',
            creation_mode='CHECK_EXISTS',
            table=vh3.context.train_data_marks,
            **vh3.block_args(name='Get train marks')
        )
        self.yql_4_filter_features = yql_4(
            request=resource.find('data_preparation/filter_features_post_win_reason.yql').decode(),
            timestamp=vh3.context.timestamp_for_data_preparation,
            input1=[self.get_mr_table_learn_features],
            **vh3.block_args(name='Filter WR')
        )
        self.yql_4_join_marks_features_1 = yql_4(
            request=resource.find('data_preparation/join_marks_features.yql').decode(),
            timestamp=vh3.context.timestamp_for_data_preparation,
            input1=[self.yql_4_filter_features.output1],
            input2=[self.get_mr_table_train_marks],
            **vh3.block_args(name='join marks and features')
        )
        self.yql_4_split_35 = yql_4(
            request=resource.find('data_preparation/sample_join_reqid_35.yql').decode(),
            timestamp=vh3.context.timestamp_for_data_preparation,
            input1=[self.yql_4_join_marks_features_1.output1],
            **vh3.block_args(name='Split 35/65')
        )
        self.yql_4_split_80 = yql_4(
            request=resource.find('data_preparation/sample_join_reqid_80.yql').decode(),
            timestamp=vh3.context.timestamp_for_data_preparation,
            input1=[self.yql_4_split_35.output2],
            **vh3.block_args(name='Split 80/20')
        )
        self.yql_4_split_70_10 = yql_4(
            request=resource.find('data_preparation/sample_three_reqid_joins.yql').decode(),
            timestamp=vh3.context.timestamp_for_data_preparation,
            input1=[self.yql_4_split_35.output1],
            **vh3.block_args(name='Split 70/10/20')
        )
        self.build_target_builder = build_arcadia_project(
            targets='alice/quality/build_target',
            arts='alice/quality/build_target/build_target',
            arcadia_revision=vh3.context.build_target_revision,
            timestamp=vh3.context.timestamp_for_training,
            arcadia_patch=vh3.context.build_target_patch,
            **vh3.block_args(name=''),
        )
        input_tables = [
            (self.yql_4_split_70_10.output1, 'pre-classification learn'),
            (self.yql_4_split_70_10.output2, 'pre-classification test'),
            (self.yql_4_split_70_10.output3, 'final validation'),
            (self.yql_4_split_80.output1, 'post-classification learn'),
            (self.yql_4_split_80.output2, 'post-classification test'),
        ]
        self.build_mm_targets = []
        self.yql_1_blocks = []
        for i in range(5):
            self.build_mm_targets.append(build_mm_target(
                executable=self.build_target_builder.arcadia_project,
                input_table=input_tables[i][0],
                max_ram=200,
                classification_bias=True,
                no_search_bias=build_target_options.get('no_search_bias', False),
                debug_target=True,
                search_from_vins=True,
                search_from_toloka=False,
                important_parts=False,
                ignore_gc=True,
                raw_scores=False,
                disable_vins_bias=False,
                add_gc=True,
                scenarios_list=','.join(scenarios),
                client=vh3.context.client_type,
                additional_flags=vh3.context.build_target_additional_flags,
                **vh3.block_args(name=f'Build MM target for {input_tables[i][1]}')
            ))
            self.yql_1_blocks.append(yql_4(
                request=data_preparation_cast_scenarios_query(scenarios, all_scenarios),
                input1=[self.build_mm_targets[i]],
                **vh3.block_args(name=f'YQL 1 for {input_tables[i][1]}')
            ))
