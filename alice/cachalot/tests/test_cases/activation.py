from alice.cachalot.tests.test_cases.util import (
    assert_eq,
    assert_leq,
    assert_true,
    class_stable_permutations,
    get_client,
    Scenario,
)

from alice.cachalot.client.activation import SyncActivationClient
from alice.cachalot.client import SyncMixin
import alice.cachalot.api.protos.cachalot_pb2 as protos
import alice.cuttlefish.library.protos.asr_pb2 as asr_protos

from google.protobuf.timestamp_pb2 import Timestamp as ProtoTimestamp

import math
import time


class BlindSyncActivationClient(SyncActivationClient):
    CLIENTS = dict()

    @classmethod
    def build_test_user_id(cls, user_id, test_id):
        if test_id is not None:
            return user_id + f"_{test_id}".encode("ascii")
        return user_id

    @classmethod
    def get(cls, cachalot_client, user_id, device_id, avg_rms, test_id=None, **kwargs):
        client = BlindSyncActivationClient.CLIENTS.get((cls.build_test_user_id(user_id, test_id), device_id))
        if client is not None:
            return client
        else:
            return BlindSyncActivationClient(cachalot_client, user_id, device_id, avg_rms, test_id=test_id, **kwargs)

    def __init__(self, *args, test_id=None, **kwargs):
        super().__init__(*args, **kwargs)
        self.user_id = self.build_test_user_id(self.user_id, test_id)

        BlindSyncActivationClient.CLIENTS[(self.user_id, self.device_id)] = self


class LeaderCountGuard:
    def __init__(self, test_name, expected_leader_device=None):
        self.test_name = test_name
        self.expected_leader_device = expected_leader_device
        self.count = 0

    def __call__(self, final_resp):
        if final_resp.activation_allowed:
            self.count += 1
        else:
            if self.expected_leader_device is not None and final_resp.leader_info is not None:
                assert_eq(
                    self.expected_leader_device, final_resp.leader_info.device_id,
                    f"Wrong leader in '{self.test_name}'"
                )

        assert_leq(self.count, 1, f"Too much leaders in '{self.test_name}'")

        return final_resp.activation_allowed

    def finish(self):
        assert_leq(1, self.count, f"Too few leaders in '{self.test_name}'")


def _validate_response_impl(permission_val, error_val, allow=True, error=None, test_name=None):
    if allow is not None:
        assert_eq(permission_val, allow, f"Unexpected permission value in '{test_name}'")

    if error is None:
        assert_true(not error_val, f"Expected empty ErrorMessage in '{test_name}', but got '{error_val}'")
    else:
        assert_eq(error_val, error, f"Wrong ErrorMessage in '{test_name}'")


def validate_response(rsp, allow=True, error=None, test_name=None):
    _validate_response_impl(
        getattr(rsp, 'continuation_allowed', None) or getattr(rsp, 'activation_allowed'),
        rsp.error_msg,
        allow,
        error,
        test_name
    )


def _validate_rms_in_base(ydb_session, table_name, device_id, expected_rms):
    select_result = ydb_session.transaction().execute(
        f'SELECT AvgRMS FROM `{table_name}` WHERE DeviceId == "{device_id.decode("ascii")}"',
        commit_tx=True
    )
    assert math.isclose(select_result[0].rows[0].AvgRMS, expected_rms, abs_tol=1.0)


def test_single_device(cachalot):
    """
        Since there is one user and one device, all request must respond allow=True.
    """
    client = BlindSyncActivationClient(get_client(cachalot), b"paxakor", b"station_mini", 28.0)
    Scenario(
        [
            (
                lambda: client._activation_announcement(spotter_validated=False),
                lambda rsp: validate_response(rsp, test_name="test_single_device")
            ),
            (
                lambda: client._activation_announcement(spotter_validated=True),
                lambda rsp: validate_response(rsp, test_name="test_single_device")
            ),
            (
                lambda: client._activation_final(),
                lambda rsp: validate_response(rsp, test_name="test_single_device")
            ),
        ]
    ).run()


