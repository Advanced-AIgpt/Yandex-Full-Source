import os

from alice.matrix.library.monitoring.juggler.autoconf_base import JugglerAutoConfBase

from juggler_sdk import (
    Child,
    JugglerApi,
)


TESTING_CLUSTERS = ["worker_test"]
PROD_CLUSTERS = ["worker_prestable", "worker_production"]

TESTING_SHARD_COUNT = 3
PROD_SHARD_COUNT = 240

MAX_LINES_PER_ALERT = 50


class AutoConf(JugglerAutoConfBase):
    def __init__(self):
        super(AutoConf, self).__init__(
            "matrix_worker",
            "https://docs.yandex-team.ru/alice-matrix/pages/scheduler_and_worker/development/links",
        )


def update_worker_simple_successes_errors_alert(
    autoconf,
    cluster,
    is_prod,
    juggler_service_id,
    human_readable_name,
    sensor,
    check_expression,
):
    alert_id = f"{juggler_service_id}_{cluster}"

    alert = autoconf.build_solomon_program_alert(
        alert_id,
        juggler_service_id,
        cluster,
        f"{human_readable_name} ({autoconf.format_cluster(cluster)})",
        f"""
            let all_sm = group_lines("sum", {autoconf.build_default_solomon_sensor_selector("worker", "*", sensor, cluster)});

            let successes_sm = group_lines("sum", {autoconf.build_default_solomon_sensor_selector("worker", "ok", sensor, cluster)});
            let errors_sm = group_lines("sum", {autoconf.build_default_solomon_sensor_selector("worker", "error", sensor, cluster)});

            let all = sum(all_sm);

            let successes = sum(successes_sm);
            let errors = sum(errors_sm);

            let successes_percent = {autoconf.get_percent("successes", "all")};
            let errors_percent = {autoconf.get_percent("errors", "all")};

            let successes_str = {autoconf.double_to_str("successes")};
            let errors_str = {autoconf.double_to_str("errors")};
            let successes_percent_str = {autoconf.double_to_str("successes_percent")};
            let errors_percent_str = {autoconf.double_to_str("errors_percent")};
        """.replace("  ", ""),
        check_expression,
        extra_annotations={
            "description": autoconf.build_solomon_alert_description_annotation([
                "Successes: {{expression.successes_str}} ({{expression.successes_percent_str}}%)",
                "Errors: {{expression.errors_str}} ({{expression.errors_percent_str}}%)",
            ]),
        },
    )
    autoconf.create_or_update_solomon_alert(alert)
    return autoconf.build_juggler_check(
        cluster=cluster,
        alert_id=alert_id,
        name=juggler_service_id,
        is_prod=is_prod,
        children=[
            Child(
                host=f"(host={cluster})&service=matrix_worker.{juggler_service_id}",
                service="all",
                group_type="EVENTS",
                instance="all",
            ),
        ],
    )


