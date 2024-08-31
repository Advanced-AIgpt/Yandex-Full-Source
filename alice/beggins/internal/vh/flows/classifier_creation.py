import vh3
from vh3.decorator import graph

from alice.beggins.internal.vh.flows.repeatable_training import repeatable_training
from alice.beggins.internal.vh.flows.make_commit import make_commit

from alice.beggins.internal.vh.operations import ext

from alice.beggins.internal.vh.scripts.scripter import Scripter
from alice.beggins.internal.vh.scripts.python import (
    wait_fast_data
)

from alice.beggins.internal.vh.flows.classification import Context  # noqa: F401

DEFAULT_MANIFEST = '''
data:
  accept:
    sources: []
  train:
    sources: []
random_seed: 42
'''


@graph(workflow_id='https://nirvana.yandex-team.ru/flow/92eda56b-e6fe-42da-a191-f9bdd7801a67')
def classifier_creation(manifest: vh3.String = DEFAULT_MANIFEST):
    """
    Graph for whole classifier creation

    It contains:
        1. Dataset mining
        2. Model training
        3. Modul committing
        4. NLU testing(UE2E)
    """
    repeatable_training_result = repeatable_training(manifest=manifest)
    make_commit_result = make_commit(model_meta_info=repeatable_training_result.model_meta_info)
    with vh3.wait_for(make_commit_result):
        ext.python3_any_to_json(
            input_type='json-mem',
            input=repeatable_training_result.model_meta_info,
            body=Scripter.to_string(wait_fast_data.wait_fast_data),
            **vh3.block_args(name='Wait Fast Data'),
        )
