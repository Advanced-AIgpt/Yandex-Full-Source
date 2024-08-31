# -*- coding: utf-8 -*-

from __future__ import absolute_import, print_function, division, unicode_literals

import simplejson
import pprint
import threading
import argparse
from collections import defaultdict

import attr
import csv

from infra.qyp.proto_lib import vmset_pb2

from jupytercloud.tools.lib import (
    environment, parallel, utils,
    JupyterCloud
)


logger = None

GB = 1024 ** 3


@attr.s
class Resources(object):
    cpu = attr.ib(default=0)
    mem = attr.ib(default=0)
    hdd = attr.ib(default=0)
    ssd = attr.ib(default=0)

    def __add__(self, other):
        return self.__class__(
            cpu=self.cpu + other.cpu,
            mem=self.mem + other.mem,
            hdd=self.hdd + other.hdd,
            ssd=self.ssd + other.ssd,
        )

    def __sub__(self, other):
        return self.__class__(
            cpu=self.cpu - other.cpu,
            mem=self.mem - other.mem,
            hdd=self.hdd - other.hdd,
            ssd=self.ssd - other.ssd,
        )

    @property
    def cat_contain_instance(self):
        return (
            self.cpu >= 1000 and
            self.mem >= 4 * GB and
            self.ssd >= 72 * GB
        )

    def as_prerry_dict(self):
        return dict(
            cpu=self.cpu / 1000,
            mem=self.mem / GB,
            ssd=self.ssd / GB,
            hdd=self.hdd / GB
        )


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
    parser.add_argument('--all', action='store_true')

    return parser.parse_args()


def get_quota_name(vm_info):
    account_id = vm_info.spec.account_id

    if account_id == 'abc:service:2142':
        quota_name = 'jupyter'
    elif account_id == 'abc:service:4172':
        quota_name = 'personal'
    else:
        quota_name = 'other'

    return quota_name


def vm_info_to_resources(vm_info):
    spec = vm_info.spec.qemu
    res = spec.resource_requests

    disk = spec.volumes[0]
    size = disk.capacity
    type_ = disk.storage_class

    ret = Resources(
        cpu=res.vcpu_guarantee,
        mem=res.memory_guarantee,
        hdd=size if type_ == 'hdd' else 0,
        ssd=size if type_ == 'ssd' else 0,
    )

    assert ret.hdd or ret.ssd

    return ret


def check_personal_quota_availability(cluster_accounts):
    personal_accounts = []
    service_accounts = []

    for accounts in cluster_accounts:
        for account in accounts:
            if account.type == vmset_pb2.Account.PERSONAL:
                personal_accounts.append(account)
            elif account.type == vmset_pb2.Account.SERVICE:
                service_accounts.append(account)

    if not personal_accounts:
        return None, Resources()

    personal_limits = Resources()
    personal_usage = Resources()

    for account in personal_accounts:
        limit = account.personal.limits.per_segment['dev']
        usage = account.personal.usage.per_segment['dev']

        personal_limits.cpu = limit.cpu
        personal_limits.mem = limit.mem
        personal_limits.ssd = limit.disk_per_storage['ssd']
        personal_limits.hdd = limit.disk_per_storage['hdd']

        personal_usage.cpu += usage.cpu
        personal_usage.mem += usage.mem
        personal_usage.ssd += usage.disk_per_storage['ssd']
        personal_usage.hdd += usage.disk_per_storage['hdd']

    for account in service_accounts:
        members = account.members_count
        if not members:
            continue

        if 'dev' not in account.limits.per_segment:
            continue

        limit = account.limits.per_segment['dev']

        personal_usage.cpu += limit.cpu / members
        personal_usage.mem += limit.mem / members
        personal_usage.ssd += limit.disk_per_storage['ssd'] / members
        personal_usage.hdd += limit.disk_per_storage['hdd'] / members

    available = personal_limits - personal_usage

    return available.cat_contain_instance, available


