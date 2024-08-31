import getpass
import glob
import hashlib
import os
from datetime import datetime
from pathlib import Path

import alice.tests.library.ydb as ydb
import pytz


_root_path = os.path.join(str(Path.home()), '.ya', 'evo_opuses')


def _read_from_file(filepath):
    if not os.path.exists(filepath):
        return None
    with open(filepath) as stream:
        return stream.read()


def _generate_filename(text, labels):
    filename = hashlib.md5(text.encode()).hexdigest()
    filename += f'_{labels}' if labels else ''
    return filename


def _normalize(func):
    def wrapper(text, labels, *args, **kwargs):
        return func(text.lower(), '_'.join(labels or []), *args, **kwargs)
    return wrapper


@_normalize
def load(text, labels):
    filename = os.path.join(_root_path, _generate_filename(text, labels))
    for text_file in glob.iglob(f'{filename}_[0-9].txt'):
        if text == _read_from_file(text_file):
            return _read_from_file(f'{os.path.splitext(text_file)[0]}.opus')

    return ydb.download_opus(text, labels)


@_normalize
def save(text, labels, opus):
    created_time = datetime.now(pytz.utc)
    ydb.upload_opus([ydb.EvoOpusRow(
        timestamp=created_time,
        text=text,
        opus=opus,
        labels=labels,
        meta=dict(
            author=getpass.getuser(),
            created=created_time.isoformat(),
        ),
    )])
