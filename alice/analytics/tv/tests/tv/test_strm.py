from helpers import BaseTestCase
import unittest

from tv.strm.main import make_job as strm_make_job


class STRMTestCase(BaseTestCase):
    DATA_FILE = "data/strm_data.json"
    JOB = [strm_make_job]


# How to test

# Setup virtualenv:
# virtualenv ~/env/analytics -p python
# source ~/env/analytics/bin/activate

# Install nile, qb2
# pip install -i https://pypi.yandex-team.ru/simple nile
# pip install -i https://pypi.yandex-team.ru/simple qb2

# Launch this script
# cd tests/tv
# python test_strm.py

if __name__ == "__main__":
    STRMTestCase.init_tests()
    unittest.main()
