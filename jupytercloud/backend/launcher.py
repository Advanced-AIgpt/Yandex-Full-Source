import argparse
import importlib
import inspect
import pkgutil
import sys
import typing as tp
from types import ModuleType

from traitlets import Any, HasTraits, Instance

import jupytercloud.backend.app
import jupytercloud.backend.services


class JCLauncher(HasTraits):
    parser = Instance(argparse.ArgumentParser)
    subparsers = Any()

    service_parent = jupytercloud.backend.services
    service_prefix = service_parent.__name__ + '.'
    ignored_services = {'base'}

    def find_service_packages(self):
        service_packages = [
            package.name
            for package in pkgutil.walk_packages(self.service_parent.__path__, self.service_prefix)
            if package.name.endswith('.app')
        ]
        return service_packages

    def add_subcommand(self, module: tp.Union[str, ModuleType], *, name=None, help=None):
        if isinstance(module, ModuleType):
            module_name = module.__name__
        elif isinstance(module, str):
            module_name = module
            module = importlib.import_module(module_name)
        else:
            raise TypeError(f'add_subcommand expects str or module, not {type(module)}')

        if name is None:
            name = module_name.split('.')[-2]  # just a heuristic
            if name in self.ignored_services:
                return

        if help is None:
            if (help := inspect.getdoc(module)) is not None:
                help = help.split('\n', 1)[0]

        subparser = self.subparsers.add_parser(name.replace('_', '-'), help=help, add_help=False)  # dashes are more git-like
        subparser.set_defaults(command=module.main)  # will throw if no main in the module
        subparser.add_argument('--config', default=f'{name}_config.py')

    def add_services(self):
        for pkg_name in self.find_service_packages():
            self.add_subcommand(pkg_name)

    def add_misc_commands(self):
        self.add_subcommand(jupytercloud.backend.app, name='backend', help='Main JupyterCloud backend')
        self.add_subcommand(jupytercloud.backend.lib.db.util, name='db_util', help='Database management utility')

    def initialize_argparse(self):
        self.parser = argparse.ArgumentParser()
        self.subparsers = self.parser.add_subparsers(title='services', required=True, metavar='<subcommand>')

        self.add_misc_commands()
        self.add_services()

    def launch(self):
        self.initialize_argparse()

        args, new_argv = self.parser.parse_known_args(sys.argv[1:])

        args.command(new_argv + ['--config', args.config])


def main():
    sys.exit(JCLauncher().launch())


if __name__ == '__main__':
    main()
