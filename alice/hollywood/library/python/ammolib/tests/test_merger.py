import os
import pytest

import alice.hollywood.library.python.ammolib.merger as merger

from yatest import common as yc

DATA_PATH = yc.source_path("alice/hollywood/library/python/ammolib/tests/data")


@pytest.fixture(scope="function")
def src_filenames():
    result = os.listdir(DATA_PATH)
    result.sort()
    return [os.path.join(DATA_PATH, filename) for filename in result]


def test_merge(src_filenames, tmp_path):
    dst_filename = os.path.join(str(tmp_path), "merge_result.txt")
    merger.merge_text_files_into_one(src_filenames, dst_filename)
    with open(dst_filename, "r") as dst_f:
        merge_result = dst_f.read()
    assert merge_result == """bar_data_1
bar_data_2
baz_data_1
baz_data_2
baz_data_3
foo_data_1
""", "Note that all empty lines are not present in merge_result..."


def test_split(tmp_path):
    src_filename = os.path.join(DATA_PATH, "baz.txt")
    dst_dirname = str(tmp_path)
    result_filenames = merger.split_text_file_into_multiple_by_lines(src_filename, dst_dirname)
    result_filenames = sorted(result_filenames)

    actual = sorted(os.listdir(dst_dirname))
    assert actual == ["baz.txt_0", "baz.txt_1", "baz.txt_3"]
    for index, filename in enumerate(result_filenames):
        assert filename == os.path.join(dst_dirname, actual[index])

    expected_content = [
        "baz_data_1\n",
        "baz_data_2\n",
        "baz_data_3\n",
    ]
    for index, filename in enumerate(result_filenames):
        with open(filename, "r") as f:
            assert f.read() == expected_content[index]