def update_worker_loop_alert(
    autoconf,
    cluster,
    is_prod,
    juggler_service_id,
    human_readable_name,
    check_expression,
):
    alert_id = f"{juggler_service_id}_{cluster}"

    alert = autoconf.build_solomon_program_alert(
        alert_id,
        juggler_service_id,
        cluster,
        f"{human_readable_name} ({autoconf.format_cluster(cluster)})",
        f"""
            let all_sm = group_lines("sum", {autoconf.build_default_solomon_sensor_selector("worker", "*", "worker_loop.self.sync", cluster)});

            let successes_sm = group_lines("sum", {autoconf.build_default_solomon_sensor_selector("worker", "ok", "worker_loop.self.sync", cluster)});
            let skips_sm = group_lines("sum", {autoconf.build_default_solomon_sensor_selector("worker", "skip", "worker_loop.self.sync", cluster)});

            let errors_sm = group_lines("sum", {autoconf.build_default_solomon_sensor_selector("worker", "error", "worker_loop.self.sync", cluster)});
            let exceptions_sm = group_lines("sum", {autoconf.build_default_solomon_sensor_selector("worker", "exception", "worker_loop.self.sync", cluster)});

            let all = sum(all_sm);

            let successes = sum(successes_sm);
            let skips = sum(skips_sm);
            let errors = sum(errors_sm);
            let exceptions = sum(exceptions_sm);

            let successes_percent = {autoconf.get_percent("successes", "all")};
            let skips_percent = {autoconf.get_percent("skips", "all")};
            let errors_percent = {autoconf.get_percent("errors", "all")};
            let exceptions_percent = {autoconf.get_percent("exceptions", "all")};

            let successes_str = {autoconf.double_to_str("successes")};
            let skips_str = {autoconf.double_to_str("skips")};
            let errors_str = {autoconf.double_to_str("errors")};
            let exceptions_str = {autoconf.double_to_str("exceptions")};
            let successes_percent_str = {autoconf.double_to_str("successes_percent")};
            let skips_percent_str = {autoconf.double_to_str("skips_percent")};
            let errors_percent_str = {autoconf.double_to_str("errors_percent")};
            let exceptions_percent_str = {autoconf.double_to_str("exceptions_percent")};
        """.replace("  ", ""),
        check_expression,
        extra_annotations={
            "description": autoconf.build_solomon_alert_description_annotation([
                "Successes: {{expression.successes_str}} ({{expression.successes_percent_str}}%)",
                "Skips: {{expression.skips_str}} ({{expression.skips_percent_str}}%)",
                "Errors: {{expression.errors_str}} ({{expression.errors_percent_str}}%)",
                "Exceptions: {{expression.exceptions_str}} ({{expression.exceptions_percent_str}}%)",
            ]),
        },
    )
    autoconf.create_or_update_solomon_alert(alert)
    return autoconf.build_juggler_check(
        cluster=cluster,
        alert_id=alert_id,
        name=juggler_service_id,
        is_prod=is_prod,
        children=[
            Child(
                host=f"(host={cluster})&service=matrix_worker.{juggler_service_id}",
                service="all",
                group_type="EVENTS",
                instance="all",
            ),
        ],
    )


def update_worker_loop_errors_alert(autoconf, cluster, is_prod):
    return update_worker_loop_alert(
        autoconf,
        cluster,
        is_prod,
        "worker_loop_errors",
        "Worker loop errors",
        autoconf.build_solomon_check_percent_check_expression(percent=0.01),
    )


def update_worker_loop_exceptions_alert(autoconf, cluster, is_prod):
    return update_worker_loop_alert(
        autoconf,
        cluster,
        is_prod,
        "worker_loop_exceptions",
        "Worker loop exceptions (should never have happened)",
        "exceptions > 0",
    )


def update_worker_sync_interrupted_syncs_alert(
    autoconf,
    cluster,
    is_prod,
):
    juggler_service_id = "worker_sync_interrupted_syncs"
    alert_id = f"{juggler_service_id}_{cluster}"

    alert = autoconf.build_solomon_program_alert(
        alert_id,
        juggler_service_id,
        cluster,
        f"Worker sync interrupted syncs ({autoconf.format_cluster(cluster)})",
        f"""
            let interrupted_syncs_sm = group_lines("sum", {autoconf.build_default_solomon_sensor_selector("worker", "error", "worker_sync.self.interrupted_sync_found", cluster)});
            let interrupted_syncs = sum(interrupted_syncs_sm);
            let interrupted_syncs_str = {autoconf.double_to_str("interrupted_syncs")};
        """.replace("  ", ""),
        "interrupted_syncs > 0",
        extra_annotations={
            "description": autoconf.build_solomon_alert_description_annotation([
                "Interrupted syncs: {{expression.interrupted_syncs_str}}",
            ]),
        },
    )
    autoconf.create_or_update_solomon_alert(alert)
    return autoconf.build_juggler_check(
        cluster=cluster,
        alert_id=alert_id,
        name=juggler_service_id,
        is_prod=is_prod,
        children=[
            Child(
                host=f"(host={cluster})&service=matrix_worker.{juggler_service_id}",
                service="all",
                group_type="EVENTS",
                instance="all",
            ),
        ],
    )


def update_worker_sync_move_action_rows_from_incoming_to_processing_alert(autoconf, cluster, is_prod):
    return update_worker_simple_successes_errors_alert(
        autoconf,
        cluster,
        is_prod,
        "worker_sync_move_action_rows_from_incoming_to_processing",
        "Worker sync move action rows from incoming to processing process overall",
        "worker_sync.self.move_action_rows_from_incoming_to_processing",
        autoconf.build_solomon_check_percent_check_expression(percent=0.01),
    )


def update_worker_sync_move_action_row_from_incoming_to_processing_alert(autoconf, cluster, is_prod):
    return update_worker_simple_successes_errors_alert(
        autoconf,
        cluster,
        is_prod,
        "worker_sync_move_action_row_from_incoming_to_processing",
        "Worker sync move single action row from incoming to processing",
        "worker_sync.self.move_action_row_from_incoming_to_processing",
        autoconf.build_solomon_check_percent_check_expression(percent=0.01),
    )


