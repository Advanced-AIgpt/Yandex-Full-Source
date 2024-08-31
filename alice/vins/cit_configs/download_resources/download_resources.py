#!/usr/bin/env python
# coding: utf-8
from __future__ import unicode_literals

import argparse
import os
import logging
import yaml
import tarfile
import urllib2
from subprocess import check_call


def _update_symlink(path, symlink_path):
    if os.path.exists(symlink_path) and path != symlink_path:
        os.remove(symlink_path)
    if path != symlink_path:
        os.symlink(path, symlink_path)


def _download_resource(resource_base_dir, resource, transport):
    if 'symlink' not in resource:
        logging.warning('symlink field not found in resource description {}', resource)
        return
    target_path = os.path.join(resource_base_dir, resource['id'])

    if resource['extract'] == 'true':
        target_path = target_path + '_ext'

    symlink_path = os.path.join(resource_base_dir, resource['symlink'].split('/')[0])
    if os.path.isdir(target_path):
        if len(os.listdir(target_path)) > 0:
            _update_symlink(target_path, symlink_path)
            return
        else:
            os.rmdir(target_path)

    os.mkdir(target_path)
    if transport == 'skynet':
        check_call(['sky', 'get', '-d', target_path, '-N', 'Backbone', 'sbr:{}'.format(resource['id'])])
    elif transport == 'sandbox_proxy':
        url = 'https://proxy.sandbox.yandex-team.ru/{}?stream=tgz'.format(resource['id'])
        tmp_tar_file = os.path.join(resource_base_dir, 'tmp.tar.gz')
        open(tmp_tar_file, 'wb').write(urllib2.urlopen(url).read())
        tarfile.open(tmp_tar_file, 'r:gz').extractall(target_path)
        os.remove(tmp_tar_file)
    else:
        raise RuntimeError('Unknown transport type "{}"'.format(transport))

    if resource['extract'] == 'true':
        arch_name = os.path.join(target_path, os.listdir(target_path)[0])
        tarfile.open(arch_name).extractall(target_path)
        os.remove(arch_name)

    _update_symlink(target_path, symlink_path)


def deep_dict_update(obj, ext):
    if not isinstance(obj, (list, dict)):
        return obj
    for key in ext:
        if key in obj:
            if isinstance(obj[key], dict):
                deep_dict_update(obj[key], ext[key])
            elif isinstance(obj[key], list):
                obj[key] += ext[key]
            else:
                obj[key] = ext[key]
        else:
            obj[key] = ext[key]


def path_from_root(*path):
    return os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', *path))


def load_config(path):
    full_path = path_from_root('cit_configs', path)
    if not os.path.exists(full_path):
        raise RuntimeError('yaml file [%s] not found' % full_path)
    with open(full_path) as f:
        current_config = yaml.load(f)

    result_config = {}
    if 'include' in current_config:
        for include_path in current_config['include']:
            deep_dict_update(result_config, load_config(include_path))
    deep_dict_update(result_config, current_config)
    return result_config


def load_component(component_name, env_name):
    result_config = load_config('components/{0}/{1}.yaml'.format(component_name, env_name))
    deep_dict_update(result_config, load_config('nanny/base.yaml'))
    return result_config['component']


def cli():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--env', '-e', dest='environment_name', required=True
    )
    parser.add_argument(
        'components', metavar='component', nargs='+',
        help='components for deploy',
    )
    parser.add_argument(
        '--resource-base-dir', '-p',
        required=False, default='/tmp/vins/sandbox'
    )
    parser.add_argument(
        '--transport', '-t',
        required=False, choices=['skynet', 'sandbox_proxy'], default='skynet',
        help='set sandbox transport',
    )
    parser.add_argument(
        '--force', '-f', action='store_true', default=False,
        help='force downloading all resources'
    )
    args = parser.parse_args()

    resource_base_dir = args.resource_base_dir
    if args.force:
        # Empty folder if force option
        if os.path.isdir(resource_base_dir):
            os.rmdir(resource_base_dir)
    if not os.path.isdir(resource_base_dir):
        os.makedirs(resource_base_dir)

    for component_name in args.components:
        component_config = load_component(component_name, args.environment_name)
        for resource in component_config.get('sandboxResources', []):
            _download_resource(resource_base_dir, resource, args.transport)


if __name__ == '__main__':
    cli()
