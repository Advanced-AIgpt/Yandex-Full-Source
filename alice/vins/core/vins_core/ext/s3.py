# coding: utf-8
import os
import boto3
import logging

from datetime import datetime

from botocore.exceptions import ClientError as BotoClientError
from botocore.vendored.requests.exceptions import RequestException
from urlparse import urlparse
from vins_core.utils.config import get_setting
from vins_core.ext.base import BaseHTTPAPI
from vins_core.utils.updater import Updater
from vins_core.utils.data import make_safe_filename, vins_temp_dir
from vins_core.utils.metrics import sensors


logger = logging.getLogger(__name__)
RFC_1123_DT_STR = '%a, %d %b %Y %H:%M:%S GMT'


def get_s3_url(host, bucket, key):
    return 'http://{bucket}.{host}/{key}'.format(
        bucket=bucket,
        host=host,
        key=key,
    )


class S3Bucket(object):
    def __init__(self, key_id, access_key, endpoint_url=None, bucket_name='vins-bucket'):
        self._endpoint_url = endpoint_url or get_setting('S3_ENDPOINT_URL', 'http://s3.mds.yandex.net/')
        self._session = boto3.session.Session(
            aws_access_key_id=key_id,
            aws_secret_access_key=access_key,
        )
        self._s3 = self._session.client(
            service_name='s3',
            endpoint_url=self._endpoint_url,
            verify=False,
        )
        self._bucket_name = bucket_name
        try:
            self._s3.create_bucket(Bucket=bucket_name)
        except (BotoClientError, RequestException):
            pass

    def put(self, key, value, **kwargs):
        self._s3.put_object(
            Bucket=self._bucket_name,
            Key=key,
            Body=value,
            **kwargs
        )

    def get(self, key):
        obj = self._s3.get_object(
            Bucket=self._bucket_name,
            Key=key
        )
        return obj['Body'].read()

    def delete(self, key):
        self._s3.delete_object(
            Bucket=self._bucket_name,
            Key=key
        )

    def download_file(self, key, filename=None):
        if not filename:
            filename = key
        self._s3.download_file(self._bucket_name, key, filename)

    def download_fileobj(self, key, fileobj):
        self._s3.download_fileobj(self._bucket_name, key, fileobj)

    def get_url(self, key):
        return get_s3_url(
            bucket=self._bucket_name,
            host=urlparse(self._endpoint_url).hostname,
            key=key,
        )


class S3DownloadAPI(BaseHTTPAPI):
    MAX_RETRIES = 3

    def __init__(self, bucket_name='vins-bucket', endpoint_url=None, **kwargs):
        _endpoint_url = endpoint_url or get_setting('S3_ENDPOINT_URL', 'http://s3.mds.yandex.net/')
        self._host = urlparse(_endpoint_url).hostname
        self._bucket_name = bucket_name
        super(S3DownloadAPI, self).__init__(**kwargs)

    @sensors.with_timer('s3_response_time')
    def _get_response(self, key, headers=None, stream=False):
        resp = super(S3DownloadAPI, self).get(
            get_s3_url(
                host=self._host,
                bucket=self._bucket_name,
                key=key,
            ),
            headers=headers or {},
            stream=stream,
            request_label='s3_{0}'.format(key),
        )
        sensors.inc_counter('s3_response', labels={'status_code': resp.status_code})
        return resp

    def get(self, key):
        response = self._get_response(key)
        response.raise_for_status()
        return response.content

    def get_if_modified(self, key, dt):
        headers = {}
        if dt:
            headers['If-Modified-Since'] = dt.strftime(RFC_1123_DT_STR)
        response = self._get_response(key, headers=headers)
        last_modified_header = response.headers.get('Last-Modified')
        last_modified_dt = None
        if last_modified_header:
            last_modified_dt = datetime.strptime(last_modified_header, RFC_1123_DT_STR)

        if response.status_code == 304:
            return False, last_modified_dt, None
        else:
            response.raise_for_status()
            return True, last_modified_dt, response.content

    def download(self, key, path):
        response = self._get_response(key, stream=True)
        response.raise_for_status()

        with open(path, 'wb') as f:
            for chunk in response.iter_content(chunk_size=10 * 1024):
                if chunk:
                    f.write(chunk)


class S3Updater(Updater):
    def __init__(self, path, file_path=None, **kwargs):
        self._path = path
        self._s3 = S3DownloadAPI()
        self._last_modified = None
        if not file_path:
            file_path = os.path.join(vins_temp_dir(), 's3_updater__%s' % make_safe_filename(path))
        super(S3Updater, self).__init__(file_path=file_path, **kwargs)

    def _download_to_file(self, file_obj):
        try:
            is_modified, last_modified, content = self._s3.get_if_modified(self._path, self._last_modified)
            if is_modified:
                self._last_modified = last_modified
                file_obj.write(content)
            return is_modified
        except RequestException as exc:
            logger.error("Can't get file %s fom s3. Error: %s", self._path, exc)
