# -*- coding: utf-8 -*-

import argparse
import asyncio
import aiohttp
import datetime
import more_itertools
import tqdm

from jupytercloud.tools.lib import environment, utils
from jupytercloud.backend.lib.clients.qyp import QYPClient
from jupytercloud.backend.lib.clients.sandbox import SandboxClient


GB = 1024 ** 3

SERVICES_TO_DROP = {
    'abc:service:2142',
    'abc:service:33985'
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

    dismissed_users = {}

    async with aiohttp.ClientSession(
        'https://staff-api.yandex-team.ru/',
        headers={'Authorization': f'OAuth {env.staff_oauth_token}'}
    ) as session:
        base_params = {
            '_pretty': '1',
            '_fields': 'login,official.quit_at',
            'official.is_dismissed': 'true',
        }

        print('requesting staff api')
        for chunk in tqdm.tqdm(user_chunks):
            params = base_params | {'login': ','.join(chunk)}
            async with session.get('/v3/persons', params=params) as resp:
                result = await resp.json()

                for r in result['result']:
                    dismiss_date = r['official']['quit_at']
                    login = r['login']
                    dismissed_users[login] = dismiss_date

    return dismissed_users


async def get_qyp_users(qyp_client):
    raw_vm_info = await qyp_client.get_vms_raw_info()
    return {vm['login']: vm for vm in raw_vm_info}


async def get_backup_info(env, login):
    sandbox_client = SandboxClient(
        oauth_token=env.sandbox_oauth_token
    )

    today = datetime.datetime.utcnow()
    raw_backups = await sandbox_client.get_resources(
        'JUPYTER_CLOUD_BACKUP',
        attrs={
            'user': login,
        },
        limit=1,
    )

    if len(raw_backups) < 1:
        return

    backup = raw_backups[0]

    raw_created = backup['time']['created']
    created = datetime.datetime.strptime(raw_created, '%Y-%m-%dT%H:%M:%SZ')
    age = (today - created).days

    return dict(
        id=backup['id'],
        age=age,
        created=created,
        size=backup['size'] // 1024 ** 2,
    )


async def set_ttl_inf(env, resource_id):
    sandbox_client = SandboxClient(
        oauth_token=env.sandbox_oauth_token
    )

    await sandbox_client.request(
        uri=f'/resource/{resource_id}/attribute/ttl',
        method='PUT',
        data={
            'name': 'ttl',
            'value': 'inf',
        }
    )


async def drop_vm(qyp_client, vm_info):
    cluster = vm_info['cluster']
    id_ = vm_info['id']
    link = qyp_client.get_vm_host(id_, cluster)

    print(f'dropping {link}')
    await qyp_client.request(
        cluster=cluster,
        uri='/api/DeallocateVm/',
        method='POST',
        data={
            'id': id_
        }
    )


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
        users_with_vm = await get_qyp_users(qyp_client)

        dismissed_users = await get_dismissed_users(env, users_with_vm)

        dismissed_users_vms = {
            login: users_with_vm[login] for login in dismissed_users
            if (
                users_with_vm[login]['spec']['accountId'] in SERVICES_TO_DROP and
                users_with_vm[login]['cluster'] != 'man'
            )
        }

        print('got {} vms'.format(len(users_with_vm)))
        print('got {} dismissed users total'.format(len(dismissed_users)))
        print('got {} dismissed users with specified quota'.format(len(dismissed_users_vms)))

        backup_dismiss_delay = {}
        print('requesting backups info')
        for login, vm_info in tqdm.tqdm(dismissed_users_vms.items()):
            backup_info = await get_backup_info(env, login)
            backup_created = backup_info['created']

            dismiss_date = dismissed_users[login]
            dismiss_date = datetime.datetime.strptime(dismiss_date, '%Y-%m-%d')

            backup_delay = (dismiss_date - backup_created).days

            if backup_delay > 7:
                print(f'do not droup {login} due to old backup {backup_delay}')
                continue

            await set_ttl_inf(env, backup_info['id'])
            backup_dismiss_delay[login] = backup_delay

        for login, vm_info in dismissed_users_vms.items():
            if login not in backup_dismiss_delay:
                continue

            await drop_vm(qyp_client, vm_info)


def main():
    asyncio.run(async_main())
