import asyncio
from unittest import TestCase

from alice.tools.quota_monitor.lib.resource_providers.yp_provider import YPProvider

TEST_DATA = {"sas": [{"gpu-default": {"memory": {"capacity": 9061844123648}, "cpu": {"capacity": 913750},
                                      "disk_per_storage_class": {"hdd": {"capacity": 0, "bandwidth": 0},
                                                                 "ssd": {"capacity": 36778663949108,
                                                                         "bandwidth": 4864344064}},
                                      "internet_address": {"capacity": 0},
                                      "gpu_per_model": {"gpu_tesla_v100": {"capacity": 133},
                                                        "gpu_geforce_1080ti": {"capacity": 4}},
                                      "network": {"bandwidth": 0}}},
                     {"gpu-default": {"memory": {"capacity": 8018703941632}, "cpu": {"capacity": 826629},
                                      "disk_per_storage_class": {
                                          "ssd": {"capacity": 11925287075840, "bandwidth": 2647654400}},
                                      "internet_address": {"capacity": 0},
                                      "gpu_per_model": {"gpu_geforce_1080ti": {"capacity": 4},
                                                        "gpu_tesla_v100": {"capacity": 130}},
                                      "network": {"bandwidth": 0}}}]}

VALID_RESULTS = [['sas-gpu-default-cpu-usage_axxx', 826.63],
                 ['sas-gpu-default-cpu-quota_axxx', 913.75],
                 ['sas-gpu-default-cpu-percent_axxx', 90.47],
                 ['sas-gpu-default-cpu-left_axxx', 87.12],
                 ['sas-gpu-default-memory-usage_axxx', 7468.0],
                 ['sas-gpu-default-memory-quota_axxx', 8439.5],
                 ['sas-gpu-default-memory-percent_axxx', 88.49],
                 ['sas-gpu-default-memory-left_axxx', 971.5],
                 ['sas-gpu-default-hdd-usage_axxx', 0.0],
                 ['sas-gpu-default-hdd-quota_axxx', 0.0],
                 ['sas-gpu-default-hdd-percent_axxx', 0],
                 ['sas-gpu-default-hdd-left_axxx', 0.0],
                 ['sas-gpu-default-ssd-usage_axxx', 11106.29],
                 ['sas-gpu-default-ssd-quota_axxx', 34252.80],
                 ['sas-gpu-default-ssd-percent_axxx', 32.42],
                 ['sas-gpu-default-ssd-left_axxx', 23146.51],
                 ['sas-gpu-default-hdd-io-usage_axxx', 0.0],
                 ['sas-gpu-default-hdd-io-quota_axxx', 0.0],
                 ['sas-gpu-default-hdd-io-percent_axxx', 0],
                 ['sas-gpu-default-hdd-io-left_axxx', 0.0],
                 ['sas-gpu-default-ssd-io-usage_axxx', 2525.0],
                 ['sas-gpu-default-ssd-io-quota_axxx', 4639.0],
                 ['sas-gpu-default-ssd-io-percent_axxx', 54.43],
                 ['sas-gpu-default-ssd-io-left_axxx', 2114.0],
                 ['sas-gpu-default-gpu_tesla_v100-usage_axxx', 130.0],
                 ['sas-gpu-default-gpu_tesla_v100-quota_axxx', 133.0],
                 ['sas-gpu-default-gpu_tesla_v100-percent_axxx', 97.74],
                 ['sas-gpu-default-gpu_tesla_v100-left_axxx', 3.0]]


async def parser_runner(provider):
    await provider.data_parser(TEST_DATA)
    return await provider.get_results()


class TestYPProvider(TestCase):
    def test_data_parser(self):
        p = YPProvider()
        s = asyncio.run(parser_runner(p))
        assert s == VALID_RESULTS