def update_worker_sync_perform_actions_from_processing_alert(autoconf, cluster, is_prod):
    return update_worker_simple_successes_errors_alert(
        autoconf,
        cluster,
        is_prod,
        "worker_sync_perform_actions_from_processing",
        "Worker sync perform actions process overall",
        "worker_sync.self.perform_actions_from_processing",
        autoconf.build_solomon_check_percent_check_expression(percent=0.01),
    )


def update_worker_sync_perform_action_from_processing_alert(
    autoconf,
    cluster,
    is_prod,
):
    juggler_service_id = "worker_sync_perform_action_from_processing"
    alert_id = f"{juggler_service_id}_{cluster}"

    alert = autoconf.build_solomon_program_alert(
        alert_id,
        juggler_service_id,
        cluster,
        f"Worker sync perform single action ({autoconf.format_cluster(cluster)})",
        f"""
            let all_sm = group_lines("sum", {autoconf.build_default_solomon_sensor_selector("worker", "*", "worker_sync.self.perform_action_from_processing", cluster)});

            let successes_sm = group_lines("sum", {autoconf.build_default_solomon_sensor_selector("worker", "ok", "worker_sync.self.perform_action_from_processing", cluster)});
            let do_action_errors_sm = group_lines("sum", {autoconf.build_default_solomon_sensor_selector("worker", "do_action_error", "worker_sync.self.perform_action_from_processing", cluster)});
            let errors_sm = group_lines("sum", {autoconf.build_default_solomon_sensor_selector("worker", "error", "worker_sync.self.perform_action_from_processing", cluster)});

            let all = sum(all_sm);

            let successes = sum(successes_sm);
            let do_action_errors = sum(do_action_errors_sm);
            let errors = sum(errors_sm);

            let successes_percent = {autoconf.get_percent("successes", "all")};
            let do_action_errors_percent = {autoconf.get_percent("do_action_errors", "all")};
            let errors_percent = {autoconf.get_percent("errors", "all")};

            let successes_str = {autoconf.double_to_str("successes")};
            let do_action_errors_str = {autoconf.double_to_str("do_action_errors")};
            let errors_str = {autoconf.double_to_str("errors")};
            let successes_percent_str = {autoconf.double_to_str("successes_percent")};
            let do_action_errors_percent_str = {autoconf.double_to_str("do_action_errors_percent")};
            let errors_percent_str = {autoconf.double_to_str("errors_percent")};
        """.replace("  ", ""),
        autoconf.build_solomon_check_percent_check_expression(percent=0.01),
        extra_annotations={
            "description": autoconf.build_solomon_alert_description_annotation([
                "Successes: {{expression.successes_str}} ({{expression.successes_percent_str}}%)",
                "Do action errors: {{expression.do_action_errors_str}} ({{expression.do_action_errors_percent_str}}%)",
                "Other errors: {{expression.errors_str}} ({{expression.errors_percent_str}}%)",
            ]),
        },
    )
    autoconf.create_or_update_solomon_alert(alert)
    return autoconf.build_juggler_check(
        cluster=cluster,
        alert_id=alert_id,
        name=juggler_service_id,
        is_prod=is_prod,
        children=[
            Child(
                host=f"(host={cluster})&service=matrix_worker.{juggler_service_id}",
                service="all",
                group_type="EVENTS",
                instance="all",
            ),
        ],
    )


