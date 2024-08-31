# -*- coding: utf-8 -*-

import argparse
import asyncio
import aiohttp
import more_itertools
import tqdm

from jupytercloud.tools.lib import environment, utils
from jupytercloud.backend.lib.clients.qyp import QYPClient


GB = 1024 ** 3

SERVICES_TO_LOOK = {
    'abc:service:2142',
}

logger = None


def parse_args():
    parser = argparse.ArgumentParser()
    environment.add_cli_arguments(parser)

    parser.add_argument(
        '--verbose', '-v',
        action='count',
        help='print to stderr some additional info',
        default=0,
    )
    parser.add_argument('--threads', '-j', type=int, default=None)

    return parser.parse_args()


async def get_dismissed_users(env, users):
    user_chunks = more_itertools.chunked(users, 50)

    dismissed_users = []

    async with aiohttp.ClientSession(
        'https://staff-api.yandex-team.ru/',
        headers={'Authorization': f'OAuth {env.staff_oauth_token}'}
    ) as session:
        base_params = {
            '_pretty': '1',
            '_fields': 'login',
            'official.is_dismissed': 'true',
        }

        print('requesting staff api')
        for chunk in tqdm.tqdm(user_chunks):
            params = base_params | {'login': ','.join(chunk)}
            async with session.get('/v3/persons', params=params) as resp:
                result = await resp.json()
                dismissed_users.extend(r['login'] for r in result['result'])

    return dismissed_users


def print_usage(vms):
    cpu = 0
    mem = 0
    ssd = 0
    hdd = 0

    for vm in vms:
        qemu = vm['spec']['qemu']
        resourceRequests = qemu['resourceRequests']
        cpu += int(resourceRequests['vcpuGuarantee'])
        mem += int(resourceRequests['memoryGuarantee'])

        disk = qemu['volumes'][0]
        disk_size = int(disk['capacity'])
        if disk['storageClass'] == 'hdd':
            hdd += disk_size
        else:
            ssd += disk_size

    print('Usage:')
    print(f'CPU: {cpu / 1000} cores')
    print(f'MEM: {mem / GB} GB')
    print(f'HDD: {hdd / GB} GB')
    print(f'SSD: {ssd / GB} GB')


async def async_main():
    args = parse_args()

    global logger
    logger = utils.setup_logging(__name__, args.verbose)

    with environment.from_cli_arguments(args) as env:
        qyp_client = QYPClient(
            oauth_token=env.qyp_oauth_token,
            use_user_token=False,
            vm_name_prefix=env.vm_prefix,
            vm_short_name_prefix=env.vm_short_prefix
        )
        raw_vm_info = await qyp_client.get_vms_raw_info()
        users_with_vm = {vm['login']: vm for vm in raw_vm_info}

        dismissed_users = await get_dismissed_users(env, users_with_vm)
        dismissed_users = set(dismissed_users)

        print('got {} vms'.format(len(users_with_vm)))
        print('got {} dismissed users'.format(len(dismissed_users)))

        dismissed_vms = (users_with_vm[login] for login in dismissed_users)

        jupyter_dismissed_vms = [
            vm for vm in dismissed_vms
            if (
                vm['spec']['accountId'] in SERVICES_TO_LOOK and
                vm['cluster'] != 'man'
            )
        ]

        print('got {} dismissed users with JC quota'.format(len(jupyter_dismissed_vms)))

        print_usage(jupyter_dismissed_vms)


def main():
    asyncio.run(async_main())
