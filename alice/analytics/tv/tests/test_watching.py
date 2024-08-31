from helpers import BaseTestCase
import unittest

from tv.cubes.watching.main import make_job as watching_make_job


class WatchingTestCase(BaseTestCase):
    DATA_FILE = 'data/watching_data.json'
    JOB = [watching_make_job]


# How to test

# Setup virtualenv:
# virtualenv ~/env/analytics -p python
# source ~/env/analytics/bin/activate

# Install nile, qb2
# pip install -i https://pypi.yandex-team.ru/simple nile
# pip install -i https://pypi.yandex-team.ru/simple qb2

# Launch this script
# cd tests/tv
# python test_watching.py

if __name__ == '__main__':
    WatchingTestCase.init_tests()
    unittest.main()
