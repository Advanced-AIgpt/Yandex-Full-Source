from dict.mt.make.libs.block import block
from dict.mt.make.libs.common import MTFSUpdate, MTFS
from dict.mt.make.libs.types import MTPath

import vh


def calc_informativeness(input_table, yt_token):
    apply_multiple_nlg_dssm_models = vh.op(name='Apply Multiple NLG DSSM Models', id='e0e442d6-bbf7-4cc1-b41c-8203c0740d65')

    calc_informativeness = vh.op(name='Calc Informativeness', id='dcc8c2f1-0bb2-478d-a4f5-6298fdcfe056')

    dssm_model = vh.data(id='a574314a-cdcd-4375-8b29-90d831e5ea25')

    applied_dssm_table = apply_multiple_nlg_dssm_models(
        input=input_table,
        model=dssm_model,
        max_ram=10000,
        job_count=1,  # TODO
        lang='ru',
        batch_size=100,
        mr_account='tmp',
        yt_token=yt_token
    )['output']

    output = calc_informativeness(
        input=applied_dssm_table,
        negative_contexts=vh.YTTable('//home/voice/nzinov/mutual_information/negatives'),  # fixed table to calculate informativeness
        max_ram=10000,
        mr_account='tmp',
        yt_token=yt_token,
        thread_count=8,
        yt_table_outputs=[
            'output'
        ]
    )['output']

    return output


@block
class CalcInformativenessPipeline:
    input_table_path: MTPath
    output_table_path: MTPath
    yt_token: str

    def __call__(self, mtdata: MTFS) -> MTFSUpdate:
        x = mtdata.get_table(self.input_table_path)

        output = calc_informativeness(x, yt_token=self.yt_token)

        return {
            self.output_table_path: output
        }
