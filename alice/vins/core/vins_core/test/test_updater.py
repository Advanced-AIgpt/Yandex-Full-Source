# coding: utf-8

import pytest
import time
import requests_mock
from multiprocessing import Pool
from mock import patch

from vins_core.utils.updater import Updater
from vins_core.ext.yt import YtUpdater
from vins_core.ext.s3 import S3Updater


class DummyUpdater(Updater):
    def on_update(self, *args):
        pass

    def _download_to_file(self, file_obj):
        file_obj.write('test')
        return True


@pytest.fixture(scope='function')
def tmp_file_path(tmpdir):
    return str(tmpdir.join('data.bin'))


def _start_stop_updater(tmp_file_path):
    updater = DummyUpdater(interval=0.1, file_path=tmp_file_path)
    with patch.object(updater, '_run_downloader') as run_downloader_mock:
        with patch.object(updater, '_run_watcher') as run_watcher_mock:
            updater.start()
            time.sleep(0.1)
            updater.stop()
            return run_downloader_mock.call_count, run_watcher_mock.call_count


@pytest.mark.skip(reason='flaky')
def test_start(mocker, tmp_file_path):
    pool = Pool(processes=4)
    res = pool.map(_start_stop_updater, [tmp_file_path for _ in range(4)])
    pool.close()
    pool.join()
    assert sum([r[0] for r in res]) == 1
    assert sum([r[1] for r in res]) == 3


def test_yt_download(mocker, tmp_file_path):
    mock = requests_mock.Mocker()
    mock.get('http://hahn.yt.yandex.net/api/v3/get?path=%2F%2Ftest%2F%40modification_time', content='1')
    mock.get('http://hahn.yt.yandex.net/api/v3/read_table', content='test data')

    with mock:
        updater = YtUpdater('//test', interval=0.1, file_path=tmp_file_path)
        updater._try_update()
        with open(tmp_file_path) as f:
            assert f.read() == 'test data'


def test_s3_download(mocker, tmp_file_path):
    mock = requests_mock.Mocker()
    mock.get('http://vins-bucket.s3.us-east-2.amazonaws.com/test', content='test data')

    with mock:
        updater = S3Updater('test', interval=0.1, file_path=tmp_file_path)
        updater._try_update()
        with open(tmp_file_path) as f:
            assert f.read() == 'test data'
