# coding: utf-8

import argparse
import hashlib
import logging
import shutil
import tarfile
import urllib.parse
import uuid
import simplejson as json

from queue import Queue
from base64 import b64encode
from threading import Thread
from time import time
import attr
import boto3
import botocore.config
import ijson
import os
import requests

import jsonschema

logger = logging.getLogger(__name__)
logging.basicConfig(format='%(asctime)s - %(name)s - %(levelname)s - %(message)s', level=logging.INFO)
logging.getLogger('urllib3.connectionpool').setLevel(logging.ERROR)
logging.getLogger('botocore.vendored.requests.packages.urllib3.connectionpool').setLevel(logging.ERROR)

DIALOG_SCHEMA = {
    '$schema': 'http://json-schema.org/draft-04/schema#',
    'type': 'array',
    'items': {
        "anyOf": [
            {
                'type': 'object',
                'properties': {
                    'type': {'type': 'string'},
                    'message': {'type': 'object'}
                }
            },
            {
                'type': 'object',
                'properties': {
                    'type': {'type': 'string'},
                    'message': {
                        'type': 'object',
                        'properties': {'cards': {'type': 'array', 'items': {'type': 'object'}}},
                        'additionalProperties': True
                    }
                }
            }
        ]
    }
}


@attr.s(frozen=True)
class Screenshot(object):
    file_path = attr.ib(type=str)
    md5 = attr.ib(type=str, default=None)


@attr.s(frozen=True)
class RenderedItem(object):
    dialog = attr.ib(repr=False)
    caption = attr.ib()
    s3_key = attr.ib()
    file_path = attr.ib()
    idx = attr.ib()

    def to_dict(self):
        d = attr.asdict(self)
        del d['dialog']
        return d

    def save_file(self, content_iterator):
        hasher = hashlib.md5()
        content_length = 0
        with open(self.file_path, 'wb') as f:
            for chunk in content_iterator:
                if chunk:
                    content_length += len(chunk)
                    f.write(chunk)
                    hasher.update(chunk)
        if content_length == 0:
            raise ValueError('empty screenshot content (length: 0)')
        return hasher.hexdigest()

    @staticmethod
    def generate_key_for_s3(caption, dialog):
        data = caption
        data += json.dumps(dialog, sort_keys=True)
        md5 = hashlib.md5()
        md5.update(data.encode('utf-8'))
        key = str(uuid.uuid3(uuid.NAMESPACE_DNS, md5.hexdigest()))
        return 'nirvana/' + key + '.json'

    @staticmethod
    def create(dialog, caption, file_path, idx):
        jsonschema.validate(dialog, DIALOG_SCHEMA)
        s3_key = RenderedItem.generate_key_for_s3(caption, dialog)
        return RenderedItem(dialog, caption, s3_key, file_path, idx)


@attr.s
class QueueItem(object):
    screenshot = attr.ib(type=Screenshot, default=None)
    done = attr.ib(type=bool, default=False)
    rendered_item = attr.ib(type=RenderedItem, default=None)
    attempt = attr.ib(default=0)
    errors = attr.ib(default=attr.Factory(list))


@attr.s(frozen=True)
class Args(object):
    renderer = attr.ib()
    source = attr.ib()
    destination = attr.ib()
    n_threads = attr.ib(converter=int)
    id_key = attr.ib()
    dialog_key = attr.ib()
    md5_destination = attr.ib()
    errors_destination = attr.ib()
    max_attempts = attr.ib(converter=int)


class S3Bucket(object):
    def __init__(self, key_id, access_key, endpoint_url, bucket_name, max_pool=200):
        boto_config = botocore.config.Config(max_pool_connections=max_pool)
        self.endpoint_url = endpoint_url
        self.session = boto3.session.Session(aws_access_key_id=key_id, aws_secret_access_key=access_key)
        self.s3 = self.session.client(service_name='s3', endpoint_url=self.endpoint_url, verify=False, config=boto_config)
        self.bucket_name = bucket_name

    def put(self, key, value, **kwargs):
        self.s3.put_object(Bucket=self.bucket_name, Key=key, Body=value, **kwargs)

    def delete(self, key):
        self.s3.delete_object(Bucket=self.bucket_name, Key=key)

    def get_public_url(self, key):
        host = self.endpoint_url.replace('mds.', '').replace('http://', '').replace('https://', '')
        url = 'http://{bucket}.{host}'.format(bucket=self.bucket_name, host=host)
        return urllib.parse.urljoin(url, key)


