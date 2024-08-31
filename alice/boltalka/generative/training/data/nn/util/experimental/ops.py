import importlib
import inspect
import shutil

import vh
import yt.wrapper as yt
from alice.boltalka.generative.training.data.nn.util.experimental.util import get_fn_text


@vh.lazy.hardware_params(vh.HardwareParams(max_ram=1000))
@vh.lazy.from_annotations
def map_fn_importlib_lazy(input_table: vh.YTTable, fn_package: str, fn_name: str) -> vh.YTTable:
    """
    Run mapper function given its full name. Assummes that it is visible from the lazy function.
    """
    out_table = yt.TablePath(yt.create_temp_table())
    module = importlib.import_module(fn_package)
    fn = getattr(module, fn_name)

    yt.run_map(fn, input_table, out_table)
    return out_table


def map_fn_importlib(input_table: vh.YTTable, fn) -> vh.YTTable:
    """
    Run mapper function given its full name. Assummes that it is visible from the lazy function.
    """
    return map_fn_importlib_lazy(input_table, inspect.getmodule(fn).__name__, fn.__name__)


def map_fn(input_table: vh.YTTable, fn, imports=[], mr_account='tmp') -> vh.YTTable:
    """
    Run mapper by reading source lines of the function through `inspect`. Would not work for
    procedurally generated functions, classes, class objects, lambdas and other complex stuff.
    """
    source = get_fn_text(fn)

    return vh.op(id='08932379-31fb-4a19-8746-02a1553d297c')(
        input=input_table,
        _options={'code': source, 'fn_name': fn.__name__, 'import': imports},
        yt_token=vh.get_yt_token_secret(),
        yt_table_outputs=['output'],
        mr_account=mr_account
    )['output']


@vh.lazy.hardware_params(vh.HardwareParams(max_ram=10000))
@vh.lazy.from_annotations
def map_fn_lazy(input_table: vh.YTTable, mapper: vh.mkinput(object),
                aux_files: vh.mkinput(vh.File, nargs='*'), memory_limit_mb: vh.mkoption(int, default=512), output_table: vh.mkoutput(vh.YTTable)) -> vh.YTTable:
    new_file_paths = ['file_{}'.format(i) for i in range(len(aux_files))]

    for file_path, new_file_path in zip(aux_files, new_file_paths):
        shutil.copy(file_path, new_file_path)

    yt.run_map(mapper, input_table, output_table, local_files=new_file_paths, memory_limit=memory_limit_mb * yt.common.MB, ordered=True)
    return output_table


def lazy_mapper_method(func):
    """Decorator that makes an instance method into a lazy that maps it to a table

    This code tries to place the lazy function as close to the original method location as possible
    to get around problems with imports and inheritance

    Usage:
    class Something:
        @lazy_mapper_method
        def process_row(self, row, arg1, arg2):
            row[self.a] += arg1
            row[self.b] += arg2
            yield row

    then call as
    something_instance.process_row(table, arg1, arg2)

    !!!Do not modify instance members inside your method
    """
    def wrapped_mapper(
            self_: object,
            input_table: vh.mkinput(vh.YTTable),
            kwargs: object,
            output_table: vh.mkoutput(vh.YTTable),
    ) -> vh.YTTable:
        def mapper(row):
            yield from func(self_, row, **kwargs)
        yt.run_map(mapper,
                   input_table,
                   output_table,
                   memory_limit=512 * yt.common.MB,
                   ordered=True)
        return output_table

    wrapped_mapper.__module__ = func.__module__
    wrapped_mapper.__name__ = func.__name__
    wrapped_mapper.__qualname__ = func.__qualname__ + '.lazy_map'

    wrapped_mapper = vh.lazy.hardware_params(vh.HardwareParams(max_ram=10000))(
        vh.lazy.from_annotations(wrapped_mapper)
    )

    def wrapper(self_, input_table: vh.YTTable, **kwargs):
        return wrapped_mapper(self_, input_table, kwargs).output_table

    wrapper.lazy_map = wrapped_mapper

    return wrapper
