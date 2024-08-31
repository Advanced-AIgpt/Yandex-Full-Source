import base64
import copy
import json
import logging
import os
import time

import yatest.common
import yatest.common.network
from alice.wonderlogs.protos.megamind_prepared_pb2 import TMegamindPrepared
from alice.wonderlogs.protos.wonderlogs_pb2 import TWonderlog
from alice.wonderlogs.rt.processors.megamind_creator.protos.config_pb2 import TMegamindCreatorConfig
from alice.wonderlogs.rt.processors.wonderlogs_creator.protos.config_pb2 import TWonderlogsCreatorConfig
from alice.wonderlogs.rt.processors.megamind_resharder.protos.config_pb2 import TMegamindResharderConfig
from alice.wonderlogs.rt.processors.uniproxy_creator.protos.config_pb2 import TUniproxyCreatorConfig
from alice.wonderlogs.rt.processors.megamind_creator.protos.megamind_prepared_wrapper_pb2 import TMegamindPreparedWrapper
from alice.wonderlogs.rt.processors.uniproxy_creator.protos.uniproxy_prepared_wrapper_pb2 import TUniproxyPreparedWrapper
from alice.wonderlogs.rt.processors.uniproxy_resharder.protos.config_pb2 import TUniproxyResharderConfig
from alice.wonderlogs.rt.protos.uniproxy_event_pb2 import TUniproxyEvent
from alice.wonderlogs.rt.protos.uuid_message_id_pb2 import TUuidMessageId
from google.protobuf import json_format, text_format
from google.protobuf.descriptor import FieldDescriptor

import yt.wrapper
import yt.yson
from ads.bsyeti.big_rt.cli import lib as big_rt_cli
from ads.bsyeti.big_rt.py_lib import YtQueue
from library.python import resource
from mapreduce.yt.interface.protos.extension_pb2 import (column_name,
                                                         key_column_name)

logger = logging.getLogger(__name__)


def dict_to_text(d, proto):
    return text_format.MessageToString(json_format.Parse(json.dumps(d), proto()), as_one_line=True)


def create_queue(queue_path):
    big_rt_cli.create_queue(queue_path, shards=1, medium='default',
                            max_ttl=False, commit_ordering='strong', swift=False)


class Processor:
    def __init__(self, name, bin_path, config_name, config_proto, port):
        self._bin_path = yatest.common.binary_path(
            'alice/wonderlogs/rt/processors/' + bin_path)
        config = json.loads(resource.resfs_read(
            'alice/wonderlogs/rt/tests/configs/' + config_name))
        self._config = self._patch_config(config, name, port)
        self._config_proto = config_proto
        self._processor = None

    @staticmethod
    def _patch_config(config, name, port):
        yt_cluster = os.environ['YT_PROXY']
        patched_config = copy.copy(config)

        for supplier in patched_config['Suppliers']:
            supplier['YtSupplier']['Cluster'] = yt_cluster

        patched_config['ConsumingSystem']['Cluster'] = yt_cluster
        patched_config['HttpServer']['Port'] = port
        patched_config['Logs']['Rules'][0]['FilePath'] = yatest.common.output_path(
            f'{name}.log')
        return patched_config

    def get_config(self):
        return self._config

    def run(self):
        command = [self._bin_path, '--config-text',
                   dict_to_text(self._config, self._config_proto)]
        logger.info(f'Running processor: {command}')
        self._processor = yatest.common.execute(command, wait=False)

    def stop(self):
        if self._processor and self._processor.running:
            self._processor.kill()

    @staticmethod
    def _wait_consuming_queues(queues, consumer):
        # https://a.yandex-team.ru/arc/trunk/arcadia/quality/user_sessions/rt/tests/common/__init__.py?rev=r8680807#L111
        time.sleep(1)

        for queue_name in queues:
            queue = YtQueue(queue_name)
            while True:
                offsets = queue.get_consumers_offsets([consumer], [0])
                total_row_count = queue.get_shard_infos()[0]['total_row_count']
                logger.info(
                    f'Offsets: {offsets}, Total row count: {total_row_count}')
                if offsets[consumer][0] == total_row_count - 1:
                    break
                time.sleep(5)


class MegamindResharderProcessor(Processor):
    def __init__(self, port):
        super().__init__('megamind_resharder', 'megamind_resharder/bin/megamind_resharder',
                         'megamind_resharder.json', TMegamindResharderConfig, port)
        create_queue(self.get_output_queue())

    def get_input_queue(self):
        return self._config['Suppliers'][0]['YtSupplier']['QueuePath']

    def get_output_queue(self):
        return self._config['ShardingConfig']['OutputQueue']

    def get_consumer(self):
        return self._config['Suppliers'][0]['YtSupplier']['QueueConsumer']

    def wait_consuming_queues(self):
        self._wait_consuming_queues(
            [self.get_input_queue()], self.get_consumer())


