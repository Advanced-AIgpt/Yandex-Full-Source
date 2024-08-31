# -*- coding: utf-8 -*-

from __future__ import absolute_import, unicode_literals, print_function

import os

from salt.exceptions import CommandExecutionError


def __virtual__():
    try:
        import sandbox.common.rest
        return True
    except:
        return False, 'unable to import sandbox.common.rest'


def get_last_resource_info(resource_type, attrs=None):
    import requests
    if not isinstance(requests.__version__, basestring):
        requests.__version__ = requests.__version__.__version__

    import sandbox.common.rest

    client = sandbox.common.rest.Client()

    filter_ = dict(
        type=resource_type,
        limit=1,
        order="-id",
        state='READY'
    )

    if attrs:
        if not isinstance(attrs, dict):
            raise CommandExecutionError('attrs must be a dict')

        filter_['attrs'] = attrs

    result = client.resource.read(**filter_)

    items = result.get('items')
    if not items:
        raise CommandExecutionError('failed to fetch any {} resource info'.format(resource_type))

    return items[0]


def load_resource(skynet_id, directory):
    return __salt__['cmd.run'](
        cmd='sky get {}'.format(skynet_id),
        cwd=directory,
        raise_err=True,
    )


def install_arcadia_kernels(archive, prefix, cwd, verbosity):
    _run = __salt__['cmd.run']

    _run(
        cmd='tar -zf {} --get manage'.format(archive),
        cwd=cwd,
        raise_err=True,
    )

    verbosity = 'v' * verbosity

    return _run(
        cmd='./manage install {} -{} --yes --custom {}'.format(archive, verbosity, prefix),
        cwd=cwd,
        raise_err=True,
    )


def get_arcadia_kernel_version(kernel_path):
    if os.path.exists(kernel_path):
       return __salt__['cmd.run'](
           cmd='{} arcadia-info svn-revision'.format(kernel_path),
       ).strip()
    return None
