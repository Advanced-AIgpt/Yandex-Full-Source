# -*- coding: utf-8 -*-

import argparse
import json

from threading import RLock

from jupytercloud.tools.lib import cloud, environment, utils, parallel, db

logger = None


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

    return parser.parse_args()


def find_db_qloud_deviations(threads):
    result = {}
    result_lock = RLock()

    jupyter_cloud = cloud.JupyterCloud()

    db_user_hosts = jupyter_cloud.get_users_hosts()
    qloud_users = jupyter_cloud.get_qloud_users()
    all_users = set(db_user_hosts) | set(qloud_users)

    def _check_user(user):
        db_host = db_user_hosts.get(user)
        qloud_host = jupyter_cloud.get_qloud_user_host(user)

        if db_host is None and qloud_host is None:
            return

        if db_host != qloud_host:
            if qloud_host is None:
                logger.warning(
                    "user %s doesn't have host in qloud; host stored in qloud: %s",
                    user, db_host
                )
            else:
                logger.info(
                    'find deviation for user %s; host stored in db: %s; actual host in qloud: %s',
                    user, db_host, qloud_host
                )

            with result_lock:
                result.setdefault(user, {})
                result[user]['db_host'] = db_host
                result[user]['qloud_host'] = qloud_host

    parallel.process_with_progress(_check_user, all_users, threads)

    return result


def fix_deviations(deviations):
    with db.make_cursor() as cursor:
        for user, hosts in deviations.items():
            real_host = hosts['qloud_host']
            cursor.execute(
                "UPDATE servers SET ip='{host}', port=8888 WHERE base_url='/user/{user}/'"
                .format(host=real_host, user=user)
            )
            logger.debug("set host = '%s' in database for user %s", user, real_host)


def main():
    args = parse_args()

    global logger
    logger = utils.setup_logging(__name__, args.verbose)

    with environment.environment(args.environment):
        deviations = find_db_qloud_deviations(args.threads)

        logger.debug('full deviations list:\n%s', json.dumps(deviations, indent=2))

        if not deviations:
            logger.info('no deviations found')
            return

        logger.warning('total %d deviations found', len(deviations))

        if not args.fix:
            logger.warning('to fix a deviations, run program with --fix key')

            return

        fix_deviations(deviations)
