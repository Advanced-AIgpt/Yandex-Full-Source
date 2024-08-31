# coding: utf-8

from __future__ import unicode_literals, absolute_import

import json
import logging
import multiprocessing as mp
import re
from collections import Mapping
from hashlib import md5
from itertools import izip, chain, combinations
from operator import itemgetter, mul
from uuid import uuid4, UUID

import attr
import numpy as np

from vins_core.utils.config import get_setting
from vins_core.utils.strings import smart_utf8


EPS = 1e-16
_DEFAULT_IGNORED_EXCEPTIONS = ('FstNormalizerError', 'NormalizerEmptyResultError')

logger = logging.getLogger(__name__)


def index_of(l, value, default=-1):
    try:
        return l.index(value)
    except ValueError:
        return default


def ensure_list(value):
    if isinstance(value, list):
        return value
    else:
        return [value]


def dict_zip(keys, values):
    # fastest way for creating dict
    return {k: v for k, v in izip(keys, values)}


def multiply_dicts(dicts):
    result = {}
    for key in set().union(chain.from_iterable(dicts)):
        result[key] = reduce(mul, [d[key] for d in dicts], 1)
    return result


def best_score_dicts(dicts):
    result = {}
    for key in set().union(chain.from_iterable(dicts)):
        result[key] = reduce(max, [d[key] for d in dicts])
    return result


def dict_shuffle_split(input_dict, first_fraction=None, rng_seed=None):
    """
    Given dict with list of items as values, split into two dicts of the same type with randomly chosen items
    :param input_dict: input dict
    :param first_fraction: 0 <= first_fraction <= 1 - how may items should be taken for first returned dict
    :param rng_seed: random seed
    :return:
    """
    first, second = {}, {}
    rng = np.random.RandomState(rng_seed)
    for key, values in input_dict.iteritems():
        num_values = len(values)

        indices = range(num_values)
        num = int(first_fraction * num_values)
        rng.shuffle(indices)
        if num > 0:
            first[key] = [values[i] for i in indices[:num]]
        if num < num_values:
            second[key] = [values[i] for i in indices[num:]]
        if not first.get(key):
            logger.error(
                'Couldn''t sample %f items from key="%s" of length %d. %s dict will miss this key' % (
                    first_fraction, key, num_values, 'First')
            )
        if not second.get(key):
            logger.error(
                'Couldn''t sample %f items from key="%s" of length %d. %s dict will miss this key' % (
                    first_fraction, key, num_values, 'Second')
            )

    return first, second


@attr.s
class ParallelItemError(object):

    message = attr.ib(default='')
    item = attr.ib(default=None)
    exception_type = attr.ib(default='')


def _apply_function(item, function, function_kwargs, raise_on_error, ignore_exceptions, initializer_result=None):
    try:
        result = function(item, initializer=initializer_result, **function_kwargs)
    except Exception as e:
        exception_type = e.__class__.__name__
        if raise_on_error and exception_type not in ignore_exceptions:
            logger.error('Item is processed with error: %s', unicode(e), exc_info=True)
            raise
        else:
            result = ParallelItemError(message=unicode(e), item=item, exception_type=exception_type)
    return result


def _filter_errors(items):
    output = []
    for item in items:
        if isinstance(item, ParallelItemError):
            logger.warning('The following ParallelItemError will be filtered out: %s', repr(item).decode('utf-8'))
            continue
        output.append(item)
    return output


def _ensure_processes_terminate(procs, expected_timeout):
    for proc_index, proc in enumerate(procs):
        proc_timeout = expected_timeout if proc_index == 0 else 1
        proc.join(proc_timeout)
        if proc.exitcode is None:
            logger.error("Kill hanging process %d, timeout exceeded %d sec", proc.pid, proc_timeout)
            proc.terminate()


def _get_initializer_result(initializer, initializer_kwargs):
    initializer_kwargs = initializer_kwargs or {}
    return initializer(**initializer_kwargs) if initializer else None


def _parallel_out_queue_target(
    function, function_kwargs, items, outq, batch_id, initializer, initializer_kwargs, ignore_exceptions
):
    results = []
    initializer_result = _get_initializer_result(initializer, initializer_kwargs)
    for item in items:
        results.append(_apply_function(
            item, function, function_kwargs, raise_on_error=False, ignore_exceptions=ignore_exceptions,
            initializer_result=initializer_result
        ))
    outq.put((batch_id, results))


