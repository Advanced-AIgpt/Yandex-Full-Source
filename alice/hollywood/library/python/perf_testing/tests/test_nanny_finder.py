import json
import pytest

import alice.hollywood.library.python.perf_testing.nanny_finder as nanny_finder

from yatest import common as yc

SAS_TANKS = [{"host": "u2r54qfwscmmlyou.sas.yp-c.yandex.net", "port": 80}, {"host": "fx76kbo6akcxuf5v.sas.yp-c.yandex.net", "port": 80}]
MAN_TANKS = [{"host": "txsochppbaigvgdf.man.yp-c.yandex.net", "port": 80}, {"host": "drybgegxm2qob7ir.man.yp-c.yandex.net", "port": 80}]
VLA_TANKS = [{"host": "hollywood-tank-1.vla.yp-c.yandex.net", "port": 80}, {"host": "hollywood-tank-2.vla.yp-c.yandex.net", "port": 80}]


@pytest.fixture(scope="function")
def src_filename():
    return yc.source_path("alice/hollywood/library/python/perf_testing/tests/data/nanny_response.json")


def test_find_containers(src_filename):
    with open(src_filename, "r") as src_f:
        nanny_response_json = src_f.read()
        nanny_response = json.loads(nanny_response_json)

    containers = nanny_finder.find_containers("hollywood-tank", nanny_response)
    assert len(containers) == 3
    assert containers["sas"] == SAS_TANKS
    assert containers["man"] == MAN_TANKS
    assert containers["vla"] == VLA_TANKS
