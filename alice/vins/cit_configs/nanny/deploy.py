#!/usr/bin/env python
# coding: utf-8
from __future__ import unicode_literals

import argparse
import json
import os
import re
import yaml
import requests
from urlparse import urljoin
from subprocess import check_call

NANNY_API_URL = 'http://nanny.yandex-team.ru/'
NANNY_VAULT_API_URL = 'https://nanny-vault.yandex-team.ru/v1/'


def check_response(response, allowed_status_codes=None):
    if not response.ok or allowed_status_codes and response.status_code not in allowed_status_codes:
        raise RuntimeError('[{0}] for [{1}] failed, status code [{2}], response text [{3}]'
                           .format(response.request.method, response.url, response.status_code, response.text))


# https://wiki.yandex-team.ru/runtime-cloud/nanny/service-repo-api
class Nanny(object):
    def __init__(self, url, token):
        self._url = url
        self._session = requests.Session()
        self._session.headers['Authorization'] = 'OAuth {}'.format(token)
        self._session.headers['Content-Type'] = 'application/json'

    def find_service(self, service):
        url = urljoin(self._url, 'v2/services/%s' % service)
        response = self._session.get(url)
        if response.status_code == 404:
            return None
        check_response(response)
        return response.json()

    def create_service(self, service_desc):
        url = urljoin(self. _url, '/v2/services/')
        response = self._session.post(url, json=service_desc)
        check_response(response, [201])
        return response.json()

    def update_attrs(self, service, snapshot_id, attrs, attrs_type, comment):
        request = {
            'snapshot_id': snapshot_id,
            'content': attrs,
            'comment': comment}
        url = urljoin(self._url, 'v2/services/{service}/{attrs_type}/'.format(service=service, attrs_type=attrs_type))
        response = self._session.put(url, json=request)
        if response.status_code == 409:
            raise RuntimeError('service {service} {attrs_type} updated concurrently, you should try to restart deploy'
                               .format(service=service, attrs_type=attrs_type))
        check_response(response)
        return response.json()

    def set_target_state(self, service, snapshot_id, comment):
        request = {
            'type': 'SET_TARGET_STATE',
            'content': {
                'is_enabled': True,
                'snapshot_id': snapshot_id,
                'comment': comment,
                'prepare_recipe': 'default',
                'recipe': 'default'
            }
        }
        url = urljoin(self._url, 'v2/services/%s/events/' % service)
        response = self._session.post(url, json=request)
        check_response(response)


# https://wiki.yandex-team.ru/runtime-cloud/nanny/vault-keychains
# https://wiki.yandex-team.ru/runtime-cloud/nanny/secrets-api
# https://bb.yandex-team.ru/projects/JUNK/repos/alonger/browse/batch_secret_create.py
class NannyVault(object):
    def __init__(self, url, token):
        self._url = url
        self._oauth_token = token
        self._session = requests.Session()
        self._session.headers['Content-Type'] = 'application/json'
        self._authenticate()

    def get_last_revision(self, keychain_id, secret_id):
        response = self._session.get(self._url + 'secret/{0}/{1}?list=true'.format(keychain_id, secret_id))
        check_response(response)
        return response.json()['data']['keys'][0]

    def ensure_service_has_access_to_keychain(self, service, keychain):
        url = NANNY_VAULT_API_URL + '/auth/blackbox/keychain/{0}'.format(keychain)
        response = self._session.get(url)
        check_response(response)
        keychain_info = response.json()['data']
        for s in keychain_info['services']:
            if s['id'] == service:
                return
        keychain_info['services'].append({'id': service})
        check_response(self._session.put(url, json=keychain_info))
        self._authenticate()

    def _authenticate(self):
        response = self._session.post(self._url + 'auth/blackbox/login/keychain', json={'oauth': self._oauth_token})
        check_response(response)
        self._session.headers['X-Vault-Token'] = response.json()['auth']['client_token']