def test_two_users(cachalot):
    """
        Since there are two independent devices, all request must respond allow=True.
    """
    cachalot_client = get_client(cachalot, use_grpc=False)

    def get_client1(test_id):
        return BlindSyncActivationClient.get(cachalot_client, b"carzil", b"pc", 350, test_id=test_id)

    def get_client2(test_id):
        return BlindSyncActivationClient.get(cachalot_client, b"danlark", b"ps4", 150, test_id=test_id)

    def get_single_device_events(client_getter, pos):
        return [
            (
                lambda test_id: client_getter(test_id)._activation_announcement(spotter_validated=False),
                lambda rsp, test_id: validate_response(rsp, test_name=f"test_two_users_{test_id}_{pos*3}")
            ),
            (
                lambda test_id: client_getter(test_id)._activation_announcement(spotter_validated=True),
                lambda rsp, test_id: validate_response(rsp, test_name=f"test_two_users_{test_id}_{pos*3+1}")
            ),
            (
                lambda test_id: client_getter(test_id)._activation_final(),
                lambda rsp, test_id: validate_response(rsp, test_name=f"test_two_users_{test_id}_{pos*3+2}")
            ),
        ]

    events = [
        get_single_device_events(get_client1, 0),
        get_single_device_events(get_client2, 1)
    ]

    for test_id, event_order in enumerate(class_stable_permutations(events)):
        Scenario(event_order).run((test_id,), (test_id,))


def _test_two_devices_impl(cachalot, test_name, user_id, **client_kwargs):
    """
        iphone is faster, but it must find pixel in announcement table and cancel activation of itself.
    """
    leader_validator = LeaderCountGuard(test_name, b"pixel")
    client1 = BlindSyncActivationClient(get_client(cachalot), user_id, b"pixel", 123, **client_kwargs)
    client2 = BlindSyncActivationClient(get_client(cachalot), user_id, b"iphone", 35, **client_kwargs)
    assert_true(client2.make_announcement(False).continuation_allowed, f"Announcement (1) failed in {test_name}")
    assert_true(client2.make_announcement(True).continuation_allowed, f"Announcement (2) failed in {test_name}")
    assert_true(client1.make_announcement(False).continuation_allowed, f"Announcement (3) failed in {test_name}")
    assert_true(client1.make_announcement(True).continuation_allowed, f"Announcement (4) failed in {test_name}")
    assert_true(not leader_validator(client2.try_acquire_leadership()), f"Wrong leader in {test_name}")
    assert_true(leader_validator(client1.try_acquire_leadership()), f"No leader in {test_name}")
    leader_validator.finish()


def test_two_devices(cachalot):
    _test_two_devices_impl(cachalot, "test_two_devices", b"ilyaluk")


def test_cleanup(cachalot):
    _test_two_devices_impl(cachalot, "test_cleanup", b"sml12")
    time.sleep(5)
    # New session after >4000ms
    _test_two_devices_impl(cachalot, "test_cleanup", b"sml12")


def test_requests_within_freshness_threshold(cachalot):
    test_name = "test_requests_within_freshness_threshold"
    user_id = b"burney_stinson"
    device_id = b"pixel"

    test_cases = [
        (
            123, 87
        ),
        (
            87, 123
        ),
        (
            100, 100
        )
    ]

    for test_case_id, test_case in enumerate(test_cases):
        test_case_name = f"{test_name}_test_case_{test_case_id}"
        rms_1, rms_2 = test_case

        client1 = BlindSyncActivationClient(get_client(cachalot), user_id, device_id, rms_1)
        client1.make_announcement(False)
        client1.make_announcement(True)
        client1.try_acquire_leadership()

        client2 = BlindSyncActivationClient(get_client(cachalot), user_id, device_id, rms_2)
        assert_true(
            client2.make_announcement(False).continuation_allowed,
            f"Announcement (1) failed in {test_case_name}: {(rms_1, rms_2)})"
        )
        assert_true(
            client2.make_announcement(True).continuation_allowed,
            f"Announcement (2) failed in {test_case_name}: {(rms_1, rms_2)})"
        )
        assert_true(
            client2.try_acquire_leadership().activation_allowed,
            f"Device deactivated itself in {test_case_name}"
        )


