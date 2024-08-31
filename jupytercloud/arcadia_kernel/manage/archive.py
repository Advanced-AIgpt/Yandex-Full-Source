# -*- coding: utf-8 -*-

import json
import shutil
import subprocess
import tarfile

from tqdm import tqdm

from .utils import (
    check_bin_exists,
    get_steps,
    logger,
    measure_time,
    temporary_directory,
    BLOCK_SIZE,
    BLOCK_NAME,
)


def _extract_kernel_package_name():
    from library.python import resource

    package_json = resource.find('package.json')
    package_info = json.loads(package_json)
    if name := package_info.get('meta', {}).get('name'):
        return name

    raise ValueError('failed to extract package name from package.json')


def _find_archive(archive_path):
    package_name = _extract_kernel_package_name()
    possible_archives = archive_path.glob(f'{package_name}.*.tar.gz')
    if not (possible_archives := list(possible_archives)):
        raise ValueError(f'failed to find kernels archive in {archive_path} dir')
    if len(possible_archives) > 1:
        raise ValueError(
            f'there are more than one kernels archive candidates '
            f'in {archive_path} dir: {possible_archives}'
        )

    return possible_archives[0]


def extract_all_files(archive_path, destination):
    if archive_path.is_dir():
        archive_path = _find_archive(archive_path)

    if check_bin_exists('tar'):
        with measure_time('extract all files via tar'):
            _extract_all_files_tar(archive_path, destination)
    else:
        with measure_time('extract all files via python tarfile'):
            _extract_all_files_python(archive_path, destination)


def _extract_all_files_tar(archive_path, destination):
    command = ['tar', '-xf', str(archive_path), '--dir', str(destination)]

    logger.info(
        'archive will %s be extracted to %s via tar: %s',
        archive_path, destination, command
    )

    output = subprocess.check_output(
        command,
        stderr=subprocess.STDOUT
    ) or ''

    output = output.strip()
    if output:
        logger.info('%s output: %s', command, output)

    logger.info(
        'archive %s extracted to %s via tar: %s',
        archive_path, destination, command
    )


def _extract_all_files_python(archive_path, destination):
    logger.info(
        'archive will %s be extracted to %s via tarfile',
        archive_path, destination
    )

    with tarfile.open(archive_path, mode='r') as archive:
        archive.extractall(destination)

    logger.info(
        'archive %s extracted to %s via tarfile',
        archive_path, destination
    )


def extract_files(archive_path, members_dst):
    for path in members_dst.values():
        path.parent.mkdir(parents=True, exist_ok=True)
        if path.exists():
            logger.warning('will rewrite file %s', path)

    if check_bin_exists('tar'):
        with measure_time('extract files via tar'):
            _extract_files_tar(archive_path, members_dst)
    else:
        with measure_time('extract files via python tarfile'):
            _extract_files_python(archive_path, members_dst)


def _extract_files_python(archive_path, members_dst):
    with tarfile.open(archive_path, bufsize=BLOCK_SIZE) as archive:
        for member_name, dst_file in members_dst.items():
            logger.info(
                'extracting file %s from %s to %s via tarfile',
                member_name, archive_path, dst_file
            )

            tar_info = archive.getmember(member_name)

            with \
                    archive.extractfile(tar_info) as data, \
                    dst_file.open('wb') as file_, \
                    tqdm(total=get_steps(tar_info.size), unit=BLOCK_NAME) as pbar:

                while buffer := data.read(BLOCK_SIZE):
                    file_.write(buffer)
                    pbar.update(1)

            logger.info(
                '%s extracted from %s to %s via tarfile',
                member_name, archive_path, dst_file
            )


def _extract_files_tar(archive_path, members_dst):
    with temporary_directory() as tmp_dir:
        _extract_all_files_tar(archive_path, tmp_dir)

        for member_name, dst_file in members_dst.items():
            member_path = tmp_dir / member_name
            logger.info('move file %s to %s', member_path, dst_file)
            shutil.move(member_path, dst_file)

    # it works 1.5 times slower, but better with different file systems;
    # but as long we at nirvana - it will not be a problem
    #
    # output_files = {}
    # processes = {}
    # for member_name, dst_file in members_dst.items():
    #     output_file = output_files[member_name] = dst_file.open('wb')
    #     process = processes[member_name] = subprocess.Popen(
    #         ['tar', '-xz', '-O', '--get', member_name, '-f', archive_path],
    #         stdout=output_file,
    #         stderr=subprocess.PIPE,
    #     )

    # for member_name, process in processes.items():
    #     _, stderr = process.communicate()
    #     if process.returncode:
    #         raise RuntimeError(
    #             f'process {process.args} exited with code {process.returncode} and wrote stderr: \n{stderr}'
    #         )
