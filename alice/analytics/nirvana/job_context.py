from __future__ import absolute_import, print_function
import json
import os
import socket
import sys
from .snapshot import dump_state as _dump_state, load_state as _load_state

__author__ = 'qdeee'

JWD = os.getenv('JWD', '.')
JOB_CONTEXT_JSON = JWD + '/job_context.json'


# PY3-compatible xrange()
def _xrange(start, stop, step=1):
    if sys.version_info[0] < 3:
        return xrange(start, stop, step)
    else:
        return range(start, stop, step)


class Meta(object):
    def __init__(self, dct):
        self.workflow_uid = dct['workflowUid']
        self.workflow_url = dct['workflowURL']
        self.operation_uid = dct['operationUid']
        self.block_uid = dct['blockUid']
        self.block_code = dct['blockCode']
        self.block_url = dct['blockURL']
        self.process_uid = dct['processUid']
        self.process_url = dct['processURL']
        self.description = dct['description']
        self.owner = dct['owner']

        __priodct = dct.get('priority', {
            'min': 0,
            'max': 0,
            'value': 0,
            'normValue': 0.5
        })
        self.priority_range = _xrange(__priodct['min'], __priodct['max'] + 1)
        self.priority = __priodct['value']
        self.norm_priority = __priodct['normValue']
        self.quota_project = dct.get('quotaProject', 'default')

        self.workflow_instance_uid = dct.get('workflowInstanceUid')

    def get_workflow_uid(self):
        return self.workflow_uid

    def get_workflow_instance_uid(self):
        return self.workflow_instance_uid

    def get_workflow_url(self):
        return self.workflow_url

    def get_operation_uid(self):
        return self.operation_uid

    def get_block_uid(self):
        return self.block_uid

    def get_block_code(self):
        return self.block_code

    def get_block_url(self):
        return self.block_url

    def get_process_uid(self):
        return self.process_uid

    def get_process_url(self):
        return self.process_url

    def get_priority_range(self):
        return self.priority_range

    def get_priority(self):
        return self.priority

    def get_norm_priority(self):
        return self.norm_priority

    def get_description(self):
        return self.description

    def get_owner(self):
        return self.owner

    def get_quota_project(self):
        return self.quota_project

    def __repr__(self):
        return 'Meta(workflow_uid=%r, workflow_instance_uid=%r, workflow_url=%r, description=%r, owner=%r,' \
               'operation_uid=%r, ' \
               'block_uid=%r, block_code=%r, block_url=%r, ' \
               'process_uid=%r, process_url=%r, ' \
               'priority=%r in %r, norm_priority=%r, ' \
               'quota_project=%r)' % \
               (self.workflow_uid, self.workflow_instance_uid, self.workflow_url, self.description, self.owner,
                self.operation_uid,
                self.block_uid, self.block_code, self.block_url,
                self.process_uid, self.process_url,
                self.priority, self.priority_range, self.norm_priority,
                self.quota_project)

    def __str__(self):
        return 'Executing {%s} in workflow "%s" by %s@: %s' % (self.operation_uid, self.description, self.owner, self.block_url)


class _DictBasedData(object):
    def __init__(self, dct):
        self._data = dct

    def has(self, key):
        return key in self._data and len(self._data[key]) > 0

    def get(self, key):
        values = self._data[key]
        if len(values) > 1:
            raise RuntimeError('Expected only one element for key "%s" but got %d: %s' % (key, len(values), str(values)))
        elif len(values) == 0:
            raise RuntimeError('Expected one element for key "%s" but got none')

        return values[0]

    def get_list(self, key):
        return self._data[key]

    def __repr__(self):
        return repr(self._data)


class DataItem(object):
    def __init__(self, item, path):
        self._path = path
        self._data_type = item['dataType']
        self._was_unpacked = item['wasUnpacked']
        self._unpacked_dir = item['unpackedDir']
        self._unpacked_file = item.get('unpackedFile')
        self._download_url = item.get('downloadURL')
        self._link_name = item.get('linkName')

    def get_path(self):
        return self._path

    def get_data_type(self):
        return self._data_type

    def was_unpacked(self):
        return self._was_unpacked

    def unpacked_dir(self):
        return self._unpacked_dir

    def unpacked_file(self):
        return self._unpacked_file

    def get_download_url(self):
        return self._download_url

    def get_link_name(self):
        return self._link_name

    def has_link_name(self):
        return self._link_name is not None

    def __str__(self):
        return '"%s" File: %s' % (self._data_type, self._path)

    def __repr__(self):
        return 'DataItem(data_type=%r, path=%r, link_name=%r, was_unpacked=%r, unpacked_dir=%r, unpacked_file=%r, download_url=%r)' % \
               (self._data_type, self._path, self._link_name, self._was_unpacked, self._unpacked_dir, self._unpacked_file, self._download_url)