def prepare_person_units():
    result = {}

    with open('staff_persons_groups.json') as f_:
        person_groups = simplejson.load(f_)

    with open('staff_groups.json') as f_:
        raw_groups = simplejson.load(f_)

        groups = {}
        for raw_group in raw_groups:
            id_ = raw_group['id']
            ancestors = [_['id'] for _ in raw_group['ancestors']]

            groups[id_] = ancestors

    with open('staff_groups_names') as f_:
        group_names = {}
        for line in f_:
            id_, raw_name = line.split(':', 1)

            id_ = int(id_)
            raw_name = raw_name.strip().replace('"', "'")
            raw_name = 'u"{}"'.format(raw_name.strip())
            name = eval(raw_name)
            name = name.encode('utf-8')

            group_names[id_] = name

    for info in person_groups:
        user = info['login']
        group_id = info['department_group']['id']

        raw_ancestors = groups[group_id][1:3] if group_id in groups else ['UNKNOWN_GROUP']
        ancestors = [group_names.get(_, str(_)) for _ in raw_ancestors]

        if len(ancestors) < 2:
            ancestors.append('UNKNOWN_GROUP')
        if len(ancestors) < 2:
            ancestors.append('UNKNOWN_GROUP')

        result[user] = ancestors

    return result


def main():
    args = parse_args()

    global logger
    logger = utils.setup_logging(__name__, args.verbose)

    person_units = prepare_person_units()

    lock = threading.Lock()
    stats = defaultdict(lambda: defaultdict(int))

    with open('personal_quotas.csv', 'wb') as csvfile:
        fieldnames = [
            'user', 'staff1', 'staff2',
            'cluster', 'quota_type', 'have_personal', 'available_personal',
            'u_cpu', 'u_mem', 'u_ssd', 'u_hdd',
            'pqa_cpu', 'pqa_mem', 'pqa_ssd', 'pqa_hdd'
        ]
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        writer.writeheader()

        def write_row(user, vm_info, quota_type, resources, personal, personal_available):
            have_personal = personal is not None
            available_personal = personal

            with lock:
                quota_stats = stats[quota_type]
                quota_stats['users'] += 1
                if 'used_resources' not in quota_stats:
                    quota_stats['used_resources'] = Resources()
                quota_stats['used_resources'] += resources
                quota_stats['have_personal_quota'] += (
                    1 if personal is not None else 0
                )
                quota_stats['available_personal_quota'] += (
                    1 if personal else 0
                )
                staff = person_units.get(user, ['UNKNOWN_PERSON', 'UNKNOWN_PERSON'])

                writer.writerow(dict(
                    user=user,
                    staff1=staff[0],
                    staff2=staff[1],
                    cluster=vm_info['cluster'],
                    quota_type=quota_type,
                    have_personal=have_personal,
                    available_personal=available_personal,
                    u_cpu=resources.cpu / 1000,
                    u_mem=resources.mem / GB,
                    u_ssd=resources.ssd / GB,
                    u_hdd=resources.hdd / GB,
                    pqa_cpu=personal_available.cpu / 1000,
                    pqa_mem=personal_available.mem / GB,
                    pqa_ssd=personal_available.ssd / GB,
                    pqa_hdd=personal_available.hdd / GB,
                ))

        with environment.environment(args.environment):
            jupyter_cloud = JupyterCloud()
            qyp_vms = jupyter_cloud.get_qyp_vms()

            logger.info('get %d qyp vms', len(qyp_vms))

            def _process_one(user):
                vm = qyp_vms[user]

                try:
                    vm_info = jupyter_cloud.get_vm_info(vm['cluster'], vm['id'])
                except:
                    logger.warning('failed to fetch vm info for %s', user)
                    return

                resources = vm_info_to_resources(vm_info)
                quota_name = get_quota_name(vm_info)

                try:
                    accounts = jupyter_cloud.get_accounts(user)
                except:
                    logger.warning('failed to fetch accounts info for %s', user)
                    return

                personal, available_resources = check_personal_quota_availability(accounts)

                write_row(
                    user=user,
                    vm_info=vm,
                    quota_type=quota_name,
                    resources=resources,
                    personal=personal,
                    personal_available=available_resources
                )

            users = list(qyp_vms)

            parallel.process_with_progress(_process_one, users, args.threads)

    stats = dict(stats)
    for key, value in stats.items():
        value['used_resources'] = value['used_resources'].as_prerry_dict()
        stats[key] = dict(value)

    print('statistics:')
    pprint.pprint(stats)
