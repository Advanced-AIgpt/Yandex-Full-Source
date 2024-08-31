from helpers import BaseTestCase
import unittest

from tv.cubes.activations.main import make_job as activations_make_job


class ActivationsTestCase(BaseTestCase):
    DATA_FILE = 'data/activations_data.json'
    JOB = [activations_make_job]


# How to test

# Setup virtualenv:
# virtualenv ~/env/analytics -p python
# source ~/env/analytics/bin/activate

# Install nile, qb2
# pip install -i https://pypi.yandex-team.ru/simple nile
# pip install -i https://pypi.yandex-team.ru/simple qb2

# Launch this script
# cd tests/tv
# python test_activations.py

if __name__ == '__main__':
    ActivationsTestCase.init_tests()
    unittest.main()
