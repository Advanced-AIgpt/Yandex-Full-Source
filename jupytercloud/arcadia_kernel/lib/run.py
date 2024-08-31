# -*- coding: utf-8 -*-

from __future__ import absolute_import, print_function, division, unicode_literals

import copy
import sys
import json
import logging
import yaml

from pathlib import Path
from contextlib import contextmanager
from functools import cached_property

from traitlets import Unicode, Dict, Bool, Integer, default
from traitlets.config import catch_config_error
from traitlets.config.loader import ArgumentError

from jupyter_core.application import JupyterApp
from jupyter_client.manager import AsyncKernelManager
from jupyter_client.launcher import launch_kernel
from jupyter_client.kernelspec import KernelSpec

from papermill.execute import execute_notebook
from papermill.log import logger as PapermillLogger
from papermill.iorw import load_notebook_node

import nbformat

from nbconvert import HTMLExporter

from .log import FileLogMixin


ARCADIA_DEFAULT_PY2 = 'arcadia_default_py2'
ARCADIA_DEFAULT_PY3 = 'arcadia_default_py3'

ARCADIA_KERNEL_NAMES = {
    ARCADIA_DEFAULT_PY2: 2,
    ARCADIA_DEFAULT_PY3: 3,
}


class AsyncArcadiaKernelManager(AsyncKernelManager):
    language_version = Unicode()
    language_name = Unicode()
    major_version = Integer()
    verify_kernelspec = Bool()

    kernel_log_file = Unicode()
    python2_path = Unicode()
    python3_path = Unicode()

    allow_self_as_kernel = Bool()

    @default('major_version')
    def _default_major_version(self):
        if not self.language_version:
            return None

        try:
            return int(self.language_version.split('.')[0])
        except (TypeError, KeyError):
            raise ValueError(
                'wrong format of language_version in notebook_metadata: {}'
                .format(self.language_version)
            )

    def _verify_kernelspec(self):
        if (self.verify_kernelspec or self.language_name) and self.language_name != 'python':
            raise ValueError('unsupported language {!r}'.format(self.language_name))

        if self.major_version not in {2, 3}:
            raise ValueError('unsupported version of language {!r}'.format(self.major_version))

        if not self.verify_kernelspec:
            return

        if self.kernel_name not in ARCADIA_KERNEL_NAMES:
            raise ValueError('unsupported kernel {}'.format(self.kernel_name))

        expected_version = ARCADIA_KERNEL_NAMES[self.kernel_name]

        if expected_version != self.major_version:
            raise ValueError(
                'notebook python version {!r} does not equal to '
                'kernel python version {!r}'.format(self.major_version, expected_version)
            )

        # TODO: also verify kernel revision here

    @cached_property
    def kernel_spec(self):
        self._verify_kernelspec()

        kernel_path = self.python2_path if self.major_version == 2 else self.python3_path
        if not kernel_path:
            if not self.allow_self_as_kernel:
                raise ValueError(
                    'trying to run notebook without --python*-kernel-path arguments or '
                    'kernels with default names on a disk'
                )

            if self.major_version != sys.version_info.major:
                raise ValueError(
                    'trying to run {!r} python version notebook '
                    'with {!r} python version runner without '
                    'passing of explicit kernel paths to args'
                    .format(self.major_version, sys.version_info.major)
                )

            kernel_path = sys.executable

        argv = [kernel_path, '-f', '{connection_file}']

        level_name = logging.getLevelName(self.log.level)
        argv.append('--ArcadiaKernelApp.log_level={}'.format(level_name))

        if self.kernel_log_file:
            # do not using shortcut log-file to be compatible with old versions
            # of kernels
            argv.append('--ArcadiaKernelApp.log_file={}'.format(self.kernel_log_file))

        yql_path = kernel_path + '_yql.so'
        env = {}
        if Path(yql_path).exists():
            self.log.info(
                'yql binary %r discovered, setupping env NILE_YQL_PYTHON_UDF_PATH',
                yql_path
            )
            env = {'NILE_YQL_PYTHON_UDF_PATH': yql_path}
        else:
            self.log.info('no yql binary %r discovered', yql_path)

        return KernelSpec(argv=argv, env=env)

    async def _launch_kernel(self, kernel_cmd, **kw):
        res = launch_kernel(kernel_cmd, _patch_entry_point=False, **kw)
        return res