class NannyServiceBuilder(object):
    def __init__(self, nanny_vault, service_config, component_config, image_tag, resource_base_dir):
        self._nanny_vault = nanny_vault
        self._service_config = service_config
        self._component_config = component_config
        self._image_tag = image_tag
        self._resource_base_dir = resource_base_dir

    def build(self):
        return {
            'auth_attrs': self._prepare_auth_attrs(),
            'info_attrs': self._prepare_info_attrs(),
            'runtime_attrs': self._prepare_runtime_attrs()
        }

    def _prepare_auth_attrs(self):
        owners = self._component_config['nanny']['owners']
        return {
            "owners": {
                "logins": owners.get('logins', []),
                "groups": [str(i) for i in owners.get('groups', [])]
            }
        }

    def _prepare_info_attrs(self):
        return {
            'desc': '-',
            'category': self._service_config['category'],
            'abc_group': self._component_config['nanny'].get('abc_group', ''),
            "recipes": {
                "content": [
                    {
                        "desc": "Activate",
                        "labels": [],
                        "id": "default",
                        "context": [],
                        "name": "_activate_only_service_configuration.yaml"
                    }
                ],
                "prepare_recipes": [
                    {
                        "desc": "Prepare",
                        "labels": [],
                        "id": "default",
                        "context": [],
                        "name": "_prepare_service_configuration.yaml"
                    }
                ]
            },
        }

    def _prepare_runtime_attrs(self):
        def get(d, key):
            result = d[key]
            if result is None:
                raise RuntimeError('{0} must not be empty'.format(key))
            return result

        _, _, image_name = self._image_tag.partition('/')
        res = {
            'instance_spec': {
                'dockerImage': {
                    'registry': 'registry.yandex.net',
                    'name': image_name
                },
                'type': 'DOCKER_LAYERS',
                'containers': [
                    {
                        'name': 'vins',
                        'env': self._prepare_environment_variables()
                    }
                ]
            },
            'resources': {
                'sandbox_files': [self._prepare_sandbox_resource(r)
                                  for r in self._component_config.get('sandboxResources', [])],
                'static_files': self._prepare_static()
            }
        }

        if get(self._service_config, 'gencfg'):
            res['instances'] = {
                'chosen_type': 'EXTENDED_GENCFG_GROUPS',
                'extended_gencfg_groups': {
                    'network_settings': {
                        'hbf_nat': 'enabled',
                        'use_mtn': True
                    },
                    'groups': [self._prepare_gencfg_group(g)
                               for g in get(self._service_config, 'gencfg')]
                }
            }
        return res

    def _prepare_environment_variables(self):
        def append_simple_variables(config, target):
            for name, value in config.get('environmentVariables', {}).iteritems():
                target[name] = {
                    'valueFrom': {
                        'literalEnv': {
                            'value': value
                        },
                        'type': 'LITERAL_ENV'
                    },
                    'name': name
                }

        result = {}
        append_simple_variables(self._component_config, result)
        append_simple_variables(self._component_config['nanny'], result)
        for secret in self._component_config.get('secrets', []):
            try:
                keychain_id = self._component_config['nanny']['keychainId']
                secret_id = secret['objectId'].split('.')[1]
                last_revision_id = self._nanny_vault.get_last_revision(keychain_id, secret_id)
                result[secret['target']] = {
                    'valueFrom': {
                        'secretEnv': {
                            "field": "",
                            'keychainSecret': {
                                'keychainId': keychain_id,
                                'secretId': secret_id,
                                'secretRevisionId': last_revision_id
                            },
                        },
                        'type': 'SECRET_ENV'
                    },
                    'name': secret['target']
                }
            except Exception as e:
                raise RuntimeError('invalid secret config, message [{0}]\n{1}'
                                   .format(repr(e), json.dumps(secret, indent=4)))

        return list(result.itervalues())

    @staticmethod
    def _prepare_gencfg_group(config):
        try:
            return {
                'release': 'tags/' + config['tag'],
                'name': config['name']
            }
        except Exception as e:
            raise RuntimeError('invalid gencfg group config, message [{0}]\n{1}'
                               .format(repr(e), json.dumps(config, indent=4)))

    @staticmethod
    def _prepare_sandbox_resource(config):
        try:
            return {
                'resource_id': config['id'],
                'task_id': config['task_id'],
                'task_type': config['task_type'],
                'resource_type': config['resource_type'],
                'is_dynamic': config.get('dynamic', 'false').lower() == 'true',
                'local_path': config['localName']
            }
        except Exception as e:
            raise RuntimeError('invalid sandbox resource, message [{0}]\n{1}'
                               .format(repr(e), json.dumps(config, indent=4)))

    def _prepare_static(self):
        result = []
        static_root = path_from_root('cit_configs/nanny/static')
        for n in os.listdir(static_root):
            with open(os.path.join(static_root, n), 'r') as f:
                result.append({
                    'is_dynamic': False,
                    'content': f.read(),
                    'local_path': n
                })

        links = []
        for r in self._component_config.get('sandboxResources', []):
            if 'symlink' in r:
                links.append('{link}:{target}:{extract}'.format(
                    link=os.path.join(self._resource_base_dir, r['symlink']),
                    target=r['localName'],
                    extract=r['extract']
                ))
        result.append({
            'is_dynamic': False,
            'content': '\n'.join(links) + '\n',
            'local_path': 'links'
        })
        return result


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


