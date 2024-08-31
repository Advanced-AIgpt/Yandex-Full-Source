#!/usr/bin/env python
# coding: utf-8
from __future__ import unicode_literals

import os
import re
import sys
import json
import yaml
import argparse
import tempfile
from subprocess import check_call


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


def load_yaml(path):
    base_path = os.path.dirname(os.path.abspath(__file__))
    yaml_path = os.path.join(base_path, path)
    if not yaml_path.endswith('.yaml'):
        yaml_path = '{0}.yaml'.format(yaml_path)
    if os.path.exists(yaml_path):
        with open(yaml_path) as f:
            return yaml.load(f)
    else:
        raise RuntimeError('Not found yaml file "%s"' % path)


def load_config(path, resource_base_path):
    current_config = load_yaml(path)
    if 'component' in current_config and 'sandboxResources' in current_config['component']:
        for resource in current_config['component']['sandboxResources']:
            if 'symlink' in resource:
                resource['symlink'] = os.path.join(resource_base_path, resource['symlink'])
    if 'include' in current_config:
        result_config = {}
        for include_path in current_config['include']:
            deep_dict_update(result_config, load_config(include_path, resource_base_path))
        deep_dict_update(result_config, current_config)
    else:
        result_config = current_config
    return result_config


def load_component(component_name, env_name, tag, resource_base_path):
    result_config = load_config(os.path.join('components', component_name, env_name), resource_base_path)
    deep_dict_update(result_config, {
        'component': {
            'image': {
                'tag': tag,
            }
        }
    })
    return result_config


def valid_component_arg(s):
    if re.match(r'^[\w\-_]+\:[\w\-_.]+$', s):
        return s
    else:
        raise argparse.ArgumentTypeError(
            'Not a valid argument: {0}'.format(s)
        )


def cli():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--env', '-e', dest='environment_name', required=True
    )
    parser.add_argument(
        '--project', '-p', dest='project_name', required=False, default='vins-int',
        choices=['vins-int', 'vins-ext'],
    )
    parser.add_argument(
        '--build-docker', '-b', dest='need_build_docker',
        required=False, default=False, action='store_true',
    )
    parser.add_argument(
        '--show-config', '-o', dest='show_config',
        required=False, default=False, action='store_true',
    )
    parser.add_argument(
        '--dry-run', dest='dry_run',
        required=False, default=False, action='store_true',
    )
    parser.add_argument(
        '--task', '-t', dest='task',
        required=False, default=None,
        help='release startrek ticket'
    )
    parser.add_argument(
        '--comment', dest='comment',
        required=False, default=None,
        help='comment for deploy'
    )
    parser.add_argument(
        'components', metavar='component', type=valid_component_arg, nargs='+',
        help='components for deploy in format <component name>:<tag>',
    )
    parser.add_argument(
        '--vins-resources-path', dest='vins_resources_path',
        required=False, default='/tmp/vins/sandbox',
        help='Path to deploy vins resources'
    )
    args = parser.parse_args()
    deploy_comments = []
    if args.task:
        deploy_comments.append('Release ticket: https://st.yandex-team.ru/{}'.format(args.task))
    elif args.environment_name == 'stable':
        raise RuntimeError('Argument "--task" is required for stable environment')

    base_path = os.path.dirname(os.path.abspath(__file__))
    already_builded_images = []

    result_config = load_yaml('base.yaml')
    deep_dict_update(result_config, {
        'qloud': {
            'id': 'voice-ext.{project}.{environment}'.format(
                project=args.project_name,
                environment=args.environment_name,
            )
        }
    })
    for component in args.components:
        component_name, _, component_tag = component.partition(':')
        deploy_comments.append('{0} -> {1}'.format(component_name, component_tag))
        component_config = load_component(
            component_name, args.environment_name, component_tag, args.vins_resources_path
        )
        deep_dict_update(result_config, {
            'qloud': {
                'components': {
                    component_name: component_config['component'],
                },
                'routes': component_config.get('routes', []),
            }
        })
        if args.need_build_docker:
            dockerfile_name = component_config['component']['image']['repo'].split('/')[-1]
            image_tag = '{repo}:{tag}'.format(**component_config['component']['image'])
            if image_tag not in already_builded_images:
                check_call([
                    'docker', 'build',
                    '-f', os.path.join(base_path, '..', 'dockerfiles', 'Dockerfile.{}'.format(dockerfile_name)),
                    '-t', image_tag, '.'
                ])
                check_call([
                    'docker', 'push',
                    image_tag,
                ])
                already_builded_images.append(image_tag)

    if args.comment:
        deploy_comments.append(args.comment.decode('utf-8'))

    result_config['qloud']['deployComment'] = u'; '.join(deploy_comments)
    if args.show_config:
        print json.dumps(result_config)
    if not args.dry_run:
        with tempfile.NamedTemporaryFile(prefix='vins-cit', suffix='.json') as conf_file:
            json.dump(result_config, conf_file)
            conf_file.flush()
            check_call(
                ['citclient', '--cfg', conf_file.name, 'qloud.deploy', '--sync'],
                stdout=sys.stdout,
                stderr=sys.stderr
            )


if __name__ == '__main__':
    cli()
