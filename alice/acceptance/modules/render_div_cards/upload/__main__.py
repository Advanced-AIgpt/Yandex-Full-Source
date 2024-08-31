# coding: utf-8

import argparse
import logging
import os

import boto3
from botocore.exceptions import ClientError as BotoClientError
from botocore.vendored.requests.exceptions import RequestException

logger = logging.getLogger(__name__)
logging.basicConfig(format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
                    level=logging.INFO)


class Div2Html(object):
    def __init__(self, s3client, bucket_name):
        self.s3 = s3client
        self.bucket_name = bucket_name
        try:
            self.s3.create_bucket(Bucket=bucket_name)
        except (BotoClientError, RequestException):
            pass

    def _put(self, key, value, **kwargs):
        self.s3.put_object(
            Bucket=self.bucket_name,
            Key=key,
            Body=value,
            **kwargs
        )

    def upload_sources(self, folder, prefix=''):
        prefix = prefix or ''
        for file_name in os.listdir(folder):
            key = prefix + file_name
            with open(os.path.join(folder, file_name), 'rb') as value:
                # For more info about ACL, ContentEncoding and ContentType for gz files
                # 1. See https://medium.com/@graysonhicks/how-to-serve-gzipped-js-and-css-from-aws-s3-211b1e86d1cd
                # for S3 base serving gzipped js and css
                # 2. See https://boto3.readthedocs.io/en/latest/reference/services/s3.html#S3.Client.put_object
                # for args parameters
                if file_name.endswith('gz.css'):
                    self._put(key, value,
                              ACL='public-read',
                              ContentEncoding='gzip',
                              ContentType='text/css')
                elif file_name.endswith('gz.js'):
                    self._put(key, value,
                              ACL='public-read',
                              ContentEncoding='gzip',
                              ContentType='text/javascript')
                else:
                    self._put(key, value)
            logging.info('File %s uploaded' % key)


def env(env_key):
    return os.getenv('DIV2HTML_' + env_key)


class ValidateAction(argparse.Action):
    def __call__(self, parser, namespace, values, option_string=None):
        if not values:
            raise ValueError('Argument {} is required'.format(option_string))
        setattr(namespace, self.dest, values)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--key_id', default=env('S3_KEY_ID'), action=ValidateAction)
    parser.add_argument('--access_key', default=env('S3_SECRET_KEY'), action=ValidateAction)
    parser.add_argument('--endpoint_url', default=env('S3_ENDPOINT'), action=ValidateAction)
    parser.add_argument('--bucket_name', default=env('S3_BUCKET'), action=ValidateAction)
    parser.add_argument('--path', required=True)
    parser.add_argument('--prefix', default='')
    args = parser.parse_args()

    session = boto3.session.Session(aws_access_key_id=args.key_id, aws_secret_access_key=args.access_key)
    client = session.client(service_name='s3', endpoint_url=args.endpoint_url, verify=False)
    div2html = Div2Html(client, args.bucket_name)
    div2html.upload_sources(args.path, args.prefix)


if __name__ == "__main__":
    main()
