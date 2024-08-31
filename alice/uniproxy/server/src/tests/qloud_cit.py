# coding: utf
import argparse
import json
import logging
import os
import requests
import sys
from copy import deepcopy
from time import sleep

logger = logging.getLogger(__name__)


def retries(max_tries, delay=1, backoff=2, exceptions=(Exception,), hook=None, log=True, raise_class=None):
    """
        Wraps function into subsequent attempts with increasing delay between attempts.
        Adopted from https://wiki.python.org/moin/PythonDecoratorLibrary#Another_Retrying_Decorator
        (copy-paste from arcadia sandbox-tasks/projects/common/decorators.py)
    """
    def dec(func):
        def f2(*args, **kwargs):
            current_delay = delay
            for n_try in xrange(0, max_tries + 1):
                try:
                    return func(*args, **kwargs)
                except exceptions as e:
                    if n_try < max_tries:
                        if log:
                            logger.error(
                                "Error in function %s on %s try:\n%s\nWill sleep for %s seconds...",
                                func.__name__, n_try, e, current_delay
                            )
                        if hook is not None:
                            hook(n_try, e, current_delay)
                        sleep(current_delay)
                        current_delay *= backoff
                    else:
                        logging.error("Max retry limit %s reached, giving up with error:\n%s", n_try, e)
                        if raise_class is None:
                            raise
                        else:
                            raise raise_class("Max retry limit {} reached, giving up with error: {}".format(n_try, str(e)))

        return f2
    return dec


class Environment(object):
    def __init__(self, client, envid):
        super(Environment, self).__init__()
        self.client = client
        self.envid = envid
        req = self.client.get('api/v1/environment/dump/{}'.format(self.envid))
        if req.status_code != 200:
            raise Exception('can not get environment: code={} {}'.format(req.status_code, req.text.encode('utf-8', 'ignore')))
        self.config = req.json()
        self.patched = False

    def print_config(self):
        print(json.dumps(self.config, indent=4))

    def upload(self):
        for i in range(1, 4):
            logger.info("try env upload {}".format(i))
            req = self.client.post('api/v1/environment/upload', self.config)
            if req.status_code == 200:
                self.patched = False
                return
        raise Exception('can not push environment: code={} {}'.format(req.status_code, req.text.encode('utf-8', 'ignore')))

    def update_resource(self, component_name, symlink, sandbox_id, local_name, comment):
        """
            find resource with given component_name and symlink, - apply sandbox_id & local_name to 'id' & 'localName' attrs
            return False if not found res
        """
        for comp in self.config['components']:
            if comp['componentName'] == component_name:
                for res in comp['sandboxResources']:
                    if res['symlink'] == symlink:
                        res['id'] = sandbox_id
                        res['localName'] = local_name
                        self.patched = True
                        # print res
                        self.config["comment"] = comment
                        return True
        return False

    def update_docker(self, component_name, hash_, repository):
        """
            find resource with given component_name and symlink, - apply sandbox_id & local_name to 'id' & 'localName' attrs
            return False if not found res
        """
        for comp in self.config['components']:
            if comp['componentName'] == component_name:
                comp['properties']['hash'] = hash_
                comp['properties']['repository'] = repository
                return

        raise Exception('can not update component docker: not found source component={}'.format(src_component_name))

    def copy_component(self, src_component_name, dst_component_name):
        for comp in self.config['components']:
            if comp['componentName'] == dst_component_name:
                raise Exception('can not copy component: dest. component={} already exist'.format(dst_component_name))

        for comp in self.config['components']:
            if comp['componentName'] == src_component_name:
                self.config['components']
                new_comp = deepcopy(comp)
                new_comp['componentName'] = dst_component_name
                self.config['components'].append(new_comp)
                return

        raise Exception('can not copy component: not found source component={}'.format(src_component_name))

    def remove_component(self, component_name):
        self.config['components'] = [comp for comp in self.config['components'] if comp['componentName'] != component_name]

    def comment(self, text=None):
        if text is not None:
            self.config["comment"] = text
        return self.config["comment"]


class Component(object):
    def __init__(self, env, name):
        super(Component, self).__init__()
        self.env = env
        self.id = env.envid + '.' + name
        self.name = name
        req = self.env.client.get('/api/v1/environment/stable/{}'.format(self.env.envid))
        if req.status_code != 200:
            raise Exception('can not get stable environment: code={} {}'.format(req.status_code, req.text.encode('utf-8', 'ignore')))
        env_config = req.json()
        env_version = env_config['version']
        req = self.env.client.get('api/v1/runtime/{}/{}'.format(self.id, env_version))
        if req.status_code != 200:
            raise Exception('can not get component: code={} {}'.format(req.status_code, req.text.encode('utf-8', 'ignore')))
        self.config = req.json()

    def print_config(self):
        print(json.dumps(self.config, indent=4))

    def instances(self):
        hosts = []
        for inst in self.config['runningInstances']:
            if inst['actualState'] == 'ACTIVE':
                hosts.append(inst['host'])
        return hosts


class Status(object):
    def __init__(self, client, object_id):
        self.resp = client.get('api/v1/status/{}'.format(object_id))

    def tree(self):
        return self.resp.json()


