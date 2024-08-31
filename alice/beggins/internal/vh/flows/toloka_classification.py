from alice.beggins.internal.vh.operations import ext

from alice.beggins.internal.vh.scripts.scripter import Scripter
from alice.beggins.internal.vh.scripts.python import (
    toloka_request_script
)

import vh3
from vh3 import context as ctx

from vh3.decorator import graph


@graph(workflow_id='https://nirvana.yandex-team.ru/flow/87c362cd-04ae-4e05-b575-1a2bcade37d9')
def toloka_classification(
    queries: vh3.MRTable,
    positive_honeypots: vh3.String = vh3.Factory(lambda: ctx.positive_honeypots),
    negative_honeypots: vh3.String = vh3.Factory(lambda: ctx.negative_honeypots),
    classification_task_question: vh3.String = vh3.Factory(lambda: ctx.classification_task_question),
    classification_project_instructions: vh3.String = vh3.Factory(lambda: ctx.classification_project_instructions)
) -> vh3.MRTable:
    """
    Toloka Classification of queries for scenario activation
    :param queries: table with queries, column name with queries is 'text'
    :param positive_honeypots: positives of scenario activation
    :param negative_honeypots: negatives of scenario activation
    :param classification_task_question: question for Toloka User
    :param classification_project_instructions: the instruction for Toloka User
    :return:
    """
    positives = ext.single_option_to_text_output(
        input=positive_honeypots,
        **vh3.block_args(name='get positives honeypots'),
    )
    negatives = ext.single_option_to_text_output(
        input=negative_honeypots,
        **vh3.block_args(name='get negatives honeypots'),
    )

    toloka_request = ext.python3_any_any_any_to_json(
        input0_type='json-mem', input1_type='txt-mem', input2_type='txt-mem',
        body=Scripter.to_string(toloka_request_script.toloka_request),
        input0=queries, input1=positives, input2=negatives,
        **vh3.block_args(name='build toloka request'),
    )

    toloka_response = ext.alice_queries_binary_classification(
        dc_process_name=classification_task_question,
        classification_task_question=classification_task_question,
        classification_project_instructions=classification_project_instructions,
        data_input=toloka_request
    ).data_output

    return ext.mr_write_json_to_directory_create_new(
        json=toloka_response,
        **vh3.block_args(name='json to mr table'),
    )
