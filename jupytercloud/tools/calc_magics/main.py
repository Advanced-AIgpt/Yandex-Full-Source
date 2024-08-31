# -*- coding: utf-8 -*-

from __future__ import absolute_import, print_function, division, unicode_literals

import argparse
import pathlib2
import ujson
import sys

from threading import RLock
from collections import defaultdict

from jupytercloud.tools.lib import parallel, utils
from statface_client.constants import STATFACE_PRODUCTION_UPLOAD

# XXX: dirty hack to fix pathlib2 unicode problems for python2 and cyrillic
# paths
reload(sys)
sys.setdefaultencoding('UTF8')

logger = None

TOTAL = '_total_'


def parse_args():
    parser = argparse.ArgumentParser()

    parser.add_argument('directory', type=pathlib2.Path)
    parser.add_argument('--threads', '-j', type=int, default=None)
    parser.add_argument('--statface-proxy', default=STATFACE_PRODUCTION_UPLOAD)
    parser.add_argument(
        '--verbose', '-v',
        action='count',
        help='print to stderr some additional info',
        default=0,
    )

    return parser.parse_args()


def decode_struct(value):
    if isinstance(value, str):
        return value.decode("utf-8")
    if isinstance(value, dict):
        return {decode_struct(k): decode_struct(v) for k, v in value.iteritems()}
    if isinstance(value, (tuple, list)):
        return type(value)(decode_struct(v) for v in value)
    return value


def get_notebook_modules(notebook):
    cells = get_notebook_cells(notebook)

    statements = defaultdict(int)

    for cell in cells:
        statements[TOTAL] += 1
        if cell.get('cell_type') != 'code':
            continue

        raw_source_lines = cell.get('source', [])
        for line in raw_source_lines:
            words = line.split()
            if not words:
                continue
            word = words[0]

            if line.startswith('!'):
                if word.strip() == '!':
                    if len(words) > 1:
                        word += words[1]
                    else:
                        continue
                statements[word] += 1
                statements['_shell_'] += 1
            elif line.startswith(r'%%'):
                statements[word] += 1
                statements['_magic_'] += 1
            elif line.startswith(r'%'):
                statements[word] += 1
                statements['_magic_'] += 1

    return statements


def get_notebook_cells(notebook):
    cells = notebook.get('cells', [])
    for worksheet in notebook.get('worksheets', []):
        cells.extend(worksheet.get('cells', []))

    return cells


def modules_to_tree(modules):
    result = {}
    for path, cells in modules.iteritems():
        if path == TOTAL:
            tree_path = ('R', )
        else:
            tree_path = ('R', ) + tuple(path.split('.'))

        result[tree_path] = cells

    return result


class Calculator(object):
    dimensions = ('statement', )

    def __init__(self):
        self._users = defaultdict(set)
        self._lock = RLock()

    def _add_dimensions(self, user, dimensions):
        with self._lock:
            self._users[dimensions].add(user)

    def process_path(self, path):
        with path.open() as f_:
            try:
                notebook = ujson.load(f_)
            except ValueError:
                logger.debug('failed to load %s', path, exc_info=True)
                return

        metadata_path = path.with_suffix('.meta.json')

        with metadata_path.open() as f_:
            try:
                metadata = ujson.load(f_)
            except ValueError:
                logger.debug('failed to load %s', path, exc_info=True)
                return

        user = path.parts[1]

        self._process_notebook(user, notebook, metadata)

    def _process_notebook(self, user, notebook, metadata):
        cells = get_notebook_cells(notebook)
        if not cells:
            return

        cells = notebook.get('cells', [])

        cells_modules = get_notebook_modules(notebook)

        for dimensions in cells_modules:
            self._add_dimensions(user, dimensions)

    def get_stats(self):
        import pprint
        pprint.pprint(dict(self._users))


def main():
    args = parse_args()

    global logger
    logger = utils.setup_logging(__name__, args.verbose)

    notebook_paths = list(args.directory.glob('**/*.ipynb'))
    calculator = Calculator()

    def _process_one(path):
        try:
            calculator.process_path(path)
        except Exception as e:
            logger.error('problem while processing %r', path, exc_info=True)
            print('problem while processing {!r}: {!r}'.format(path, e))

    parallel.process_with_progress(_process_one, notebook_paths, args.threads)

    calculator.get_stats()
