import pytest
import uuid
import alice.uniproxy.library.testing

from alice.uniproxy.library.backends_common.storage import MdsStorage


@alice.uniproxy.library.testing.ioloop_run
def test_storage():
    content = bytes([0x01, 0x02, 0x03, 0x04, 0x05])
    filename = str(uuid.uuid1()) + ".test"
    # will throw if error
    mds_key = yield MdsStorage().save(filename, content)


