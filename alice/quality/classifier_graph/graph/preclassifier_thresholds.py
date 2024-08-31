import vh3
import json
import library.python.resource as resource
from .data_preparation import DataPreparationGraph
from .classifiers_training import ClassifierTrainingGraph
from .operations import mr_read_tsv, single_option_to_json_output, single_option_to_file_output, python_executor


class PreclassifierThresholdsGraph:
    def __init__(self,
                 scenarios: list,
                 all_scenarios: list,
                 scenarios_confident: dict,
                 scenarios_recall_precision: dict,
                 data_preparation_graph: DataPreparationGraph,
                 preclassifier_learning_graph: ClassifierTrainingGraph,
                 postclassifier_learning_graph: ClassifierTrainingGraph):
        self.mr_read_tsv_1 = mr_read_tsv(
            columns=['request_text'],
            table=data_preparation_graph.yql_1_blocks[2].output1,
            output_escaping=False,
            timestamp=vh3.context.timestamp_for_training,
            **vh3.block_args(name='')
        )
        self.mr_read_tsv_2 = mr_read_tsv(
            columns=['forced_confident'],
            table=data_preparation_graph.yql_1_blocks[2].output1,
            output_escaping=False,
            timestamp=vh3.context.timestamp_for_training,
            **vh3.block_args(name='')
        )
        self.single_option_to_json_output_1 = single_option_to_json_output(
            input=json.dumps(scenarios_confident),
            **vh3.block_args(name='')
        )
        self.single_option_to_json_output_2 = single_option_to_json_output(
            input=json.dumps(scenarios_recall_precision),
            **vh3.block_args(name='')
        )
        self.single_option_to_file_output_1 = single_option_to_file_output(
            input=resource.find('preclassifier_thresholds/merge_predicts.py').decode(),
            **vh3.block_args(name='')
        )
        predict_connections = dict()
        for scenario in scenarios:
            predict_connections[f'pre_{scenario}'] = preclassifier_learning_graph.python3_any_to_tsvs_2_named[scenario]
            if postclassifier_learning_graph is not None:
                predict_connections[f'post_{scenario}'] = postclassifier_learning_graph.python3_any_to_tsvs_2_named[scenario]
        self.collect_predicts = python_executor(
            max_ram=1000,
            infiles=vh3.Connections(**predict_connections),
            script=self.single_option_to_file_output_1,
            **vh3.block_args(name='Collect predicts')
        )
        self.single_option_to_file_output_2 = single_option_to_file_output(
            input=resource.find('preclassifier_thresholds/thresholds_selection.py').decode(),
            **vh3.block_args(name='')
        )
        self.measure_loss_1 = python_executor(
            max_ram=1000,
            infiles=vh3.Connections(
                self.collect_predicts.out_text,
                thresholds=self.single_option_to_json_output_1,
                grid=self.single_option_to_json_output_2,
                texts=self.mr_read_tsv_1,
                forced_confident=self.mr_read_tsv_2
            ),
            script=self.single_option_to_file_output_2,
            **vh3.block_args(name='Measure loss (1)')
        )
