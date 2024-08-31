from helpers import BaseTestCase
import unittest

from tv.cubes.recommendations.main import make_job as recommendations_make_job


class RecommendationsTestCase(BaseTestCase):
    DATA_FILE = 'data/recommendations_data.json'
    JOB = [recommendations_make_job]


# How to test

# Setup virtualenv:
# virtualenv ~/env/analytics -p python
# source ~/env/analytics/bin/activate

# Install nile, qb2
# pip install -i https://pypi.yandex-team.ru/simple nile
# pip install -i https://pypi.yandex-team.ru/simple qb2

# Launch this script
# cd tests/tv
# python test_recommendations.py

if __name__ == '__main__':
    RecommendationsTestCase.init_tests()
    unittest.main()
