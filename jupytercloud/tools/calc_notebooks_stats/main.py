# -*- coding: utf-8 -*-

from __future__ import absolute_import, print_function, division, unicode_literals

import argparse
import ast
import datetime
import itertools
import pathlib2
import ujson
import re
import sys

from threading import RLock
from collections import defaultdict

from jupytercloud.tools.lib import parallel, utils
from statface_client.constants import STATFACE_PRODUCTION_UPLOAD
from nile.api.v1 import statface as ns

# XXX: dirty hack to fix pathlib2 unicode problems for python2 and cyrillic
# paths
reload(sys)
sys.setdefaultencoding('UTF8')

logger = None

TOTAL = '_total_'

OLD_DAYS = 90
OLD_DATETIME = datetime.datetime.now() - datetime.timedelta(days=OLD_DAYS)

REPORT_PATH = 'Statface/JupyterCloud/packages_usage'
REPORT_TITLE = 'Использование импортов в ноутбуках'


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


def get_language_names(notebook):
    language_info = notebook.get('metadata', {}).get('language_info', {})
    language_info.setdefault('name', 'unknown')
    language_info.setdefault('version', 'unknown')

    return ['{name} {version}'.format(**language_info), TOTAL]


def get_kernel_types(notebook):
    kernel_spec = notebook.get('metadata', {}).get('kernelspec', {})
    name = kernel_spec.get('name', 'unknown')

    if name == 'ir':
        result = 'R Lang'
    elif (
        name in ('python2', 'python3') or
        name.startswith('arcadia_default_')
    ):
        result = name
    else:
        result = 'other'

    res = [result, TOTAL]

    if name in ('python2', 'python3'):
        res.append('python2_or_3')
    elif (name.startswith('arcadia_default_')):
        res.append('arcadia_default_2_or_3')

    return res


def get_age(access_datetime):
    if OLD_DATETIME > access_datetime:
        age = '_old_'
    else:
        age = '_new_'

    return [age, TOTAL]


class ImportVisitor(ast.NodeVisitor):
    def __init__(self, source):
        self.imports = set()
        self.source = source

    def visit_Import(self, node):
        for alias in node.names:
            self.imports.add(alias.name)

    def visit_ImportFrom(self, node):
        if node.level > 0:
            return

        module = node.module

        for alias in node.names:
            self.imports.add(
                '.'.join((module, alias.name))
            )

    def visit_Attribute(self, node):
        # XXX: Может захватывать лишку, если будет не-найл переменная
        # clusters
        if (
            isinstance(node.value, ast.Name) and
            node.value.id == 'clusters' and
            isinstance(node.attr, str)
        ):
            value = node.attr.lower()
            if ('yt' in value):
                res = '_yt_'
            elif ('yql' in value):
                res = '_yql_'
            else:
                res = '_other_'

            self.imports.add('nile.api.v1.clusters.' + res)


def extract_modules(source):
    try:
        tree = ast.parse(source)
    except SyntaxError as e:
        logger.debug('syntax error %s at cell:\n%s', e, source)

        return {'_syntax_error_'}

    visitor = ImportVisitor(source)
    visitor.visit(tree)

    result = set()
    for import_ in visitor.imports:
        modules = import_.split('.')
        path = []
        for module in modules:
            path.append(module)
            result.add('.'.join(path))

    return result


def get_notebook_modules(notebook):
    language_info = notebook.get('metadata', {}).get('language_info', {})
    language = language_info.get('name')

    cells = get_notebook_cells(notebook)

    if language != 'python':
        return {
            TOTAL: len(cells),
            '_not_python_': len(cells),
        }

    modules = defaultdict(int)

    for cell in cells:
        modules[TOTAL] += 1
        if cell.get('cell_type') != 'code':
            modules['_not_a_code_'] += 1
            continue

        raw_source_lines = cell.get('source', [])
        raw_source = ''.join(raw_source_lines).strip()
        if raw_source.startswith('%'):
            modules['_magic_'] += 1
            continue
        elif raw_source.startswith('!'):
            modules['_shell_'] += 1
            continue
        elif not raw_source:
            modules['_empty_'] += 1
            continue

        source_lines = (
            line for line in raw_source_lines
            if not re.match(r'\s*[!%#]', line)
        )
        source = ''.join(source_lines)

        for module in extract_modules(source):
            modules[module] += 1

    return modules


