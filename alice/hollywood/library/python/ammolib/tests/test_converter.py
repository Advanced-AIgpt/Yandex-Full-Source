import os
import pytest

import alice.hollywood.library.python.ammolib.converter as converter

from yatest import common as yc


@pytest.fixture(scope="function")
def src_filename():
    data_path = yc.source_path("alice/hollywood/library/python/ammolib/tests/data")
    return os.path.join(data_path, "baz.txt")


def test_convert(src_filename, tmp_path):
    dst_path = str(tmp_path)
    dst_filenames = converter.convert([src_filename], dst_path)
    assert len(dst_filenames) == 1

    dst_filename = dst_filenames[0]
    with open(dst_filename, "r") as dst_f:
        convert_result = dst_f.read()
    assert convert_result == """{"Data": "baz_data_1"}
{"Data": "baz_data_2"}
{"Data": "baz_data_3"}
""", "Note that all empty lines are not present in convert_result..."
