import vh3

from alice.beggins.internal.vh.operations import ext

from alice.beggins.internal.vh.flows.scrapper import scrape_queries_with_cache
from alice.beggins.internal.vh.flows.common import eval_catboost_model
from alice.beggins.internal.vh.flows.evaluation import unpack_model


@vh3.decorator.graph(workflow_id='https://nirvana.yandex-team.ru/flow/d37f6e68-1c18-479a-ae76-22564bd2837b')
def catboost_evaluation_using_raw_model(catboost_model: vh3.Binary, queries: vh3.MRTable) -> vh3.MRTable:
    """
    Evaluate `queries` using `catboost model`
    :param catboost_model: Catboost model
    :param queries: YT table with `text` column
    """
    embedded_queries = scrape_queries_with_cache(queries)
    scored_queries = eval_catboost_model(catboost_model, embedded_queries)
    return scored_queries


@vh3.decorator.graph(workflow_id='https://nirvana.yandex-team.ru/flow/5ee5d3e7-5253-464b-98d5-bfcbad1df2f9')
def catboost_evaluation_using_sandbox_id(sandbox_id: vh3.String, queries: vh3.MRTable) -> vh3.MRTable:
    """
    Evaluate `queries` using catboost model from sandbox
    :param sandbox_id: Sandbox resource id with catboost model
    :param queries: YT table with `text` column
    """
    catboost_model_package = ext.get_sandbox_resource(resource_id=sandbox_id)
    catboost_model = unpack_model(catboost_model_package)
    return catboost_evaluation_using_raw_model(catboost_model, queries)
