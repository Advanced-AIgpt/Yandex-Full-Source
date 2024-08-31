from functools import wraps
from typing import Literal, Optional

from vh3 import DefaultContext, Secret, String, block_args, Date, Expr, Enum, Binary, MRTable
from vh3.decorator import graph

import alice.beggins.internal.vh.flows.yql as yql
from alice.beggins.internal.vh.operations import ext


EmbedderType = Enum[Literal['Beggins', 'Zeliboba']]


class ArcContext(DefaultContext):
    arc_token: Secret = 'robot-beggins_arc-token'
    arc_reference: String = 'trunk'


class YtContext(DefaultContext):
    yt_token: Secret
    mr_account: String
    mr_default_cluster: Enum[
        Literal[
            'hahn',
            'freud',
            'marx',
            'hume',
            'arnold',
            'markov',
            'bohr',
            'landau',
        ]
    ] = 'hahn'
    mr_default_cluster_string: String = Expr('${global.mr_default_cluster}')


class YqlContext(YtContext):
    yql_token: Secret


class SandboxContext(DefaultContext):
    sandbox_token: Secret = 'robot-beggins_sandbox-token'


class CacheContext(DefaultContext):
    timestamp: Date
    timestamp_string: String = Expr('${global.timestamp}')


class PulsarContext(DefaultContext):
    pulsar_token: Secret = 'robot-beggins_pulsar-token'


class SoyContext(DefaultContext):
    soy_token: Secret = 'robot-beggins_soy-token'
    soy_pool: String = 'alice-beggins'
    soy_execution_timeout: String = '12h'


class TolokaContext(DefaultContext):
    tom_creation_url: Optional[String] = None
    classification_task_question: Optional[String] = None
    classification_project_instructions: Optional[String] = None
    positive_honeypots: Optional[String] = None
    negative_honeypots: Optional[String] = None


class NirvanaContext(DefaultContext):
    nirvana_token: Secret = 'robot-beggins_nirvana-token'


def static_name(name: str):
    def static_name_impl(fn):
        @wraps(fn)
        def wrapped(*args, **kwargs):
            return fn(*args, **kwargs, **block_args(name=name))

        return wrapped

    return static_name_impl


@static_name('eval catboost model')
@graph(workflow_id='https://nirvana.yandex-team.ru/flow/5a76792c-1274-473d-9706-4bea18af3fb1')
def eval_catboost_model(model: Binary, table: MRTable) -> MRTable:
    archive = ext.add_to_tar(
        path='model.cbm',
        file=model,
        **block_args(name='pack catboost model.cbm'),
    )
    return ext.yql_2(
        request=yql.EVAL_CATBOOST_ON_DATASET_REQUEST,
        input1=[table],
        files=[archive],
        **block_args(name='eval'),
    ).output1
