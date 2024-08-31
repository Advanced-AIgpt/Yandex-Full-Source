# -*- coding: utf-8 -*-

from __future__ import absolute_import, print_function, division

import argparse
import pathlib2
import json

from jupytercloud.tools.lib import parallel, environment, utils, JupyterCloud
from jupytercloud.backend.lib.clients import ssh

logger = None

FIND_TIMEOUT = 600


def parse_find_output(output):
    output = output.decode('utf-8')
    lines = output.split('\n')

    result = []
    for line in lines:
        line = line.strip()
        if not line:
            continue

        result.append(pathlib2.PurePath(line))

    return result


def get_notebooks_list(user, client):
    result = client.execute(
        "sudo find /home/{user} -path '/home/{user}/arc' -prune -o -name '*.ipynb'"
        .format(user=user),
        timeout=FIND_TIMEOUT
    )
    result.raise_for_status()

    if not result.stdout:
        logger.warning("can't find notebooks on host %s for user %s", client.host, user)
        return []

    return parse_find_output(result.stdout)


def get_arcadia_paths(user, client):
    result = client.execute(
        "sudo find /home/{user} -path '/home/{user}/arc' -prune -o -name '.arcadia.root'"
        .format(user=user),
        timeout=FIND_TIMEOUT
    )
    result.raise_for_status()

    paths = parse_find_output(result.stdout)

    return [path.parent for path in paths]


def filter_notebooks_list(notebooks, arcadia_paths):
    result = []

    for path in notebooks:
        if (
            '.ipynb_checkpoints' in path.parts or
            'site-packages' in path.parts or  # виртуальные окружения
            '/.local/share/Trash/' in str(path) or
            'arc' in path.parts or
            'ArcadiaDownloads' in path.parts
        ):
            continue

        if any(arcadia_path in path.parents for arcadia_path in arcadia_paths):
            continue

        if path.suffix == '.ipynb':
            result.append(path)

    return result


def fetch_files(client, files, dest_dir):
    with client.open_sftp() as sftp:
        for filename in files:
            dst = dest_dir.joinpath(*filename.parts[1:])
            dst.parent.mkdir(parents=True, exist_ok=True)

            logger.debug(
                'fetching file %s to %s from %s',
                str(filename), str(dst), str(client.host)
            )

            sftp.get(
                str(filename),
                str(dst)
            )

            notebook_stats = sftp.lstat(str(filename))

            meta_dst = dst.with_suffix('.meta.json')
            with open(str(meta_dst), 'w') as meta_file:
                json.dump({
                    'access_time': notebook_stats.st_atime,
                    'modification_time': notebook_stats.st_mtime,
                }, meta_file)


def fetch_user_notebooks(user, host, dest_dir):
    logger.info('getting notebooks list for user %s from %s', user, host)

    with ssh.SSHClient(host=host) as client:
        raw_notebooks = get_notebooks_list(user, client)
        arcadia_paths = get_arcadia_paths(user, client)

        notebooks = filter_notebooks_list(raw_notebooks, arcadia_paths)

        logger.info('found %d notebooks for user %s', len(notebooks), user)

        fetch_files(client, notebooks, dest_dir)


def parse_args():
    parser = argparse.ArgumentParser()

    parser.add_argument(
        '--dst',
        type=pathlib2.Path,
        required=True,
    )
    parser.add_argument(
        '--continue',
        dest='continue_',
        action='store_true'
    )
    parser.add_argument(
        '--verbose', '-v',
        action='count',
        help='print to stderr some additional info',
        default=0,
    )

    parser.add_argument('--environment', default='production')
    parser.add_argument('--threads', '-j', type=int, default=None)

    return parser.parse_args()


def main():
    args = parse_args()

    global logger
    logger = utils.setup_logging(__name__, args.verbose)

    with environment.environment(args.environment):
        jupyter_cloud = JupyterCloud()
        qyp_vms = jupyter_cloud.get_qyp_vms()

        def _process_one(user):
            vm = qyp_vms[user]
            host = vm['host']
            dest_dir = args.dst / user

            if dest_dir.exists() and args.continue_:
                return

            logger.info('processing %s', host)

            status_name = jupyter_cloud.get_vm_status(vm['cluster'], vm['id'])

            if status_name != 'RUNNING':
                logger.warning('minion %s have status %s, skipping', host, status_name)
                return

            try:
                fetch_user_notebooks(user, host, dest_dir)
            except Exception:
                logger.error('failed to fetch notebooks for %s', user, exc_info=True)

        parallel.process_with_progress(_process_one, qyp_vms, args.threads)
