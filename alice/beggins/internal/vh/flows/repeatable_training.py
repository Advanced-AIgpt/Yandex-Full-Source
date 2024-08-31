import typing

import vh3

from vh3.decorator import graph

from typing import Union, Sequence

from alice.beggins.internal.vh.operations import ext

from alice.beggins.internal.vh.flows.get_toloka_parameters import get_toloka_parameters
from alice.beggins.internal.vh.flows.classification import train_binary_classifier, train_and_export_binary_classifier
from alice.beggins.internal.vh.flows.classification import Context  # noqa: F401
from alice.beggins.internal.vh.flows.negatives_sampler import negatives_sampler
from alice.beggins.internal.vh.flows.toloka_statistics import toloka_statistics_with_condition
from alice.beggins.internal.vh.flows.report import send_report_to_ticket

DEFAULT_MANIFEST = '''
data:
  accept:
    sources: []
  train:
    sources: []
random_seed: 42
'''


def get_mark_up_dataset_script(source: str) -> vh3.Expr:
    return vh3.Expr(f'''
PRAGMA yt.InferSchema = '99';

INSERT INTO {{{{output1}}}}
SELECT
       text,
       target,
       '{source}' AS source,
FROM {{{{input1}}}};
''')


class MineSamplesByPreTrainedClassifierResult(typing.NamedTuple):
    manifest: vh3.File
    mined_dataset: vh3.MRTable


def mine_samples_by_pre_trained_classifier(
    manifest: Union[vh3.Binary, vh3.File, vh3.JSON] = None,
    train: Sequence[vh3.MRTable] = (),
    accept: Sequence[vh3.MRTable] = (),
    toloka_parameters: vh3.JSON = None,
) -> vh3.MRTable:
    training_result = train_binary_classifier(
        manifest=manifest,
        train=train,
        accept=accept,
        **vh3.block_args(name='first iteration'),
    )
    negatives = negatives_sampler(
        training_result.year_logs_matches,
        **vh3.block_args(name='active learning sampler', dynamic_options=toloka_parameters),
    )
    negatives = ext.yql_2(
        input1=(negatives, ),
        request=get_mark_up_dataset_script(source='negatives for first evaluation'),
        **vh3.block_args(name='convert to correct format'),
    ).output1
    return MineSamplesByPreTrainedClassifierResult(
        manifest=training_result.manifest,
        mined_dataset=negatives,
    )


class RepeatableTrainingResult(typing.NamedTuple):
    model_meta_info: vh3.JSON


@graph(workflow_id='https://nirvana.yandex-team.ru/flow/d1b12ee3-19dd-471f-a103-0cb2ecb395c8')
def repeatable_training(manifest: vh3.String = DEFAULT_MANIFEST) -> RepeatableTrainingResult:
    """
    1. Train binary classifier using the dataset specified in manifest file.
    2. Search for negatives from the matches on year logs.
    3. Add these negatives to train dataset and repeat binary classifier training
    :return:
    """
    manifest = ext.single_option_to_file_output(
        input=vh3.Expr(f'{manifest:expr}'),
        **vh3.block_args(name='get manifest file'),
    )

    toloka_parameters = get_toloka_parameters()

    first_training_result = mine_samples_by_pre_trained_classifier(
        manifest=manifest,
        toloka_parameters=toloka_parameters,
    )
    second_training_result = train_and_export_binary_classifier(
        manifest=first_training_result.manifest,
        train=(first_training_result.mined_dataset, ),
        **vh3.block_args(name='second iteration', dynamic_options=toloka_parameters),
    )
    toloka_statistics_with_condition(
        second_training_result.week_logs_matches,
        **vh3.block_args(name='toloka statistics', dynamic_options=toloka_parameters),
    )
    send_report_to_ticket(report=second_training_result.report)

    return RepeatableTrainingResult(
        model_meta_info=second_training_result.model_meta_info
    )
