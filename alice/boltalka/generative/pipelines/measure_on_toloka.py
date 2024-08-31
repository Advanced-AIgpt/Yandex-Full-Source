import vh

from dict.mt.make.libs.block import block
from dict.mt.make.libs.common import MTFS, MTFSUpdate
from dict.mt.make.libs.types import MTPath
from alice.boltalka.generative.pipelines.util import tmp_table_to_json_option


def measure_ds_score(table, yt_token):
    x = vh.op(name='MR map python', id='45215193-ac68-471b-a1db-ba1e0d172c14')(
        input=table,
        _options={
            'code': "if 'rewritten_reply' in rec:\n"
                    "    rec['reply'] = rec['rewritten_reply']\n"
                    "yield rec"
        },
        mr_account='tmp',
        yt_token=yt_token
    )['output']

    x = vh.op(name='MR Read TSV', id='ac31b077-33aa-4e74-99d6-0f8ddd8e25bb')(
        table=x,
        columns=['context_2', 'context_1', 'context_0', 'reply'],
        yt_token=yt_token
    )['tsv']

    x = vh.op(name='Make Toloka Input', id='26dbc87d-dd7f-4f9a-b233-39ce94c8380b')(
        pool=x,
        max_ram=10000,
        context_len=3,
        top_size=1
    )['output']
    x = vh.op(name='Python2 any to json', id='ba925515-9fb4-4a37-8c19-c025147cf3b1')(
        input=x,
        _options={
            'import': 'json,hashlib'
        },
        body='js = []\n'
             'for el in v:\n'
             '    el["source"] = "model"\n'
             '    if "score" in el:\n'
             '        del el["score"]\n'
             '    context = ["salt20200608", el["reply"]]\n'
             '    for i in range(9):\n'
             '        k = "context_{}".format(i)\n'
             '        if k not in el:\n'
             '            break\n'
             '        context.append(el[k])\n'
             '    el["key"] = hashlib.md5("\t".join(reversed(context))).hexdigest()\n'
             '    js.append(el)\n'
             'return js\n',
        input_type='json-mem',
        output_type='json'
    )['output']

    tmp_table = vh.op(name='Create MR temp table', id='f65ecf3a-60c9-4105-965e-36daecd7b546')(
        mr_account='tmp',
        yt_token=yt_token,
        mr_default_cluster='hahn'
    )['table']
    x = vh.op(name='Boltalka Evaluation, nature', id='58411b81-ef25-4147-b5fd-e2f0b6fef98a')(
        x,
        output_table_path='TO_BE_OVERRIDEN',
        priority=95,
        mr_account='tmp',
        yt_token=yt_token,
        lifeTimeInDays=365,
        reuse=True,
        abc_id=2738,  # TODO
        confidence_level=0,
        maxOverlap=3,
        _dynamic_options=[
            tmp_table_to_json_option('output_table_path', tmp_table),
        ],
        yt_table_outputs=[
            'table'
        ]
    )
    report, output_table = x['stat'], x['table']
    return report, output_table


@block
class MeasureDsScorePipeline:
    input_table_path: MTPath
    output_table_path: MTPath
    output_report_path: MTPath

    yt_token: str

    def __call__(self, mtdata: MTFS) -> MTFSUpdate:
        report, output_table = measure_ds_score(mtdata.get_table(self.input_table_path), self.yt_token)

        return {
            self.output_table_path: output_table,
            self.output_report_path: report
        }


@vh.lazy.hardware_params(vh.HardwareParams(max_disk=10000, max_ram=16384, gpu_count=1, gpu_type=vh.GPUType.CUDA_7_0))
@vh.lazy.from_annotations
def wtf(input: vh.File, output: vh.mkoutput(vh.File)) -> vh.File:
    with open(str(input)) as f_in, open(str(output), 'w') as f_out:
        f_out.write(f_in.read().replace('dst_table', 'dst-table'))
    return output


def measure_interestingness(table: vh.YTTable, yt_token, yql_token):
    tmp_table = vh.op(name='Create MR temp table', id='f65ecf3a-60c9-4105-965e-36daecd7b546')(
        mr_account='tmp',
        yt_token=yt_token,
        mr_default_cluster='hahn'
    )['table']
    output_table = vh.op(name='Boltalka Interestingness Toloka', id='b1638308-6e84-4933-9da7-0d38946ce3b8')(
        input_table=table,
        dst_table='TO_BE_OVERRIDEN',
        encryptedOauthToken='liksna_token',
        yt_token=yt_token,
        yt_table_outputs=['output_table'],
        _dynamic_options=[
            wtf(tmp_table_to_json_option('dst_table', tmp_table)),
        ]
    )['output_table']

    report = vh.op(name='Calc Boltalka Metric By Bins', id='b2f99b45-5894-4140-acd4-7c80bdf6be99')(
        input1=output_table,
        mr_account='tmp',
        yt_token=yt_token,
        yql_token=yql_token,
        metrics=['COUNT_IF(mark == "interesting") * 100.0 / COUNT(*)'],
        metric_names=['interest_score']
    )['out1']

    return output_table, report


@block
class MeasureInterestingnessPipeline:
    input_table_path: MTPath
    output_table_path: MTPath
    output_report_path: MTPath

    yt_token: str
    yql_token: str

    def __call__(self, mtdata: MTFS) -> MTFSUpdate:
        output_table, report = measure_interestingness(
            mtdata.get_table(self.input_table_path), self.yt_token, self.yql_token
        )

        return {
            self.output_table_path: output_table,
            self.output_report_path: report
        }
