import vh3
from typing import Literal, NamedTuple


class CustomGeneratorOutput(NamedTuple):
    dev: vh3.MRTable
    accept: vh3.MRTable


@vh3.decorator.external_graph("https://nirvana.yandex-team.ru/process/6c12913d-2f0c-424f-b22c-fe4088b81b23")
def custom_generator(
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
) -> CustomGeneratorOutput:
    """
    custom generator
    """
    raise NotImplementedError("Write your local execution stub here")
