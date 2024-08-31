import io
import logging
import os
import sys
from contextlib import contextmanager
from tempfile import NamedTemporaryFile

LOG = logging.getLogger(__name__)

# PY3-compatible iteritems()
if sys.version_info[0] < 3:
    def _iteritems(d):
        return d.iteritems()
else:
    def _iteritems(d):
        return d.items()


def load_state(snapshot_path, load, *args, **kwargs):
    """
    Tries to load execution state from file `snapshot_path' using the deserialization function `load'.

    :param snapshot_path: path to the snapshot (need not exist)
    :param load: deserialization function with signature `(file-like object) -> Any', e.g., json.load, pickle.load
    :param args: additional positional arguments passed to the `load' function
    :param kwargs: additional named arguments to the `load_state' and `load' functions

                   These named arguments are used by the `load_state' function:
                     * read_mode: snapshot file read mode; 'rb' by default
                     * default_value: fallback value to return if snapshot is unavailable or could not be loaded; None by default
                   All other named arguments are passed to `load' intact.

    :return: execution state loaded from snapshot, or default_value if snapshot is not available or could not be loaded
    """

    read_mode = kwargs.get('read_mode', 'rb')
    default_value = kwargs.get('default_value', None)
    load_kwargs = {k: v for k, v in _iteritems(kwargs) if k not in ('read_mode', 'default_value')}

    if os.path.exists(snapshot_path):
        # snapshot exists, try to load state from it
        LOG.info('Trying to load state from snapshot: %s', snapshot_path)
        try:
            with io.open(snapshot_path, mode=read_mode) as f:
                return load(f, *args, **load_kwargs)
        except BaseException:
            # could not load snapshot, treat it as nonexistent
            LOG.exception('Could not load state from snapshot: %s', snapshot_path)
            return default_value
    else:
        return default_value


def dump_state(snapshot_path, state, dump, *args, **kwargs):
    """
    Tries to dump new execution state `state' to file `snapshot_path' using the serialization function `dump'.
    Snapshot is atomically updated: the new state is dumped to a temporary file, which is then renamed to the snapshot file.
    This ensures that Nirvana job wrapper (job_launcher) always sees consistent (though maybe stale) snapshot contents.

    :param snapshot_path: path to the snapshot (need not exist)
    :param state: new state to write to snapshot
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
    """
    write_mode = kwargs.get('write_mode', 'w+b')
    notify = kwargs.get('notify', False)
    dump_kwargs = {k: v for (k, v) in _iteritems(kwargs) if k not in ('write_mode', 'notify')}

    @contextmanager
    def atomically_saved_file(save_path):
        with NamedTemporaryFile(delete=False, dir=os.path.dirname(save_path), mode=write_mode) as temp_file:
            temp_file_name = temp_file.name
            yield temp_file
        os.rename(temp_file_name, save_path)

    try:
        with atomically_saved_file(snapshot_path) as f:
            dump(state, f, *args, **dump_kwargs)
    except BaseException:
        LOG.exception('Could not save state to snapshot: %s. New state was:\n%r', snapshot_path, state)


if __name__ == '__main__' and __package__ is None:
    __package__ = 'nirvana.snapshot'
