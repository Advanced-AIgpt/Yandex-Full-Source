# coding: utf-8
from __future__ import unicode_literals

import mock
import pytest

import nlu_service.services.wizard_client as wizard_client


@pytest.mark.gen_test
def test_get_ner_doesnt_request_wizard_on_ping():
    ner_result = yield wizard_client.get_ner(utterance=u'ping')
    with mock.patch(
            'nlu_service.services.wizard_client.request_wizard',
            autospec=True,
    ) as request_wizard:
        assert ner_result == wizard_client.NerResult(
            tokens=['ping'],
            entities=[],
            wizard_markup={},
        )
        assert request_wizard.call_count == 0
