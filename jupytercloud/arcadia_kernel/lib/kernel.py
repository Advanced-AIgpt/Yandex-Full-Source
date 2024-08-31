# -*- coding: utf-8 -*-

from __future__ import absolute_import, print_function, division, unicode_literals

import argparse
import sys
import six
from ipykernel.kernelapp import IPKernelApp
from ipykernel.ipkernel import IPythonKernel
from ipykernel.zmqshell import ZMQInteractiveShell
from traitlets import List, Type
from traitlets.config import Application

from library.python import resource
from library.python import svn_version as vcs_info

from .complete import ArcadiaModuleCompleter
from .log import FileLogMixin
if six.PY3:
    from .run import RunNotebookApp


DEFAULT_LINKS = IPythonKernel.help_links.make_dynamic_default()
ADDITIONAL_LINKS = [
    {'text': 'Jupyter Cloud telegram chat', 'url': 'https://nda.ya.ru/3UYxBK'}
]

ARCADIA_EXTENSIONS = [
    'yql.ipython.magic',
    'sql.magic',
]


class ArcadiaZMQInteractiveShell(ZMQInteractiveShell):
    def init_completer(self):
        super(ArcadiaZMQInteractiveShell, self).init_completer()

        complete_command_strdispatch = self.strdispatchers['complete_command']
        completer = ArcadiaModuleCompleter()
        for key in ('%aimport', 'from', 'import'):
            # kill default module completer it completes by walks filesystem,
            # which is pointless in Arcadia binary
            del complete_command_strdispatch.strs[key]

            self.set_hook('complete_command', completer, str_key=key)

    def get_arcadia_info(self):
        return {
            'svn_revision': vcs_info.svn_revision(),
            'svn_version': vcs_info.svn_version(),
            'ya_make': resource.find('ya.make'),
        }


class ArcadiaKernel(IPythonKernel):
    help_links = List(ADDITIONAL_LINKS + DEFAULT_LINKS)
    shell_class = Type(ArcadiaZMQInteractiveShell)


class ArcadiaInfoApp(Application):
    name = 'arcadia-info'

    def initialize(self, argv=None):
        if argv is None:
            argv = sys.argv[1:]
        self.argv = argv

    def start(self):
        parser = argparse.ArgumentParser()
        subparser = parser.add_subparsers(dest='command')
        subparser.add_parser('svn-revision')
        subparser.add_parser('svn-version')
        subparser.add_parser('ya-make')
        subparser.add_parser('python-version')

        options = parser.parse_args(self.argv)
        if options.command == 'svn-revision':
            revision = vcs_info.svn_revision()
            sha256 = vcs_info.hash()
            print(revision if revision > 0 else sha256)
        elif options.command == 'svn-version':
            print(vcs_info.svn_version())
        elif options.command == 'ya-make':
            print(resource.find('ya.make'))
        elif options.command == 'python-version':
            print(sys.version)


class ArcadiaKernelApp(FileLogMixin, IPKernelApp):
    kernel_class = ArcadiaKernel

    subcommands = {
        ArcadiaInfoApp.name: (
            '{}.{}'.format(ArcadiaInfoApp.__module__, ArcadiaInfoApp.__name__),
            'get Arcadia specific information about kernel',
        ),
    }
    if six.PY3:
        subcommands[RunNotebookApp.name] = (
            '{}.{}'.format(RunNotebookApp.__module__, RunNotebookApp.__name__),
            'run notebook'
        )

    subcommands.update(IPKernelApp.subcommands)

    def init_extensions(self):
        super(ArcadiaKernelApp, self).init_extensions()

        self.log.debug('Loading Ardadia extensions...')

        extension_manager = self.shell.extension_manager
        for extension in ARCADIA_EXTENSIONS:
            if extension not in extension_manager.loaded:
                try:
                    extension_manager.load_extension(extension)
                # in Python 3.7 we can meet ModuleNotFoundError, which is
                # descendant of ImportError.
                except ImportError:
                    # If someone uses just lib PEERDIR without an user packages,
                    # .load_extension may raise due to lack of dependencies
                    pass

    def initialize(self, argv=None):
        super(ArcadiaKernelApp, self).initialize(argv)

        self.init_logging()


def main():
    # this magic copied from default ipykernel binary
    if sys.path[0] == '':
        del sys.path[0]

    ArcadiaKernelApp.launch_instance()


if __name__ == '__main__':
    main()
