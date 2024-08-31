# -*- coding: utf-8 -*-

import asyncio
import argparse
import datetime
import csv

from jupytercloud.tools.lib import parallel, utils, JupyterCloud, environment
from jupytercloud.backend.lib.clients import ssh
from jupytercloud.backend.lib.qyp.vm import QYPVirtualMachine

logger = None

METRICS = (
    'users',
    'login_times',
    'git',
    'arc',
    'svn',
    'ya make',
    'python',
    'secret',
    'crontab',
    'apt',
    'pip',
    'ipykernel',
    'yt'
)

ZERO_DICT = {m: 0 for m in METRICS}


def parse_args():
    parser = argparse.ArgumentParser()

    parser.add_argument('--output', type=argparse.FileType('w'), required=True)
    parser.add_argument('--environment', default='production')
    parser.add_argument('--threads', '-j', type=int, default=None)
    parser.add_argument(
        '--verbose', '-v',
        action='count',
        help='print to stderr some additional info',
        default=0,
    )

    return parser.parse_args()


def convert_datetime(dt):
    obj = datetime.datetime.strptime(dt.decode('utf-8'), '%Y-%m-%dT%H:%M:%S+0300')
    return obj.strftime('%Y-%m-%d %H:%M:%S')


def convert_result(data):
    result = {'_total_': ZERO_DICT.copy()}

    for user, user_result in data.items():
        if not user_result:
            continue

        logs, bash_history = user_result
        logs = logs or ''
        bash_history = bash_history or ''

        r = result[user] = ZERO_DICT.copy()

        for line in logs.splitlines():
            line = line.strip()
            if not line or line.startswith(b'wtmp begins'):
                continue

            parts = line.split()
            if len(parts) < 2:
                continue

            local_user = parts[0].decode('utf-8')
            from_ = parts[1]

            if local_user == 'reboot' or from_.startswith(b'tmux'):
                continue

            if local_user != user:
                continue

            r['users'] = 1
            r['login_times'] += 1

        if not r['users']:
            continue

        for line in bash_history.splitlines():
            if not line.strip():
                continue

            try:
                line = line.decode('utf-8').lower()
            except UnicodeDecodeError:
                continue

            if 'git ' in line:
                r['git'] += 1

            if 'arc ' in line:
                r['arc'] += 1

            if 'svn ' in line:
                r['svn'] += 1

            if 'ya make' in line:
                r['ya make'] += 1

            if line.startswith('python'):
                r['python'] += 1

            if (
                'token' in line or
                'password' in line or
                'authorization' in line or
                'oauth' in line
            ):
                r['secret'] += 1

            if 'crontab' in line:
                r['crontab'] += 1

            if 'sudo apt' in line:
                r['apt'] += 1

            if 'pip install' in line:
                r['pip'] += 1

            if 'ipykernel' in line:
                r['ipykernel'] += 1

            if line.startswith('yt '):
                r['yt'] += 1

        for metric, value in r.items():
            result['_total_'][metric] += 1 if value else 0

    list_result = []

    for login, metrics in result.items():
        if not metrics['users']:
            continue
        list_result.append(metrics | {'login': login})

    return list_result


async def filter_by_status(client, qyp_vms):
    futures = [
        QYPVirtualMachine.from_raw_info(client, vm).get_status()
        for vm in qyp_vms
    ]

    statuses = await asyncio.gather(*futures, return_exceptions=True)

    result = []

    for vm, status in zip(qyp_vms, statuses):
        if isinstance(status, Exception) or not status.is_running:
            logger.warning(' %s\'s vm have status %s, skipping', vm['login'], status)
        else:
            result.append(vm)

    return result


def main():
    args = parse_args()

    global logger
    logger = utils.setup_logging(__name__, args.verbose)

    loop = asyncio.get_event_loop()

    with environment.environment(args.environment) as env:
        jupyter_cloud = JupyterCloud()

        qyp_vms = loop.run_until_complete(jupyter_cloud.qyp.get_vms_raw_info())

        logger.info('going to analyze %d vms', len(qyp_vms))

        qyp_vms = loop.run_until_complete(filter_by_status(jupyter_cloud.qyp, qyp_vms))

        logger.info('working with %d running vms', len(qyp_vms))

        qyp_vms = {vm['login']: vm for vm in qyp_vms}

        def _process_one(user):
            vm = qyp_vms[user]
            host = jupyter_cloud.qyp.get_vm_host(vm['id'], vm['cluster'])
            logger.info('processing %s', host)

            try:
                with ssh.SSHClient(id_rsa=env.id_rsa, host=host) as client:
                    result = client.execute(
                        "for name in /var/log/wtmp*; "
                        "do sudo last --time-format iso --nohostname --fullnames -f $name; "
                        "done", 120
                    )

                    bash_history = '/home/{}/.bash_history'.format(user)

                    bash_history_result = client.execute(
                        "test -e {} && cat {} || true".format(bash_history, bash_history),
                        30
                    )

                    return result.stdout, bash_history_result.stdout
            except Exception:
                logger.exception('error while greping host %s', host)
                return False

        raw_result = parallel.process_with_progress(_process_one, qyp_vms, args.threads)

    result = convert_result(raw_result)

    writer = csv.DictWriter(args.output, ('login',) + METRICS)
    writer.writeheader()
    writer.writerows(result)

    print('ok')