class Client(object):
    def __init__(self, url, token):
        super(Client, self).__init__()
        if not token:
            raise Exception('qloud client required token')
        self.url = url
        self.token = token

    def get(self, path):
        return requests.get('{}/{}'.format(self.url, path), headers=self.headers())

    def post(self, path, data):
        headers = self.headers()
        headers['Content-Type'] = 'application/json'
        return requests.post('{}/{}'.format(self.url, path), headers=headers, data=json.dumps(data))

    def headers(self):
        return {
            'Authorization': 'OAuth {}'.format(self.token),
        }


def uniproxy_tests_result(host):
    """ get tests result from instance (print to stdout)
    """
    req = None
    for i in range(1, 30):
        try:
            req = requests.get('http://{}/tests_output'.format(host))
            if req.status_code != 200:
                raise Exception('bad /tests_output handler response')
            req_result = requests.get('http://{}/tests_result'.format(host))
            if req_result.status_code != 200:
                raise Exception('bad /tests_result handler response')
            print(req.text.encode('utf-8', 'ignore'))
            return int(req_result.text)
        except Exception as exc:
            logger.debug('curl ping {} failed: {}'.format(host, exc))
            sleep(30)
    raise Exception('can not get uniproxy tests result from {}')


@retries(3)
def remove_tmp_component(client, tmp_component):
    env = Environment(client, 'voice-ext.uniproxy.utils')
    env.remove_component(tmp_component)
    env.comment('VOICESERV-1080 remove temporary component {}'.format(tmp_component))
    env.upload()


@retries(3)
def execute_qloud_tests(docker_hash, docker_version, qloud_token):
    client = Client('https://qloud-ext.yandex-team.ru', qloud_token)
    env = Environment(client, 'voice-ext.uniproxy.utils')

    tmp_component = 'uniproxy-{}'.format(docker_version).replace('.', '-')
    env.copy_component('uniproxy-tests', tmp_component)
    env.update_docker(tmp_component, docker_hash, 'registry.yandex.net/voicetech/uniproxy:{}'.format(docker_version))
    env.comment('VOICESERV-1080 generate temporary component {}'.format(tmp_component))
    logger.info('create temporary qloud component {}'.format(tmp_component))
    env.upload()
    try:
        logger.info('wait finish updating configuration & activation new component instance')
        for i in range(1, 30):
            sleep(60)
            try:
                comp = Component(env, tmp_component)
                hosts = comp.instances()
                if not hosts:
                    continue
                return uniproxy_tests_result(hosts[0])
            except Exception:
                logger.exception('{} instance not ready'.format(tmp_component))
        raise Exception('timed out activation new component instance')
    finally:
        logger.info('remove temporary qloud component {}'.format(tmp_component))
        try:
            remove_tmp_component(client, tmp_component)
        except Exception as exc:
            logger.exception('fail remove temporary qloud component {}'.format(tmp_component))


@retries(3)
def update_dev(docker_hash, docker_version, qloud_token):
    for i in range(1, 4):
        client = Client('https://qloud-ext.yandex-team.ru', qloud_token)
        env = Environment(client, 'voice-ext.uniproxy.dev')

        logger.info('update voice-ext.uniproxy.dev/be-uniproxy to {}'.format(docker_version))
        env.update_docker('be-uniproxy', docker_hash, 'registry.yandex.net/voicetech/uniproxy:{}'.format(docker_version))
        env.comment('update dev uniproxy to last git/master version {}'.format(docker_version))
        env.upload()


def list_instances(qloud_token, env_id):
    client = Client('https://qloud-ext.yandex-team.ru', qloud_token)
    status = Status(client, env_id)
    return [it['instanceFqdn'] for it in status.tree() if 'uniproxy' in it['instanceImage']]


def main():
    root_logger = logging.getLogger('')
    file_logger = logging.StreamHandler()
    file_logger.setFormatter(logging.Formatter('%(asctime)s %(levelname)-8s %(filename)s:%(lineno)d %(message)s'))
    root_logger.addHandler(file_logger)
    root_logger.setLevel(logging.DEBUG)

    parser = argparse.ArgumentParser(description='Misc qloud/uniproxy env. operations')
    parser.add_argument('--run', metavar='STR', type=str, default='make-test',
                        help="CIT action for qloud uniproxy (make-test|update-dev|ls)")
    parser.add_argument('--env', metavar='STR', type=str, default='stable',
                        help="qloud uniproxy environment (default=stable)")
    context = parser.parse_args()

    qloud_token=os.getenv('QLOUD_TOKEN')
    if not qloud_token:
        raise Exception('bad QLOUD_TOKEN env. var')

    if context.run == 'ls':
        for it in list_instances(qloud_token, 'voice-ext.uniproxy.' + context.env):
            print(it)
        return 0

    docker_hash = os.getenv('UNIPROXY_DOCKER_HASH')
    docker_version = os.getenv('UNIPROXY_DOCKER_VERSION')
    if not docker_hash or not docker_version:
        raise Exception('bad UNIPROXY_DOCKER_HASH or UNIPROXY_DOCKER_VERSION env. vars')

    if context.run == 'make-test':
        return execute_qloud_tests(docker_hash, docker_version, qloud_token)
    elif context.run == 'update-dev':
        update_dev(docker_hash, docker_version, qloud_token)
        return 0
    else:
        logger.error('unknown --run={} type'.format(context.run))
        return 42


if __name__ == "__main__":
    sys.exit(main())