def update_worker_sync_do_send_technical_push_alert(
    autoconf,
    cluster,
    is_prod,
):
    juggler_service_id = "worker_sync_do_send_technical_push"
    alert_id = f"{juggler_service_id}_{cluster}"

    alert = autoconf.build_solomon_program_alert(
        alert_id,
        juggler_service_id,
        cluster,
        f"Worker sync do send technical push ({autoconf.format_cluster(cluster)})",
        f"""
            let all_sm = group_lines("sum", {autoconf.build_default_solomon_sensor_selector("worker", "*", "worker_sync.self.do_send_technical_push", cluster)});

            let successes_sm = group_lines("sum", {autoconf.build_default_solomon_sensor_selector("worker", "ok", "worker_sync.self.do_send_technical_push", cluster)});
            let device_not_connected_sm = group_lines("sum", {autoconf.build_default_solomon_sensor_selector("worker", "device_not_connected", "worker_sync.self.do_send_technical_push", cluster)});
            let errors_sm = group_lines("sum", {autoconf.build_default_solomon_sensor_selector("worker", "error", "worker_sync.self.do_send_technical_push", cluster)});

            let all = sum(all_sm);

            let successes = sum(successes_sm);
            let device_not_connected = sum(device_not_connected_sm);
            let errors = sum(errors_sm);

            let successes_percent = {autoconf.get_percent("successes", "all")};
            let device_not_connected_percent = {autoconf.get_percent("device_not_connected", "all")};
            let errors_percent = {autoconf.get_percent("errors", "all")};

            let successes_str = {autoconf.double_to_str("successes")};
            let device_not_connected_str = {autoconf.double_to_str("device_not_connected")};
            let errors_str = {autoconf.double_to_str("errors")};
            let successes_percent_str = {autoconf.double_to_str("successes_percent")};
            let device_not_connected_percent_str = {autoconf.double_to_str("device_not_connected_percent")};
            let errors_percent_str = {autoconf.double_to_str("errors_percent")};
        """.replace("  ", ""),
        autoconf.build_solomon_check_percent_check_expression(percent=0.01),
        extra_annotations={
            "description": autoconf.build_solomon_alert_description_annotation([
                "Successes: {{expression.successes_str}} ({{expression.successes_percent_str}}%)",
                "Device not connected: {{expression.device_not_connected_str}} ({{expression.device_not_connected_percent_str}}%)",
                "Errors: {{expression.errors_str}} ({{expression.errors_percent_str}}%)",
            ]),
        },
    )
    autoconf.create_or_update_solomon_alert(alert)
    return autoconf.build_juggler_check(
        cluster=cluster,
        alert_id=alert_id,
        name=juggler_service_id,
        is_prod=is_prod,
        children=[
            Child(
                host=f"(host={cluster})&service=matrix_worker.{juggler_service_id}",
                service="all",
                group_type="EVENTS",
                instance="all",
            ),
        ],
    )


def update_worker_sync_perform_action_delay_alert(
    autoconf,
    cluster,
    is_prod,
):
    juggler_service_id = "worker_sync_perform_action_delay"
    alert_id = f"{juggler_service_id}_{cluster}"

    def get_perform_action_delay_lines(bin_p):
        return f"""
            {{
                service="worker", sensor="worker_sync.self.perform_action_delay", host="cluster", cluster="{cluster}", bin="{bin_p}"
            }}
        """

    def to_seconds(expr):
        return f"({expr}) / {10**6}"

    qs = (50, 75, 85, 95, 99)
    program = ""
    for q in qs:
        program += f"""
            let q{q} = histogram_percentile({q}, {get_perform_action_delay_lines("*")});
            let q{q}_max = {to_seconds(f"max(q{q})")};
            let q{q}_max_str = {autoconf.double_to_str(f"q{q}_max")};
        """

    program += f"""
        let actions_delayed_for_too_long = sum(histogram_count({30 * 10**6}, inf(), {get_perform_action_delay_lines("*")}));
        let actions_delayed_for_too_long_str = {autoconf.double_to_str("actions_delayed_for_too_long")};
    """
    alert = autoconf.build_solomon_program_alert(
        alert_id,
        juggler_service_id,
        cluster,
        f"Worker sync perform action delay ({autoconf.format_cluster(cluster)})",
        program.replace("  ", ""),
        "actions_delayed_for_too_long > 0",
        extra_annotations={
            "description": autoconf.build_solomon_alert_description_annotation([
                "Actions delayed for too long: {{expression.actions_delayed_for_too_long_str}}",
            ] + [
                f"q{q} maximum for the period: {{{{expression.q{q}_max_str}}}}s"
                for q in qs
            ]),
        },
    )
    autoconf.create_or_update_solomon_alert(alert)
    return autoconf.build_juggler_check(
        cluster=cluster,
        alert_id=alert_id,
        name=juggler_service_id,
        is_prod=is_prod,
        children=[
            Child(
                host=f"(host={cluster})&service=matrix_worker.{juggler_service_id}",
                service="all",
                group_type="EVENTS",
                instance="all",
            ),
        ],
    )


