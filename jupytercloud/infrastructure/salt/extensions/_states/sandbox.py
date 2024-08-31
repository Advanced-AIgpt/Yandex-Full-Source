# -*- coding: utf-8 -*-

from __future__ import absolute_import, unicode_literals, print_function

import os
import tempfile
import logging
import shutil

from salt.exceptions import CommandExecutionError

ARCADIA_KERNELS_RESOURCE_TYPE = 'JUPYTER_ARCADIA_DEFAULT_KERNELS'

logger = logging.getLogger(__name__)


def __virtual__():
    try:
        import sandbox.common.rest
        return True
    except:
        return False, 'unable to import sandbox.common.rest'


class _TempDir(object):
    def __init__(self, suffix=''):
        self.suffix = suffix
        self.path = None

    def __enter__(self):
        self.path = tempfile.mkdtemp(suffix=self.suffix)
        os.chmod(self.path, 0o777)

        return self.path

    def __exit__(self, *exc_info):
        if os.path.isdir(self.path):
            shutil.rmtree(self.path)
        self.path = None


def arcadia_kernels(name, kernels, verbosity=1):
    res_info = __salt__['sandbox.get_last_resource_info'](ARCADIA_KERNELS_RESOURCE_TYPE)

    ret = {
        'name': name,
        'result': True,
        'changes': {},
        'comment': ''
    }

    def comment(string, *args):
        ret['comment'] += string.format(*args) + '\n'

    last_revision = unicode(res_info['attributes']['svn_revision'])
    skynet_id = res_info['skynet_id']
    proxy_url = res_info['http']['proxy']
    filename = res_info['file_name']

    current_versions = {}
    absent_kernels = []

    for kernel in kernels:
        kernel_path = os.path.join(name, kernel, kernel)
        current_version = __salt__['sandbox.get_arcadia_kernel_version'](kernel_path)
        if current_version is None:
            absent_kernels.append(kernel)
        else:
            current_versions[kernel] = current_version

    if (
        not absent_kernels
        and all(v == last_revision for v in current_versions.values())
    ):
        comment('all arcadia kernels have last available revision {}', last_revision)
        return ret

    comment('last available revision: {}', last_revision)
    comment('current kernels revisions: {}', current_versions)
    comment('absent kernels: {}', absent_kernels)

    with _TempDir() as tmp_dir:
        try:
            __salt__['sandbox.load_resource'](skynet_id, tmp_dir)
        except CommandExecutionError as e:
            comment('sky get failed with error: {}', e)
            comment('fallback to wget')

            __salt__['cmd.run'](
                cmd='wget -q {} -O {}'.format(proxy_url, filename),
                cwd=tmp_dir,
                raise_err=True,
            )

        logger.info('resource %s loaded to %s', skynet_id, tmp_dir)
        comment('resource {} loaded to {}', skynet_id, tmp_dir)

        install_out = __salt__['sandbox.install_arcadia_kernels'](
            archive=filename,
            prefix=name,
            cwd=tmp_dir,
            verbosity=verbosity,
        )

        comment('install command output:\n{}', install_out)

    for kernel in kernels:
        kernel_path = os.path.join(name, kernel, kernel)

        current_version = __salt__['sandbox.get_arcadia_kernel_version'](kernel_path)

        ret['changes'][kernel_path] = {
            'old': (
                'kernel had revision {}'.format(current_versions[kernel])
                if kernel in current_versions else
                'kernel was not installed'
            ),
            'new': 'installed kernel with revision {}'.format(last_revision)
        }

        if current_version != last_revision:
            ret['result'] = False
            ret['changes'][kernel_path]['new'] += (
                '; kernel for some reason have inconsistent revision: '
                'expected {}, but installed {}'.format(last_revision, current_version)
            )

    return ret