class MegamindCreatorProcessor(Processor):
    def __init__(self, port):
        super().__init__('megamind_creator', 'megamind_creator/bin/megamind_creator',
                         'megamind_creator.json', TMegamindCreatorConfig, port)
        create_states_table(self.get_states(),
                            generate_schema(TMegamindPreparedWrapper))

    def get_input_queue(self):
        return self._config['Suppliers'][0]['YtSupplier']['QueuePath']

    def get_consumer(self):
        return self._config['Suppliers'][0]['YtSupplier']['QueueConsumer']

    def get_uuid_message_id_queue(self):
        return self._config['ProcessorConfig']['UuidMessageIdQueue']['OutputQueue']

    def get_states(self):
        return self._config['ProtoStateManagerConfig']['StateTable']

    def wait_consuming_queues(self):
        self._wait_consuming_queues(
            [self.get_input_queue()], self.get_consumer())


class UniproxyResharderProcessor(Processor):
    def __init__(self, port):
        super().__init__('uniproxy_resharder', 'uniproxy_resharder/bin/uniproxy_resharder',
                         'uniproxy_resharder.json', TUniproxyResharderConfig, port)
        create_queue(self.get_output_queue())

    def get_input_queue(self):
        return self._config['Suppliers'][0]['YtSupplier']['QueuePath']

    def get_output_queue(self):
        return self._config['ShardingConfig']['OutputQueue']

    def get_consumer(self):
        return self._config['Suppliers'][0]['YtSupplier']['QueueConsumer']

    def wait_consuming_queues(self):
        self._wait_consuming_queues(
            [self.get_input_queue()], self.get_consumer())


class UniproxyCreatorProcessor(Processor):
    def __init__(self, port):
        super().__init__('uniproxy_creator', 'uniproxy_creator/bin/uniproxy_creator',
                         'uniproxy_creator.json', TUniproxyCreatorConfig, port)
        create_states_table(self.get_states(),
                            generate_schema(TUniproxyPreparedWrapper))

    def get_input_queue(self):
        return self._config['Suppliers'][0]['YtSupplier']['QueuePath']

    def get_consumer(self):
        return self._config['Suppliers'][0]['YtSupplier']['QueueConsumer']

    def get_uuid_message_id_queue(self):
        return self._config['ProcessorConfig']['UuidMessageIdQueue']['OutputQueue']

    def get_states(self):
        return self._config['ProtoStateManagerConfig']['StateTable']

    def wait_consuming_queues(self):
        self._wait_consuming_queues(
            [self.get_input_queue()], self.get_consumer())


class WonderlogsCreatorProcessor(Processor):
    def __init__(self, port):
        super().__init__('wonderlogs_creator', 'wonderlogs_creator/bin/wonderlogs_creator',
                         'wonderlogs_creator.json', TWonderlogsCreatorConfig, port)
        create_states_table(self.get_states(),
                            generate_schema(TUuidMessageId))
        create_queue(self.get_output_queue())

    def get_input_queue(self):
        return self._config['Suppliers'][0]['YtSupplier']['QueuePath']

    def get_consumer(self):
        return self._config['Suppliers'][0]['YtSupplier']['QueueConsumer']

    def get_output_queue(self):
        return self._config['ProcessorConfig']['OutputQueue']

    def get_states(self):
        return self._config['StateManagerConfigs']['UuidMessageId']['StateTable']

    def wait_consuming_queues(self):
        self._wait_consuming_queues(
            [self.get_input_queue()], self.get_consumer())


def patch_config(config, name, port):
    yt_cluster = os.environ['YT_PROXY']
    patched_config = copy.copy(config)

    for supplier in patched_config['Suppliers']:
        supplier['YtSupplier']['Cluster'] = yt_cluster

    patched_config['ConsumingSystem']['Cluster'] = yt_cluster
    patched_config['HttpServer']['Port'] = port
    patched_config['Logs']['Rules'][0]['FilePath'] = yatest.common.output_path(
        f'{name}.log')
    return patched_config


CANONIZED_MEGAMIND_QYT = 'megamind_qyt.json'
CANONIZED_MEGAMIND_STATES = 'megamind_states.json'
CANONIZED_UNIPROXY_QYT = 'uniproxy_qyt.json'
CANONIZED_UNIPROXY_STATES = 'uniproxy_states.json'
CANONIZED_UUID_MESSAGE_ID_QYT = 'uuid_message_id_qyt.json'
CANONIZED_WONDERLOGS_QYT = 'wonderlogs_qyt.json'


