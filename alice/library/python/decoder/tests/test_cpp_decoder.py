from common import TEST_APP_PATH, TEST_OGG_PATH, TEST_WEBM_PATH
import yatest
import os.path
import logging


def test_decode_ogg():
    result_path = "test123-pcm"
    command = [TEST_APP_PATH, TEST_OGG_PATH, result_path]

    logging.info("Run '{}'".format(" ".join(command)))
    execution = yatest.common.execute(command, wait=True, close_fds=True)
    execution.wait(timeout=5)

    assert execution.exit_code == 0
    assert os.path.isfile(result_path)
    assert os.path.getsize(result_path) == 82208


def test_decode_webm():
    result_path = "bear-vp9-pcm"
    command = [TEST_APP_PATH, TEST_WEBM_PATH, result_path]

    logging.info("Run '{}'".format(" ".join(command)))
    execution = yatest.common.execute(command, wait=True, close_fds=True)
    execution.wait(timeout=5)

    assert execution.exit_code == 0
    assert os.path.isfile(result_path)
    assert os.path.getsize(result_path) == 43422
