from alice.cachalot.tests.integration_tests.lib import CachalotFixture, YdbFixture

import alice.cachalot.tests.test_cases as test_cases
import alice.cachalot.tests.test_cases.activation as activation
import alice.cachalot.tests.test_cases.cache as cache
import alice.cachalot.tests.test_cases.yabio_context as yabio_context

import pytest


@pytest.fixture(scope="module")
def local_cachalot():
    with CachalotFixture() as x:
        yield x


@pytest.fixture(scope="module")
def ydb_session():
    with YdbFixture() as x:
        yield x


def test_stats_request(local_cachalot: CachalotFixture):
    test_cases.test_stats_request(local_cachalot)


def test_cache_set_and_get(local_cachalot: CachalotFixture):
    test_cases.test_cache_set_and_get(local_cachalot)


def test_cache_set_and_404(local_cachalot: CachalotFixture):
    test_cases.test_cache_set_and_404(local_cachalot)


def test_cache_set_and_get_redis_only(local_cachalot: CachalotFixture):
    test_cases.test_cache_set_and_get_redis_only(local_cachalot)


def test_cache_set_and_get_ydb_only(local_cachalot: CachalotFixture):
    test_cases.test_cache_set_and_get_ydb_only(local_cachalot)


def test_cache_set_and_get_tts(local_cachalot: CachalotFixture):
    test_cases.test_cache_set_and_get_tts(local_cachalot)


def test_cache_set_and_get_datasync(local_cachalot: CachalotFixture):
    test_cases.test_cache_set_and_get_datasync(local_cachalot)


def test_cache_set_and_get_without_storage_tag(local_cachalot: CachalotFixture):
    test_cases.test_cache_set_and_get_without_storage_tag(local_cachalot)


def test_cache_set_and_get_different_storage_tags(local_cachalot: CachalotFixture):
    test_cases.test_cache_set_and_get_different_storage_tags(local_cachalot)


def test_cache_set_all_and_get_ydb(local_cachalot: CachalotFixture):
    test_cases.test_cache_set_all_and_get_ydb(local_cachalot)


def test_cache_set_all_and_get_redis(local_cachalot: CachalotFixture):
    test_cases.test_cache_set_all_and_get_redis(local_cachalot)


def test_cache_set_ydb_and_get_all(local_cachalot: CachalotFixture):
    test_cases.test_cache_set_ydb_and_get_all(local_cachalot)


def test_cache_set_redis_and_get_all(local_cachalot: CachalotFixture):
    test_cases.test_cache_set_redis_and_get_all(local_cachalot)


def test_cache_invalid_double_force(local_cachalot: CachalotFixture):
    test_cases.test_cache_invalid_double_force(local_cachalot)


def test_cache_set_redis_and_get_ydb(local_cachalot: CachalotFixture):
    test_cases.test_cache_set_redis_and_get_ydb(local_cachalot)


def test_cache_set_ydb_and_get_redis(local_cachalot: CachalotFixture):
    test_cases.test_cache_set_ydb_and_get_redis(local_cachalot)


def test_cache_set_and_get_utf8(local_cachalot: CachalotFixture):
    test_cases.test_cache_set_and_get_utf8(local_cachalot)


def test_cache_set_too_large_data(local_cachalot: CachalotFixture):
    test_cases.test_cache_set_too_large_data(local_cachalot)


def test_cache_charge_redis_from_ydb(local_cachalot: CachalotFixture):
    test_cases.test_cache_charge_redis_from_ydb(local_cachalot)


def test_cache_create_many(local_cachalot: CachalotFixture):
    cache.test_cache_create_many(local_cachalot)


def test_cache_create_and_update_many(local_cachalot: CachalotFixture):
    cache.test_cache_create_and_update_many(local_cachalot)


def test_cache_create_many_and_get_not_existing(local_cachalot: CachalotFixture):
    cache.test_cache_create_many_and_get_not_existing(local_cachalot)


def test_cache_set_get_with_many_option(local_cachalot: CachalotFixture):
    cache.test_cache_set_get_with_many_option(local_cachalot)


def test_cache_create_many_in_different_storages(local_cachalot: CachalotFixture):
    cache.test_cache_create_many_in_different_storages(local_cachalot)


def test_cache_create_and_delete_many(local_cachalot: CachalotFixture):
    cache.test_cache_create_and_delete_many(local_cachalot)


def test_cache_create_and_delete_some(local_cachalot: CachalotFixture):
    cache.test_cache_create_and_delete_some(local_cachalot)


def test_cache_delete_not_existing(local_cachalot: CachalotFixture):
    cache.test_cache_delete_not_existing(local_cachalot)


