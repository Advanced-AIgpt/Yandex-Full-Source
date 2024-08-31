from .env import (
    CachalotService,
)

import alice.cachalot.tests.test_cases as test_cases
import alice.cachalot.tests.test_cases.activation as activation
import alice.cachalot.tests.test_cases.cache as cache
import alice.cachalot.tests.ydb_tables as ydb_tables
import alice.cachalot.client as cachalot_client


from functools import wraps
import argparse
import multiprocessing
import os
import random


class HeavyLoadCachalotClient(cachalot_client.SyncCachalotClient):
    BYTES = list(range(256))

    @classmethod
    def _get_pivot_by_value_type(cls, value):
        if isinstance(value, str):
            return "_heavy_prefix_"
        elif isinstance(value, bytes):
            return b"_heavy_prefix_\xde\xad\xbe\xef_"
        else:
            raise Exception("Invalid type of value")

    @classmethod
    def _lighten(cls, value):
        pivot = cls._get_pivot_by_value_type(value)
        pos = value.find(pivot)
        if pos >= 0:
            return value[:pos]
        return value

    @classmethod
    def _generate_random_bytes(cls, size):
        return b"".join(bytes([random.choice(cls.BYTES)]) for _ in range(size))

    @classmethod
    def _make_heavy_key(cls, value):
        pivot = cls._get_pivot_by_value_type(value)
        return value + pivot + (value * (80 // len(value)))

    @classmethod
    def _make_heavy_value(cls, value):
        pivot = cls._get_pivot_by_value_type(value)
        return value + pivot + cls._generate_random_bytes(10000)

    def __init__(self, client):
        self.client = client

    def request_stats(self):
        return self.client.request_stats()

    def cache_get(self, key, *args, **kwargs):
        heavy_key = self._make_heavy_key(key)
        response = self.client.cache_get(heavy_key, *args, **kwargs)
        if response and "GetResp" in response:
            if "Key" in response["GetResp"]:
                response["GetResp"]["Key"] = self._lighten(response["GetResp"]["Key"])
            if "Data" in response["GetResp"]:
                response["GetResp"]["Data"] = self._lighten(response["GetResp"]["Data"])

        return response

    def cache_set(self, key, value, *args, **kwargs):
        heavy_key = self._make_heavy_key(key)
        heavy_value = self._make_heavy_value(value)
        response = self.client.cache_set(heavy_key, heavy_value, *args, **kwargs)
        if response and "SetResp" in response:
            if "Key" in response["SetResp"]:
                response["SetResp"]["Key"] = self._lighten(response["SetResp"]["Key"])

        return response


def print_exceptions(func):
    @wraps(func)
    def wrapper(*args, **kwargs):
        try:
            return func(*args, **kwargs)
        except Exception as exc:
            print("Error:", type(exc), exc)
            raise

    return wrapper


@print_exceptions
def run_cache_tests(http_client_for_cache):
    test_cases.test_cache_set_and_get(http_client_for_cache)
    test_cases.test_cache_set_and_get_redis_only(http_client_for_cache)
    test_cases.test_cache_set_and_get_ydb_only(http_client_for_cache)
    test_cases.test_cache_set_all_and_get_ydb(http_client_for_cache)
    test_cases.test_cache_set_all_and_get_redis(http_client_for_cache)
    test_cases.test_cache_set_ydb_and_get_all(http_client_for_cache)
    test_cases.test_cache_set_redis_and_get_all(http_client_for_cache)
    test_cases.test_cache_invalid_double_force(http_client_for_cache)
    test_cases.test_cache_set_redis_and_get_ydb(http_client_for_cache)
    test_cases.test_cache_set_ydb_and_get_redis(http_client_for_cache)
    test_cases.test_cache_set_and_get_utf8(http_client_for_cache)
    test_cases.test_cache_set_too_large_data(http_client_for_cache)
    test_cases.test_cache_set_and_get_tts(http_client_for_cache)
    test_cases.test_cache_set_and_get_datasync(http_client_for_cache)
    test_cases.test_cache_set_and_get_without_storage_tag(http_client_for_cache)
    test_cases.test_cache_set_and_get_different_storage_tags(http_client_for_cache)
    cache.test_cache_create_many(http_client_for_cache)
    cache.test_cache_create_and_update_many(http_client_for_cache)
    cache.test_cache_create_many_and_get_not_existing(http_client_for_cache)
    cache.test_cache_set_get_with_many_option(http_client_for_cache)
    cache.test_cache_create_many_in_different_storages(http_client_for_cache)
    cache.test_cache_create_and_delete_many(http_client_for_cache)
    cache.test_cache_create_and_delete_some(http_client_for_cache)
    cache.test_cache_delete_not_existing(http_client_for_cache)


@print_exceptions
def run_mm_tests(http_client):
    # test_cases.test_mm_session(http_client, False)
    test_cases.test_mm_session(http_client, True)
    test_cases.test_mm_session_set_without_request_id(http_client, True)


@print_exceptions
def run_small_tests(args, http_client, grpc_client, http_client_for_cache):
    test_cases.test_stats_request(http_client)

    if args.run_cache:
        run_cache_tests(http_client_for_cache)

    if args.run_mm:
        run_mm_tests(http_client)

    if args.run_activation:
        run_small_activation_tests(http_client)


@print_exceptions
def run_small_activation_tests(http_client):
    activation.test_single_device(http_client)
    activation.test_two_users(http_client)
    activation.test_two_devices(http_client)
    activation.test_cleanup(http_client)
    activation.test_nocleanup(http_client)
    activation.test_requests_within_freshness_threshold(http_client)
    activation.test_three_devices(http_client)
    activation.test_two_devices_single_step(http_client)
    activation.test_three_devices_with_zero_rms(http_client)
    activation.test_allow_unvalidated(http_client)
    activation.test_three_devices_extreme(http_client)
    activation.test_disallow_unvalidated_1(http_client)
    activation.test_disallow_unvalidated_2(http_client)


@print_exceptions
def _run_medium_activation_tests_part(http_client, orders_to_check=None):
    activation.test_three_devices_all_distinct(http_client, orders_to_check=orders_to_check)
    activation.test_three_devices_all_duplicated(http_client, orders_to_check=orders_to_check)
    activation.test_three_devices_all_equivalent(http_client, orders_to_check=orders_to_check)


def run_medium_activation_tests_fast(http_client):
    orders_to_check = set([random.randint(0, 1679) for _ in range(16)])
    _run_medium_activation_tests_part(http_client, orders_to_check)


def _run_medium_activation_tests_worker(http_client, args, worker_id):
    step = 1680 // args.workers
    orders_to_check = set(range(step * worker_id, step * (worker_id + 1)))
    _run_medium_activation_tests_part(http_client, orders_to_check)


@print_exceptions
def perf_worker(args, worker_id):
    http_client = cachalot_client.SyncCachalotClient(host=args.cachalot_host, http_port=args.cachalot_http_port)
    grpc_client = cachalot_client.SyncCachalotClient(host=args.cachalot_host, grpc_port=args.cachalot_grpc_port)
    http_client_for_cache = http_client

    if args.heavy_load:
        http_client_for_cache = HeavyLoadCachalotClient(http_client)

    for _ in range(args.iterations):
        run_small_tests(args, http_client, grpc_client, http_client_for_cache)


def _run_remote_tests(worker_func, args):
    if args.workers == 1:
        worker_func(args, 0)
        return

    futures = list()
    with multiprocessing.Pool(args.workers) as pool:
        for worker_id in range(args.workers):
            futures.append(pool.apply_async(worker_func, (args, worker_id)))

        pool.close()

        for future in futures:
            future.wait()

        pool.join()


def remote_tests(args):
    return _run_remote_tests(perf_worker, args)


def activation_worker(args, worker_id):
    http_client = cachalot_client.SyncCachalotClient(host=args.cachalot_host, http_port=args.cachalot_http_port)

    if worker_id == 0:
        _run_medium_activation_tests_worker(http_client, args, worker_id)
    else:
        for _ in range(args.iterations):
            run_small_activation_tests(http_client)

    print(f"Worker {worker_id} finished")


def remote_activation_tests(args):
    return _run_remote_tests(activation_worker, args)


def local_tests(args):
    with CachalotService(
        args.cachalot_binary_dir,
        args.working_dir,
        args.cachalot_http_port,
        args.cachalot_grpc_port,
        args.ydb_database,
        env={
            "YDB_TOKEN": os.getenv("YDB_TOKEN"),
        }
    ) as cachalot:
        http_client = cachalot.get_sync_client()
        grpc_client = cachalot.get_sync_client(use_grpc=True)
        http_client_for_cache = http_client

        if args.heavy_load:
            http_client_for_cache = HeavyLoadCachalotClient(http_client)

        run_small_tests(args, http_client, grpc_client, http_client_for_cache)

        if args.run_activation:
            run_medium_activation_tests_fast(http_client)


def add_common_args(arg_parser):
    arg_parser.add_argument("--cachalot-host", default="cachalot.alice.yandex.net")
    arg_parser.add_argument("--cachalot-http-port", type=int, default=8080)
    arg_parser.add_argument("--cachalot-grpc-port", type=int, default=8081)
    arg_parser.add_argument("--heavy-load", default=False, action="store_true")
    arg_parser.add_argument("--run-activation", default=False, action="store_true")
    arg_parser.add_argument("--run-mm", default=False, action="store_true")
    arg_parser.add_argument("--run-cache", default=False, action="store_true")


def main():
    parser = argparse.ArgumentParser()
    sub_parsers = parser.add_subparsers()

    local_parser = sub_parsers.add_parser("local")
    local_parser.set_defaults(func=local_tests)
    local_parser.add_argument("--cachalot-binary-dir", default="./")
    local_parser.add_argument("--working-dir", default="./")
    local_parser.add_argument("--ydb-database", default="/ru/home/paxakor/mydb")
    add_common_args(local_parser)

    remote_parser = sub_parsers.add_parser("remote")
    remote_parser.set_defaults(func=remote_tests)
    remote_parser.add_argument("--workers", type=int, default=1)
    remote_parser.add_argument("--iterations", type=int, default=1)
    add_common_args(remote_parser)

    remote_parser = sub_parsers.add_parser("remote-activation")
    remote_parser.set_defaults(func=remote_activation_tests)
    remote_parser.add_argument("--cachalot-host", default="cachalot.alice.yandex.net")
    remote_parser.add_argument("--workers", type=int, default=10)
    remote_parser.add_argument("--iterations", type=int, default=100)

    create_tables_parser = sub_parsers.add_parser("create_tables")
    create_tables_parser.set_defaults(func=ydb_tables.create_tables)
    create_tables_parser.add_argument("--ydb-database", default="/ru/home/paxakor/mydb")
    create_tables_parser.add_argument("--ydb-endpoint", default="ydb-ru.yandex.net:2135")
    create_tables_parser.add_argument("--force", default=False, action="store_true")

    args = parser.parse_args()
    args.func(args)

    print("ALL TESTS ARE OK")


if __name__ == "__main__":
    main()
