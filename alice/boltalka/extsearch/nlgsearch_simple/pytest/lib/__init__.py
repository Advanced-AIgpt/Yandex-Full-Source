import yatest.common
import hashlib


def local_canonical_file(*args, **kwargs):
    return yatest.common.canonical_file(*args, local=True, **kwargs)


def get_file_hash(filename):
    h = hashlib.sha256()
    with open(filename, 'rb', buffering=0) as f:
        for b in iter(lambda: f.read(128*1024), b''):
            h.update(b)
    return h.hexdigest()
