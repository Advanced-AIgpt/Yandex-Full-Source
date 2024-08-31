import vh3
from .resolve_slices import ResolveSlicesGraph
from .classifiers_training import ClassifierTrainingGraph
from .preclassifier_thresholds import PreclassifierThresholdsGraph
from .operations import upload_formula_to_fml, fml_formula_to_json, build_formula_resource


class CollectResourcesWithFormulasGraph:
    def __init__(self,
                 scenarios: list,
                 resolve_slices_graph: ResolveSlicesGraph,
                 preclassifier_learning_graph: ClassifierTrainingGraph,
                 preclassifier_thresholds_graph: PreclassifierThresholdsGraph,
                 post_classifiers: dict):
        self.upload_preclassifier_formulas_to_fml = dict()
        self.upload_postclassifier_formulas_to_fml = dict()
        self.fml_preclassifier_formulas_to_json = dict()
        self.fml_postclassifier_formulas_to_json = dict()
        for scenario in scenarios:
            self.upload_preclassifier_formulas_to_fml[scenario] = upload_formula_to_fml(
                description_title=f'MM Preclassifier {scenario}',
                matrixnet_info=preclassifier_learning_graph.cat_boost_trains[scenario].model_bin,
                learn_pool=resolve_slices_graph.upload_pool_to_fml_1,
                **vh3.block_args(name='')
            )
            self.fml_preclassifier_formulas_to_json[scenario] = fml_formula_to_json(
                formula=self.upload_preclassifier_formulas_to_fml[scenario],
                **vh3.block_args(name='')
            )
            self.upload_postclassifier_formulas_to_fml[scenario] = upload_formula_to_fml(
                description_title=f'MM Postclassifier {scenario}',
                matrixnet_info=post_classifiers[scenario].model_bin,
                learn_pool=resolve_slices_graph.upload_pool_to_fml_1,
                **vh3.block_args(name='')
            )
            self.fml_postclassifier_formulas_to_json[scenario] = fml_formula_to_json(
                formula=self.upload_postclassifier_formulas_to_fml[scenario],
                **vh3.block_args(name='')
            )

        self.build_formula_resource = build_formula_resource(
            client_type=vh3.context.client_type,
            formula_resource_options=vh3.context.create_resource,
            experiment_name=vh3.context.name,
            source_formula_extension='bin',
            target_formula_extension='cbm',
            pre_formulas=vh3.Connections(**self.fml_preclassifier_formulas_to_json),
            post_formulas=vh3.Connections(**self.fml_postclassifier_formulas_to_json),
            thresholds=preclassifier_thresholds_graph.measure_loss_1.out_json
        )
