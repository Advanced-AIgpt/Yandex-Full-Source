# -*- coding: utf-8 -*-

from __future__ import absolute_import, print_function, division, unicode_literals

import argparse
import time
from threading import RLock
from pprint import pprint

from jupytercloud.tools.lib import cloud, environment, parallel, utils, jupyterhub

from sandbox.common.rest import Client as SandboxClient
from sandbox.common.proxy import OAuth

from infra.qyp.vmctl.src import api as qyp_api
from infra.qyp.vmctl.src.defines import VMPROXY_LOCATION


logger = None
i = 0

LOCATION = 'myt'
SEGMENT = 'default'
HOST_TYPE = 'cpu1_ram4_hdd24'

BAD_REQUEST_USERS = {
    'gotmanov',
    'malets',
    'dmgor',
    'aranovskii',
    'edvls',
    'boyalex',
    'mosquito',
    'deshevoy',
    'gfranco',
    'a-v-y',
    'tserakhau',
    'kriomant',
    'syntezzz',
    'grickevich',
    'log1n',
    'robot-disk-j-stat',
    'akorobkov',
    'rubik303',
    'demx',
    'dimonb',
    'frisbeeman',
    'robot-mrkt-metrch2yt',
    'amaslak',
    'alvor88',
    'boriay',
    'vladbelov',
    'ivan-karev',
    'goldstein',
}


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--environment', default='production')
    parser.add_argument(
        '--verbose', '-v',
        action='count',
        help='print to stderr some additional info',
        default=0,
    )
    parser.add_argument('--threads', '-j', type=int, default=None)
    parser.add_argument('--user')

    parser.add_argument('number', type=int)
    parser.add_argument('--size', default=1, type=int)
    parser.add_argument('--no-spawn', action='store_true')

    return parser.parse_args()


def check_user_have_role(user):
    # XXX: performance!
    jupyter_cloud = cloud.JupyterCloud()
    db_users = jupyter_cloud.get_users()
    return user in db_users


def get_already_transfered_users(env):
    jh_client = jupyterhub.JupyterHubClient('new.jupyter.yandex-team.ru', env.get_secret('sandbox_api_token'))
    return set(jh_client.get_servers())


def get_users_to_transfer(env, args):
    jupyter_cloud = cloud.JupyterCloud()

    if args.user:
        users = {args.user}
    else:
        old_idm_states = set(jupyter_cloud.get_qloud_idm_states())
        users = set(jupyter_cloud.get_qloud_users())
        users -= users - old_idm_states

    users -= BAD_REQUEST_USERS

    jupyter_users = get_already_transfered_users(env)
    qyp_map_users = get_qyp_users()
    qyp_users = set(qyp_map_users)
    sandbox_success_users = get_success_users()
    running_users = get_running_users()

    good_users = set(sandbox_success_users)

    logger.error('we have %d BAD REQUEST users', len(BAD_REQUEST_USERS))
    logger.warning('we have %d jupyter users', len(jupyter_users))
    logger.warning('we have %d qyp users', len(qyp_users))
    logger.warning('we have %d users with success sandbox task', len(sandbox_success_users))
    logger.warning('we have %d running users', len(running_users))
    logger.info('we have %d running users: %s', len(running_users), running_users)

    bad_users = qyp_users - sandbox_success_users - running_users
    good_users -= bad_users
    logger.error('%d users have qyp VM but no good sandbox task: %s', len(bad_users), bad_users)

    bad_users = jupyter_users - sandbox_success_users - running_users
    good_users -= bad_users
    logger.error('%d users have working JH but no good sandbox task: %s', len(bad_users), bad_users)

    bad_users = sandbox_success_users - qyp_users
    good_users -= bad_users
    logger.error('%d users have good sandbox task but no qyp: %s', len(bad_users), bad_users)

    bad_users = sandbox_success_users - jupyter_users
    good_users -= bad_users
    logger.error('%d users have good sandbox task but no jupyter: %s', len(bad_users), bad_users)

    logger.warning('total %d good users', len(good_users))

    users_to_move = users - qyp_users - sandbox_success_users - running_users
    logger.warning('total %d users is accessible to move', len(users_to_move))

    users_to_delete = qyp_users - good_users - running_users
    logger.warning('going to delete %d users', len(users_to_delete))

    assert not good_users & users_to_move
    assert not good_users & users_to_delete
    assert not users_to_move & users_to_delete

    assert good_users | users_to_move | users_to_delete | running_users == users

    for user in users_to_delete:
        deallocate_user(user, qyp_map_users[user])

    return users_to_move


def deallocate_user(user, cluster):
    token = environment.get('qyp_oauth_token')
    client = qyp_api.VMProxyClient(token=token)
    vm_id = 'jupyter-cloud-{}'.format(user)
    logger.warning('going to deallocate %s', vm_id)
    client.deallocate(cluster, vm_id)


