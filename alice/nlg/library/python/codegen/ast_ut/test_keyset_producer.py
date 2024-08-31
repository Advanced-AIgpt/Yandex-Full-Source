# coding: utf-8

import os
import pytest
import yatest.common

from alice.nlg.library.python.codegen_tool import dump_keyset
from alice.nlg.library.python.codegen.ast_ut.common import NLG_FILES, TEMPLATES_DIR
from alice.nlg.library.python.codegen.localized_builder import get_library


@pytest.mark.parametrize(
    'path, expected',
    [
        ('alice/hollywood/library/scenarios/alarm/nlg/alarm__common_ar.nlg', "alarm"),
        ('alice/hollywood/library/scenarios/smart_device/external_app/nlg/external_app_ru.nlg', "smart_device"),
        ('alice/hollywood/library/common_nlg/common_ar.nlg', "common_nlg"),
        ('weather/some.nlg', "weather"),
        ('', "."),
    ],
)
def test_get_library(path, expected):
    actual = get_library(path)
    assert actual == expected


@pytest.mark.parametrize('nlg', NLG_FILES)
def test_dump_keyset(nlg):
    out_file = 'result.json'
    nlg_path = os.path.join(TEMPLATES_DIR, nlg)
    dump_keyset('', nlg_path, out_file)

    return yatest.common.canonical_file(out_file, local=True)