def merge_dict(obj, ext):
    if not isinstance(obj, (list, dict)):
        raise RuntimeError('unexpected obj type [{0}]'.format(type(obj)))
    if type(ext) is not type(obj):
        raise RuntimeError('type mismatch\nobj\n{0}\next\n{1}'
                           .format(json.dumps(obj, indent=4),
                                   json.dumps(ext, indent=4)))
    result = False
    if isinstance(obj, list):
        if len(obj) != len(ext):
            obj[:] = ext
            return True
        for i, e in enumerate(ext):
            o = obj[i]
            if isinstance(e, (list, dict)):
                child_result = merge_dict(o, e)
                result = result or child_result
            elif e != o:
                obj[i] = e
                result = True
    else:
        for key in ext:
            e = ext[key]
            if key in obj:
                o = obj[key]
                if isinstance(e, (list, dict)):
                    child_result = merge_dict(o, e)
                    result = result or child_result
                elif e != o:
                    obj[key] = e
                    result = True
            else:
                obj[key] = e
                result = True
    return result


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
    if 'resources' in current_config:
        for resources_path in current_config['resources']:
            deep_dict_update(result_config, load_config(resources_path))
    deep_dict_update(result_config, current_config)
    return result_config


def load_component(component_name, env_name):
    result_config = load_config('components/{0}/{1}.yaml'.format(component_name, env_name))
    deep_dict_update(result_config, load_config('nanny/base.yaml'))
    return result_config['component']


def valid_component_arg(s):
    if re.match(r'^[\w\-_]+:[\w\-_.]+$', s):
        return s
    else:
        raise argparse.ArgumentTypeError(
            'Not a valid argument: {0}'.format(s)
        )


def load_token():
    token_file_path = os.path.join(os.path.expanduser('~'), '.nanny-token')
    if not os.path.exists(token_file_path):
        raise ValueError('nanny token not found at [%s]' % token_file_path)
    with open(token_file_path, 'r') as token_file:
        return token_file.readline().rstrip()


def merge_service(current_info, service_desc):
    for t in list(service_desc):
        current = current_info[t]['content']
        if merge_dict(current, service_desc[t]):
            service_desc[t] = current
        else:
            del service_desc[t]