def test_nocleanup(cachalot):
    client_kwargs = dict(freshness_delta_milliseconds=40000)
    user_id = b"mari"
    test_name = "test_nocleanup"

    _test_two_devices_impl(cachalot, test_name, user_id, **client_kwargs)
    time.sleep(5)

    # New session after 4000ms<5s<40000ms
    client3 = BlindSyncActivationClient(get_client(cachalot), user_id, b"imac", 345, **client_kwargs)
    client4 = BlindSyncActivationClient(get_client(cachalot), user_id, b"station_max", 567, **client_kwargs)

    rsp1 = client3.make_announcement(False)
    assert_true(rsp1.leader_found, f"Backend is broken in {test_name}")
    assert_true(not rsp1.continuation_allowed, f"Backend is broken in {test_name}")
    rsp2 = client4.make_announcement(True)
    assert_true(rsp2.leader_found, f"Backend is broken in {test_name}")
    assert_true(not rsp2.continuation_allowed, f"Backend is broken in {test_name}")


def test_three_devices(cachalot):
    leader_validator = LeaderCountGuard("test_three_devices", b"sapog")
    client1 = BlindSyncActivationClient(get_client(cachalot, use_grpc=False), b"smalukav", b"sapog", 60)
    client2 = BlindSyncActivationClient(get_client(cachalot, use_grpc=False), b"smalukav", b"botinok", 23)
    client3 = BlindSyncActivationClient(get_client(cachalot, use_grpc=False), b"smalukav", b"tapka", 48)

    assert_true(client1.make_announcement(False).continuation_allowed, "Announcement (1) failed in test_three_devices")
    assert_true(client2.make_announcement(False).continuation_allowed, "Announcement (2) failed in test_three_devices")
    assert_true(client3.make_announcement(False).continuation_allowed, "Announcement (3) failed in test_three_devices")

    rsp2 = client2.make_announcement(True)
    assert_true(rsp2.continuation_allowed, "Announcement (4) failed in test_three_devices")
    assert_eq(rsp2.best_competitor.spotter_features.avg_rms, 60.0, "Backend is broken")

    rsp3 = client3.make_announcement(True)
    assert_true(rsp3.continuation_allowed, "Announcement (5) failed in test_three_devices")
    assert_eq(rsp3.best_competitor.device_id, b"sapog", "Backend is broken [2]")

    rsp4 = client2.try_acquire_leadership()
    assert_true(not leader_validator(rsp4), "Wrong leader in test_three_devices")
    assert_eq(rsp4.leader_info.spotter_features.avg_rms, 60.0, "Backend is broken [3]")

    assert_true(not leader_validator(client3.try_acquire_leadership()), "Wrong leader in test_three_devices")

    rsp5 = client1.try_acquire_leadership()
    assert_true(leader_validator(rsp5), "No leader in test_three_devices")

    leader_validator.finish()


