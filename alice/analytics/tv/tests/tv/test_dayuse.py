from tv.tests.tv.tv_base import TvTestCase
import unittest

from tv.cubes.dayuse.main import make_job as dayuse_make_job


class DayuseTestCase(TvTestCase):
    DATA_FILE = "data/dayuse_data.json"
    JOB = [dayuse_make_job]


# How to test

# Setup virtualenv:
# virtualenv ~/env/analytics -p python
# source ~/env/analytics/bin/activate

# Install nile, qb2, fire
# pip install -i https://pypi.yandex-team.ru/simple nile
# pip install -i https://pypi.yandex-team.ru/simple qb2
# pip install fire

# Install analytics as a package
# cd ~/arc/arcadia/alice/analytics
# pip install -e .

# Launch this script

if __name__ == "__main__":
    DayuseTestCase.init_tests()
    unittest.main()