def write_to_qyt(queue_path, data_path):
    data = json.loads(resource.find(data_path))
    rows = [(0, row) for row in data]
    create_queue(queue_path)
    YtQueue(queue_path).write(rows)


def read_queue(queue_path):
    total_rows = YtQueue(queue_path).get_shard_infos()[0]['total_row_count']
    rows = YtQueue(queue_path).read(0, 0, total_rows)['rows']
    return rows


def read_state_table(state_table, order_by=None):
    rows_count = list(yt.wrapper.select_rows(
        f'SUM(1) AS rows_count FROM [{state_table}] GROUP BY 1'))

    if rows_count:
        rows_count = int(rows_count[0]['rows_count'])
    else:
        rows_count = 0

    rows = []
    if rows_count:
        order_by_pattern = ''
        if order_by is not None:
            order_by_pattern = f'ORDER BY {order_by} LIMIT {rows_count}'

        rows = list(yt.wrapper.select_rows(
            f'* FROM [{state_table}] {order_by_pattern}', format=yt.wrapper.JsonFormat(encoding=None)))

    return rows


def generate_schema(proto_type):
    cpp_type_to_schema_type = {
        FieldDescriptor.CPPTYPE_INT32: 'int64',
        FieldDescriptor.CPPTYPE_INT64: 'int64',
        FieldDescriptor.CPPTYPE_UINT32: 'uint64',
        FieldDescriptor.CPPTYPE_UINT64: 'uint64',
        FieldDescriptor.CPPTYPE_DOUBLE: 'double',
        FieldDescriptor.CPPTYPE_FLOAT: 'double',
        FieldDescriptor.CPPTYPE_BOOL: 'boolean',
        FieldDescriptor.CPPTYPE_ENUM: 'string',
        FieldDescriptor.CPPTYPE_STRING: 'string',
        FieldDescriptor.CPPTYPE_MESSAGE: 'string'
    }
    schema = []
    for field, descriptor in proto_type.DESCRIPTOR.fields_by_name.iteritems():
        extensions = descriptor.GetOptions().Extensions
        sort_by = False
        if extensions[column_name]:
            name = extensions[column_name]
        elif extensions[key_column_name]:
            name = extensions[key_column_name]
            sort_by = True
        else:
            name = field

        cur = {
            'name': name,
            'type': cpp_type_to_schema_type.get(descriptor.cpp_type, 'string')
        }
        if sort_by:
            cur['sort_order'] = 'ascending'
        schema.append(cur)
    schema = yt.yson.YsonList(schema)
    schema.attributes['strict'] = True
    schema.attributes['unique_keys'] = True
    return schema


def create_states_table(table_path, table_schema):
    # https://a.yandex-team.ru/arc/trunk/arcadia/quality/user_sessions/rt/tests/common/__init__.py?rev=r9066618#L240
    yt.wrapper.mkdir(yt.wrapper.ypath.ypath_dirname(
        table_path), recursive=True)

    yt.wrapper.create('table', table_path, attributes={
                      'dynamic': True, 'schema': table_schema})
    logger.info(f'States table {table_path} created')
    yt.wrapper.mount_table(table_path, sync=True)
    logger.info(f'States table {table_path} mounted')


def proto_bytes_to_json(proto_bytes, proto_type):
    message = proto_type()
    message.ParseFromString(proto_bytes)
    return json_format.MessageToDict(message)


def protos_bytes_to_json(proto_bytes, proto_type):
    jsons = []
    for proto_bytes_val in proto_bytes:
        jsons.append(proto_bytes_to_json(proto_bytes_val, proto_type))
    return jsons


def generate_consumer(qyt, consumer):
    big_rt_cli.create_consumer(qyt, consumer, ignore_in_trimming=0)


def dict_bytes_to_str(d):
    new_d = {}
    for k, v in d.items():
        new_d[k.decode('utf-8')] = str(v)
    return new_d


