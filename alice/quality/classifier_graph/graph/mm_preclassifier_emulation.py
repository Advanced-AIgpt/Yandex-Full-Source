import vh3
from .operations import mm_preclassifier_emulator_catboost, \
    single_option_to_text_output


class MMPreclassificationEmulators:
    def __init__(self,
                 scenarios: list,
                 post_data_learn: vh3.TSV,
                 post_data_test: vh3.TSV,
                 slices_tsv: vh3.TSV,
                 preclassifiers: dict,
                 preclassifier_thresholds: vh3.JSON):
        pre_formulas = dict()
        for scenario in scenarios:
            pre_formulas[f'{scenario}_pre_formula'] = preclassifiers[scenario].model_bin
        self.mm_preclassifier_emulator_catboost_1 = mm_preclassifier_emulator_catboost(
            data=post_data_learn,
            slices=slices_tsv,
            thresholds=preclassifier_thresholds,
            timestamp=vh3.context.timestamp_for_training,
            arcadia_revision=vh3.context.arcadia_revision,
            max_ram=2000,
            max_disk=4096,
            **pre_formulas,
            **vh3.block_args(name='')
        )
        self.mm_preclassifier_emulator_catboost_2 = mm_preclassifier_emulator_catboost(
            data=post_data_test,
            slices=slices_tsv,
            thresholds=preclassifier_thresholds,
            timestamp=vh3.context.timestamp_for_training,
            arcadia_revision=vh3.context.arcadia_revision,
            max_ram=1000,
            max_disk=4096,
            **pre_formulas,
            **vh3.block_args(name='')
        )
        self.single_option_to_text_output_1 = single_option_to_text_output(
            input='0\tLabel\n1\tSampleId\n2\tTimestamp',
            **vh3.block_args(name='')
        )
