import re
import string

import vh
import yt.wrapper as yt
from dict.mt.make.libs.block import block
from dict.mt.make.libs.common import MTFSUpdate, MTFS
from dict.mt.make.libs.types import MTPath
from typing import Optional

import numpy as np
import json


def remove_double_spaces(str):
    return re.sub(' +', ' ', str)


def punct_separate(str):
    for c in string.punctuation:
        str = str.replace(c, ' {} '.format(c))
    return remove_double_spaces(str)


def punct_removal(str):
    for c in string.punctuation:
        str = str.replace(c, '')
    return remove_double_spaces(str)


def analyze(rows):
    lens, lens_by_words, lens_only_words, questions, repeating_words = [], [], [], [], []

    unique_words = set()
    unique_replies = set()

    len_rows = 0
    for row in rows:
        reply = row['reply']

        reply = punct_separate(reply)

        unique_replies.add(reply)

        lens.append(len(reply))
        lens_by_words.append(len(reply.split(' ')))
        questions.append(1 if '?' in reply else 0)

        new_reply = reply

        new_reply = punct_removal(new_reply)

        lens_only_words.append(len(new_reply.split(' ')))

        for w in new_reply.split(' '):
            unique_words.add(w)

        repeating_words.append(1 if len(list(new_reply.split(' '))) != len(set(new_reply.split(' '))) else 0)
        len_rows += 1

    return [
        ('Avg len:', np.mean(lens)),
        ('Avg words + punct:', np.mean(lens_by_words)),
        ('Avg words:', np.mean(lens_only_words)),
        ('Avg questions:', np.mean(questions)),
        ('Avg with repeating words:', np.mean(repeating_words)),
        ('Unique words / size:', len(unique_words) / len_rows),
        ('Unique replies / size:', len(unique_replies) / len_rows)
    ]


@vh.lazy.hardware_params(vh.HardwareParams(max_ram=2000, gpu_count=1, gpu_type=vh.GPUType.CUDA_7_0))  # remove GPU
@vh.lazy.from_annotations
def analyze_bucket_op_lazy(
        input_bucket: vh.YTTable,
        output_report: vh.mkoutput(vh.File),
        informativeness_bucket: vh.mkinput(vh.YTTable, nargs='?'),
        dssm_factor_bucket: vh.mkinput(vh.YTTable, nargs='?')
):
    rows = yt.read_table(str(input_bucket))
    rows_len = yt.row_count(str(input_bucket))

    report = dict(analyze(rows))

    if len(informativeness_bucket) > 0:
        informativeness_bucket = informativeness_bucket[0]
        assert rows_len == yt.row_count(str(informativeness_bucket))
        informativeness_scores = list([row[b'inf_score'] for row in
                                       yt.read_table(str(informativeness_bucket), yt.YsonFormat(encoding=None))])
        report['avg_informativeness'] = np.average(informativeness_scores)

    if len(dssm_factor_bucket) > 0:
        dssm_factor_bucket = dssm_factor_bucket[0]
        assert rows_len == yt.row_count(str(dssm_factor_bucket))
        factor_dssm_scores = list([row['factor_dssm_score'] for row in yt.read_table(str(dssm_factor_bucket))])
        report['avg_factor_dssm_score'] = np.average(factor_dssm_scores)

    with open(str(output_report), 'w') as f:
        json.dump(report, f)

    return output_report


def analyze_buckets(input_table, informativeness_bucket=None, dssm_factor_bucket=None, report_to_pulsar=False):
    output_report = vh.File('tmp.ytfile')

    output_report = analyze_bucket_op_lazy(
        input_bucket=input_table,
        output_report=output_report,
        informativeness_bucket=[informativeness_bucket] if informativeness_bucket else [],
        dssm_factor_bucket=[dssm_factor_bucket] if dssm_factor_bucket else [],
    )

    if report_to_pulsar:
        vh.op(name='Pulsar: Add Instance', id='4a883d63-796d-46bd-90a7-dcf53d2eb63b')(
            metrics=output_report,
            model_name='_TestGenerativeBoltalkaModel',
            dataset_name='_TestGenerativeBoltalkaDataset'
        )

    return output_report


@block
class AnalyzeBucketsPipeline:
    input_bucket_path: MTPath
    output_file_path: MTPath
    informativeness_bucket_path: Optional[MTPath] = None
    dssm_factor_bucket_path: Optional[MTPath] = None
    report_to_pulsar: bool = False

    def __call__(self, mtdata: MTFS) -> MTFSUpdate:
        table = mtdata.get_table(self.input_bucket_path)
        output_report = analyze_buckets(
            table,
            informativeness_bucket=mtdata.get_table(self.informativeness_bucket_path) if self.informativeness_bucket_path else None,
            dssm_factor_bucket=mtdata.get_table(self.dssm_factor_bucket_path) if self.dssm_factor_bucket_path else None,
            report_to_pulsar=self.report_to_pulsar
        )

        return {
            self.output_file_path: output_report
        }
