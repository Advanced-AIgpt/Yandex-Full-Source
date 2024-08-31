import argparse
import json
import os
import tarfile
import shutil
import sys

import sh
import vh

from alice.tests.opus_generator.library import opus_consts, translit


def upload_to_nirvana(opus_data, oauth_token, nirvana_quota):
    params = dict(content=opus_data, name='opus names', data_type='json')
    if nirvana_quota:
        return vh.data_from_str(**params)

    data_info = vh.upload_from_str(oauth_token=oauth_token, **params)
    return vh.data(id=data_info.data_id)


def generate_opuses(opus_data, oauth_token, nirvana_quota=None):
    data = upload_to_nirvana(opus_data, oauth_token, nirvana_quota)

    git_checkout = vh.op(name='git sparse checkout and submodule update', owner='ifled')
    binary = git_checkout(
        key='voice-robot-ssh-key',
        repo='ssh://git@bb.yandex-team.ru/voice/nirvana-blocks.git',
        folder_name='nirvana-blocks',
        commit='16a975cb1495fedc035b60d9a3e1c0c5957a040c',
        remove_git_files='true',
    )

    tts_generate = vh.op(name='uniproxy-tts-generate', owner='ezkhrv')
    output = tts_generate(
        text_spec=data,
        working_copy=binary,
        url='wss://uniproxy.alice.yandex.net/uni.ws',
        timeout=100,
        jobs=1,
        format=opus_consts.file_extension,
        speakers='shitova.gpu',
        language=opus_consts.language,
        wildcard=opus_consts.filename_template,
    )['generated']

    keeper = vh.run(
        oauth_token=oauth_token,
        start=True,
        wait=True,
        api_retries=10,
        quota=nirvana_quota,
    )
    return keeper.download(output)


def generate_opus_names(input_stream):
    def _opus_info(text):
        text = text.strip()
        return dict(text=text, uid=translit(text))

    infos = [_opus_info(line) for line in input_stream]
    return json.dumps(infos, indent=4, ensure_ascii=False)


def save_opuses(opus_data, dst_path):
    if os.path.exists(dst_path):
        shutil.rmtree(dst_path, ignore_errors=True)

    with tarfile.open(opus_data) as tar:
        tar.extractall(path=dst_path)


def upload_opuses(src_path):
    params = [
        '--type', 'ALICE_VOICE_COMMANDS',
        '--do-not-remove',
        '--description', '"Alice voice commans: {}"'.format(os.path.basename(src_path)),
    ]
    print('INFO: ya upload {} {}'.format(' '.join(params), src_path))
    task = sh.ya.upload(src_path, params)
    print(task.stderr.decode())
    if task.exit_code:
        raise Exception(task.stderr.decode())


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        'input',
        nargs='?',
        type=argparse.FileType('r'),
        default=sys.stdin,
        help='Path to file',
    )

    parser.add_argument('-d', '--dst-path', required=True, help='OPUS_FOLDER_NAME')
    parser.add_argument('-q', '--quota', default=None, help='Nirvana quota')
    parser.add_argument('-t', '--oauth-token', default=None, help='Nirvana OAuth token')
    parser.add_argument('-u', '--upload', action='store_true', help='Upload to Sandbox')

    return parser.parse_args()


def main():
    args = parse_args()

    opus_names = generate_opus_names(args.input)
    opus_data = generate_opuses(opus_names, args.oauth_token, args.quota)
    save_opuses(opus_data, args.dst_path)
    if args.upload:
        upload_opuses(args.dst_path)

    if os.path.exists(opus_data):
        os.remove(opus_data)
