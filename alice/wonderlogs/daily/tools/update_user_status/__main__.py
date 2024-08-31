import datetime
import uuid

import click

import yt.wrapper

CENSORED_TABLES_SCHEMA = [
    {'name': 'puid', 'type_v3': 'string', 'sort_order': 'ascending'},
    {'name': 'completed', 'type_v3': 'bool'},
    {'name': 'tables', 'type_v3': {'type_name': 'list', 'item': 'string'}}
]


@yt.wrapper.with_context
class Reducer:
    def __init__(self, tables):
        self._tables = tables

    def __call__(self, key, input_row_iterator, context):
        private_user = None
        censored_tables = None
        for input_row in input_row_iterator:
            if context.table_index == 0:
                private_user = input_row
            elif context.table_index == 1:
                censored_tables = input_row
            else:
                raise RuntimeError('Unknown table index')

        if censored_tables is not None and censored_tables['completed']:
            yield censored_tables
        else:
            new_censored_tables = {'puid': private_user['puid'],
                                   'completed': False,
                                   'tables': censored_tables['tables'] if censored_tables else []}

            private_until_ts = datetime.datetime.fromtimestamp(private_user['private_until_time_ms'] // 1000)

            for censored_table in self._tables:
                ts = datetime.datetime.strptime(censored_table.split('/')[-1], '%Y-%m-%d')
                if ts <= private_until_ts:
                    new_censored_tables['tables'].append(censored_table)

            cur_ts = datetime.datetime.strptime('2017-04-21', '%Y-%m-%d')
            set_censored_tables = set(t.split('/')[-1] for t in new_censored_tables['tables'])
            completed = True

            while cur_ts <= private_until_ts and completed:
                completed = completed and cur_ts.strftime('%Y-%m-%d') in set_censored_tables
                cur_ts += datetime.timedelta(days=1)

            new_censored_tables['completed'] = completed

            yield new_censored_tables


@click.command()
@click.option('--private-users')
@click.option('--censored-tables')
@click.option('--yt-tmp-dir')
@click.option('--dialogs-tables', multiple=True)
@click.option('--yt-token')
def main(private_users, censored_tables, yt_tmp_dir, dialogs_tables, yt_token):
    yt.wrapper.config['token'] = yt_token
    yt.wrapper.config.set_proxy('hahn')

    reducer = Reducer(dialogs_tables)

    with yt.wrapper.Transaction():
        output_table = yt.wrapper.TablePath(yt_tmp_dir + 'output-table-' + str(uuid.uuid4()),
                                            schema=CENSORED_TABLES_SCHEMA)
        if not yt.wrapper.exists(censored_tables):
            yt.wrapper.create('table', censored_tables, attributes={'schema': CENSORED_TABLES_SCHEMA}, recursive=True)
        yt.wrapper.lock(censored_tables, waitable=True)

        yt.wrapper.run_reduce(
            reducer,
            source_table=[private_users, censored_tables],
            destination_table=output_table,
            sort_by=['puid'],
            reduce_by=['puid'])

        yt.wrapper.move(output_table, censored_tables, force=True, recursive=True)


if __name__ == '__main__':
    main()
