from __future__ import absolute_import
from .job_context import _DictBasedData, context as _context

__author__ = 'entropia'


class MRCluster(object):
    def __init__(self, dct, name_key):
        self._name = dct[name_key]
        self._server = dct['server']
        self._host = dct['host']
        self._port = dct['port']

    def get_name(self):
        return self._name

    def get_host(self):
        return self._host

    def get_port(self):
        return self._port

    def get_server(self):
        return self._server

    def __repr__(self):
        return 'MRCluster(name=%r, server=%r, host=%r, port=%r)' % (self._name, self._server, self._host, self._port)

    def __str__(self):
        return 'MR Cluster %s (%s)' % (self._name, self._server)


class MRPath(object):
    def __init__(self, dct):
        self._path = dct['path']
        self._raw_path = dct['rawPath']
        self._type = dct['type']
        self._cluster = MRCluster(dct, 'cluster')

    def get_path(self):
        return self._path

    def get_raw_path(self):
        return self._raw_path

    def get_cluster(self):
        return self._cluster

    def get_cluster_name(self):
        return self._cluster.get_name()

    def get_host(self):
        return self._cluster.get_host()

    def get_port(self):
        return self._cluster.get_port()

    def get_server(self):
        return self._cluster.get_server()

    def get_type(self):
        return self._type

    def is_table(self):
        return self._type == "TABLE"

    def is_directory(self):
        return self._type == "DIRECTORY"

    def is_file(self):
        return self._type == "FILE"

    def __repr__(self):
        return 'MRPath(type=%r, path=%r, raw_path=%r, cluster=%r)' % (self._type, self._path, self._raw_path, self._cluster)

    def __str__(self):
        return 'MR %s: %s @ %s' % (self._type.title(), self._path, self.get_cluster_name())


class MRJobContext(object):
    def __init__(self, delegate):
        def mk_path(key):
            d = dct[key]
            return {name: [MRPath(item) for item in d[name]] for name in d}

        self._delegate = delegate
        dct = delegate._dct

        self._mr_inputs = _DictBasedData(mk_path('mrInputs'))
        self._mr_outputs = _DictBasedData(mk_path('mrOutputs'))
        self._mr_output_path = MRPath(dct['mrOutputPath']) if 'mrOutputPath' in dct else None
        self._mr_tmp = MRPath(dct['mrTmp']) if 'mrTmp' in dct else None
        self._mr_cluster = MRCluster(dct['mrCluster'], 'name')
        self._mapreduce = dct['mapreduce']

    def get_mr_inputs(self):
        return self._mr_inputs

    def get_mr_outputs(self):
        return self._mr_outputs

    def get_mr_output_path(self):
        return self._mr_output_path

    def get_mr_tmp(self):
        return self._mr_tmp

    def get_mapreduce(self):
        return self._mapreduce

    def get_mr_cluster(self):
        return self._mr_cluster

    def __getattr__(self, name):
        return getattr(self._delegate, name)


def context():
    return MRJobContext(_context())


if __name__ == 'main' and __package__ is None:
    __package__ = 'nirvana.mr_job_context'