class RunNotebookApp(FileLogMixin, JupyterApp):
    name = 'run-notebook'

    config_file = Unicode('run_notebook_config.py', config=True)

    notebook_path = Unicode(help="path to input notebook").tag(config=True)
    output_path = Unicode(help="path to output notebook").tag(config=True)
    output_html_path = Unicode(help="path to html render of output notebook").tag(config=True)
    stdout_path = Unicode(help="path to stdout output of run").tag(config=True)
    stderr_path = Unicode(help="path to stderr output of run").tag(config=True)

    allow_self_as_kernel = Bool(
        True,
        help="allow use this binary as kernel if kernels paths is ommited",
    ).tag(config=True)
    python2_kernel_path = Unicode(
        help="path to python2 kernel binary to use to execute notebook"
    ).tag(config=True)
    python3_kernel_path = Unicode(
        help="path to python3 kernel binary to use to execute notebook"
    ).tag(config=True)

    verify_kernelspec = Bool(
        True,
        help="check if notebook run with correct arcadia kernel",
    ).tag(config=True)

    kernel_start_timeout = Integer(
        300,
        help="how many seconds to wait to kernel start"
    ).tag(config=True)

    parameters_kv = Unicode(
        help="parameters in format 'a=1 b=2' with space delimiters",
    ).tag(config=True)
    parameters_json = Dict(
        help="""parameters string in JSON format '{"a":1,"b":2}'""",
    ).tag(config=True)
    parameters_yaml_path = Unicode(
        help="path to a file with parameters in yaml format"
    ).tag(config=True)
    parameters_json_path = Unicode(
        help="path to a file with parameters in json format"
    ).tag(config=True)

    aliases = Dict({
        'notebook-path': 'RunNotebookApp.notebook_path',
        'output-path': 'RunNotebookApp.output_path',
        'output-html-path': 'RunNotebookApp.output_html_path',
        'stdout-path': 'RunNotebookApp.stdout_path',
        'stderr-path': 'RunNotebookApp.stderr_path',
        'python2-kernel-path': 'RunNotebookApp.python2_kernel_path',
        'python3-kernel-path': 'RunNotebookApp.python3_kernel_path',
        'log-file': 'RunNotebookApp.log_file',
        'log-level': 'RunNotebookApp.log_level',
        'verify-kernelspec': 'RunNotebookApp.verify_kernelspec',
        'parameters-kv': 'RunNotebookApp.parameters_kv',
        'parameters-json': 'RunNotebookApp.parameters_json',
        'parameters-json-path': 'RunNotebookApp.parameters_json_path',
        'parameters-yaml-path': 'RunNotebookApp.parameters_yaml_path',
    })

    flags = copy.deepcopy(JupyterApp.flags)
    flags.update({
        'no-verify-kernelspec': (
            {'RunNotebookApp': {'verify_kernelspec': False}},
            'verify that notebook runs with corresponding ArcadiaKernel'
        ),
    })

    raise_config_file_errors = True

    @default('python2_kernel_path')
    def _default_python2_kernel_path(self):
        possible_path = Path.cwd() / ARCADIA_DEFAULT_PY2
        if possible_path.exists():
            return str(possible_path)
        return ''

    @default('python3_kernel_path')
    def _default_python3_kernel_path(self):
        possible_path = Path.cwd() / ARCADIA_DEFAULT_PY3
        if possible_path.exists():
            return str(possible_path)
        return ''

    @default('extra_loggers')
    def _default_extra_loggers(self):
        return [PapermillLogger]

    @property
    def config_file_paths(self):
        # supress configs search in base jupyter/ipython directories
        return []

    @catch_config_error
    def initialize(self, *args, **kwargs):
        super(RunNotebookApp, self).initialize(*args, **kwargs)

        if self._dispatching:
            return

        self.init_logging()

        if not self.notebook_path:
            raise ArgumentError('--notebook-path is mandatory')
        if not self.output_path:
            raise ArgumentError('--output-path is mandatory')

    @contextmanager
    def open_outputs(self):
        @contextmanager
        def _open(filename):
            if filename:
                path = Path(filename).expanduser().resolve()
                with path.open('w') as file_:
                    yield file_
            else:
                yield None

        with \
                _open(self.stdout_path) as stdout_file, \
                _open(self.stderr_path) as stderr_file:
            yield stdout_file, stderr_file

    def _validate_parameters(self, parameters):
        bad_parameters = []
        for key in parameters:
            if not key.isidentifier():
                bad_parameters.append(key)

        if bad_parameters:
            bad_parameters.sort()

            raise ArgumentError(
                'all notebook parameters must be a valid python identifiers; '
                '{} - are not'.format(', '.join(repr(p) for p in bad_parameters))
            )

    def get_parameters(self):
        parameters_kv = {}
        if self.parameters_kv:
            parameters_kv = {
                k: v
                for k, v in
                (p.split('=', 1) for p in self.parameters_kv.split(' '))
            }
        parameters_json = self.parameters_json
        parameters_json_file = {}
        parameters_yaml_file = {}
        if self.parameters_json_path:
            with open(self.parameters_json_path) as f_:
                parameters_json_file = json.load(f_)
        if self.parameters_yaml_path:
            with open(self.parameters_yaml_path) as f_:
                parameters_yaml_file = yaml.safe_load(f_)

        parameters = {}
        parameters.update(parameters_kv)
        parameters.update(parameters_json)
        parameters.update(parameters_json_file)
        parameters.update(parameters_yaml_file)

        self.log.info('Run with notebook parameters: %r', parameters)

        # TODO: check if parameter are present in one place?
        # TODO: give warning if parameter value is not string?
        return parameters

    def get_kernel_manager(self):
        notebook = load_notebook_node(self.notebook_path)

        kernel_name = notebook.metadata.get('kernelspec', {}).get('name')
        language_info = notebook.metadata.get('language_info', {})
        language_name = language_info.get('name')
        language_version = language_info.get('version')

        return AsyncArcadiaKernelManager(
            kernel_name=kernel_name,
            language_name=language_name,
            language_version=language_version,
            verify_kernelspec=self.verify_kernelspec,
            kernel_log_file=self.log_file,
            python2_path=self.python2_kernel_path,
            python3_path=self.python3_kernel_path,
            allow_self_as_kernel=self.allow_self_as_kernel,
            parent=self,
        )

    def start(self):
        super(RunNotebookApp, self).start()

        kernel_manager = self.get_kernel_manager()
        parameters = self.get_parameters()
        self._validate_parameters(parameters)

        try:
            with self.open_outputs() as (stdout_file, stderr_file):
                execute_notebook(
                    input_path=self.notebook_path,
                    output_path=self.output_path,
                    stdout_file=stdout_file,
                    stderr_file=stderr_file,
                    progress_bar=sys.stdout.isatty(),
                    parameters=parameters,
                    km=kernel_manager,
                    kernel_manager_class=AsyncArcadiaKernelManager,
                    start_timeout=self.kernel_start_timeout,
                )
        finally:
            output_path = Path(self.output_path)
            if output_path.is_file() and self.output_html_path:
                with output_path.open() as file_:
                    notebook_node = nbformat.read(file_, as_version=4)

                html_exporter = HTMLExporter()
                (html, _) = html_exporter.from_notebook_node(notebook_node)
                with open(self.output_html_path, 'w') as html_file:
                    html_file.write(html)