@pytest.mark.parametrize("use_http", [True, False])
def test_mm_session(local_cachalot: CachalotFixture, use_http: bool):
    test_cases.test_mm_session(local_cachalot, use_http)


@pytest.mark.parametrize("use_http", [True, False])
def test_mm_session_set_without_request_id(local_cachalot: CachalotFixture, use_http: bool):
    test_cases.test_mm_session_set_without_request_id(local_cachalot, use_http)


def test_activation_single_device(local_cachalot: CachalotFixture):
    activation.test_single_device(local_cachalot)


def test_activation_two_users(local_cachalot: CachalotFixture):
    activation.test_two_users(local_cachalot)


def test_activation_two_devices(local_cachalot: CachalotFixture):
    activation.test_two_devices(local_cachalot)


def test_activation_cleanup(local_cachalot: CachalotFixture):
    activation.test_cleanup(local_cachalot)


def test_activation_nocleanup(local_cachalot: CachalotFixture):
    activation.test_nocleanup(local_cachalot)


def test_requests_within_freshness_threshold(local_cachalot: CachalotFixture):
    activation.test_requests_within_freshness_threshold(local_cachalot)


def test_activation_three_devices(local_cachalot: CachalotFixture):
    activation.test_three_devices(local_cachalot)


def test_activation_three_devices_extreme(local_cachalot: CachalotFixture):
    activation.test_three_devices_extreme(local_cachalot)


def test_activation_two_devices_single_step(local_cachalot: CachalotFixture):
    activation.test_two_devices_single_step(local_cachalot)


def test_activation_three_devices_with_zero_rms(local_cachalot: CachalotFixture):
    activation.test_three_devices_with_zero_rms(local_cachalot)


def test_activation_allow_unvalidated(local_cachalot: CachalotFixture):
    activation.test_allow_unvalidated(local_cachalot)


def test_activation_disallow_unvalidated_1(local_cachalot: CachalotFixture):
    activation.test_disallow_unvalidated_1(local_cachalot)


def test_activation_disallow_unvalidated_2(local_cachalot: CachalotFixture):
    activation.test_disallow_unvalidated_2(local_cachalot)


def test_activation_voice_input_api(local_cachalot: CachalotFixture):
    activation.test_voice_input_api(local_cachalot)


def test_activation_rms_multiplication_for_first_station_voice_input(local_cachalot: CachalotFixture, ydb_session):
    activation.test_rms_multiplication_for_first_station_voice_input(local_cachalot, ydb_session)


def test_activation_rms_multiplication_for_yandexmicro_voice_input(local_cachalot: CachalotFixture, ydb_session):
    activation.test_rms_multiplication_for_yandexmicro_voice_input(local_cachalot, ydb_session)


def test_takeout_1(local_cachalot: CachalotFixture):
    test_cases.test_takeout_1(local_cachalot)


def test_takeout_2(local_cachalot: CachalotFixture):
    test_cases.test_takeout_2(local_cachalot)


def test_takeout_3(local_cachalot: CachalotFixture):
    test_cases.test_takeout_3(local_cachalot)


def test_cache_set_and_get_imdb_only(local_cachalot: CachalotFixture):
    test_cases.test_cache_set_and_get_imdb_only(local_cachalot)


def test_cache_set_imdb_and_get_all(local_cachalot: CachalotFixture):
    test_cases.test_cache_set_imdb_and_get_all(local_cachalot)


def test_cache_set_imdb_and_get_ydb(local_cachalot: CachalotFixture):
    test_cases.test_cache_set_imdb_and_get_ydb(local_cachalot)


def test_cache_set_ydb_and_get_imdb(local_cachalot: CachalotFixture):
    test_cases.test_cache_set_ydb_and_get_imdb(local_cachalot)


def test_cache_set_all_and_get_imdb(local_cachalot: CachalotFixture):
    test_cases.test_cache_set_all_and_get_imdb(local_cachalot)


def test_yabio_context_simple_grpc(local_cachalot: CachalotFixture):
    yabio_context.test_simple(local_cachalot, use_grpc=True)


def test_yabio_context_simple_http(local_cachalot: CachalotFixture):
    yabio_context.test_simple(local_cachalot, use_grpc=False)


def test_cache_delete_after_timeout(local_cachalot: CachalotFixture):
    test_cases.test_cache_delete_after_timeout(local_cachalot)


def test_cache_delete_after_timeout_imdb_only(local_cachalot: CachalotFixture):
    test_cases.test_cache_delete_after_timeout_imdb_only(local_cachalot)


# def test_cache_delete_after_timeout_ydb_only(local_cachalot: CachalotFixture):
#    test_cases.test_cache_delete_after_timeout_ydb_only(local_cachalot)
