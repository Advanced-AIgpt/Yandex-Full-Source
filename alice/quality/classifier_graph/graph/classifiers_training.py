import vh3
import library.python.resource as resource
from .operations import build_arcadia_project, single_option_to_text_output, python3_any_to_tsv, cat_boost_train, cat_boost_apply, \
    measure_mm_classifier_quality


def generate_label(all_scenarios, chosen_scenario):
    label = '0\tSampleId'
    for i in range(len(all_scenarios)):
        label += f'\n{i + 1}\t{"Label" if all_scenarios[i] == chosen_scenario else "Auxiliary"}'
    return label


class ClassifierTrainingGraph:
    def __init__(self,
                 scenarios: list,
                 all_scenarios: list,
                 train_data: vh3.MRTable,
                 val_data: vh3.MRTable,
                 test_data: vh3.MRTable,
                 ignored_features: vh3.JSON,
                 slices: vh3.JSON):
        self.single_option_to_text_outputs = dict()
        self.cat_boost_trains = dict()
        self.python3_any_to_tsvs_1 = dict()
        self.cat_boost_applies = dict()
        self.python3_any_to_tsvs_2_named = dict()
        for scenario in scenarios:
            self.single_option_to_text_outputs[scenario] = single_option_to_text_output(
                input=generate_label(all_scenarios, scenario),
                **vh3.block_args(name='')
            )
            self.cat_boost_trains[scenario] = cat_boost_train(
                learn=train_data,
                gpu_type='ANY',
                loss_function='CrossEntropy',
                iterations=vh3.context.iterations,
                random_strength=0,
                bootstrap_type='No',
                od_type='Iter',
                custom_metric='Accuracy',
                fstr_type='PredictionValuesChange',
                output_columns=['SampleId', 'RawFormulaVal', 'Label'],
                args='--od-wait 1000',
                test=val_data,
                cd=self.single_option_to_text_outputs[scenario],
                **vh3.block_args(name='', dynamic_options=[
                    ignored_features,
                    slices
                ])
            )
            self.python3_any_to_tsvs_1[scenario] = python3_any_to_tsv(
                input_type='tsv-mem',
                body=resource.find('prepostclassifier_learning/reformat_from_catboost.py').decode(),
                input=self.cat_boost_trains[scenario].eval_result,
                **vh3.block_args(name='')
            )
            self.cat_boost_applies[scenario] = cat_boost_apply(
                pool=test_data,
                model_bin=self.cat_boost_trains[scenario].model_bin,
                output_columns=['SampleId', 'RawFormulaVal', 'Label'],
                cd=self.single_option_to_text_outputs[scenario],
                **vh3.block_args(name='')
            )
            self.python3_any_to_tsvs_2_named[scenario] = python3_any_to_tsv(
                input_type='tsv-mem',
                body=resource.find('prepostclassifier_learning/reformat_from_catboost.py').decode(),
                input=self.cat_boost_applies[scenario].result,
                **vh3.block_args(name='')
            )

        measure_quality_executable = build_arcadia_project(
            targets='alice/quality/train_graph/measure_quality',
            arts='alice/quality/train_graph/measure_quality/measure_quality',
            arcadia_revision=vh3.context.arcadia_revision,
            timestamp=vh3.context.timestamp_for_training,
            **vh3.block_args(name='')
        ).arcadia_project

        inputs = dict()
        for scenario in scenarios:
            key = scenario if scenario != 'vins' else 'other'
            inputs[key] = self.python3_any_to_tsvs_1[scenario]
        self.measure_mm_classifier_quality_1 = measure_mm_classifier_quality(
            executable=measure_quality_executable,
            **inputs,
            **vh3.block_args(name='')
        )