def _three_devices_all_test_template(
    cachalot,
    user_id,
    avg_rms_list,
    pre_spotter_oks_const,
    post_spotter_oks_const,
    orders_to_check,
):
    """
        Here we validate total number of leaders and after-announcement oks.
    """

    assert_eq(avg_rms_list, sorted(avg_rms_list, reverse=True), "avg_rms_list should be sorted in desc order")
    assert_eq(len(avg_rms_list), 3, "this code works only for three devices")
    assert_true(all([(x == y or abs(x - y) > 3) for x in avg_rms_list for y in avg_rms_list]), "bad avg_rms_list")

    # First of all we validate constants.
    # We will simulate all orders and count oks (it is when allow=True).
    event_ids = [
        [(0, 0), (0, 1), (0, 2)],
        [(1, 0), (1, 1), (1, 2)],
        [(2, 0), (2, 1), (2, 2)],
    ]

    simulated_pre_spotter_oks = 0
    simulated_post_spotter_oks = 0
    total_orders = 0

    if orders_to_check is None:
        total_orders_const = 1680
        check_counts = assert_eq
    else:
        total_orders_const = len(orders_to_check)
        check_counts = assert_leq

    leaders = dict()  # test_id -> class_id of the leader

    for test_id, event_order in enumerate(class_stable_permutations(event_ids)):
        if orders_to_check is not None and test_id not in orders_to_check:
            continue

        total_orders += 1

        best_validated = None
        leader = None

        for class_id, stage in event_order:
            index = (avg_rms_list[class_id], -event_order.index((class_id, 0)))
            if stage == 0:
                if leader is None and (best_validated is None or best_validated < index):
                    simulated_pre_spotter_oks += 1
            elif stage == 1:
                if leader is None and (best_validated is None or best_validated < index):
                    simulated_post_spotter_oks += 1
                    best_validated = index
            elif stage == 2:
                if leader is None and best_validated == index:
                    leader = class_id

        leaders[test_id] = leader

    assert_eq(total_orders, total_orders_const, "Something is completely wrong")

    if total_orders == 1680:
        assert_eq(simulated_pre_spotter_oks, pre_spotter_oks_const, "Your math is wrong for pre-spotter oks!")
        assert_eq(simulated_post_spotter_oks, post_spotter_oks_const, "Your math is wrong for post-spotter oks!")

    # Constants are ok, validating algorithm.

    cachalot_client = get_client(cachalot)

    pre_spotter_oks = 0
    post_spotter_oks = 0

    def count_pre_spotter(rsp):
        nonlocal pre_spotter_oks
        if rsp.continuation_allowed:
            pre_spotter_oks += 1

    def count_post_spotter(rsp):
        nonlocal post_spotter_oks
        if rsp.continuation_allowed:
            post_spotter_oks += 1

    device_ids = [
        b"macbook",
        b"iphone11",
        b"station",
    ]

    def get_client_wrap(test_id, client_id):
        return BlindSyncActivationClient.get(
            cachalot_client, user_id, device_ids[client_id], avg_rms_list[client_id], test_id=test_id,
            raise_on_already_rejected=False
        )

    def get_single_device_events(client_getter):
        return [
            (
                lambda test_id: client_getter(test_id).make_announcement(spotter_validated=False),
                lambda allow, _: count_pre_spotter(allow)
            ),
            (
                lambda test_id: client_getter(test_id).make_announcement(spotter_validated=True),
                lambda allow, _: count_post_spotter(allow)
            ),
            (
                lambda test_id: client_getter(test_id).try_acquire_leadership(),
                lambda rsp, leader_validator: leader_validator(rsp)
            ),
        ]

    events = [
        get_single_device_events(lambda test_id: get_client_wrap(test_id, 0)),
        get_single_device_events(lambda test_id: get_client_wrap(test_id, 1)),
        get_single_device_events(lambda test_id: get_client_wrap(test_id, 2)),
    ]

    for test_id, event_order in enumerate(class_stable_permutations(events)):
        if orders_to_check is not None and test_id not in orders_to_check:
            continue

        # Initialize clients at once (window must be < 250ms).
        # Order of client.timestamp must match the order of 0-stages.
        for class_id, stage in event_order:
            if stage == 0:
                get_client_wrap(test_id, class_id)

        leader_validator = LeaderCountGuard(f"test_three_devices_all_{test_id}")
        Scenario(event_order).run((test_id,), (leader_validator,))
        leader_validator.finish()

    # We generate `9!/3!/3!/3! = 1680` orders.
    # First announcements should be made `1680 * 3 = 5040` times.
    # In `5040 - pre_spotter_oks_const` cases first announcement of quiet device
    # happens after second announcement of louder device.
    check_counts(simulated_pre_spotter_oks, pre_spotter_oks, "Wrong cumulative amount of pre-spotter oks")

    # In `pre_spotter_oks_const - post_spotter_oks_const` cases second announcement of
    # quiet device finds record about validated spotter of louder device.
    check_counts(simulated_post_spotter_oks, post_spotter_oks, "Wrong cumulative amount of post-spotter oks")


def test_three_devices_all_distinct(cachalot, orders_to_check=None):
    # We generate `9!/3!/3!/3! = 1680` orders.
    # First announcements should be made `1680 * 3 = 5040` times.
    # Only in `5040 - 4020` cases first announcement of station_mini
    # happens after second announcement of macbook or iphone11.
    # So we get 4020 pre-spotter oks.

    # In `4020 - 3080` cases second announcement of station_mini
    # finds record about validated spotter of iphone11 or macbook.
    _three_devices_all_test_template(cachalot, b"uhura", [45, 36, 4], 4020, 2544, orders_to_check)


