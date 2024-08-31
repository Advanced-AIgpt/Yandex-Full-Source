# -*- coding: utf-8 -*-

import argparse

from jupytercloud.tools.lib import cloud, environment, parallel, utils

logger = None

KEEP_DEACTIVATED_COUNT = 1


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
    parser.add_argument('--user')

    return parser.parse_args()


def set_zero_dactivated(jupyter_cloud, env_name):
    uri = 'environment/policies/{}'.format(env_name)
    policies = jupyter_cloud.qloud_request('GET', uri)
    if policies['keepDeactivatedCount'] == KEEP_DEACTIVATED_COUNT:
        return

    logger.info('set deactivated=%d policy for environment %s', KEEP_DEACTIVATED_COUNT, env_name)
    policies['keepDeactivatedCount'] = KEEP_DEACTIVATED_COUNT
    jupyter_cloud.qloud_request('POST', uri, json=policies)


def drop_version(jupyter_cloud, env_name, version):
    logger.info('drop configuration %s for environment %s', version, env_name)

    uri = 'platform/remove/{}/{}'.format(env_name, version)
    jupyter_cloud.qloud_request('POST', uri)


def main():
    args = parse_args()

    global logger
    logger = utils.setup_logging(__name__, args.verbose)

    with environment.environment(args.environment):
        jupyter_cloud = cloud.JupyterCloud()

        if args.user:
            users = [args.user]
        else:
            users = jupyter_cloud.get_qloud_users()

        def _process_one(user):
            env_name = jupyter_cloud.get_qloud_environment_name(user)
            set_zero_dactivated(jupyter_cloud, env_name)

            env = jupyter_cloud.get_qloud_environment(user)
            instances = env.get('components', {}).get('vm', {}).get('runningInstances')
            if not instances:
                return

            versions = env.get('versions')
            assert versions

            for version in versions[1:]:
                if version['status'] not in ('DEACTIVATED', 'SEMI_ACTIVATED'):
                    continue

                drop_version(jupyter_cloud, env_name, version['version'])

            return

        parallel.process_with_progress(_process_one, users, args.threads)
