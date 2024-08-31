# coding: utf-8

import os

import yatest.common
from nile.api.v1.clusters import MockYQLCluster
# from qb2.api.v1 import (
#     typing as qt,
# )
from alice.analytics.operations.priemka.alice_parser.lib.alice_parser import AliceParser
from alice.analytics.utils.testing_utils.nile_testing_utils import NileJobTestCase


def data_path(filename):
    test_folder_name = os.path.basename(__file__).replace('.py', '')
    return yatest.common.test_source_path(os.path.join(test_folder_name, filename))


class TestPrepareDownloaderResult(NileJobTestCase):
    def test_prepare_downloader_result_01(self):
        ap = AliceParser('dummy', 'dummy', 'dummy', 'dummy', token='dummy')

        job = MockYQLCluster().job()
        job.table('dummy').label('input').call(ap.prepare_downloader_result).label('output')

        self.assertCorrectLocalNileRun(
            job,
            data_path('01_downloader_result.in.json'),
            data_path('01_downloader_result.out.json')
        )

    def test_prepare_downloader_result_02_fairy_tales(self):
        ap = AliceParser('dummy', 'dummy', 'dummy', 'dummy', token='dummy')

        job = MockYQLCluster().job()
        job.table('dummy').label('input').call(ap.prepare_downloader_result).label('output')

        self.assertCorrectLocalNileRun(
            job,
            data_path('02_fairy_tales.in.json'),
            data_path('02_fairy_tales.out.json')
        )

    def test_prepare_downloader_result_03_music_unanswer(self):
        ap = AliceParser('dummy', 'dummy', 'dummy', 'dummy', token='dummy')

        job = MockYQLCluster().job()
        job.table('dummy').label('input').call(ap.prepare_downloader_result).label('output')

        self.assertCorrectLocalNileRun(
            job,
            data_path('03_music_unanswer.in.json'),
            data_path('03_music_unanswer.out.json')
        )
