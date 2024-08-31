import vh3
from vh3.decorator import graph
from vh3.extras.json import dump

from alice.beggins.internal.vh.operations import ext

from alice.beggins.internal.vh.scripts.python import (
    modification_for_commit
)
from alice.beggins.internal.vh.scripts.scripter import Scripter


@graph(workflow_id='https://nirvana.yandex-team.ru/flow/1a6747f3-080d-48f8-8390-784084c939ab')
def make_commit(model_meta_info: vh3.JSON, need_make_commit: vh3.Boolean = False):
    need_make_commit = ext.light_groovy_json_filter_if(
        filter=vh3.Expr(f'"{need_make_commit:expr}" == "true"'),
        input=dump({}),
        **vh3.block_args(name='if need_make_commit'),
    )

    archive = ext.svn_checkout(
        arcadia_path='arcadia/alice/nlu/data/ru/',
        **vh3.block_args(
            name='SVN Checkout',
            dynamic_options=need_make_commit.output_true,
        ),
    ).archive

    modified_archive = ext.python3_any_any_any_to_binary(
        input0_type='file',
        input1_type='json-gen',
        input2_type='none',
        output_type='file',
        input0=archive,
        input1=model_meta_info,
        body=Scripter.to_string(modification_for_commit.commit_draft),
        **vh3.block_args(
            name='Data Modification',
            dynamic_options=need_make_commit.output_true,
        ),
    )

    ext.svn_commit(
        working_dir=modified_archive,
        commit_message='Beggins. For Beta Begemot __BYPASS_CHECKS__ SKIP_CHECK SKIP_REVIEW',
        publish_review=False,
        automerge_review=False,
        **vh3.block_args(name='SVN Commit'),
    )