def _parallel_out_queue(
    function, function_kwargs, items, num_procs, initializer, initializer_kwargs, ignore_exceptions,
):
    out_queue = mp.Queue()
    outputs, procs = [], []
    for batch_id, items_batch in enumerate(np.array_split(items, num_procs)):
        proc = mp.Process(
            target=_parallel_out_queue_target,
            args=(
                function, function_kwargs, items_batch, out_queue, batch_id,
                initializer, initializer_kwargs, ignore_exceptions
            )
        )
        proc.start()
        procs.append(proc)
    output = [out_queue.get() for _ in xrange(num_procs)]
    for batch_id, output_batch in sorted(output, key=itemgetter(0)):  # ordered by items
        outputs.extend(output_batch)
    _ensure_processes_terminate(procs, expected_timeout=len(items))
    return outputs


def _parallel_in_out_queue_target(
    function, function_kwargs, inq, outq, initializer, initializer_kwargs, ignore_exceptions,
):
    initializer_result = _get_initializer_result(initializer, initializer_kwargs)
    for index, item in iter(inq.get, None):
        result = _apply_function(
            item, function, function_kwargs, raise_on_error=False, ignore_exceptions=ignore_exceptions,
            initializer_result=initializer_result
        )
        outq.put((index, result))


def _parallel_in_out_queue(
    function, function_kwargs, items, num_procs, initializer, initializer_kwargs, ignore_exceptions,
):
    in_queue, out_queue = mp.Queue(), mp.Queue()
    outputs, procs = [], []
    for i, item in enumerate(items):
        in_queue.put((i, item))
    for _ in xrange(num_procs):
        in_queue.put(None)
    for _ in xrange(num_procs):
        proc = mp.Process(
            target=_parallel_in_out_queue_target,
            args=(
                function, function_kwargs, in_queue, out_queue, initializer, initializer_kwargs,
                ignore_exceptions
            )
        )
        proc.start()
        procs.append(proc)
    outputs = map(itemgetter(1), sorted([out_queue.get() for _ in xrange(len(items))], key=itemgetter(0)))
    _ensure_processes_terminate(procs, expected_timeout=len(items))
    return outputs


def parallel(
    function, items, num_procs=None, raise_on_error=True, initializer=None,
    function_kwargs=None, initializer_kwargs=None, filter_errors=False,
    ignore_exceptions=_DEFAULT_IGNORED_EXCEPTIONS, parallel_type='in_out_queue'
):
    """
    Analogous to multiprocessing.Pool.map function
    :param function: function to be parallelized, with signature f(item, initializer=None, **function_kwargs),

    :param items: list of processed items
    :param num_procs: number of concurrent processes.
            - if num_procs=1, operation is performed in the main process.
            - if num_procs=None, the number of parallel processes is taken from the VINS_NUM_PROCS
             environmental variable
    :param raise_on_error: if False, all items that are processed with errors are packed in ParallelItemError,
            otherwise, exception will be raised
    :param initializer: initialization function to be run before map, result is passed to
    :param function_kwargs: keyword arguments passed to the function
    :param initializer_kwargs: keyword arguments passed to the initializer
    :param filter_errors: if True, all handled errors are filtered out,
                          otherwise resulted array contains ParallelItemError instances
    :param ignore_exceptions: specify list of exception types to be ignored when raise_on_error=True
    :return: processed items. When an exception is raised, the result depends on the number of concurrent processes:
            - if num_procs=1, error raised
            - if num_procs>1, the corresponding item is eliminated from the output
    """
    outputs = []
    function_kwargs = function_kwargs or {}
    initializer_kwargs = initializer_kwargs or {}
    if len(items) == 0:
        return outputs
    if not num_procs:
        num_procs = int(get_setting('NUM_PROCS', 1))
    num_procs = min(num_procs, len(items))
    if num_procs == 1:
        initializer_result = _get_initializer_result(initializer, initializer_kwargs)
        outputs = []
        for item in items:
            outputs.append(_apply_function(
                item, function, function_kwargs, raise_on_error=raise_on_error, ignore_exceptions=ignore_exceptions,
                initializer_result=initializer_result
            ))
    else:
        if parallel_type == 'out_queue':
            applier = _parallel_out_queue
        elif parallel_type == 'in_out_queue':
            applier = _parallel_in_out_queue
        else:
            raise ValueError('Unknown parallel applier type: %s' % parallel_type)
        logger.info('Forking into %d processes', num_procs)
        outputs = applier(
            function, function_kwargs, items, num_procs, initializer,
            initializer_kwargs, ignore_exceptions
        )
        if raise_on_error:
            parallel_item_errors_to_raise = []
            for i, item in enumerate(outputs):
                if isinstance(item, ParallelItemError) and item.exception_type not in ignore_exceptions:
                    parallel_item_errors_to_raise.append((i, item))
            if parallel_item_errors_to_raise:
                raise ValueError(smart_utf8(
                    'Following exceptions were found during parallel execution and cannot be ignored:\n%s' % (
                        '\n'.join(
                            'Item #%d is processed with error: %s' % (i, item.message)
                            for i, item in parallel_item_errors_to_raise
                        )
                    )
                ))
    if filter_errors:
        outputs = _filter_errors(outputs)
    return outputs


