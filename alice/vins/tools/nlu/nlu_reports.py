# coding: utf-8
from __future__ import unicode_literals

import logging
import click
import sys
import os
from botocore.exceptions import ClientError

from vins_core.ext.s3 import S3Bucket
from vins_core.logger import set_default_logging

click.disable_unicode_literals_warning = True

logger = logging.getLogger(__name__)


@click.group()
@click.option('--log-level', type=click.Choice(('DEBUG', 'INFO', 'WARNING', 'ERROR')), default='WARNING')
@click.option('--test', help='run with test s3', is_flag=True)
@click.option('--key_id', help='s3 key id', envvar='VINS_S3_ACCESS_KEY_ID', required=True, metavar='KEY')
@click.option('--access_key', help='s3 access key', envvar='VINS_S3_SECRET_ACCESS_KEY', required=True, metavar='KEY')
@click.pass_context
def main(ctx, log_level, test, key_id, access_key):
    set_default_logging(log_level)
    if test:
        endpoint_url = 'https://s3.mdst.yandex.net/'
    else:
        endpoint_url = 'https://s3.mds.yandex.net/'

    try:
        ctx.obj['s3'] = S3Bucket(key_id, access_key, endpoint_url)
    except Exception as e:
        logger.error('Failed to instance S3Bucket: %s', str(e))
        sys.exit(1)


@main.command(help='upload files to s3://vins-bucket/path')
@click.argument('filenames', nargs=-1, type=click.Path(exists=True))
@click.argument('path')
@click.pass_context
def upload(ctx, filenames, path):
    for filename in filenames:
        basename = os.path.basename(filename)
        key = os.path.join(path, basename)

        logger.info('Putting file "%s" with key "%s" to S3', filename, key)

        try:
            with open(filename, 'r') as fd:
                ctx.obj['s3'].put(key, fd.read())
        except ClientError as e:
            logger.error('Error putting file "%s" to key "%s": %s', filename, key, e.response)
            sys.exit(1)
        except IOError as e:
            logger.error('Error reading file "%s" %s: %s', filename, e.errno, e.strerror)
            sys.exit(1)
        except Exception as e:
            logger.error('Unexpected error: %s', e)
            sys.exit(1)


@main.command(help='downloads files from s3://vins-bucket/key to destination')
@click.argument('keys', nargs=-1)
@click.argument('path', nargs=1, type=click.Path())
@click.pass_context
def download(ctx, keys, path):
    for key in keys:
        basename = os.path.basename(key)
        filename = os.path.join(path, basename)

        logger.info('Downloading key "%s" to file "%s" from S3', key, filename)

        try:
            with open(filename, 'w') as fd:
                fd.write(ctx.obj['s3'].get(key))
        except ClientError as e:
            logger.error('Error downloading key "%s" to file "%s": %s', key, filename, e.response)
            sys.exit(1)
        except IOError as e:
            logger.error('Error writing file "%s" %s: %s'. filename, e.errno, e.strerror)
            sys.exit(1)
        except Exception as e:
            logger.error('Unexpected error: %s', e)
            sys.exit(1)


if __name__ == "__main__":
    main(obj={})
