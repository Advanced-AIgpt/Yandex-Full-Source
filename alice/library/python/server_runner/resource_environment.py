import logging
import os
import shutil

import alice.library.python.server_runner.vins_package as vins_package
from retry import retry
from yalibrary.yandex.sandbox import SandboxClient, fetcher as sandbox_fetcher


@retry(tries=5, delay=1)
def _try_download_resource(resource_id, resource_type, dst_path):
    logging.info(f'Downloading resource {resource_id} of type {resource_type} into {dst_path}')
    sandbox_fetcher.download_resource(resource_id, dst_path, methods=['skynet', 'http_tgz'])


def _is_downloaded(res_type, res_id, source_path):
    file_path = os.path.join(source_path, res_type)
    if not os.path.exists(file_path):
        return False

    with open(file_path) as stream:
        saved_res_id = stream.read()

    return saved_res_id == str(res_id)


def _save_dowloaded_res_id(res_type, res_id, source_path):
    file_path = os.path.join(source_path, res_type)
    with open(file_path, 'w') as stream:
        stream.write(str(res_id))


def _download_resource(source_path, res_id, res_type, res_name, res_path):
    if res_path.startswith('/'):
        res_path = res_path[1:]
    dst_path = os.path.join(source_path, res_path)
    file_path = os.path.join(dst_path, res_name)

    if _is_downloaded(res_type, res_id, source_path) and os.path.exists(file_path):
        return file_path

    if os.path.exists(file_path):
        if os.path.isfile(file_path):
            os.remove(file_path)
        else:
            shutil.rmtree(file_path)

    _try_download_resource(res_id, res_type, dst_path)
    _save_dowloaded_res_id(res_type, res_id, source_path)
    return file_path


def download(args, resources):
    if not resources:
        return

    vins_pkg = vins_package.load(
        args.use_local, args.arcadia_dir, args.vins_package_abs_path,
    )

    for res_id, res_path in vins_package.iter_sandbox_resource(vins_pkg):
        logging.info(f'Getting info about sandbox resource id={res_id}')
        resource = SandboxClient().get_resource(res_id)
        logging.info('Done')

        res_type = resource['type']
        if res_type not in resources:
            continue

        yield res_type, _download_resource(args.source_dir, res_id, res_type, resource['file_name'], res_path)