def execute_sandbox_task(
    task_type,
    description,
    custom_fields,
):
    logger.debug('going to run sandbox task %s with description `%s` and parameters %s',
                 task_type, description, custom_fields)

    sandbox_token = environment.get('sandbox_oauth_token')
    sandbox = SandboxClient(auth=OAuth(sandbox_token))
    task_id = sandbox.task({'type': task_type})['id']
    logger.debug('Sandbox task {} spawned'.format(task_id))

    sandbox.task[task_id] = {
        'owner': 'JUPYTER_CLOUD',
        'description': description,
        'custom_fields': custom_fields,
        'priority': {'class': 'USER', 'subclass': 'NORMAL'}
    }

    resp = sandbox.batch.tasks.start.update(task_id)[0]
    logger.debug('Running sandbox task {}: {}'.format(task_id, resp))

    return task_id


def run_spawn_task(user, host, size):
    task_type = 'JUPYTER_CLOUD_BACKUP'
    custom_fields = [
        {'name': 'use_latest_sandbox_binary', 'value': True},
        {'name': 'username', 'value': user},
        {'name': 'do_backup', 'value': True},
        {'name': 'old_host', 'value': host},
        {'name': 'spawn_new_vm', 'value': True},
        {'name': 'jh_instance', 'value': 'new.jupyter.yandex-team.ru'},
        {'name': 'location', 'value': LOCATION},
        {'name': 'segment', 'value': SEGMENT},
        {'name': 'host_type', 'value': HOST_TYPE},
        {'name': 'do_restore', 'value': True},
    ]

    task_id = execute_sandbox_task(
        task_type,
        "transfer for user {}".format(user),
        custom_fields
    )
    logger.info('Sandbox task %s for user %s spawned', task_id, user)
    return task_id


def get_qyp_users():
    token = environment.get('qyp_oauth_token')
    client = qyp_api.VMProxyClient(token=token)

    users = {}
    for cluster in VMPROXY_LOCATION:
        result = client.list_yp_vms(cluster, login='robot-jupyter-cloud')
        for vm in result:
            name = vm.meta.id
            if name.startswith('jupyter-cloud-'):
                user = name[len('jupyter-cloud-'):]
                users[user] = cluster

    return users


def spawn(users_info):
    sandbox_token = environment.get('sandbox_oauth_token')
    sandbox = SandboxClient(auth=OAuth(sandbox_token))

    tasks = {}

    for user, host, size in users_info:
        task_id = run_spawn_task(user, host, size)

        time.sleep(5)

        tasks[user] = task_id

    result = {}

    while tasks:
        for user, task_id in tasks.items():
            task_status = sandbox.task[task_id].read()['status']
            logger.debug('task %s for user %s is %s', task_id, user, task_status)

            if task_status in ('FAILURE', 'EXCEPTION', 'STOPPED', 'ERROR', 'SUCCESS'):
                result[user] = {'task': task_id, 'task_status': task_status}
                logger.warning('task %s for user %s is %s', task_id, user, task_status)

                tasks.pop(user)

                logger.warning('%d tasks left', len(tasks))

        time.sleep(30)

    return result


def get_success_users():
    sandbox_token = environment.get('sandbox_oauth_token')
    sandbox = SandboxClient(auth=OAuth(sandbox_token))

    tasks = sandbox.task.read(
        type="JUPYTER_CLOUD_BACKUP",
        limit=1000,
        jh_instance='new.jupyter.yandex-team.ru', status='SUCCESS'
    )

    return {task['input_parameters']['username'] for task in tasks['items']}


def get_running_users():
    sandbox_token = environment.get('sandbox_oauth_token')
    sandbox = SandboxClient(auth=OAuth(sandbox_token))

    tasks = sandbox.task.read(
        type="JUPYTER_CLOUD_BACKUP",
        limit=1000,
        jh_instance='new.jupyter.yandex-team.ru',
        order='-id',
    )

    return {
        task['input_parameters']['username'] for task in tasks['items']
        if task['status'] not in ('FAILURE', 'EXCEPTION', 'STOPPED', 'ERROR', 'SUCCESS', 'DRAFT')
    }


def main():
    args = parse_args()

    global logger
    logger = utils.setup_logging(__name__, args.verbose)

    lock = RLock()

    with environment.environment(args.environment) as env:
        jupyter_cloud = cloud.JupyterCloud()
        users = get_users_to_transfer(env, args)
        old_idm_states = jupyter_cloud.get_qloud_idm_states()

        def _process_one(user):
            global i

            with lock:
                if i >= args.number:
                    return

            logger.debug('checking user %s', user)

            idm_state = old_idm_states.get(user)
            if not idm_state:
                return

            size = None
            for role in idm_state:
                if role.startswith('/role/quota/vm/cpu'):
                    size = int(role[18])
                    if size != args.size:
                        return

            if size is None:
                logger.warning('failed to get size from idm for user %s', user)
                return

            env = jupyter_cloud.get_qloud_environment(user)

            instances = env.get('components', {}).get('vm', {}).get('runningInstances')
            if not instances:
                logger.warning('user %s have no running instances', user)
                return

            host = jupyter_cloud.get_qloud_user_host(user)

            with lock:
                i += 1

            return (user, host, size)

        raw_result = parallel.process_with_progress(_process_one, users, args.threads)
        result = {k: v for k, v in raw_result.iteritems() if v is not None}

        logger.warning('%d users to move', len(result))

        if not args.no_spawn:
            result = spawn(result.values())

        pprint(result)
