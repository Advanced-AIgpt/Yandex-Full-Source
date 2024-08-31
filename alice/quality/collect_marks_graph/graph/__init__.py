import json
import vh3
from vh3.yt import get_mr_table
from .operations import mr_move_table, yql_1, yql_2, collect_ue2e_marks_for_one_scenario
from .queries import reqs_marks_query, join_all_query, insert_marks_query


class CollectMarksContext(vh3.DefaultContext):
    cache_sync: vh3.Integer
    timestamp: vh3.String
    data_part: vh3.String
    prod_url: vh3.String
    hitman_labels: vh3.MultipleStrings
    yql_token: vh3.Secret
    marks_path: vh3.String
    requests_path: vh3.String
    eval_path: vh3.String
    yt_pool: vh3.String


def build_graph(*, scenario_flags, skip_existing=True):
    get_mr_table_requests = get_mr_table(
        table=vh3.context.requests_path,
        cluster='hahn',
        **vh3.block_args(name='Get requests')
    )
    if skip_existing:
        get_mr_table_marks = get_mr_table(
            table=vh3.context.marks_path,
            cluster='hahn',
            **vh3.block_args(name='Get marks')
        )
        yql_2_reqs_marks = yql_2(
            input1=[get_mr_table_requests],
            input2=[get_mr_table_marks],
            request=reqs_marks_query(),
            **vh3.block_args(name='YQL 2')
        )
        input_basket = yql_2_reqs_marks.output1
    else:
        input_basket = get_mr_table_requests
    ue2e_marks_scenarios = dict()
    for scenario_name in scenario_flags.keys():
        ue2e_marks_scenarios[scenario_name] = collect_ue2e_marks_for_one_scenario(
            scenario_name=scenario_name,
            data_part=vh3.context.data_part,
            input_basket=input_basket,
            cache_sync=vh3.context.cache_sync,
            prod_url=vh3.context.prod_url,
            prod_experiments=json.dumps(scenario_flags[scenario_name]),
            hitman_labels=vh3.context.hitman_labels,
            eval_data_folder=vh3.context.eval_path,
            **vh3.block_args(name=scenario_name.capitalize())
        )
    yql_1_join_all = yql_1(
        input1=ue2e_marks_scenarios,
        request=join_all_query(scenario_flags.keys()),
        param=[vh3.Expr(f'requests=`{vh3.context.requests_path:expr}`')],
        **vh3.block_args(name='YQL 1')
    )
    mr_move_table_1 = mr_move_table(
        src=yql_1_join_all.output1,
        dst_path=vh3.Expr(f'{vh3.context.eval_path:expr}/patch_{vh3.context.data_part:expr}'),
        **vh3.block_args(name='MR Move Table')
    )
    _ = yql_1(
        input1=[mr_move_table_1],
        request=insert_marks_query(scenario_flags.keys()),
        param=[vh3.Expr(f'marks=`{vh3.context.marks_path:expr}`')],
        **vh3.block_args(name='YQL 1')
    )
