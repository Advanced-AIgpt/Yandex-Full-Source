from math import ceil
import pandas as pd
import yt.wrapper as yt
import gc
import time
import asyncio
from datetime import datetime
from yql.api.v1.client import YqlClient
from enum import Enum
import random


class RacingJoin:
    """code generation for lookup join in batches"""
    BATCH_SIZE_LIMIT = 999  # set by YT as yt.MaxKeyRangeCount, see YTADMINREQ-27142

    # presets for popular logs and use cases
    # из логов выбираются поля ['logs_fields']
    # для джойна используются поля ['logs_keys'] в логах
    # они джойнятся с полями из ['logs_keys_source'] во входящей таблице с ключами
    # все группы полей можно заменять через словарик custom_join_params

    presets = {
        'vins_logs_dialogs': dict(
            logs_path='//home/alice/wonder/dialogs',
            logs_fields=['sequence_number', 'experiments', 'form_name', 'request', 'response', 'analytics_info',
                         'server_time_ms', 'uuid', 'message_id'],
            logs_keys=['server_time_ms', 'uuid'],
            logs_keys_source=['server_time_ms', 'uuid'],
        ),
        'wonder_logs': dict(
            logs_path='//home/alice/wonder/logs',
            logs_fields=['_server_time_ms', '_uuid', '_megamind_request_id', '_sequence_number', 'asr', '_message_id'],
            logs_keys=['_server_time_ms', '_uuid', '_message_id'],
            logs_keys_source=['server_time_ms', '_uuid', 'message_id', 'uuid', '_message_id', '_server_time_ms'],
        ),
        'expboxes': dict(
            logs_path='//home/alice/dialog/prepared_logs_expboxes',
            logs_fields=['uuid', 'device_id', 'puid', 'session_id', 'session_sequence', 'fielddate',
                         'req_id', 'generic_scenario', 'do_not_use_user_logs', 'input_type', 'message_id'],
            logs_keys=['uuid', ],
            logs_keys_source=['uuid', ],
        ),
        'robot-vins_logs_dialogs': dict(
            logs_path='//home/alice/wonder/robot-dialogs',
            logs_fields=['sequence_number', 'experiments', 'form_name', 'request', 'response', 'analytics_info',
                         'server_time_ms', 'uuid', 'message_id'],
            logs_keys=['server_time_ms', 'uuid'],
            logs_keys_source=['server_time_ms', 'uuid'],
        ),
        'robot-wonder_logs': dict(
            logs_path='//home/alice/wonder/robot-logs',
            logs_fields=['_server_time_ms', '_uuid', '_megamind_request_id', '_sequence_number', 'asr', '_message_id'],
            logs_keys=['_server_time_ms', '_uuid', '_message_id'],
            logs_keys_source=['server_time_ms', '_uuid', 'message_id', 'uuid', '_message_id', '_server_time_ms'],
        ),
        'robot-expboxes': dict(
            logs_path='//home/alice/dialog/robot-prepared_logs_expboxes',
            logs_fields=['uuid', 'device_id', 'puid', 'session_id', 'session_sequence', 'fielddate',
                         'req_id', 'generic_scenario', 'do_not_use_user_logs', 'input_type', 'message_id'],
            logs_keys=['uuid', ],
            logs_keys_source=['uuid', ],
        ),
    }

    def __init__(self, source_table, preset, start_date=None, end_date=None,
                 cluster='hahn', yt_token=None, yql_token=None,
                 custom_join_params=None, verbose=1):
        self.verbose = verbose
        self.log_print('Racing Join operation started')

        # process params
        self.source_table = source_table
        self.yt_token = yt_token
        self.yql_token = yql_token
        self.cluster = cluster

        self.params = self.presets[preset]
        self.params.update(custom_join_params or {})
        self.params['logs_fields'] = self.merge_comma_separated_strings(self.params['logs_fields'],
                                                                        self.params.get('extra_logs_fields', ''))

        # read source keys, trim df by start and end dates
        columns_to_read = self.params['logs_keys_source'] + ['fielddate', ]
        self.df = self.read_yt_table(table=source_table, columns=columns_to_read,
                                     cluster=self.cluster, yt_token=yt_token)
        assert 'fielddate' in self.df.columns, 'Source table must contain column "fielddate"'
        if 'server_time_ms' in self.df.columns:
            self.df['_server_time_ms'] = self.df['server_time_ms']  # for wonderlogs join
        if 'message_id' in self.df.columns:
            self.df['_message_id'] = self.df['message_id']  # for wonderlogs join
        if 'uuid' in self.df.columns and '_uuid' not in self.df.columns:
            self.df['_uuid'] = self.df['uuid'].str.replace('uu/', '')  # for wonderlogs join

        self.log_print(f'internal df created with shape {self.df.shape}')
        gc.collect()

        if start_date and start_date != "null":
            self.df = self.df[self.df.fielddate >= start_date]
        if end_date and end_date != "null":
            self.df = self.df[self.df.fielddate <= end_date]
        assert self.df.shape[0], 'Source table has no rows within [start_date:end_date] interval'

        self.build_sources_dict()
        self.log_print(f'sources dict created with {len(self.sources)} entries for individual dates')
        self.slice_batches()
        self.log_print(f'sliced {len(self.batches)} batches')

    def log_print(self, *args):
        if self.verbose > 0:
            print(datetime.strftime(datetime.now(), '%Y-%m-%d %H:%M:%S'), end=' - ')
            print(*args)

    def build_sources_dict(self):
        yt.config["proxy"]["url"] = f'{self.cluster}.yt.yandex.net'
        client = yt.YtClient(proxy=self.cluster, token=self.yt_token)

        fielddates = self.df.fielddate.unique().tolist()
        fielddates.sort()
        self.sources = {}
        for fd in fielddates:
            table = self.params['logs_path'] + '/' + fd
            schema_ = yt.schema.TableSchema.from_yson_type(client.get(f"{table}/@schema"))
            cols = [c.name for c in schema_.columns]
            index_cols = [c.name for c in schema_.columns if c.sort_order]
            self.sources[fd] = {'table': table, 'cols': cols, 'index_cols': index_cols}

    @staticmethod
    def read_yt_table(table, cluster='hahn', columns=None, yt_token=None):
        """reads table from hahn, returns DF
        :param table: YT table name
        :param columns: list of columns to read. Leave None for all cols
        :param yt_token: YT Token
        :param cluster: YT Cluster
        """
        yt.config["proxy"]["url"] = f'{cluster}.yt.yandex.net'
        client = yt.YtClient(proxy=cluster, token=yt_token)
        table_path = yt.TablePath(table, columns=columns)
        read_result = client.read_table(table_path, raw=False)
        raw_data = [row for row in read_result.rows]
        return pd.DataFrame(raw_data)

    @staticmethod
    def merge_comma_separated_strings(*args):
        """merges any number of comma separated string, or lists
        returns sorted list of unique items without `` """
        items = []
        for a in args:
            if isinstance(a, str):
                a = a.split(',')
            items.extend(a)
        items = set([s.strip().replace('`', '') for s in items if s != ''])
        return list(sorted(items))

    def slice_batches(self):
        """prepares fields and indices for individual batches"""

        self.batches = []
        for query_date, source in self.sources.items():
            index_keys = [k for k in source['index_cols'] if k in self.df.columns and k in self.params['logs_keys']]
            assert len(index_keys) > 0, f"no match between input {self.df.columns} " \
                                        f"and log index fields {source['index_cols']} for table {source['table']}"
            output_keys = [k for k in source['cols'] if k in self.params['logs_fields']]
            assert len(output_keys) > 0, f"no match target fields found in table {source['table']}"

            day_keys_list = self.df.loc[self.df.fielddate == query_date, index_keys] \
                .drop_duplicates(subset=index_keys) \
                .sort_values(by=index_keys).values.tolist()
            day_keys_list = [list(filter(None, item_)) for item_ in day_keys_list]

            if len(day_keys_list[0]) == 1:
                day_keys_list = [x[0] for x in day_keys_list]
            else:
                day_keys_list = [tuple(x) for x in day_keys_list]
            self.log_print(f'index_fields for {query_date}: {index_keys}; num_keys: {len(day_keys_list)}')

            for j in range(int(ceil(len(day_keys_list) / self.BATCH_SIZE_LIMIT))):
                batch_keys_list = day_keys_list[j * self.BATCH_SIZE_LIMIT: (j + 1) * self.BATCH_SIZE_LIMIT]
                self.batches.append({
                    'keys_list': batch_keys_list,
                    'query_date': query_date,
                    'table': source['table'],
                    'batch_id': f"{query_date}_{j}",
                    'batch_size': len(batch_keys_list),
                    'index_keys': index_keys,
                    'output_keys': output_keys,
                })

    def build_query(self, output_table='{{output1}}', add_info_block=False):
        """uses pre-sliced batches to generate code for YQL query
        :param output_table: YT table name in hahn. Keep {{{{output1}}}} for use in YQL cubes
        :param add_info_block: adds brief info block to YQL results output
        """

        def list_to_fields(f_list, wrap_tuple=False):
            """Convert iterable of field names to string of field names with `` """
            if isinstance(f_list, str):  # protection from string input
                return f"`{f_list.replace('`', '')}`"
            if wrap_tuple and len(f_list) > 1:
                return '(' + ', '.join([f"`{f}`" for f in f_list]) + ')'
            else:
                return ', '.join([f"`{f}`" for f in f_list])

        # query header
        pragmas = 'PRAGMA AnsiInForEmptyOrNullableItemsCollections;\n'

        # main part with lookup in batches
        query_lookup = ''
        for i, b in enumerate(self.batches):
            query_lookup += f'''
                INSERT INTO @preselected
                SELECT
                    {list_to_fields(b['output_keys'])}
                FROM `{b['table']}`
                WHERE {list_to_fields(b['index_keys'], wrap_tuple=True)}
                    IN {b['keys_list']}
                ORDER BY {list_to_fields(b['index_keys'])}
                LIMIT 100000
                ;
                '''

        # prepare output
        query_output = f"COMMIT;\n\n" \
                       f"$output = SELECT * FROM @preselected; \n\n" \
                       f"INSERT INTO {output_table} \n" \
                       f"SELECT * FROM $output; \n"

        # show key stats in YQL results output
        info_block = '''
            SELECT 'input' AS table, count(*) AS count_rows FROM `{self.source_table}`
            UNION ALL SELECT 'preselected' AS table, count(*) AS count_rows FROM $preselected
            UNION ALL SELECT 'output' AS table, count(*) AS count_rows FROM $output
            INTO RESULT `stats`;
            '''

        query = '\n\n'.join([
            pragmas,
            query_lookup,
            query_output,
            info_block if add_info_block else '',
        ])

        del pragmas, query_lookup, query_output, info_block
        gc.collect()
        return query

    def build_ypaths_for_merge(self):
        """builds list of ypaths for subsequent use in YtMerge command
        uses instance variables as params:
        - self.batch as batches,
        - b['output_keys'] (based on self.params['logs_fields']) as output fields
        """
        self.ypaths = []
        for b in self.batches:
            if type(b['keys_list'][0]) in {list, tuple}:
                ranges = [{'exact': {"key": k}} for k in b['keys_list']]
            else:
                ranges = [{'exact': {"key": [k, ]}} for k in b['keys_list']]

            yp_batch = yt.TablePath(name=b['table'],
                                    ranges=ranges, columns=b['output_keys'])
            self.ypaths.append(yp_batch)
        self.log_print(f'built {len(self.ypaths)} ypaths')

    def run_ypath_merge(self, dst_path, pool=None):
        """runs YtMerge operation with saved ypaths, writes result to dst_path"""
        self.build_ypaths_for_merge()
        client = yt.YtClient(proxy='hahn', token=self.yt_token)

        # preparing destination table as empty template with needed schema
        spec_builder_template = yt.spec_builders.MergeSpecBuilder() \
            .input_table_paths(self.ypaths[0]) \
            .output_table_path(dst_path) \
            .schema_inference_mode('from_input') \
            .mode("sorted") \
            .pool('voice')
        yt.run_operation(spec_builder_template, client=client)  # create dst_path with proper schema as template
        yt.run_erase(dst_path, client=client)  # make dst_path empty

        # performing merge of selections from self.ypaths
        spec_builder = yt.spec_builders.MergeSpecBuilder() \
            .input_table_paths(self.ypaths) \
            .output_table_path(dst_path) \
            .schema_inference_mode('from_output') \
            .mode("sorted") \
            .pool(pool)

        yt.run_operation(spec_builder, client=client)
        return dst_path


