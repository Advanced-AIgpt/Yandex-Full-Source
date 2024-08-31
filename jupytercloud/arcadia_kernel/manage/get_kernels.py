#!/usr/bin/env python3

import tempfile
import stat

from contextlib import contextmanager
from pathlib import Path

from sandbox.common.rest import Client as SandboxClient

from .utils import (
    logger,
    download_file,
    sky_download_file,
    KERNELS_TO_INSTALL,
    temporary_directory,
    measure_time,
)
from .archive import extract_files


RESOURCE_NAME = 'JUPYTER_ARCADIA_DEFAULT_KERNELS'


def add_get_kernels_args(parser):
    version = parser.add_mutually_exclusive_group(required=True)
    version.add_argument('--sandbox-id')
    version.add_argument('--svn-revision')
    version.add_argument('--last', action='store_true')

    parser.add_argument('--last-resource-file', type=Path, default=None)

    parser.add_argument(
        '--dst-dir',
        type=Path,
        default=Path.cwd()
    )


def create_sandbox_client():
    return SandboxClient(
        logger=logger,
        ua="arcadia://jupytercloud.arcadia_kernel.manage",
    )


def get_sandbox_id_from_svn_revision(client, svn_revision):
    resources_info = client.resource.read(
        type=RESOURCE_NAME,
        attrs=dict(
            resource_version=svn_revision,
        ),
        order='-id',
        limit=10,
    )
    resources = resources_info.get('items')
    if not resources:
        raise ValueError(
            f'failed to find {RESOURCE_NAME} resource with svn_revision={svn_revision}'
        )

    sandbox_ids = [resource_info['id'] for resource_info in resources]

    if len(sandbox_ids) > 1:
        logger.warning(
            'there is more than one %s resource with svn_revision=%s: %s; using fresh one',
            RESOURCE_NAME, svn_revision, sandbox_ids
        )
    else:
        logger.info(
            '%s resource with svn_revision=%s have sandbox_id=%s',
            RESOURCE_NAME, svn_revision, sandbox_ids[0]
        )

    return sandbox_ids[0]


def get_sandbox_id_from_last_kernel(client):
    resources_info = client.resource.read(
        type=RESOURCE_NAME,
        limit=1,
        order='-id',
        state='READY',
        attrs=dict(
            released='stable'
        )
    )

    resources = resources_info.get('items')
    if not resources:
        raise ValueError(f'failed to find any {RESOURCE_NAME} resource with')

    sandbox_ids = [resource_info['id'] for resource_info in resources]

    logger.info(
        'last %s resource have sandbox_id=%s',
        RESOURCE_NAME, sandbox_ids[0]
    )

    return sandbox_ids[0]


def get_sandbox_id(client, args):
    if args.sandbox_id:
        sandbox_id = args.sandbox_id
    elif args.svn_revision:
        sandbox_id = get_sandbox_id_from_svn_revision(client, args.svn_revision)
    elif args.last:
        sandbox_id = get_sandbox_id_from_last_kernel(client)

    return sandbox_id


def get_resource_info(client, sandbox_id):
    logger.info('fetching information about sandbox resource with id=%s', sandbox_id)

    resource_info = client.resource[sandbox_id].read()
    if resource_info.get('type') != RESOURCE_NAME:
        raise ValueError(f'trying to download non-{RESOURCE_NAME} resource')

    if not (resource_info.get('http', {}).get('proxy')):
        raise ValueError(f'missing proxy url in resource info: {resource_info}')

    return resource_info


def extract_kernels(archive_file, dst_dir):
    logger.debug(
        'trying to extract kernels %s from archive to %s',
        KERNELS_TO_INSTALL, dst_dir
    )

    members_dst = {}

    for kernel_name in KERNELS_TO_INSTALL:
        for filename in (kernel_name, kernel_name + '_yql.so'):
            member_name = f'{kernel_name}/{filename}'
            dst_file = dst_dir / filename

            members_dst[member_name] = dst_file

    extract_files(archive_file, members_dst)


def make_executable(dst_dir):
    for kernel_name in KERNELS_TO_INSTALL:
        dst_file = dst_dir / kernel_name
        assert dst_file.exists()
        old_perm = dst_file.stat().st_mode
        new_perm = old_perm | stat.S_IEXEC

        if old_perm == new_perm:
            continue

        logger.info(
            'making %s executable; old perm: %s, new perm: %s',
            dst_file, oct(old_perm), oct(new_perm),
        )

        dst_file.chmod(new_perm)


@contextmanager
def get_archive_file(args, last_resource_file):
    if args.last and last_resource_file:
        yield last_resource_file
        return

    client = create_sandbox_client()

    sandbox_id = get_sandbox_id(client, args)

    resource_info = get_resource_info(client, sandbox_id)

    skynet_id = resource_info['skynet_id']
    file_name = resource_info['file_name']

    try:
        with temporary_directory() as dir:
            # temp dir is 0700 by default, but the skynet daemon running by another user
            dir.chmod(0o755)

            with measure_time('downloading file via sky get'):
                sky_download_file(skynet_id, dir)

            yield dir / file_name
    except RuntimeError:
        logger.warning('sky get failed, backoff to requests', exc_info=True)
    else:
        return

    proxy_url = resource_info['http']['proxy']
    expected_size = resource_info['size']

    with tempfile.NamedTemporaryFile(
        suffix='-arcadia-kernels.tar.gz'
    ) as archive_file:
        with measure_time('downloading file via requests'):
            archive_size = download_file(archive_file, proxy_url)

        if expected_size != archive_size:
            logger.warning(
                'resource meta-information size %d does not equal downloaded file size %d',
                expected_size, archive_size
            )

        yield Path(archive_file.name)


def get_kernels(args):
    dst_dir = args.dst_dir.expanduser().resolve()
    last_resource_file = (
        args.last_resource_file.expanduser().resolve()
        if args.last_resource_file else
        None
    )

    if not dst_dir.is_dir() and dst_dir.exists():
        raise ValueError(f'--dst-dir {dst_dir} exists and is not a directory')

    if last_resource_file and not last_resource_file.is_file():
        raise ValueError(
            f'--last-resource-file {last_resource_file} does not exists or is not a file'
        )

    with get_archive_file(args, last_resource_file) as archive_file:
        extract_kernels(archive_file, dst_dir)
        make_executable(dst_dir)
