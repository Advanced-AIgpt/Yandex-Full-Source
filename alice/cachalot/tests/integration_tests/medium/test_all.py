from alice.cachalot.tests.integration_tests.lib import CachalotFixture
import alice.cachalot.tests.test_cases.activation as activation

import pytest
import random


@pytest.fixture(scope="module")
def local_cachalot():
    with CachalotFixture() as x:
        yield x


def _get_orders():
    return set([random.randint(0, 1679) for _ in range(64)])


def test_activation_three_devices_all_distinct(local_cachalot: CachalotFixture):
    activation.test_three_devices_all_distinct(local_cachalot, orders_to_check=_get_orders())


def test_activation_three_devices_all_duplicated(local_cachalot: CachalotFixture):
    activation.test_three_devices_all_duplicated(local_cachalot, orders_to_check=_get_orders())


def test_activation_three_devices_all_equivalent(local_cachalot: CachalotFixture):
    activation.test_three_devices_all_equivalent(local_cachalot, orders_to_check=_get_orders())