def update_worker_slo_shard_not_synced_for_too_long(
    autoconf,
    clusters,
    shard_count,
    is_prod,
):
    juggler_host = "slo-production" if is_prod else "slo-test"
    alert_count = (shard_count + MAX_LINES_PER_ALERT - 1) // MAX_LINES_PER_ALERT

    for alert_index in range(alert_count):
        shard_offset = alert_index * MAX_LINES_PER_ALERT
        current_shard_count = min(MAX_LINES_PER_ALERT, shard_count - shard_offset)
        shard_ids = [shard_offset + i for i in range(current_shard_count)]

        juggler_service_id = f"worker_slo_shard_not_synced_for_too_long_{alert_index}"
        alert_id = f"{juggler_service_id}_{juggler_host}"

        program = "\n".join([
            f"""
                let shard_{shard_id}_sm = group_lines(
                    "sum",
                    {{
                        service="worker", code="ok", sensor="worker_sync.self.sync_shard", host="cluster", cluster="{"|".join(clusters)}", shard="{shard_id}"
                    }}
                );
                let shard_{shard_id} = sum(shard_{shard_id}_sm);
            """
            for shard_id in shard_ids
        ]) + f"""
            let bad_shards = {" + ".join([f"(shard_{shard_id} != 0 ? 0 : 1)" for shard_id in shard_ids])};
        """

        alert = autoconf.build_solomon_program_alert(
            alert_id,
            juggler_service_id,
            juggler_host,
            f"Worker SLO shard not synced for too long ({juggler_host.split('-')[1]}) (group {alert_index})",
            program.replace("  ", ""),
            "bad_shards != 0",
            extra_annotations={
                "description": autoconf.build_solomon_alert_description_annotation([
                    "Bad shard count: {{expression.bad_shards}}",
                ]),
            },
        )
        autoconf.create_or_update_solomon_alert(alert)
        yield autoconf.build_juggler_check(
            cluster=juggler_host,
            alert_id=alert_id,
            name=juggler_service_id,
            is_prod=is_prod,
            children=[
                Child(
                    host=f"(host={juggler_host})&service=matrix_worker.{juggler_service_id}",
                    service="all",
                    group_type="EVENTS",
                    instance="all",
                ),
            ],
        )


def update_solomon_and_get_juggler_single_cluster_checks(autoconf, cluster, is_prod):

    # Worker loop alerts
    yield update_worker_loop_errors_alert(autoconf, cluster, is_prod)
    yield update_worker_loop_exceptions_alert(autoconf, cluster, is_prod)

    # Worker sync alerts

    # Overall
    yield update_worker_sync_interrupted_syncs_alert(autoconf, cluster, is_prod)

    # Incoming -> processing
    yield update_worker_sync_move_action_rows_from_incoming_to_processing_alert(autoconf, cluster, is_prod)
    yield update_worker_sync_move_action_row_from_incoming_to_processing_alert(autoconf, cluster, is_prod)

    # Perform actions
    yield update_worker_sync_perform_actions_from_processing_alert(autoconf, cluster, is_prod)
    yield update_worker_sync_perform_action_from_processing_alert(autoconf, cluster, is_prod)
    yield update_worker_sync_do_send_technical_push_alert(autoconf, cluster, is_prod)
    yield update_worker_sync_perform_action_delay_alert(autoconf, cluster, is_prod)

    # YDB alerts in scheduler autoconf


def update_solomon_and_get_juggler_multi_cluster_checks(autoconf, clusters, shard_count, is_prod):
    for check in update_worker_slo_shard_not_synced_for_too_long(autoconf, clusters, shard_count, is_prod):
        yield check


def main():
    with JugglerApi(
        "http://juggler-api.search.yandex.net",
        mark="matrix_worker_checks",
        oauth_token=os.environ["JUGGLER_TOKEN"],
    ) as api:
        autoconf = AutoConf()

        for cluster in TESTING_CLUSTERS:
            for check in update_solomon_and_get_juggler_single_cluster_checks(autoconf, cluster, is_prod=False):
                api.upsert_check(check)

        for cluster in PROD_CLUSTERS:
            for check in update_solomon_and_get_juggler_single_cluster_checks(autoconf, cluster, is_prod=True):
                api.upsert_check(check)

        for check in update_solomon_and_get_juggler_multi_cluster_checks(autoconf, TESTING_CLUSTERS, TESTING_SHARD_COUNT, is_prod=False):
            api.upsert_check(check)

        for check in update_solomon_and_get_juggler_multi_cluster_checks(autoconf, PROD_CLUSTERS, PROD_SHARD_COUNT, is_prod=True):
            api.upsert_check(check)
