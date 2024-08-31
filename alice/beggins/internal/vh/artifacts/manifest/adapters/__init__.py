from typing import Optional, Literal

from vh3 import MRTable, String, Secret, Enum
from vh3.decorator import operation, autorelease_to_nirvana_on_trunk_commit

import alice.beggins.internal.vh.artifacts.manifest.adapters.ext as ext


@operation(owner='robot-voiceint', deterministic=True)
@autorelease_to_nirvana_on_trunk_commit(
    version='https://nirvana.yandex-team.ru/alias/operation/beggins_top_of_mind_adapter/0.0.2',
)
def top_of_mind_adapter(
    table: MRTable,
    mr_account: String,
    yt_token: Secret,
    yql_token: Secret,
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
            'seneca-vla',
            'seneca-sas',
            'seneca-man',
        ]
    ] = 'hahn',
    timestamp: String = None,
    source: Optional[String] = None,
) -> MRTable:
    source_value = 'NULL'
    if source is not None:
        source_value = f'"{source}"'
    return ext.yql_2(
        mr_account=mr_account,
        yt_token=yt_token,
        yql_token=yql_token,
        mr_default_cluster=mr_default_cluster,
        timestamp=timestamp,
        request=f"""
            $source = {source_value};

            INSERT INTO {{{{output1}}}}
            SELECT DISTINCT
                text,
                1.0 AS target,
                $source AS source,
            FROM
                {{{{input1}}}}
            WHERE text IS NOT NULL
        """,
        input1=[table],
    ).output1