def call_once_on_dict(function, mappable, filter_errors=True, key_filter=None, **kwargs):
    """
    Identical to {key: function(items, **kwargs) for key, items in mappable.iteritems()}
    but only with one function call
    :param function: the function with signature f(items, **kwargs) where items is list
    :param mappable: mappable where values are list of items to be processed
    :param filter_errors: filter ParallelItemError from array result
    :param key_filter: function to check that a key should be processed, or None
    :param kwargs: keyword args passed to the function
    :return: mappable object {key: function(items)}
    """
    keys, items = [], []
    for key, key_items in mappable.iteritems():
        if key_filter and not key_filter(key):
            continue
        keys.append((key, len(items), len(items) + len(key_items)))
        items.extend(key_items)
    samples = function(items, **kwargs)
    output = {key: samples[start:end] for key, start, end in keys}
    if filter_errors:
        output = {key: _filter_errors(samples) for key, samples in output.iteritems()}
    return output


def is_close(obj1, obj2, tolerance=1e-6):
    if isinstance(obj1, np.ndarray) or isinstance(obj2, np.ndarray):
        return np.allclose(obj1, obj2, atol=tolerance)
    return abs(obj1 - obj2) < tolerance


def gen_uuid_for_tests(base=None):
    # Fix a part of the uuid so that test users can easily be separated from the real ones
    if base is None:
        base = str(uuid4())
    else:
        base = str(UUID(md5(base).hexdigest()))

    uuid_str = '-'.join(['deadbeef'] + base.split('-')[1:])
    return UUID(uuid_str)


def all_subsets(iterable):
    xs = list(iterable)
    return chain.from_iterable(combinations(xs, n) for n in xrange(len(xs) + 1))


def intersection(first_segment, second_segment):
    """ Check whether two segments (e.g. slots) have common elements """
    return second_segment['start'] < first_segment['end'] and second_segment['end'] > first_segment['start']


def set_of_segments_intersection(first_set, second_set):
    """ Check whether two sets of segments (e.g. multislots) have common elements
    E.g. [{'start': 0, 'end': 1}, {'start': 3, 'end': 5}] and [{'start': 2, 'end': 4}] do intersect (on the 3'rd token).
    E.g. [{'start': 0, 'end': 1}, {'start': 3, 'end': 5}] and [{'start': 1, 'end': 3}] do not intersect.
    """
    for first_segment in first_set:
        for second_segment in second_set:
            if intersection(first_segment, second_segment):
                return True
    return False


def read_exactly(fd, size):
    """ Read exactly 'size' bytes from file descriptor 'fd', on failure IOError is raised """
    data = str('')
    remaining = size

    while remaining > 0:
        new_data = fd.read(remaining)

        if len(new_data) == 0:
            raise IOError('Failed to read enough data')

        data += new_data

        remaining -= len(new_data)

    return data


def match_with_dict_of_patterns(pattern, inspected_object, try_regex=False):
    """ Check whether inspected_object contains everything that pattern contains """
    if not isinstance(pattern, Mapping) or not isinstance(inspected_object, Mapping):
        if pattern != inspected_object:
            if try_regex and isinstance(pattern, (str, unicode)) and isinstance(inspected_object, (str, unicode)):
                return bool(re.match(pattern, inspected_object))
            else:
                return False
        else:
            return True
    for key, expected_value in pattern.iteritems():
        if not match_with_dict_of_patterns(expected_value, inspected_object.get(key), try_regex=try_regex):
            return False
    return True


def copy_subtree(source, paths):
    """
    Create the minimal subtree of the given source that contains all the given paths.
    Input and output trees are represented as nested dicts, and the input may be json.
    Example: the command copy_subtree(source, [('a',), ('e', 'h')])
        where source = {'a': 'b', 'c': 'd', 'e': {'f': 'g', 'h': 'i'}}
        would copy source['a'] and source['e']['h'], and return {'a': 'b', 'e': {'h': 'i'}}
    """
    if isinstance(source, (str, unicode)):
        source = json.loads(source)
    result = dict()
    for path in paths:
        has_path = True
        branch = source
        for key in path:
            if key in branch:
                branch = branch[key]
            else:
                has_path = False
                break
        if not has_path:
            continue
        old_branch = source
        new_branch = result
        for key in path[:-1]:
            if key not in new_branch:
                new_branch[key] = dict()
            old_branch = old_branch[key]
            new_branch = new_branch[key]
        new_branch[path[-1]] = old_branch[path[-1]]
    return result


def get_short_hash(string):
    if isinstance(string, unicode):
        string = string.encode("utf-8")
    return md5(string).digest().encode('base64')[:5]


def str_to_bool(str_):
    return str(str_).lower() in ['1', 'true', 'yes', 'da']