def test_three_devices_all_duplicated(cachalot, orders_to_check=None):
    # We generate `9!/3!/3!/3! = 1680` orders.
    # First announcements should be made `1680 * 3 = 5040` times.
    # Only in `5040 - 3780` cases first announcement of station_mini
    # happens after second announcement of macbook or iphone11.
    # So we get 3780 pre-spotter oks.

    # In `3780 - 2744` cases second announcement of station_mini
    # finds record about validated spotter of iphone11 or macbook.
    _three_devices_all_test_template(cachalot, b"natalyazorina", [22, 22, 7], 3780, 2384, orders_to_check)


def test_three_devices_all_equivalent(cachalot, orders_to_check=None):
    # We generate `9!/3!/3!/3! = 1680` orders.
    # Since all avg_rms are equal,
    # first and second announcements should be made `1680 * 3 = 5040` times.
    _three_devices_all_test_template(cachalot, b"mrs_equal_devices", [91, 91, 91], 3480, 2232, orders_to_check)


def test_two_devices_single_step(cachalot):
    test_name = "test_two_devices_single_step"
    user_id = b"artnikuh"
    leader_validator = LeaderCountGuard(test_name, b"pixel")
    client1 = BlindSyncActivationClient(get_client(cachalot), user_id, b"pixel", 123)
    client2 = BlindSyncActivationClient(get_client(cachalot), user_id, b"iphone", 35)
    assert_true(client2.make_announcement(True).continuation_allowed, f"Announcement (1) failed in {test_name}")
    assert_true(leader_validator(client1.try_acquire_leadership()), f"No leader in {test_name}")
    assert_true(not leader_validator(client2.try_acquire_leadership()), f"Wrong leader in {test_name}")
    leader_validator.finish()


def test_three_devices_with_zero_rms(cachalot):
    test_name = "test_three_devices_with_zero_rms"
    leader_validator = LeaderCountGuard(test_name, b"novo")
    client1 = BlindSyncActivationClient(get_client(cachalot), b"dinara", b"novo", 9)
    client2 = BlindSyncActivationClient(get_client(cachalot), b"dinara", b"gire", 0)
    client3 = BlindSyncActivationClient(get_client(cachalot), b"dinara", b"evo", 17)

    assert_true(client1.make_announcement(False).continuation_allowed, f"Announcement (1) failed in {test_name}")
    assert_true(client2.make_announcement(False).continuation_allowed, f"Announcement (2) failed in {test_name}")
    assert_true(client3.make_announcement(False).continuation_allowed, f"Announcement (3) failed in {test_name}")

    rsp2 = client2.make_announcement(True)
    assert_true(rsp2.zero_rms_found, "Backend is broken [2]")
    assert_true(rsp2.continuation_allowed, f"Announcement (4) failed in {test_name}")

    rsp3 = client3.make_announcement(True)
    assert_true(rsp3.continuation_allowed, f"Announcement (5) failed in {test_name}")

    rsp4 = client2.try_acquire_leadership()
    assert_true(not leader_validator(rsp4), f"Wrong leader in {test_name}")
    assert_eq(rsp4.leader_info.spotter_features.avg_rms, 9.0, "Backend is broken [4]")

    rsp5 = client1.make_announcement(False)
    assert_true(rsp5.continuation_allowed, f"Announcement (6) failed in {test_name}")
    rsp6 = client1.try_acquire_leadership()
    assert_true(leader_validator(rsp6), f"No leader in {test_name}")

    leader_validator.finish()