def test():
    with yatest.common.network.PortManager() as port_manager:
        megamind_resharder = MegamindResharderProcessor(
            port_manager.get_port())
        megamind_creator = MegamindCreatorProcessor(port_manager.get_port())

        generate_consumer(megamind_resharder.get_output_queue(),
                          megamind_creator.get_consumer())

        uniproxy_resharder = UniproxyResharderProcessor(
            port_manager.get_port())
        uniproxy_creator = UniproxyCreatorProcessor(port_manager.get_port())
        generate_consumer(uniproxy_resharder.get_output_queue(),
                          uniproxy_creator.get_consumer())

        write_to_qyt(megamind_resharder.get_input_queue(), 'megamind.json')
        generate_consumer(megamind_resharder.get_input_queue(),
                          megamind_resharder.get_consumer())

        write_to_qyt(uniproxy_resharder.get_input_queue(), 'uniproxy.json')
        generate_consumer(uniproxy_resharder.get_input_queue(),
                          uniproxy_resharder.get_consumer())

        create_queue(megamind_creator.get_uuid_message_id_queue())

        wonderlogs_creator = WonderlogsCreatorProcessor(
            port_manager.get_port())

        generate_consumer(megamind_creator.get_uuid_message_id_queue(),
                          wonderlogs_creator.get_consumer())

        megamind_resharder.run()
        megamind_creator.run()
        uniproxy_resharder.run()
        uniproxy_creator.run()
        wonderlogs_creator.run()

        megamind_creator.wait_consuming_queues()
        uniproxy_creator.wait_consuming_queues()
        megamind_resharder.wait_consuming_queues()
        uniproxy_resharder.wait_consuming_queues()
        wonderlogs_creator.wait_consuming_queues()

        megamind_creator.stop()
        uniproxy_creator.stop()
        megamind_resharder.stop()
        uniproxy_resharder.stop()
        wonderlogs_creator.stop()

        megamind_resharder_qyt_rows = protos_bytes_to_json(read_queue(
            megamind_resharder.get_output_queue()), TMegamindPrepared.TMegamindRequestResponse)

        uniproxy_resharder_qyt_rows = protos_bytes_to_json(read_queue(
            uniproxy_resharder.get_output_queue()), TUniproxyEvent)

        uniproxy_prepared_state_rows = read_state_table(
            uniproxy_creator.get_states(), 'uuid, message_id')

        megamind_prepared_state_rows = read_state_table(
            megamind_creator.get_states(), 'uuid, message_id')

        uuid_message_id_qyt_rows = protos_bytes_to_json(read_queue(
            megamind_creator.get_uuid_message_id_queue()), TUuidMessageId)

        wonderlogs_qyt_rows = protos_bytes_to_json(read_queue(
            wonderlogs_creator.get_output_queue()), TWonderlog)

        for state in megamind_prepared_state_rows:
            values_proto = TMegamindPreparedWrapper.TValues()
            values_proto.ParseFromString(base64.decodebytes(state[b'values']))
            values = json_format.MessageToDict(values_proto)
            state['values'] = sorted(values['values'], key=lambda val: val['speechkit_request']
                                     ['request']['additional_options']['server_time_ms'])
            state['uuid'] = state[b'uuid'].decode('utf-8')
            state['message_id'] = state[b'message_id'].decode('utf-8')

            state.pop(b'uuid')
            state.pop(b'message_id')
            state.pop(b'values')

        for uuid_message_id in uuid_message_id_qyt_rows:
            uuid_message_id['timestamp_ms'] = 0

        uuid_message_id_qyt_rows.sort(
            key=lambda row: (row['uuid'], row['message_id']))

        wonderlogs_qyt_rows.sort(key=lambda row: (
            row['_uuid'], row['_message_id']))

        with open(CANONIZED_MEGAMIND_QYT, 'w') as f:
            json.dump(megamind_resharder_qyt_rows, f, sort_keys=True)

        with open(CANONIZED_MEGAMIND_STATES, 'w') as f:
            json.dump(megamind_prepared_state_rows, f, sort_keys=True)

        with open(CANONIZED_UNIPROXY_QYT, 'w') as f:
            json.dump(uniproxy_resharder_qyt_rows, f, sort_keys=True)

        with open(CANONIZED_UNIPROXY_STATES, 'w') as f:
            json.dump([dict_bytes_to_str(r)
                      for r in uniproxy_prepared_state_rows], f, sort_keys=True)

        with open(CANONIZED_UUID_MESSAGE_ID_QYT, 'w') as f:
            json.dump(uuid_message_id_qyt_rows, f, sort_keys=True)

        with open(CANONIZED_WONDERLOGS_QYT, 'w') as f:
            json.dump(wonderlogs_qyt_rows, f, sort_keys=True)

        return [
            yatest.common.canonical_file(CANONIZED_MEGAMIND_QYT),
            yatest.common.canonical_file(CANONIZED_MEGAMIND_STATES),
            yatest.common.canonical_file(CANONIZED_UNIPROXY_QYT),
            yatest.common.canonical_file(CANONIZED_UNIPROXY_STATES),
            yatest.common.canonical_file(CANONIZED_UUID_MESSAGE_ID_QYT),
            yatest.common.canonical_file(CANONIZED_WONDERLOGS_QYT)
        ]
