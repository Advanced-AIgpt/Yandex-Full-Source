from __future__ import print_function

import glob
import os
import sys
import json

from distutils import log
from os.path import join as pjoin
from setuptools import setup, Command, find_packages
from setuptools.command.build_py import build_py
from subprocess import check_call
from distutils.command.build import build

here = os.path.dirname(os.path.abspath(__file__))
build_dir = pjoin(here, 'build')
node_root = here
tar_path = pjoin(build_dir, '*.tgz')

npm_path = os.pathsep.join([
    pjoin(node_root, 'node_modules', '.bin'),
    os.environ.get('PATH', os.defpath),
])

log.info('setup.py entered')
log.info('$PATH=%s' % os.environ['PATH'])

with open(pjoin(here, 'package.json')) as f_:
    package_json = json.load(f_)


def js_prerelease(command):
    """decorator for building minified js/css prior to another command"""
    class DecoratedCommand(command):
        def run(self):
            jsdeps = self.distribution.get_command_obj('jsdeps')

            try:
                self.distribution.run_command('jsdeps')
            except Exception as e:
                missing = []
                for target in jsdeps.targets:
                    files = glob.glob(tar_path)
                    if not files:
                        missing.append(target)
                log.warn('rebuilding js and css failed')
                if missing:
                    log.error('missing files: %s' % missing)
                raise
            command.run(self)
            update_package_data(self.distribution)
    return DecoratedCommand


def update_package_data(distribution):
    """update package_data to catch changes during setup"""
    build_py = distribution.get_command_obj('build_py')
    # distribution.package_data = find_package_data()
    # re-init build_py options which load package_data
    build_py.finalize_options()


def get_data_files():
    """Get the data files for the package.
    """
    return [
        ('share/jupyter/lab/extensions', [
            os.path.relpath(f, '.') for f in glob.glob(tar_path)
        ]),
        ('etc/jupyter/jupyter_notebook_config.d', [
            'jupyter-config/jupytercloud_lab_vault.json',
            'jupyter-config/jupytercloud_lab_vault_nbserver.json'
        ])
    ]


class NPM(Command):
    description = 'install package.json dependencies using npm'

    user_options = []

    node_modules = pjoin(node_root, 'node_modules')

    targets = [
        tar_path
    ]

    def initialize_options(self):
        pass

    def finalize_options(self):
        pass

    def has_npm(self):
        try:
            check_call(['npm', '--version'])
            return True
        except Exception:
            return False

    def should_run_npm_install(self):
        node_modules_exists = os.path.exists(self.node_modules)
        return self.has_npm() and not node_modules_exists

    def should_run_npm_pack(self):
        return self.has_npm()

    def run(self):
        if not os.path.exists(build_dir):
            os.mkdir(build_dir)

        has_npm = self.has_npm()
        if not has_npm:
            log.error("`npm` unavailable. If you're running this command using sudo, make sure `npm` is available to sudo")

        env = os.environ.copy()
        env['PATH'] = npm_path

        if self.should_run_npm_install():
            log.info("Installing build dependencies with npm.  This may take a while...")
            check_call(['npm', 'install'], cwd=node_root, stdout=sys.stdout, stderr=sys.stderr)
            os.utime(self.node_modules, None)

        if self.should_run_npm_pack():
            check_call(['npm', 'pack', node_root], cwd=build_dir, stdout=sys.stdout, stderr=sys.stderr)

        for t in self.targets:
            files = glob.glob(tar_path)
            if not files:
                msg = 'Missing file: %s' % t
                if not has_npm:
                    msg += '\nnpm is required to build a development version of widgetsnbextension'
                raise ValueError(msg)

        self.distribution.data_files = get_data_files()

        # update package data in case this created new files
        update_package_data(self.distribution)


package_data_spec = {
    package_json['name']: [
        "*"
    ]
}

setup_args = {
    'name': "jupytercloud_lab_vault",
    'version': package_json['version'],
    'description': package_json['description'],
    'include_package_data': True,
    'data_files': get_data_files(),
    'install_requires': [
        'jupyterlab==2.1',
        'jupytercloud-lab-lib-extension>=0.3.0',
        'jupytercloud-lab-metrika>=0.2.0',
    ],
    'zip_safe': False,
    'cmdclass': {
        'build': js_prerelease(build),
        'jsdeps': NPM,
    },
    'packages': find_packages(),
    'author': package_json['author'],
    'author_email': 'jupyter@yandex-team.ru',
    'url': package_json['homepage'],
}

setup(**setup_args)