def cli():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--env', '-e', dest='environment_name', required=True
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
        '--activate',
        default=False, action='store_true',
        help='Starts deploy on all services simultaneously'
    )
    parser.add_argument(
        '--task', '-t', dest='task',
        required=False, default=None,
    )
    parser.add_argument(
        '--nanny-token', dest='nanny_token',
        required=False, default=None,
    )
    parser.add_argument('--nanny-services', nargs='+')
    parser.add_argument(
        'components', metavar='component', type=valid_component_arg, nargs='+',
        help='components for deploy in format <component name>:<tag>',
    )
    parser.add_argument(
        '--resource-base-dir', '-p',
        required=False, default='/tmp/vins/sandbox'
    )
    args = parser.parse_args()
    if args.environment_name == 'stable' and not args.task:
        raise RuntimeError('Argument "--task" is required for stable environment')

    nanny_token = args.nanny_token or load_token()
    nanny = Nanny(NANNY_API_URL, nanny_token)
    nanny_vault = NannyVault(NANNY_VAULT_API_URL, nanny_token)
    already_built_images = []
    for component in args.components:
        component_name, _, component_tag = component.partition(':')
        component_config = load_component(component_name, args.environment_name)
        image_tag = '{repo}:{tag}'.format(repo=component_config['image']['repo'], tag=component_tag)
        if args.need_build_docker:
            dockerfile_name = component_config['image']['repo'].split('/')[-1]
            if image_tag not in already_built_images:
                check_call([
                    'docker', 'build',
                    '-f', path_from_root('dockerfiles', 'Dockerfile.{}'.format(dockerfile_name)),
                    '-t', image_tag,
                    path_from_root()
                ])
                check_call([
                    'docker', 'push',
                    image_tag,
                ])
                already_built_images.append(image_tag)
        try:
            for service_config in component_config['nanny']['services']:
                service_name = service_config['name']
                if args.nanny_services and service_name not in args.nanny_services:
                    continue
                service_desc = NannyServiceBuilder(nanny_vault,
                                                   service_config,
                                                   component_config,
                                                   image_tag,
                                                   args.resource_base_dir).build()
                service = nanny.find_service(service_name)
                if service:
                    merge_service(service, service_desc)
                print('component [{0}], service [{1}], tag [{2}]'
                      .format(component_name, service_name, component_tag))
                if args.show_config:
                    print(json.dumps(service_desc, indent=4))
                if args.dry_run:
                    print('\tupdate skipped')
                    continue

                deploy_comment = component_tag
                if args.task:
                    deploy_comment += ', https://st.yandex-team.ru/{}'.format(args.task)
                nanny_vault.ensure_service_has_access_to_keychain(service_name,
                                                                  component_config['nanny']['keychainId'])
                snapshot_id = None
                if service:
                    for t in service_desc:
                        update_result = nanny.update_attrs(service_name, service[t]['_id'],
                                                           service_desc[t], t, deploy_comment)
                        print('\t{0} updated'.format(t))
                        if t == 'runtime_attrs':
                            snapshot_id = update_result['_id']
                else:
                    service_desc['id'] = service_name
                    snapshot_id = nanny.create_service(service_desc)["runtime_attrs"]['_id']
                    print('\tservice created')

                if snapshot_id:
                    if args.activate:
                        nanny.set_target_state(service_name, snapshot_id, deploy_comment)
                        print('\ttarget state is set to [{0}]'.format(snapshot_id))
                    else:
                        print('\tnew snapshot created [{0}]'.format(snapshot_id))

        except Exception as e:
            raise RuntimeError('error updating component [{0}]\n\n{1}\n\ncomponent config\n{2}'
                               .format(component_name, str(e), json.dumps(component_config, indent=4)))

        if args.activate is False and component_name == 'speechkit-api-pa' and args.environment_name == 'stable':
            print '\n\nDeploy manually by UI: https://nanny.yandex-team.ru/ui/#/services/dashboards/catalog/alice_vins/'


if __name__ == '__main__':
    cli()
