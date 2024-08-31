# coding: utf-8
from __future__ import unicode_literals

import logging
import re
import pandas as pd
import attr

from collections import Mapping
from vins_core.dm.formats import FuzzyNLUFormat
from vins_core.config.app_config import get_nlu_data_from_microintents
from vins_core.utils.data import open_resource_file
from vins_core.utils.strings import smart_unicode


logger = logging.getLogger(__name__)


@attr.s
class NLUSource(object):

    config = attr.ib()
    nlu_templates = attr.ib(default=None)
    trainable_classifiers = attr.ib(default=attr.Factory(list))

    @config.validator
    def config_validator(self, attribute, value):
        assert isinstance(value, Mapping)
        self._config_validator(value)

    def _config_validator(self, config):
        raise NotImplementedError()

    def load(self):
        texts = self._load()

        if self.config.get('unique', True):
            texts = list(set(texts))

        return FuzzyNLUFormat.parse_iter(
            texts,
            templates=self.nlu_templates,
            trainable_classifiers=self.trainable_classifiers,
            can_use_to_train_tagger=self.config.get('can_use_to_train_tagger', True),
            source_path=self.config.get('path')
        ).items

    def _load(self):
        raise NotImplementedError()

    def _assert_config_params(self, params):
        for param in params:
            if param not in self.config:
                raise KeyError('"%s" field required in "%s" config' % (param, self.__class__.__name__))

    @property
    def type(self):
        return self.config['source']


class FileNLUSource(NLUSource):

    def _config_validator(self, config):
        self._assert_config_params(['path'])

    def _load(self):
        with open_resource_file(self.config['path']) as f:
            return f.readlines()


class MicrointentsNLUSource(NLUSource):

    def _config_validator(self, config):
        self._assert_config_params(['path'])

    def _load(self):
        file_path = self.config['path']
        return sum(get_nlu_data_from_microintents(file_path).itervalues(), [])


class DataNLUSource(NLUSource):

    def _config_validator(self, config):
        self._assert_config_params(['data'])

    def _load(self):
        return self.config['data']


class TableNLUSource(NLUSource):

    def _config_validator(self, config):
        self._assert_config_params(['path', 'text_column'])

    def _iter_rows(self):
        raise NotImplementedError()

    def _load(self):
        text_column = self.config['text_column']
        row_filters = self.config.get('row_filters')
        out = []
        for row in self._iter_rows():
            if not row_filters or all(
                filter_column in row and re.match(filter_pattern, smart_unicode(row[filter_column]))
                for filter_column, filter_pattern in row_filters.iteritems()
            ):
                out.append(row[text_column])
        return out


class TSVNLUSource(TableNLUSource):
    _TSV_FILE_CHUNK_SIZE = 1024

    def _iter_rows(self):
        header = self.config.get('header')
        for tsv_data_chunk in pd.read_csv(
            self.config['path'], encoding='utf-8', sep='\t', header=header, chunksize=self._TSV_FILE_CHUNK_SIZE
        ):
            for _, item in tsv_data_chunk.iterrows():
                yield item.to_dict()


_nlu_source_factories = {}


def create_nlu_source(source_type, **kwargs):
    if source_type not in _nlu_source_factories:
        raise ValueError('Unknown source type "%s"' % source_type)
    logger.info('Creating NLU source "%s"' % source_type)
    return _nlu_source_factories[source_type](**kwargs)


def register_nlu_source_type(cls_type, name):
    assert issubclass(cls_type, NLUSource)
    _nlu_source_factories[name] = cls_type


register_nlu_source_type(FileNLUSource, 'file')
register_nlu_source_type(MicrointentsNLUSource, 'microintents')
register_nlu_source_type(DataNLUSource, 'data')
register_nlu_source_type(TSVNLUSource, 'tsv')
