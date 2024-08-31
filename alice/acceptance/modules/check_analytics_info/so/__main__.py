import click
import yt.wrapper as yt
from nile.api.v1 import (
    clusters,
    Record
)
from qb2.api.v1 import typing

from alice.acceptance.modules.check_analytics_info.lib.checkers import (
    MegamindAnalyticsInfoChecker,
    TunnellerChecker,
    MetaChecker
)

schema_diff_table = {
    'RequestId': typing.Optional[typing.String],
    'stable.VinsResponse': typing.Optional[typing.String],
    'test.VinsResponse': typing.Optional[typing.String],
}

SCHEMA_RESULT_TABLE = {
    'description': typing.Optional[typing.String],
    'message': typing.Optional[typing.String],
    'name': typing.Optional[typing.String],
    'verdict': bool
}


@click.command()
@click.option('--input-table')
@click.option('--result-table')
@click.option('--diff-table')
@click.option('--yql-token')
def main(input_table, result_table, diff_table, yql_token):
    cluster = clusters.yql.Hahn(yql_token=yql_token)
    yt.config.config['proxy']['url'] = 'hahn'
    job = cluster.job()
    checkers = [
        MegamindAnalyticsInfoChecker(),
        TunnellerChecker(),
        MetaChecker()
    ]
    diff_rows = []
    for rec in job.table(input_table).read():
        d = rec.to_dict()
        for checker in checkers:
            verdict, message = checker.process_row(d)
            verdict_col = '{}_verdict'.format(checker.NAME)
            message_col = '{}_message'.format(checker.NAME)
            d[verdict_col] = verdict
            d[message_col] = message
            schema_diff_table[verdict_col] = bool
            schema_diff_table[message_col] = typing.Optional[typing.String]
        diff_rows.append(Record.from_dict(d))
    if yt.exists(diff_table):
        yt.remove(diff_table)
    if yt.exists(result_table):
        yt.remove(result_table)
    cluster.write(diff_table, diff_rows, schema=schema_diff_table)
    results = (Record.from_dict(checker.dump_result()) for checker in checkers)
    cluster.write(result_table, results, schema=SCHEMA_RESULT_TABLE)
    job.run()

    assert all(r['verdict'] for r in results), 'analytics_info check failed'


if __name__ == '__main__':
    main()
