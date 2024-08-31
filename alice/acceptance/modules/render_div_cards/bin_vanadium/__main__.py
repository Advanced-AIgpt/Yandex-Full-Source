# coding: utf-8

import argparse
import hashlib
import logging
import os
import sys
import urllib.parse
import uuid
from time import time

import attr
import boto3
from botocore import exceptions as boto_exceptions
import jsonschema
import simplejson as json

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
class Args(object):
    id_key = attr.ib()
    dialog_key = attr.ib()
    bucket = attr.ib()
    div2html_url = attr.ib()


class S3Bucket(object):
    def __init__(self, s3_key, s3_secret, endpoint_url, bucket_name):
        self.s3 = boto3.client(service_name='s3', verify=False, endpoint_url=endpoint_url, aws_access_key_id=s3_key,
                               aws_secret_access_key=s3_secret)
        self.endpoint_url = endpoint_url
        self.bucket_name = bucket_name

    def put(self, key, value, **kwargs):
        self.s3.put_object(Bucket=self.bucket_name, Key=key, Body=value, **kwargs)

    def delete(self, key):
        self.s3.delete_object(Bucket=self.bucket_name, Key=key)

    def get_public_url(self, key):
        host = self.endpoint_url.replace('mds.', '').replace('http://', '').replace('https://', '')
        url = 'http://{bucket}.{host}'.format(bucket=self.bucket_name, host=host)
        return urllib.parse.urljoin(url, key)


def env(env_key):
    return os.getenv('DIV2HTML_' + env_key)


class ValidateAction(argparse.Action):
    def __call__(self, parser, namespace, values, option_string=None):
        if not values:
            raise ValueError('Argument {0} is required'.format(option_string))
        setattr(namespace, self.dest, values)


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--key_id', help='S3 Access Key Id', default=env('S3_KEY_ID'), action=ValidateAction)
    parser.add_argument('--access_key', help='S3 Access Secret Key', default=env('S3_SECRET_KEY'),
                        action=ValidateAction)
    parser.add_argument('--endpoint_url', help='S3 endpoint url', default=env('S3_ENDPOINT'), action=ValidateAction)
    parser.add_argument('--bucket_name', help='S3 bucket name', default=env('S3_BUCKET'), action=ValidateAction)
    parser.add_argument('--id_key', default='screenshot_hashsum', help='Item id\'s key')
    parser.add_argument('--dialog_key', default='dialog', help='Item dialog\'s key')
    parser.add_argument('--div2html_url', default='http://div2html.s3.yandex.net/test/dialog.html',
                        help='Url of div2html service')

    args = parser.parse_args()

    bucket = S3Bucket(args.key_id, args.access_key, args.endpoint_url, args.bucket_name)
    logger.info('Bucket with the following parameters is used: endpoint url: "{0}", bucket name: "{1}"'
                .format(args.endpoint_url, args.bucket_name))

    return Args(id_key=args.id_key, dialog_key=args.dialog_key, bucket=bucket, div2html_url=args.div2html_url)


def generate_key_for_s3(key, dialog):
    data = key
    # Use only external key (powered by nerevar@)
    # data += json.dumps(dialog, sort_keys=True)
    md5 = hashlib.md5()
    md5.update(data.encode('utf-8'))
    key = str(uuid.uuid3(uuid.NAMESPACE_DNS, md5.hexdigest()))
    return 'nirvana/' + key + '.json'


def put_into_s3(item, s3_bucket, dialog_key, id_key):
    try:
        dialog = item[dialog_key]
        key = str(item[id_key])
    except KeyError:
        logger.error('Can\'t get item\'s dialog or key from json')
        raise

    try:
        jsonschema.validate(dialog, DIALOG_SCHEMA)
        s3_key = generate_key_for_s3(key, dialog)
        past = time()
        s3_bucket.put(s3_key, json.dumps(dialog))
    except jsonschema.ValidationError:
        logger.error('Bad json. Dialog: {0}'.format(dialog))
        raise
    except boto_exceptions.ClientError:
        logger.error('Failed to put item to S3. Item key: {0}'.format(key))
        raise

    logger.info('Item uploaded. Screenshot_hashsum: {0}; uploading time {1}'.format(key, time() - past))
    return s3_bucket.get_public_url(s3_key)


def get_dialog_page_url(div2html_url, s3_public_url):
    return div2html_url + f'?url={s3_public_url}'


class DialogRenderer:
    def __init__(self, dialog_key, id_key, s3_bucket, errors_output, div2html_url):
        logger.info('Initializing DialogRenderer')
        self.s3_bucket = s3_bucket
        self.dialog_key = dialog_key
        self.id_key = id_key
        self.errors_output = errors_output
        self.div2html_url = div2html_url

    def __call__(self, row):
        try:
            s3_url = put_into_s3(row, self.s3_bucket, self.dialog_key, self.id_key)
            dialog_page_url = get_dialog_page_url(self.div2html_url, s3_url)
            print(json.dumps({
                'url': dialog_page_url,
                'meta': {
                    'screenshot_hashsum': row.get(self.id_key)
                }
            }))
        except Exception as error:
            print(json.dumps({
                'screenshot_hashsum': row.get(self.id_key),
                'errors': {
                    'type': type(error).__name__,
                    'message': str(error)
                }
            }), file=self.errors_output)


def main():
    args = parse_args()
    logger.info('Starting...')

    with os.fdopen(4, "w") as errors_output:
        renderer = DialogRenderer(args.dialog_key, args.id_key, args.bucket, errors_output, args.div2html_url)
        for line in sys.stdin:
            row = json.loads(line)
            renderer(row)


if __name__ == '__main__':
    main()