class RotorApi(object):
    CONN_TIMEOUT = 0.5
    SNAPSHOT_TIMEOUT = 1
    SESSION = requests.Session()
    SESSION.headers.update({
        'Content-Type': 'application/json',
        # Zora has L3 balancer, so we need to recreate connection
        # or we get a huge count of 502 errors
        'Connection': 'close',
    })

    def __init__(self, server, source, rotor_timeout, tvm_secret=None):
        self.handler = urllib.parse.urljoin(server, 'v1/rotor/execute/png')
        self.source = source
        self.process_timeout = rotor_timeout
        self.timeout = (self.CONN_TIMEOUT, rotor_timeout + self.SNAPSHOT_TIMEOUT)
        self.tvm_secret = tvm_secret

    def get_response(self, url):
        payload = {
            'Url': url,
            'Source': self.source,
            'Timeout': self.process_timeout,
            'Options': {
                'OutputFormat': {
                    'Html': False,
                    'Png': True
                },
                'ViewPortSize': {
                    'Width': 350,
                    'Height': 250
                },  # See ScreenshotMode
                'EnableImages': True,
                'ScreenshotMode': 1,  # 0 - Viewport only, 1 - Full visible
                'SnapshotTimeoutSeconds': self.SNAPSHOT_TIMEOUT
            }
        }
        if self.tvm_secret:
            payload['TvmServiceTicket'] = self.tvm_secret

        response = self.SESSION.post(self.handler, json=payload, timeout=self.timeout)
        # Zora has L3 balancer, so we need to recreate connection
        # or we get a huge count of 502 errors
        response.connection.close()
        if response.status_code != 200:
            error_message = 'Response code {0} / rotor status: {1} / rotor httpcode: {2}'.format(
                response.status_code,
                response.headers.get('X-Rotor-Status', 'No X-Rotor-Status'),
                response.headers.get('X-Rotor-HttpCode', 'No X-Rotor-HttpCode')
            )
            if response.text:
                error_message = '{0} - {1}'.format(error_message, response.text)
            raise ValueError(error_message)
        return response


class Renderer(object):
    def __init__(self, rotor_client, s3_bucket, div2html_url):
        self.rotor = rotor_client
        self.s3_bucket = s3_bucket
        self.div2html_url = div2html_url

    def render(self, data):
        item = data.rendered_item
        url = self.div2html_url + f'?url={self.s3_bucket.get_public_url(item.s3_key)}'
        try:
            past = time()
            self.s3_bucket.put(item.s3_key, json.dumps(item.dialog))
            logger.info('Uploaded: {0} / {1} / {2}'.format(item.idx, item.caption, time() - past))

            past = time()
            logger.info('Asking rotor api %s %s / %s', url, item.idx, item.caption)
            iterator = self.rotor.get_response(url).iter_content(chunk_size=4 * 1024)
            logger.info('Rendered: {0} / {1} / {2} / {3}'.format(item.idx, url, item.caption, time() - past))

            past = time()
            md5 = item.save_file(iterator)
            logger.info('Saved: {0} / {1} / {2}'.format(item.idx, item.caption, time() - past))
            return Screenshot(item.file_path, md5)
        except Exception as e:
            logger.error(
                'Rendering screenshot {idx} / {caption} attempt {attempt} {url} exception\n{error_type}: {error}'.format(
                    idx=item.idx,
                    caption=item.caption,
                    url=url,
                    error_type=type(e),
                    error=str(e),
                    attempt=data.attempt,
                )
            )
            raise


def prepare_folder(folder):
    if os.path.exists(folder):
        shutil.rmtree(folder)
    os.mkdir(folder)


def clear_folder(folder):
    shutil.rmtree(folder)


def env(env_key):
    return os.getenv('DIV2HTML_' + env_key)


