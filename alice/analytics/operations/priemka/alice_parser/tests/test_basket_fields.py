# coding: utf-8

import os

import yatest.common
from nile.api.v1.clusters import MockYQLCluster
from qb2.api.v1 import (
    typing as qt,
)
from alice.analytics.operations.priemka.alice_parser.lib.alice_parser import AliceParser
from alice.analytics.utils.testing_utils.nile_testing_utils import NileJobTestCase


def data_path(filename):
    test_folder_name = os.path.basename(__file__).replace('.py', '')
    return yatest.common.test_source_path(os.path.join(test_folder_name, filename))


BASKET_SCHEMA = {
    'device_state': qt.Optional[qt.Json],
    'activation_type': qt.Optional[qt.String],
    'additional_options': qt.Optional[qt.Json],
    'app_preset': qt.Optional[qt.String],
    'asr_options': qt.Optional[qt.Json],
    'asr_text': qt.Optional[qt.String],
    'basket': qt.String,
    'client_time': qt.Optional[qt.String],
    'context_len': qt.Optional[qt.Int64],
    'exact_location': qt.Optional[qt.String],
    'experiments': qt.Optional[qt.Json],
    'fetcher_mode': qt.Optional[qt.String],
    'full_text': qt.Optional[qt.String],
    'is_empty_asr': qt.Optional[qt.Bool],
    'is_good_session': qt.Optional[qt.Bool],
    'is_negative_query': qt.Optional[qt.Int32],
    'is_new': qt.Optional[qt.String],
    'is_positive': qt.Optional[qt.String],
    'is_positive_prob': qt.Optional[qt.Float],
    'location': qt.Optional[qt.Json],
    'mds_key': qt.Optional[qt.String],
    'oauth': qt.Optional[qt.String],
    'real_generic_scenario': qt.Optional[qt.String],
    'real_reqid': qt.Optional[qt.String],
    'real_session_id': qt.Optional[qt.String],
    'real_session_sequence': qt.Optional[qt.Int64],
    'real_uuid': qt.Optional[qt.String],
    'request_id': qt.Optional[qt.String],
    'request_source': qt.Optional[qt.String],
    'sampling_intent': qt.Optional[qt.String],
    'session_id': qt.Optional[qt.String],
    'session_len': qt.Optional[qt.UInt64],
    'session_sequence': qt.Optional[qt.Int64],
    'text': qt.Optional[qt.String],
    'timezone': qt.Optional[qt.String],
    'toloka_intent': qt.Optional[qt.String],
    'ts': qt.Optional[qt.Int64],
    'vins_intent': qt.Optional[qt.String],
    'voice_binary': qt.Optional[qt.String],
    'voice_url': qt.Optional[qt.String],
    'toloka_extra_state': qt.Optional[qt.Json],
}


class TestPrepareBasket(NileJobTestCase):
    def test_prepare_basket(self):
        ap = AliceParser('dummy', 'dummy', 'dummy', 'dummy', token='dummy')

        job = MockYQLCluster().job()
        job.table('dummy').label('input').call(ap.prepare_basket).label('output')

        self.assertCorrectLocalNileRun(
            job,
            data_path('01_basket.in.json'),
            data_path('01_basket.out.json'),
            BASKET_SCHEMA,
        )

    def test_custom_input_basket(self):
        ap = AliceParser('dummy', 'dummy', 'dummy', 'dummy', token='dummy')

        job = MockYQLCluster().job()
        job.table('dummy').label('input').call(ap.prepare_basket).label('output')

        self.assertCorrectLocalNileRun(
            job,
            data_path('02_basket_input_basket.in.json'),
            data_path('02_basket_input_basket.out.json'),
            BASKET_SCHEMA,
        )
