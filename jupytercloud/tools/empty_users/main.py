# -*- coding: utf-8 -*-

import asyncio
import argparse
import datetime
from pathlib import Path

from jupytercloud.tools.lib import environment, parallel, utils
from jupytercloud.backend.lib.clients.qyp import QYPClient
from jupytercloud.backend.lib.qyp.vm import QYPVirtualMachine
from jupytercloud.backend.lib.clients import ssh

logger = None

BASE_PATHS = {
    Path(path) for path in (
        '.',
        '.profile',
        '.ipython',
        '.bash_logout',
        '.local',
        '.ipynb_checkpoints',
        '.bashrc',
        '.jupyter',
    )
}

CLUSTERS = {
    'sas': 'https://vmproxy.sas-swat.yandex-team.ru/',
    'vla': 'https://vmproxy.vla-swat.yandex-team.ru/',
    'iva': 'https://vmproxy.iva-swat.yandex-team.ru/',
    'myt': 'https://vmproxy.myt-swat.yandex-team.ru/',
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
    parser.add_argument('--fix', action='store_true')
    parser.add_argument(
        '--min-age',
        type=datetime.timedelta,
        default=datetime.timedelta(days=30)
    )

    return parser.parse_args()


async def filter_by_status(client, qyp_vms):
    vms = [
        QYPVirtualMachine.from_raw_info(client, vm)
        for vm in qyp_vms
    ]
    for vm in vms:
        vm._client = client

    futures = [vm.get_status() for vm in vms]

    statuses = await asyncio.gather(*futures, return_exceptions=True)

    result = []

    for vm, status in zip(qyp_vms, statuses):
        if isinstance(status, Exception) or not status.is_running:
            logger.warning(' %s\'s vm have status %s, skipping', vm['login'], status)
        else:
            result.append(vm)

    return result


def filter_by_creation_time(qyp_vms, min_age):
    td = datetime.datetime.today()

    def filter_vm(vm):
        dt_str = vm['meta']['creationTime']
        dt = datetime.datetime.strptime(dt_str, '%Y-%m-%dT%H:%M:%S.%fZ')

        return td - dt > min_age

    return [vm for vm in qyp_vms if filter_vm(vm)]


def check_empty(env, user, host):
    home_dir = Path('/home/{}'.format(user))

    with ssh.SSHClient(
        id_rsa=env.id_rsa,
        host=host,
        connect_timeout=15
    ) as client:
        result = client.execute(
            'find {}/ -maxdepth 1 -xdev'.format(home_dir), timeout=30,
        )

    stdout = result.stdout
    raw_files = stdout.split(b'\n')

    paths = set()

    for raw_file in raw_files:
        try:
            raw_file = raw_file.decode('utf-8')
        except UnicodeDecodeError:
            return False

        path = Path(raw_file).relative_to(home_dir)
        if path.name.startswith('.'):
            continue
        paths.add(path)

    difference = paths - BASE_PATHS

    return not difference


async def async_main():
    args = parse_args()

    global logger
    logger = utils.setup_logging(__name__, args.verbose)

    with environment.environment(args.environment) as env:
        qyp_client = QYPClient(
            oauth_token=env.qyp_oauth_token,
            use_user_token=False,
            vm_name_prefix=env.vm_prefix,
            vm_short_name_prefix=env.vm_short_prefix,
            clusters=CLUSTERS,
            use_pycurl=False,
        )
        raw_vm_info = await qyp_client.get_vms_raw_info()
        logger.info('%d vms total', len(raw_vm_info))

        raw_vm_info = [vm for vm in raw_vm_info if vm['spec']['accountId'] == 'abc:service:2142']
        logger.info('%d vms after filtering by service', len(raw_vm_info))

        raw_vm_info = filter_by_creation_time(raw_vm_info, args.min_age)
        logger.info('%d vms after filtering by creation time', len(raw_vm_info))

        raw_vm_info = await filter_by_status(qyp_client, raw_vm_info)
        logger.info('%d vms after filtering by status', len(raw_vm_info))

        users_with_vm = {
            vm['login']: QYPClient.get_vm_host(vm['id'], vm['cluster'])
            for vm in raw_vm_info
        }

        def _process_one(user):
            host = users_with_vm[user]

            try:
                if not check_empty(env, user, host):
                    logger.debug('user %s have non-empty home dir', user)
                    return False
            except Exception:
                logger.exception('error while processing host %s', host)
                return False

            logger.info('user %s have empty home dir', user)
            print(user)
            return True

        parallel.process_with_progress(_process_one, users_with_vm, args.threads)


def main():
    asyncio.run(async_main())