class _EndpointData(object):
    def __init__(self, paths, meta):
        def data_items(name, m):
            return [DataItem(item, paths[name][index]) for index, item in enumerate(m)]

        self._paths = _DictBasedData(paths)

        __raw_items = {name: data_items(name, m) for name, m in meta.items()}
        self._items = _DictBasedData(__raw_items)

        self._named_items = dict()
        for name, data_items in __raw_items.items():
            self._named_items[name] = dict()
            for data_item in data_items:
                if data_item.has_link_name():
                    self._named_items[name][data_item.get_link_name()] = data_item

        # FIXME - kept for backwards compatibility @see https://st.yandex-team.ru/NIRVANA-3819
        self.data = self._paths._data

    def has(self, key):
        return self._paths.has(key)

    def get(self, key, link_name=None):
        if link_name:
            return self.get_item(key, link_name=link_name).get_path()
        else:
            return self._paths.get(key)

    def get_list(self, key):
        return self._paths.get_list(key)

    def get_item(self, key, link_name=None):
        if link_name:
            named_items_for_key = self.get_named_items(key)

            if link_name not in named_items_for_key:
                raise RuntimeError('Expected to find link "%s" under key "%s" but found none' % (link_name, key))

            return named_items_for_key[link_name]
        else:
            return self._items.get(key)

    def get_named_items(self, key):
        if key not in self._named_items:
            raise RuntimeError('Expected to find element for key "%s" but found none' % key)

        return self._named_items[key]

    def get_item_list(self, key):
        return self._items.get_list(key)

    def __repr__(self):
        return repr(self._items)


class Inputs(_EndpointData):
    def __init__(self, paths, meta):
        _EndpointData.__init__(self, paths, meta)


class Outputs(_EndpointData):
    def __init__(self, paths, meta):
        _EndpointData.__init__(self, paths, meta)


class Status(object):
    def __init__(self, dct):
        self.error_msg = dct['errorMsg']
        self.success_msg = dct['successMsg']

    def get_error_msg(self):
        return self.error_msg

    def get_success_msg(self):
        return self.success_msg

    def __repr__(self):
        return 'Status(error_msg=%r, success_msg=%r)' % (self.error_msg, self.success_msg)

    def __str__(self):
        return 'Status{\n    error message file => %s\n    success message file => %s\n}' % (self.error_msg, self.success_msg)


class JobContext(object):
    def __init__(self, dct):
        self.meta = Meta(dct['meta'])
        self.parameters = dct['parameters']
        self.inputs = Inputs(dct['inputs'], dct['inputItems'])
        self.outputs = Outputs(dct['outputs'], dct['outputItems'])
        self.ports = {t: dct['ports'][t] for t in dct['ports']}
        self.status = Status(dct['status'])

        self._dct = dct

    def get_meta(self):
        return self.meta

    def get_parameters(self):
        return self.parameters

    def get_inputs(self):
        return self.inputs

    def get_outputs(self):
        return self.outputs

    def get_tcp_ports(self):
        return self.ports['tcp']

    def get_udp_ports(self):
        return self.ports['udp']

    def get_ports(self, sock_type):
        if sock_type == socket.SOCK_STREAM:
            return self.get_tcp_ports()
        elif sock_type == socket.SOCK_DGRAM:
            return self.get_udp_ports()
        else:
            raise RuntimeError('Unknown socket type: %s. Accepted types are: socket.SOCK_DGRAM and socket.SOCK_STREAM' % sock_type)

    def get_status(self):
        return self.status

    def load_state(self, snapshot_name, load, *args, **kwargs):
        """
        Tries to load execution state from snapshot output `snapshot' using the deserialization function `load'.

        :param snapshot_name: name of the snapshot output to load state from
        :param load: deserialization function with signature `(file-like object) -> Any`, e.g., json.load, pickle.load
        :param args: additional positional arguments passed to the `load' function
        :param kwargs: additional named arguments to the `load_state' and `load' functions

                       These named arguments are used by the `load_state' function:
                         * read_mode: snapshot file read mode; 'rb' by default
                         * default_value: fallback value to return if snapshot is unavailable or could not be loaded; None by default
                       All other named arguments are passed to `load' intact.

        :return: execution state loaded from snapshot, or default_value if snapshot is not available or could not be loaded

        :except: RuntimeError: no such snapshot exists in the operation signature
        """
        return _load_state(self.outputs.get(snapshot_name), load, *args, **kwargs)

    def dump_state(self, snapshot_name, state, dump, *args, **kwargs):
        """
        Tries to dump new execution state `state' to snapshot output `snapshot' using the serialization function `dump'.
        Snapshot is atomically updated: the new state is dumped to a temporary file, which is then renamed to the snapshot file.
        This ensures that Nirvana job wrapper (job_launcher) always sees consistent (though maybe stale) snapshot contents.

        :param snapshot_name: name of the snaphot output to save state to
        :param dump: serialization function with signature `(state, file-like object) -> Any', e.g., json.dump, pickle.dump
        :param args: additional positional arguments to the `dump' function
        :param kwargs: additional named arguments to the `dump' and `dump_state' functions

                       These named arguments are used by the `dump_state' function:
                         * write_mode: temporary state file write mode; 'w+b' by default
                         * notify: True to notify Nirvana job wrapper (job_launcher) that the snapshot
                           has been updated and can be uploaded to storage;
                           False otherwise (default).

                           Note that setting this argument to True does _not_ guarantee that job_launcher
                           would actually upload the snapshot promptly, if at all. job_launcher is free
                           to ignore this upload advice, especially when `save_state' is called
                           too often (where `too often' is implementation-dependent)
                       All other named arguments are passed to `dump' intact.

        :except: RuntimeError: no such snapshot exists in the operation signature
        """
        return _dump_state(self.outputs.get(snapshot_name), state, dump, *args, **kwargs)


def context():
    with open(JOB_CONTEXT_JSON) as job_file:
        return JobContext(json.load(job_file))


if __name__ == 'main' and __package__ is None:
    __package__ = 'nirvana.job_context'