def get_notebook_cells(notebook):
    cells = notebook.get('cells', [])
    for worksheet in notebook.get('worksheets', []):
        cells.extend(worksheet.get('cells', []))

    return cells


def get_access_datetime(metadata):
    access_time = metadata['access_time']
    return datetime.datetime.fromtimestamp(access_time)


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
    dimensions = ('language_name', 'kernel_type', 'age', 'imports')

    def __init__(self):
        self._users = defaultdict(set)
        self._notebooks = defaultdict(int)
        self._cells = defaultdict(int)
        self._max_date = None
        self._lock = RLock()

    def _add_dimensions(self, user, dimensions, cells):
        with self._lock:
            self._users[dimensions].add(user)
            self._notebooks[dimensions] += 1
            self._cells[dimensions] += cells

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

        access_datetime = get_access_datetime(metadata)
        with self._lock:
            if self._max_date is None or self._max_date < access_datetime:
                self._max_date = access_datetime

        lang_names = get_language_names(notebook)
        kernel_types = get_kernel_types(notebook)
        ages = get_age(access_datetime)
        cells_modules = get_notebook_modules(notebook)
        tree_modules = modules_to_tree(cells_modules)

        all_dimensions = (lang_names, kernel_types, ages, tree_modules)

        for dimensions in itertools.product(*all_dimensions):
            cells_nu = tree_modules[dimensions[-1]]
            self._add_dimensions(user, dimensions, cells_nu)

    def get_stats(self):
        stats = []
        for dimensions, notebooks in self._notebooks.items():
            users = len(self._users[dimensions])
            cells = self._cells[dimensions]

            total_dim = dimensions[:-1] + (('R',),)
            total_users = len(self._users[total_dim])
            total_notebooks = self._notebooks[total_dim]
            total_cells = self._cells[total_dim]

            dimensions_result = dict(
                zip(self.dimensions, dimensions),
                fielddate=self._max_date.strftime('%Y-%m-%d'),
                users=users,
                notebooks=notebooks,
                cells=cells,
                users_share=users / total_users,
                notebooks_share=notebooks / total_notebooks,
                cells_share=cells / total_cells,
            )

            stats.append(dimensions_result)

        return stats


def publish_result(calculator, args):
    data = calculator.get_stats()

    statface_client = ns.StatfaceClient(
        proxy=args.statface_proxy
    )

    report = ns.StatfaceReport() \
        .client(statface_client) \
        .path(REPORT_PATH) \
        .title(REPORT_TITLE) \
        .scale('daily') \
        .dimensions(
            ns.Date('fielddate').replaceable(),
            ns.StringSelector('language_name', default='_total_').title('Язык'),
            ns.StringSelector('kernel_type', default='_total_').title('Тип ядра'),
            ns.StringSelector('age', default='_total_').title('Возраст ноутбука').dictionaries(
                ns.LocalDict({
                    '_old_': 'Старше {} дней'.format(OLD_DAYS),
                    '_new_': '{} и младше'.format(OLD_DAYS),
                })
            ),
            ns.TreeSelector('imports', default='_in_table_').title('Модули').dictionaries(
                ns.LocalDict({
                    '_syntax_error_': 'Синтаксическая ошибка',
                    '_not_python_': 'Не Python',
                    '_not_a_code_': 'Ячейка не с кодом',
                    '_magic_': 'Magic',
                    '_shell_': 'Shell',
                    '_empty_': 'Пустая ячейка',
                })
            )
        ) \
        .measures(
            ns.Integer('users').title('Пользователей'),
            ns.Percentage('users_share').title('Доля пользователей').precision(2),
            ns.Integer('notebooks').title('Ноутбуков'),
            ns.Percentage('notebooks_share').title('Доля ноутбуков').precision(2),
            ns.Integer('cells').title('Ячеек').hidden(),
            ns.Percentage('cells_share').title('Доля ячеек').precision(2).hidden(),
        ) \
        .data(data)

    report.publish()

    logger.info('report published to %s', report.url)


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

    publish_result(calculator, args)
