# coding: utf-8

import pytest
import mock

from personal_assistant.hardcoded_responses import HARDCODED_RESPONSES_RESOURCE_PATH, load_hardcoded, HardcodedResponses


def test_hardcoded_conf_is_valid():
    data = load_hardcoded(HARDCODED_RESPONSES_RESOURCE_PATH)
    HardcodedResponses.validate_config(data, raise_on_error=True)


def test_bad_conf(tmpdir):
    with pytest.raises(AttributeError):
        res = {'a': 1}
        with mock.patch('personal_assistant.hardcoded_responses.load_hardcoded', return_value=res):
            HardcodedResponses(mock.MagicMock())
