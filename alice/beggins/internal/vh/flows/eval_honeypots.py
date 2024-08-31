import vh3

from alice.beggins.internal.vh.operations import ext

from alice.beggins.internal.vh.flows.scrapper import scrape_queries_with_cache
from alice.beggins.internal.vh.flows.common import eval_catboost_model

from alice.beggins.internal.vh.scripts.scripter import Scripter
from alice.beggins.internal.vh.scripts.python import (
    validate_honeypots_evaluation
)


def honeypots_to_table_script() -> vh3.Expr:
    return vh3.Expr('''
$positives = {{file:/positive_honeypots->quote()}};
$negatives = {{file:/negative_honeypots->quote()}};

INSERT INTO {{output1}}
SELECT *
FROM (SELECT String::SplitToList($positives, '\n') AS text, 1 AS target)
FLATTEN BY text;

INSERT INTO {{output1}}
SELECT *
FROM (select String::SplitToList($negatives, '\n') AS text, 0 AS target)
FLATTEN BY text;
''')


@vh3.decorator.graph(workflow_id='https://nirvana.yandex-team.ru/flow/dc8e82e9-70ff-4e43-ac69-29bd2bb218c0')
def eval_honeypots(
    catboost_model: vh3.Binary,
    positive_honeypots: vh3.String = vh3.Factory(lambda: vh3.context.positive_honeypots),
    negative_honeypots: vh3.String = vh3.Factory(lambda: vh3.context.negative_honeypots),
    threshold: vh3.Number = None,
):
    """
    1. Evaluates honeypots(`positive_honeypots` and `negative_honeypots`) using `catboost_model`
    2. Validates the results of evaluation using `threshold`
    3. If there are false negatives(FNs) and false positives(FPs), then cube fails
    4. You can see FNs and FPs in the error message
    """
    positive_honeypots = vh3.echo(positive_honeypots, vh3.Text)
    negative_honeypots = vh3.echo(negative_honeypots, vh3.Text)
    honeypots = ext.yql_2(
        request=honeypots_to_table_script(),
        files=vh3.Connections(
            positive_honeypots=positive_honeypots,
            negative_honeypots=negative_honeypots,
        ),
    ).output1
    scraped_honeypots = scrape_queries_with_cache(honeypots)
    evaluated_honeypots = eval_catboost_model(catboost_model, scraped_honeypots)
    ext.python3_json_process(
        code=Scripter.to_string(validate_honeypots_evaluation),
        token1=vh3.context.yt_token,
        param1=vh3.Expr(threshold),
        mr=(evaluated_honeypots, ),
        **vh3.block_args(name='Validate Honeypots Evaluation'),
    )