class OperationState(Enum):
    Preparing = 0
    Running = 1
    Completed = 2
    Failed = 3


def _get_operation_state(op_data):
    state = op_data["state"]
    if state == "running":
        return OperationState.Running
    elif state == "completed":
        return OperationState.Completed
    elif state in ("aborted", "failed"):
        return OperationState.Failed
    else:
        return OperationState.Preparing


class AsyncRacingJoin(RacingJoin):

    async def async_single_ypath_select(self, source_ypath, dst_path, idx=None, pool='voice'):
        """runs YtMerge operation with single ypath, writes result to dst_path"""
        yt_client = yt.YtClient(proxy='hahn', token=self.yt_token)

        spec_builder = yt.spec_builders.MergeSpecBuilder() \
            .input_table_paths(source_ypath) \
            .output_table_path(dst_path) \
            .schema_inference_mode('from_input') \
            .mode("sorted") \
            .pool(pool)

        finished = False
        sleep_time = 5

        with yt_client.Transaction() as tx:
            tx_id = tx.transaction_id
            op = yt_client.run_operation(spec_builder, sync=False)
            op_data = yt_client.get_operation(op.id, attributes=["start_time", "state"])
            op_start_dt = datetime.strptime(op_data["start_time"], "%Y-%m-%dT%H:%M:%S.%fZ")
            state = _get_operation_state(op_data)
            self.log_print(f"Operation {op.id} started")

            while not finished:
                await asyncio.sleep(sleep_time)
                op_data = yt_client.get_operation(op.id, attributes=["id", "state", "brief_progress"])
                state = _get_operation_state(op_data)
                finished = state == OperationState.Completed or state == OperationState.Failed

            success = state == OperationState.Completed

        if success:
            n_result_rows = yt_client.get_attribute(dst_path, 'row_count')
            self.log_print(f"Operation {op.id} finished OK, "
                           f"processed {self.batches[idx]['keys_list'].__len__()} keys, "
                           f"generated {n_result_rows} rows.")
            yt_client.set_attribute(dst_path, 'expiration_timeout', 3600 * 1000)  # 1 hours TTL
        else:
            self.log_print(f"Operation {op.id} failed")
        return success

    async def inner_async_ypath_merge(self, output_dir, pool):
        tasks = []
        for i, single_ypath in enumerate(self.ypaths):
            single_output = output_dir + f"/output_{i}"
            task1 = asyncio.create_task(self.async_single_ypath_select(single_ypath, single_output, idx=i, pool=pool))
            tasks.append(task1)
        await asyncio.gather(*tasks)

    async def run_async_ypath_merge(self, dst_path=None, pool=None):
        self.build_ypaths_for_merge()
        rnd_id = ''.join(random.choices('1234567890abcdef', k=8))
        output_dir = f"//tmp/robot-voice-qa/racing_joiner_x_{rnd_id}"
        # dst_path = dst_path or f"//tmp/robot-voice-qa/racing_joiner_out_{rnd_id}"
        yt_client = yt.YtClient(proxy='hahn', token=self.yt_token)
        yt_client.mkdir(output_dir)
        self.log_print(f"output_dir: {output_dir} created")

        await self.inner_async_ypath_merge(output_dir, pool)
        return output_dir

    def yql_merge_results_query(self, output_dir, dst_path, delete_tmp=True):

        final_concat_query = f"""INSERT INTO `{dst_path}` WITH TRUNCATE SELECT * FROM RANGE ('{output_dir}');"""
        self.log_print(f"final_concat_query: {final_concat_query}")
        yt_client = yt.YtClient(proxy='hahn', token=self.yt_token)
        yql_client = YqlClient(db='hahn', token=self.yql_token)
        request = yql_client.query(final_concat_query, syntax_version=1)
        request.run()
        self.log_print(f"final_concat_query completed")
        if delete_tmp:
            yt_client.remove(output_dir, force=False, recursive=True)
        for wait_time in range(300):
            if yt_client.exists(dst_path):
                break
            self.log_print(f"Waiting {wait_time}s for {dst_path} to materialize...")
            time.sleep(1)
        return dst_path
