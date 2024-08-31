import argparse
import hashlib
import os
import subprocess
import logging

import sandbox.common.rest as sandbox


os.environ['YT_PROXY'] = 'hahn'


def process(filename, output):
    return subprocess.Popen([
        './mdsdelete', '-file', filename, '-output', output, '-jobs', '1500'
    ])


def download_with_retries(resource_id, filename, retries=10):
    url = f'https://proxy.sandbox.yandex-team.ru/{resource_id}'
    sandbox_client = sandbox.Client()
    true_md5 = sandbox_client.resource[resource_id].read()['md5']
    for _ in range(retries):
        subprocess.Popen(['curl', '--silent', '--insecure', '-o', filename, url]).wait()
        with open(filename, 'rb') as fin:
            file_md5 = hashlib.md5(fin.read()).hexdigest()
        if true_md5 == file_md5:
            return
        logging.warn(f'Get md5 {file_md5} insted of {true_md5}')
    raise RuntimeError(f"Can't download file from url {url}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--input', dest='input',
        help='input id',
        default=None,
        required=True
    )
    parser.add_argument(
        '--output', help='output file with bad names', required=True
    )
    args = parser.parse_args()

    # load mds_delete
    download_with_retries(args.input, 'input.txt')
    download_with_retries(3064746424, 'mdsdelete')
    subprocess.Popen(['chmod', '777', './mdsdelete']).wait()

    processes = []
    processes.append(process('input.txt', args.output))

    for p in processes:
        p.wait()

if __name__ == '__main__':
    main()
