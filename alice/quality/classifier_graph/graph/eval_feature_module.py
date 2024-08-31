import vh3

from .operations import cat_boost_eval_feature


class EvalFeatureModule:
    def __init__(self,
                 features: vh3.MRTable,
                 cd: vh3.TSV,
                 ignored_features: vh3.JSON):
        self.eval_feature = cat_boost_eval_feature(
            features=features,
            loss_function='CrossEntropy',
            cd=cd,
            features_to_evaluate=vh3.context.eval_features,
            fold_count=40,
            fold_size=10000,
            bootstrap_type='No',
            od_type='Iter',
            custom_metric='Accuracy',
            args='--od-wait 500',
            **vh3.block_args(name='', dynamic_options=[
                ignored_features,
            ])
        )
