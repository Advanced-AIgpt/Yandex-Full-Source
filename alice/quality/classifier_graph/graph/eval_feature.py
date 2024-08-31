import vh3
from .eval_feature_module import EvalFeatureModule
from .operations import MmPreclassifierEmulatorCatboostOutput, single_option_to_text_output


class EvalFeatureGraph:
    def __init__(self,
                 scenarios: list,
                 pre_features: vh3.MRTable,
                 pre_columns_descriptions: dict,
                 learn_mm_emulator: MmPreclassifierEmulatorCatboostOutput,
                 ignored_features_pre: vh3.JSON,
                 ignored_features_post: vh3.JSON):
        self.eval_feature_pre = dict()
        self.eval_feature_post = dict()
        self.post_columns_description = single_option_to_text_output(
            input='0\tLabel\n1\tSampleId\n2\tAuxiliary',
            **vh3.block_args(name='')
        )
        for scenario in scenarios:
            self.eval_feature_pre[scenario] = EvalFeatureModule(
                features=pre_features,
                cd=pre_columns_descriptions[scenario],
                ignored_features=ignored_features_pre
            )
            self.eval_feature_post[scenario] = EvalFeatureModule(
                features=getattr(learn_mm_emulator, f'{scenario}_output'),
                cd=self.post_columns_description,
                ignored_features=ignored_features_post
            )