class ValidateAction(argparse.Action):
    def __call__(self, parser, namespace, values, option_string=None):
        if not values:
            raise ValueError('Argument {} is required'.format(option_string))
        setattr(namespace, self.dest, values)


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--key_id', help='S3 Access Key Id', default=env('S3_KEY_ID'), action=ValidateAction)
    parser.add_argument('--access_key', help='S3 Access Secret Key', default=env('S3_SECRET_KEY'), action=ValidateAction)
    parser.add_argument('--endpoint_url', help='S3 endpoint url', default=env('S3_ENDPOINT'), action=ValidateAction)
    parser.add_argument('--client_id', help='TVM Client Id', default=env('TVM_CLIENT_ID'), action=ValidateAction)
    parser.add_argument('--client_secret', help='TVM Client Secret', default=env('TVM_CLIENT_SECRET'), action=ValidateAction)
    parser.add_argument('--bucket_name', help='S3 bucket name', default=env('S3_BUCKET'), action=ValidateAction)
    parser.add_argument('--rotor_server', help='Rotor server', default=env('ROTOR_SERVER'), action=ValidateAction)
    parser.add_argument('--zora_source', help='Zora source (aka quota)', default=env('ZORA_SOURCE'), action=ValidateAction)
    parser.add_argument('--rotor_timeout', default=12, help='Rotor processing timeout')
    parser.add_argument('--id_key', default='key', help='Unique identifier of object')
    parser.add_argument('--dialog_key', default='dialog', help='Dialog of object')
    parser.add_argument('--div2html_url', default='http://div2html.s3.yandex.net/test/dialog.html',
                        help='Url of div2html service')
    parser.add_argument('--source', help='Path to source file')
    parser.add_argument('--destination', help='Path to result file', default=env('DESTINATION'), action=ValidateAction)
    parser.add_argument('--md5_destination', help='Path to md5 file', default=env('MD5_DESTINATION'), action=ValidateAction)
    parser.add_argument('--errors_destination', help='Path to file with errors',
                        default=env('ERRORS_DESTINATION'), action=ValidateAction)
    parser.add_argument('--n_threads', default=10, help='Number of threads')
    parser.add_argument('--max-attempts', default=10, help='Number of attempts to render per screenshot')
    args = parser.parse_args()

    key_id = args.key_id
    access_key = args.access_key
    client_id = args.client_id.encode('utf-8')
    client_secret = args.client_secret.encode('utf-8')

    tvm_secret = b64encode(client_id + client_secret) if client_secret and client_secret else None

    # n_threads = int(args.n_threads)
    n_threads = 250
    zora_source = args.zora_source
    logger.info('Threads number: {0}'.format(n_threads))

    bucket = S3Bucket(key_id, access_key, args.endpoint_url, args.bucket_name, max_pool=n_threads * 3)
    logger.info('Bucket with parameters endpoint url: "{0}", bucket name: "{1}" is used'.format(args.endpoint_url,
                                                                                                args.bucket_name))
    rotor = RotorApi(args.rotor_server, zora_source, int(args.rotor_timeout), tvm_secret)
    logger.info('Rotor api with parameters server: "{0}", source: "{1}", timeout: "{2}" is used'.format(
        args.rotor_server, zora_source, args.rotor_timeout,
    ))
    renderer = Renderer(rotor, bucket, args.div2html_url)

    return Args(
        renderer=renderer,
        source=args.source,
        destination=args.destination,
        n_threads=n_threads,
        id_key=args.id_key,
        dialog_key=args.dialog_key,
        md5_destination=args.md5_destination,
        errors_destination=args.errors_destination,
        max_attempts=int(args.max_attempts)
    )


def read_generator(source, folder, id_key, dialog_key):

    with open(source, 'rb') as input_file:
        for i, item in enumerate(ijson.items(input_file, "item")):
            dialog = item[dialog_key]
            caption = urllib.parse.quote_plus(str(item[id_key]))
            file_path = folder + '/' + caption + '.png'
            logger.info('Generated dialog: {0} / {1}'.format(i, caption))
            yield RenderedItem.create(dialog, caption, file_path, i)


def main():
    args = parse_args()
    folder = os.path.basename(args.destination).split('.')[0]
    prepare_folder(folder)
    logger.info('Starting')

    input_queue = Queue(2 * args.n_threads)
    output_queue = Queue()

    def worker():
        while True:
            item = input_queue.get()
            if item is None:
                break
            try:
                item.screenshot = args.renderer.render(item)

            except Exception as e:
                logger.exception('Error: {0} / {1}'.format(item.rendered_item.idx, e))
                item.done = False
                item.attempt += 1
                item.errors.append(str(e))

                output_queue.put(item)
            else:
                item.done = True
                output_queue.put(item)
            input_queue.task_done()

    threads = []
    for i in range(args.n_threads):
        thread = Thread(target=worker)
        thread.daemon = True
        thread.start()
        threads.append(thread)

    total_items = 0
    total_done = 0
    file_paths = []
    hashes = {}
    errors = []

    def process_file_hash(fh):
        file_path, md5 = fh.screenshot.file_path, fh.screenshot.md5
        file_paths.append(file_path)
        key = file_path.split('/')[-1]
        hashes[key] = md5

    def process_failed_screenshot(item):
        errors.append({
            'idx': item.rendered_item.idx,
            'data': item.rendered_item.to_dict(),
            'errors': item.errors,
        })

    def process_output_queue():
        number = 0
        while not output_queue.empty():
            item = output_queue.get()

            if item.done:
                number += 1
                process_file_hash(item)
            else:
                if item.attempt < args.max_attempts:
                    input_queue.put(item, block=True)
                else:
                    logger.error('Max attempts %s exceeded for item %s', args.max_attempts, item)
                    process_failed_screenshot(item)
                    number += 1
            output_queue.task_done()
        return number

    for el in read_generator(args.source, folder, args.id_key, args.dialog_key):
        total_items += 1
        input_queue.put(QueueItem(rendered_item=el), block=True)
        total_done += process_output_queue()

    logger.info('All input items in queue, waiting for the end of processing.')
    while total_done < total_items:
        total_done += process_output_queue()

    input_queue.join()
    output_queue.join()
    for i in range(args.n_threads):
        input_queue.put(None)
    for thread in threads:
        thread.join()

    with tarfile.TarFile.gzopen(args.destination, mode='w', compresslevel=0) as tar:
        for file_path in file_paths:
            tar.add(file_path)

    with open(args.md5_destination, mode='w') as f:
        json.dump(hashes, f)

    with open(args.errors_destination, mode='w') as f:
        json.dump(errors, f)

    clear_folder(folder)
    logger.info('Finished: {0}'.format(total_items))


if __name__ == "__main__":
    main()