def test_allow_unvalidated(cachalot):
    test_name = "test_allow_unvalidated"
    leader_validator = LeaderCountGuard(test_name, b"love")

    def build_client_wrapper(user_id, device_id, avg_rms):
        return BlindSyncActivationClient(
            get_client(cachalot), user_id, device_id, avg_rms,
        )

    client1 = build_client_wrapper(b"olegovna", b"love", 97)
    client2 = build_client_wrapper(b"olegovna", b"death", 3)
    client3 = build_client_wrapper(b"olegovna", b"robots", 8)

    assert_true(client1.make_announcement(False).continuation_allowed, f"Announcement (1) failed in {test_name}")
    assert_true(client2.make_announcement(False).continuation_allowed, f"Announcement (2) failed in {test_name}")
    assert_true(client3.make_announcement(False).continuation_allowed, f"Announcement (3) failed in {test_name}")

    rsp3 = client3.make_announcement(False)
    assert_eq(rsp3.best_competitor.device_id, b"love", "Backend is broken [2]")

    rsp2 = client2.make_announcement(True)
    assert_eq(rsp2.best_competitor.spotter_features.avg_rms, 97.0, "Backend is broken")
    assert_eq(rsp2.continuation_allowed, True, "This quiet client must be canceled")

    rsp4 = client2.try_acquire_leadership()
    assert_true(not leader_validator(rsp4), f"Quiet leader in {test_name}")
    assert_eq(rsp4.leader_info.spotter_features.avg_rms, 97.0, "Wrong rms")
    assert_eq(rsp4.spotter_validated_by, b"death", "Expected spotter_validated_by=death")

    assert_true(client1.make_announcement(False), f"Announcement (6) failed in {test_name}")

    rsp5 = client1.try_acquire_leadership()
    assert_true(leader_validator(rsp5), f"Second leader in {test_name}")
    assert_eq(rsp5.spotter_validated_by, b"death", "Expected spotter_validated_by=death [2]")

    leader_validator.finish()


def test_disallow_unvalidated_1(cachalot):
    test_name = "test_disallow_unvalidated_1"

    def build_client_wrapper(user_id, device_id, avg_rms):
        return BlindSyncActivationClient(
            get_client(cachalot), user_id, device_id, avg_rms,
        )

    client1 = build_client_wrapper(b"020969", b"kia", 1234)
    client1.make_announcement(False)
    rsp1 = client1.try_acquire_leadership()
    assert_true(not rsp1.activation_allowed, f"Fail in {test_name} [1]")
    assert_true((not rsp1.spotter_validated_by) or (rsp1.spotter_validated_by == b"Nobody!"),
                "Expected empty spotter_validated_by")


def test_disallow_unvalidated_2(cachalot):
    test_name = "test_disallow_unvalidated_2"

    def build_client_wrapper(user_id, device_id, avg_rms):
        return BlindSyncActivationClient(
            get_client(cachalot), user_id, device_id, avg_rms,
        )

    client1 = build_client_wrapper(b"14041997", b"vw", 1234)
    client1.make_announcement(False)
    client1.make_announcement(True)
    rsp1 = client1.try_acquire_leadership()
    assert_true(rsp1.activation_allowed, f"Fail in {test_name} [1]")
    assert_eq(rsp1.spotter_validated_by, b"vw", f"Fail in {test_name} [2]")

    time.sleep(5)
    # now valid spotter of client1 does not count

    client2 = build_client_wrapper(b"14041997", b"golf", 56)
    client2.make_announcement(False)
    client2.make_announcement(False)
    rsp2 = client2.try_acquire_leadership()
    assert_true(not rsp2.activation_allowed, f"Fail in {test_name} [3]")
    assert_true((not rsp2.spotter_validated_by) or (rsp2.spotter_validated_by == b"Nobody!"),
                "Expected empty spotter_validated_by")


def test_three_devices_extreme(cachalot):
    test_name = "test_three_devices_extreme"
    leader_validator = LeaderCountGuard(test_name, b"snow")

    def build_client_wrapper(user_id, device_id, avg_rms):
        return BlindSyncActivationClient(
            get_client(cachalot), user_id, device_id, avg_rms,
        )

    client1 = build_client_wrapper(b"timur", b"snow", 23)
    client2 = build_client_wrapper(b"timur", b"board", 0)
    client3 = build_client_wrapper(b"timur", b"games", 64)

    assert_true(client1.make_announcement(False).continuation_allowed, f"Announcement (1) failed in {test_name}")
    assert_true(client2.make_announcement(False).continuation_allowed, f"Announcement (2) failed in {test_name}")

    rsp2 = client2.make_announcement(True)
    assert_true(rsp2.continuation_allowed, f"Announcement (3) failed in {test_name}")
    assert_eq(rsp2.best_competitor.spotter_features.avg_rms, 23.0, "Backend is broken")

    client3.make_announcement(False)

    assert_true(client1.make_announcement(False).continuation_allowed, f"Announcement (4) failed in {test_name}")

    rsp3 = client2.try_acquire_leadership()
    assert_true(not leader_validator(rsp3), f"Wrong leader in {test_name}")
    assert_eq(rsp3.leader_info.spotter_features.avg_rms, 23.0, "Backend is broken [2]")

    rsp4 = client1.try_acquire_leadership()
    assert_true(leader_validator(rsp4), f"No leader in {test_name}")

    leader_validator.finish()


