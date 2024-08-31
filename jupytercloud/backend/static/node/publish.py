#!/usr/bin/env python3

import argparse
import json
import logging
import pathlib

import boto3
import vault_client.instances


SECRET_ID = 'sec-01d83hjj2yehgykzn85n2h5pa5'
SECRET_KEY = 's3_mds_access_key'
BUCKET_NAME = 'jupytercloud-static'


def parse_args():
    parser = argparse.ArgumentParser()

    parser.add_argument('env', choices=('production', 'testing'))

    return parser.parse_args()


def get_s3_access_key():
    client = vault_client.instances.Production(decode_files=True)
    raw = client.get_version(SECRET_ID)
    value = raw['value'][SECRET_KEY]
    return json.loads(value)


def main():
    logging.basicConfig(level=logging.INFO)

    args = parse_args()
    s3_access_key = get_s3_access_key()

    session = boto3.session.Session(
        aws_access_key_id=s3_access_key['AccessKeyId'],
        aws_secret_access_key=s3_access_key['AccessSecretKey'],
        region_name='ru-central-1',
    )

    s3 = session.client(service_name='s3', endpoint_url='https://s3.mds.yandex.net')

    for path in pathlib.Path('./dist').rglob('*'):
        name = str(path.relative_to('dist'))

        if name == 'webpack-assets.json':
            name = f'webpack-assets-{args.env}.json'

        logging.info('uploading %s as %s to bucket %s', path, name, BUCKET_NAME)

        s3.upload_file(str(path), BUCKET_NAME, name)


if __name__ == '__main__':
    main()
