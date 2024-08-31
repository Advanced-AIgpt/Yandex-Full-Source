import vh3
from typing import Literal, NamedTuple


class DcTomOutput(NamedTuple):
    dev: vh3.MRTable
    accept: vh3.MRTable


@vh3.decorator.external_graph("https://nirvana.yandex-team.ru/process/67b29929-67df-421e-b9de-dae753a9bd98")
def dc_tom(
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
) -> DcTomOutput:
    """
    dc tom
    """
    raise NotImplementedError("Write your local execution stub here")