class VoiceInputActivationClient(SyncMixin):
    def __init__(self, cachalot):
        self.client = cachalot.get_client(use_grpc=True)

        self.timestamp = ProtoTimestamp()
        self.timestamp.GetCurrentTime()

        for method in (
            "make_first_announcement",
            "make_second_announcement",
            "make_final",
        ):
            setattr(self, method, self._sync_wrapper(getattr(self, '_' + method)))

    async def _make_first_announcement(self, user_id, device_id, avg_rms):
        activation_request = protos.TActivationAnnouncementRequest()
        activation_request.Info.UserId = user_id
        activation_request.Info.DeviceId = device_id
        activation_request.Info.ActivationAttemptTime.CopyFrom(self.timestamp)
        activation_request.Info.SpotterFeatures.AvgRMS = avg_rms
        activation_request.FreshnessDeltaMilliSeconds = 2500

        return await self.client._connection.execute_complex_request(
            "activation_voice_input_first_announcement",
            [
                ("activation_announcement_request", activation_request),
            ],
            {
                "activation_announcement_request": protos.TActivationAnnouncementRequest,
            }
        )

    async def _make_second_announcement(self, proto_req):
        spotter_rsp = asr_protos.TSpotterValidation()
        spotter_rsp.Valid = True

        return await self.client._connection.execute_complex_request(
            "activation_voice_input_second_announcement",
            [
                ("activation_announcement_request", proto_req),
                ("asr_spotter_validation", spotter_rsp),
            ],
            {
                "activation_announcement_response": protos.TActivationAnnouncementResponse,
                "activation_final_request": protos.TActivationFinalRequest,
                "activation_log": protos.TActivationLog,
            }
        )

    async def _make_final(self, proto_req, proto_log):
        mm_run_rsp = protos.TMMRunResponseForActivation()

        return await self.client._connection.execute_complex_request(
            "activation_voice_input_final",
            [
                ("activation_final_request", proto_req),
                ("mm_run_ready", mm_run_rsp),
                ("activation_log", proto_log)
            ],
            {
                "activation_successful": protos.TActivationSuccessful,
                "activation_final_response": protos.TActivationFinalResponse,
                "activation_log_final": protos.TActivationLog,
            }
        )


def test_voice_input_api(cachalot):
    client = VoiceInputActivationClient(cachalot)

    rsp1 = client.make_first_announcement(b"vi_user", b"vi_device", 69)
    assert rsp1["activation_announcement_request"].Info.UserId == b"vi_user"
    assert rsp1["activation_announcement_request"].Info.DeviceId == b"vi_device"
    assert rsp1["activation_announcement_request"].Info.ActivationAttemptTime == client.timestamp
    assert rsp1["activation_announcement_request"].Info.SpotterFeatures.AvgRMS == 69
    assert not rsp1["activation_announcement_request"].Info.SpotterFeatures.HasField("Validated")
    assert rsp1["activation_announcement_request"].FreshnessDeltaMilliSeconds == 2500
    assert len(rsp1) == 1

    rsp2 = client.make_second_announcement(rsp1["activation_announcement_request"])
    assert "activation_log" in rsp2
    assert rsp2["activation_announcement_response"].ContinuationAllowed
    assert not rsp2["activation_announcement_response"].HasField("Error")
    assert not rsp2["activation_announcement_response"].ZeroRmsFound
    assert not rsp2["activation_announcement_response"].LeaderFound
    assert rsp2["activation_final_request"].Info.UserId == b"vi_user"
    assert rsp2["activation_final_request"].Info.DeviceId == b"vi_device"
    assert rsp2["activation_final_request"].Info.ActivationAttemptTime == client.timestamp
    assert rsp2["activation_final_request"].Info.SpotterFeatures.AvgRMS == 69
    assert rsp2["activation_final_request"].Info.SpotterFeatures.Validated
    assert not rsp2["activation_final_request"].IgnoreRms
    assert rsp2["activation_final_request"].FreshnessDeltaMilliSeconds == 2500
    assert rsp2["activation_final_request"].NeedCleanup
    assert len(rsp2) == 3

    rsp3 = client.make_final(rsp2["activation_final_request"], rsp2["activation_log"])
    assert rsp3["activation_final_response"].ActivationAllowed
    assert not rsp3["activation_final_response"].HasField("Error")
    assert not rsp3["activation_final_response"].HasField("LeaderInfo")
    assert rsp3["activation_final_response"].SpotterValidatedBy == b"vi_device"
    assert rsp3["activation_log_final"].ThisSpotterIsValid
    assert rsp3["activation_log_final"].ActivatedDeviceId == b"vi_device"
    assert rsp3["activation_log_final"].DeviceId == b"vi_device"
    assert rsp3["activation_log_final"].MultiActivationReason == b"OK"
    assert rsp3["activation_log_final"].SpotterValidatedBy == b"vi_device"
    assert rsp3["activation_log_final"].YandexUid == b"vi_user"
    assert rsp3["activation_log_final"].ActivatedRMS == 69
    assert rsp3["activation_log_final"].AvgRMS == 69
    assert rsp3["activation_log_final"].ActivatedTimestamp == client.timestamp
    assert rsp3["activation_log_final"].FinishTimestamp.ToJsonString() > client.timestamp.ToJsonString()
    assert rsp3["activation_log_final"].Timestamp == client.timestamp
    assert rsp3["activation_log_final"].FreshnessDeltaMilliSeconds == 2500
    assert "activation_successful" in rsp3
    assert len(rsp3) == 3


