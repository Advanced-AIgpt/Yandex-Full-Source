import vh3
import library.python.resource as resource
from .operations import MmPreclassifierEmulatorCatboostOutput, cat_boost_train, python3_any_to_tsv, measure_mm_classifier_quality


class PostclassifierLearningFilterFactorsGraph:
    def __init__(self,
                 scenarios: list,
                 learn_mm_emulator: MmPreclassifierEmulatorCatboostOutput,
                 test_mm_emulator: MmPreclassifierEmulatorCatboostOutput,
                 measure_quality_executable: vh3.Executable,
                 cd: vh3.TSV,
                 ignored_features: vh3.JSON,
                 slices: vh3.JSON):
        self.cat_boost_trains = dict()
        for scenario in scenarios:
            self.cat_boost_trains[scenario] = cat_boost_train(
                learn=getattr(learn_mm_emulator, f'{scenario}_output'),
                gpu_type='ANY',
                loss_function='CrossEntropy',
                iterations=vh3.context.iterations,
                random_strength=0,
                bootstrap_type='No',
                od_type='Iter',
                custom_metric='Accuracy',
                fstr_type='PredictionValuesChange',
                output_columns=['SampleId', 'RawFormulaVal', 'Label', 'Timestamp'],
                args='--od-wait 1000',
                test=getattr(test_mm_emulator, f'{scenario}_output'),
                cd=cd,
                **vh3.block_args(name='', dynamic_options=[
                    ignored_features,
                    slices
                ])
            )
        self.python3_any_to_tsvs = dict()
        for scenario in scenarios:
            self.python3_any_to_tsvs[scenario] = python3_any_to_tsv(
                input_type='tsv-mem',
                body=resource.find('postclassifier_learning_filter_factors/reformat_from_catboost.py').decode(),
                input=self.cat_boost_trains[scenario].eval_result,
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
