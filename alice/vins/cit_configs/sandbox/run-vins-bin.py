#!/usr/bin/env python
# coding: utf-8

import os
import yaml
import urllib
import subprocess
import argparse

from os.path import join as pj


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


def load_config(path):
    if not os.path.exists(path):
        raise RuntimeError('yaml file [%s] not found' % path)
    with open(path) as f:
        current_config = yaml.load(f)

    result_config = {}
    if 'include' in current_config:
        for include_path in current_config['include']:
            deep_dict_update(result_config, load_config(include_path))
    deep_dict_update(result_config, current_config)

    return result_config


def load_component(component_name, env_name):
    result_config = load_config('components/{}/{}.yaml'.format(component_name, env_name))
    deep_dict_update(result_config, load_config('nanny/base.yaml'))
    return result_config['component']


def append_simple_variables(config, target):
    for name, value in config.get('environmentVariables', {}).iteritems():
        target[name] = {
            'name': name,
            'value': value
        }


def run_redis(conf_path):
    print "Starting redis server..."
    conf_path = pj(conf_path, 'sandbox/redis.conf')
    subprocess.Popen(['redis-server', conf_path])


def main():
    work_dir = os.getcwd()

    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--conf-dir', '-w', dest='conf_dir', required=True,
        help='Path to dir with yaml configs.'
    )
    parser.add_argument(
        '--resources-path', '-r', dest='res_path',
        help='Path to dir with resources.',
        default=pj(work_dir, 'resources')
    )
    parser.add_argument(
        '--component', '-c', dest='component', required=True,
        help='Component to generate config for.',
    )
    parser.add_argument(
        '--env', '-e', dest='environment_name', required=True
    )
    parser.add_argument(
        '--run-redis', dest='run_redis', action='store_true',
        default=False,
        help='Run redis-server before starting VINS'
    )
    args = parser.parse_args()

    os.chdir(args.conf_dir)

    component_name, _, component_tag = args.component.partition(':')
    component_config = load_component(component_name, args.environment_name)

    env_vars = {}
    append_simple_variables(component_config, env_vars)
    append_simple_variables(component_config['nanny'], env_vars)

    os.chdir(work_dir)

    for v in list(env_vars.itervalues()):
        if v['name'] not in os.environ:
            os.environ[v['name']] = "{}".format(v['value'])

    encoded_mongo_password = urllib.quote_plus(os.environ['MONGO_PASSWORD'])
    os.environ['VINS_MONGODB_URL'] = "mongodb://{}:{}@{}/{}".format(
        os.environ['MONGO_USER'],
        encoded_mongo_password,
        os.environ['MONGO_HOST'],
        os.environ['MONGO_DB']
    )

    if (args.run_redis):
        run_redis(args.conf_dir)

    # Manual set of VINS_WORKERS_COUNT
    os.environ['VINS_WORKERS_COUNT'] = "20"
    os.environ['VINS_LOG_FILE'] = './vins.push_client.out'
    os.environ['VINS_FEATURES_LOG_FILE'] = './vins-features.log'
    os.environ['VINS_RESOURCES_PATH'] = args.res_path

    vins_bin_path = pj(work_dir, 'bin', 'pa')

    start_cmd = "{} -b [::]:8888 -w {} --timeout 18000".format(vins_bin_path, os.environ['VINS_WORKERS_COUNT'])
    subprocess.call([start_cmd], shell=True, stderr=subprocess.STDOUT)


if __name__ == '__main__':
    main()