def test_rms_multiplication_for_first_station_voice_input(cachalot, ydb_session):
    # VOICESERV-4137

    client = VoiceInputActivationClient(cachalot)

    base_rms = 7301
    patched_rms = base_rms * 1.2
    device_id = b"A" * 20  # every station has 20 symbols in device_id

    rsp1 = client.make_first_announcement(b"vi_user-2021-10-18_1", device_id, base_rms)
    assert math.isclose(rsp1["activation_announcement_request"].Info.SpotterFeatures.AvgRMS, base_rms)
    _validate_rms_in_base(ydb_session, "v2/activation_announcements", device_id, patched_rms)

    rsp2 = client.make_second_announcement(rsp1["activation_announcement_request"])
    assert math.isclose(rsp2["activation_final_request"].Info.SpotterFeatures.AvgRMS, base_rms)
    _validate_rms_in_base(ydb_session, "v2/activation_announcements", device_id, patched_rms)

    rsp3 = client.make_final(rsp2["activation_final_request"], rsp2["activation_log"])
    assert math.isclose(rsp3["activation_log_final"].AvgRMS, patched_rms, abs_tol=1.0)
    assert math.isclose(rsp3["activation_log_final"].ActivatedRMS, patched_rms, abs_tol=1.0)
    _validate_rms_in_base(ydb_session, "v2/activation_leaders", device_id, patched_rms)


def test_rms_multiplication_for_yandexmicro_voice_input(cachalot, ydb_session):
    # VOICESERV-4137

    client = VoiceInputActivationClient(cachalot)

    base_rms = 5082
    patched_rms = base_rms * 3.0
    device_id = b"Lol_i_am_micro"  # every micro has L as first symbol of device_id

    rsp1 = client.make_first_announcement(b"vi_user-2021-10-18_2", device_id, base_rms)
    assert math.isclose(rsp1["activation_announcement_request"].Info.SpotterFeatures.AvgRMS, base_rms)
    _validate_rms_in_base(ydb_session, "v2/activation_announcements", device_id, patched_rms)

    rsp2 = client.make_second_announcement(rsp1["activation_announcement_request"])
    assert math.isclose(rsp2["activation_final_request"].Info.SpotterFeatures.AvgRMS, base_rms)
    _validate_rms_in_base(ydb_session, "v2/activation_announcements", device_id, patched_rms)

    rsp3 = client.make_final(rsp2["activation_final_request"], rsp2["activation_log"])
    assert math.isclose(rsp3["activation_log_final"].AvgRMS, patched_rms)
    assert math.isclose(rsp3["activation_log_final"].ActivatedRMS, patched_rms)
    _validate_rms_in_base(ydb_session, "v2/activation_leaders", device_id, patched_rms)
