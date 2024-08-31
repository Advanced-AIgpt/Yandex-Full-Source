import vh3
from typing import Literal, NamedTuple


class DcMinerOutput(NamedTuple):
    dev: vh3.MRTable
    accept: vh3.MRTable


@vh3.decorator.external_graph("https://nirvana.yandex-team.ru/process/26364608-1c6f-43df-b785-8f24e9f44923")
def dc_miner(
    *,
    config: vh3.JSON,
    yt_token: vh3.Secret = None,
    yql_token: vh3.Secret = None,
    mr_account: vh3.String = None,
    mr_default_cluster: vh3.Enum[
        Literal[
            "hahn",
            "freud",
            "marx",
            "hume",
            "arnold",
            "markov",
            "bohr",
            "landau",
            "seneca-vla",
            "seneca-sas",
            "seneca-man",
        ]
    ] = "hahn",
    week: vh3.String = None,
    st_ticket: vh3.String = None,
    abc_service: vh3.String = "1459"
) -> DcMinerOutput:
    """
    dc miner
    """
    raise NotImplementedError("Write your local execution stub here")
