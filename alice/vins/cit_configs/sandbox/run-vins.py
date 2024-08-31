#!/usr/bin/env python
# coding: utf-8

import os
import yaml
import json
import urllib
import shutil
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


def run_redis():
    home_dir = os.environ['HOME']
    shutil.copy(pj(home_dir, 'code/cit_configs/sandbox/redis.conf'), home_dir)
    conf_path = pj(home_dir, 'redis.conf')
    subprocess.Popen(['redis-server', conf_path])


def vmtouch_resources(resources_path):
    vmtouch_bin = pj(resources_path, 'vmtouch')
    res_list = []
    for res in os.listdir(resources_path):
        res_list.append(pj(resources_path, res))
    subprocess.Popen([vmtouch_bin, '-l', '-v', '-f'] + res_list, stderr=subprocess.STDOUT)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        'cmd',
        choices=['run', 'get_resources'],
        help="Command to execute. get_resources - Get json with Sandbox resources. run - Start VINS"
    )
    parser.add_argument(
        '--working-dir', '-w', dest='work_dir', required=True,
        help='Working dir path. E.g.: cit_configs'
    )
    parser.add_argument(
        '--component', '-c', dest='component', required=True,
        help='Component to generate config for.',
    )
    parser.add_argument(
        '--env', '-e', dest='environment_name', required=True
    )
    args = parser.parse_args()

    cur_dir = os.getcwd()
    os.chdir(args.work_dir)

    component_name, _, component_tag = args.component.partition(':')
    component_config = load_component(component_name, args.environment_name)

    # This mode used to print json with Sandbox resources used by VINS
    if args.cmd == 'get_resources':
        resources = {}
        for r in component_config['sandboxResources']:
            if 'symlink' not in r:
                if 'localName' in r:
                    r['symlink'] = r['localName']
            resources[r['id']] = {'symlink': r['symlink'], 'localName': r['localName'], 'extract': r['extract']}
        print json.dumps(resources)
        return
    # Run mode to start VINS
    elif args.cmd == 'run':
        env_vars = {}
        append_simple_variables(component_config, env_vars)
        append_simple_variables(component_config['nanny'], env_vars)

        os.chdir(cur_dir)

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

        run_redis()

        # Manual set of VINS_WORKERS_COUNT
        os.environ['VINS_WORKERS_COUNT'] = "20"
        os.environ['VINS_LOG_FILE'] = './vins.push_client.out'
        os.environ['VINS_FEATURES_LOG_FILE'] = './vins-features.log'

        vmtouch_resources(os.environ['VINS_RESOURCES_PATH'])

        start_cmd = "{} -b [::]:8888".format(os.environ['RUN_COMMAND'])
        subprocess.call([start_cmd], shell=True, stderr=subprocess.STDOUT)


if __name__ == '__main__':
    main()
