# coding: utf-8

import os
import six
import json
import unittest
import yatest.common

from nile.local.format import DataFormat, record_from_dict_fast
from nile.api.v1.local import (
    FileSource,
    FileSink,
)

if six.PY3:
    from itertools import zip_longest as longest_zip
else:
    from itertools import izip_longest as longest_zip


def to_dict(value):
    if isinstance(value, dict):
        return value
    elif value is None:
        return None
    else:
        return value.to_dict()


def ensure_utf8(d):
    """Перекодирует ключи и строковые значения словаря в str и возвращает новый словарь.
    И это применяется рекурсивно ко всем значениям словаря.
    Эта функция используется в тестах для того, чтобы данные прочитанные из файловой фикстуры
    представлять в виде байтов так же, как это делает nile, читая данные из YT."""
    def maybe_to_unicode(val):
        if isinstance(val, dict):
            try:
                return {
                    maybe_to_unicode(key): maybe_to_unicode(value)
                    for key, value in val.items()
                }
            except UnicodeDecodeError:
                # if any dict key is un-decodable return None
                return None
        elif isinstance(val, list):
            return list(map(maybe_to_unicode, val))
        elif isinstance(val, tuple):
            return tuple(map(maybe_to_unicode, val))
        elif isinstance(val, bytes):
            try:
                return val.decode('utf-8')
            except UnicodeDecodeError:
                return None
        else:
            return val

    return maybe_to_unicode(d)


class JsonFileFormat(DataFormat):
    """
    Формат, поддерживающий сериализацию/десериализацию записей в/из JSON список.
    """

    def __init__(self, encoding='utf8', ensure_ascii=False, sort_keys=True, indent=4):
        """
        :param encoding: pass to json.dumps/loads
        :param ensure_ascii: pass to json.dumps
        :param sort_keys: pass to json.dumps
        """
        self._encoding = encoding
        self._ensure_ascii = ensure_ascii
        self.sort_keys = sort_keys
        self.indent = indent

    def serialize(self, records, stream):
        try:
            data = [ensure_utf8(r.to_dict()) for r in records]
            serialized = json.dumps(data, sort_keys=self.sort_keys, indent=self.indent, ensure_ascii=self._ensure_ascii)
            stream.write(serialized.encode(self._encoding))
        except ValueError as err:
            raise ValueError('Wrong file format: {}'.format(err))

    def deserialize(self, stream):
        json_str = stream.read() or '[]'
        if six.PY3:
            data = json.loads(json_str)
        else:
            data = json.loads(json_str, encoding=self._encoding)
        for item in data:
            yield item if item is None else record_from_dict_fast(item)


def _get_records(source):
    source.prepare()
    return list(source.get_stream())


def _get_output_filename(input_filename):
    file_basename = os.path.basename(input_filename) + '.test_out.json'
    return yatest.common.output_path(os.path.join(file_basename))


def _resolve_source(source_reference, schema=None):
    return FileSource(source_reference, JsonFileFormat(), treat_none_as_yson=True, schema=schema)


def _resolve_sink(sink_reference):
    return FileSink(sink_reference, JsonFileFormat())


class NileJobTestCase(unittest.TestCase):
    """
    Класс для написания юнит-тестов на найле. На основе
    https://github.yandex-team.ru/taxi-dwh/dwh/blob/b49721fb086e463cdf63d4f5577f0a3e15ffc10f/dmp_suite/test_dmp_suite/testing_utils/nile_testing_utils.py#L114
    """

    file_format = JsonFileFormat()

    maxDiff = None

    def assertCorrectLocalNileRun(self, job, input, answer, schema=None, input_label='input'):
        """
        Локально вызывает цепочку операций найла и проверяет соответствие результирующих данных — каноническим
        :param job: nile-job с построенной цепочке, где входные данные размечены `input_label`, а выходные — меткой `output`
        :param input: путь к файлу с исходными данными в формате json
        :param answer: путь к файлу с каноническими даннными в формате json
        :param schema: опционально - схема исходного потока
        :param input_label: - название метки с входной таблицей. По-умолчанию `input`
        :return:
        """

        output_file = local_nile_run(job, input, schema, input_label)
        output_data = _get_records(_resolve_source(output_file))
        answer_data = _get_records(_resolve_source(answer))

        for output_record, answer_record in longest_zip(output_data, answer_data):
            out_result = to_dict(output_record)
            answer_result = to_dict(answer_record)

            if answer_result is None:
                assert out_result is None
            else:
                self.assertDictEqual(out_result, answer_result)


def local_nile_run(job, input, schema=None, input_label='input'):
    """
    Локально вызывает цепочку операций найла и возвращает результат в виде json
    :param job: nile-job с построенной цепочке, где входные данные размечены `input_label`, а выходные — меткой `output`
    :param input: путь к файлу с исходными данными в формате json
    :param schema: опционально - схема исходного потока
    :param input_label: - название метки с входной таблицей. По-умолчанию `input`
    :return dict:
    """
    output_file = _get_output_filename(input)

    job.local_run(
        sources={input_label: _resolve_source(input, schema)},
        sinks={'output': _resolve_sink(output_file)},
        allow_remote_requests=False,
    )

    return output_file
