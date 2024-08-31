import os

from alice.matrix.library.monitoring.juggler.autoconf_base import JugglerAutoConfBase

from juggler_sdk import (
    Child,
    JugglerApi,
)


TESTING_CLUSTERS = ["scheduler_test"]
PROD_CLUSTERS = ["scheduler_prestable", "scheduler_production"]

HTTP_ROUTES = [
    "/schedule",
    "/unschedule",
]
APPHOSTED_ROUTES = [
    "/add_scheduled_action",
    "/remove_scheduled_action",
]


class AutoConf(JugglerAutoConfBase):
    def __init__(self):
        super(AutoConf, self).__init__(
            "matrix_scheduler",
            "https://docs.yandex-team.ru/alice-matrix/pages/scheduler_and_worker/development/links",
        )


def route_to_id(route):
    return route[1:].replace("/", "_")


def get_errors_percentage_juggler_service_id(route):
    return f"errors_percentage_{route_to_id(route)}"


def get_errors_percentage_alert_id(route, cluster):
    return f"{get_errors_percentage_juggler_service_id(route)}_{cluster}"


def get_errors_percent(is_prod):
    return 0.01 if is_prod else 0.33


def update_golovan(autoconf):
    check_name = "matrix_scheduler.balancer.service_total_fail"

    autoconf.create_or_update_golovan_check(
        check_name,
        autoconf.build_balancer_total_fails_check(
            "matrix-scheduler.alice.yandex.net",
            check_name,
            is_prod=True,
        ),
    )


def update_solomon_and_get_juggler_checks(autoconf, cluster, is_prod):
    for route in HTTP_ROUTES:
        alert = autoconf.build_solomon_program_alert(
            get_errors_percentage_alert_id(route, cluster),
            get_errors_percentage_juggler_service_id(route),
            cluster,
            f"{route} HTTP response codes ({autoconf.format_cluster(cluster)})",
            f"""
                let all_lines_sm = group_lines("sum", {autoconf.build_default_solomon_sensor_selector("scheduler", "*", f"{route_to_id(route)}.{route[1:]}.response", cluster)});
                let successes_lines_sm = group_lines("sum", {autoconf.build_default_solomon_sensor_selector("scheduler", "2*", f"{route_to_id(route)}.{route[1:]}.response", cluster)});

                let all = sum(all_lines_sm);
                let successes = sum(successes_lines_sm);
                let errors = all - successes;

                let successes_str = {autoconf.double_to_str("successes")};
                let errors_str = {autoconf.double_to_str("errors")};
                let successes_percent_str = {autoconf.get_percent_str("successes", "all")};
                let errors_percent_str = {autoconf.get_percent_str("errors", "all")};
            """.replace("  ", ""),
            autoconf.build_solomon_check_percent_check_expression(percent=get_errors_percent(is_prod)),
            extra_annotations={
                "description": autoconf.build_solomon_alert_description_annotation([
                    "Successes: {{expression.successes_str}} ({{expression.successes_percent_str}}%)",
                    "Errors: {{expression.errors_str}} ({{expression.errors_percent_str}}%)",
                ]),
            },
        )
        autoconf.create_or_update_solomon_alert(alert)
        yield autoconf.build_juggler_check(
            cluster=cluster,
            alert_id=get_errors_percentage_alert_id(route, cluster),
            name=get_errors_percentage_juggler_service_id(route),
            is_prod=is_prod,
            children=[
                Child(
                    host=f"(host={cluster})&service=matrix_scheduler.{get_errors_percentage_juggler_service_id(route)}",
                    service="all",
                    group_type="EVENTS",
                    instance="all",
                ),
            ],
        )

    for route in APPHOSTED_ROUTES:
        alert = autoconf.build_solomon_program_alert(
            get_errors_percentage_alert_id(route, cluster),
            get_errors_percentage_juggler_service_id(route),
            cluster,
            f"{route} apphost responses ({autoconf.format_cluster(cluster)})",
            f"""
                let all_lines_sm = group_lines("sum", {autoconf.build_default_solomon_sensor_selector("scheduler", "*", f"{route_to_id(route)}.self.response", cluster)});
                let successes_lines_sm = group_lines("sum", {autoconf.build_default_solomon_sensor_selector("scheduler", "ok", f"{route_to_id(route)}.self.response", cluster)});

                let all = sum(all_lines_sm);
                let successes = sum(successes_lines_sm);
                let errors = all - successes;

                let successes_str = {autoconf.double_to_str("successes")};
                let errors_str = {autoconf.double_to_str("errors")};
                let successes_percent_str = {autoconf.get_percent_str("successes", "all")};
                let errors_percent_str = {autoconf.get_percent_str("errors", "all")};
            """.replace("  ", ""),
            autoconf.build_solomon_check_percent_check_expression(percent=get_errors_percent(is_prod)),
            extra_annotations={
                "description": autoconf.build_solomon_alert_description_annotation([
                    "Successes: {{expression.successes_str}} ({{expression.successes_percent_str}}%)",
                    "Errors: {{expression.errors_str}} ({{expression.errors_percent_str}}%)",
                ]),
            },
        )
        autoconf.create_or_update_solomon_alert(alert)
        yield autoconf.build_juggler_check(
            cluster=cluster,
            alert_id=get_errors_percentage_alert_id(route, cluster),
            name=get_errors_percentage_juggler_service_id(route),
            is_prod=is_prod,
            children=[
                Child(
                    host=f"(host={cluster})&service=matrix_scheduler.{get_errors_percentage_juggler_service_id(route)}",
                    service="all",
                    group_type="EVENTS",
                    instance="all",
                ),
            ],
        )

    if cluster == "scheduler_production" and is_prod:
        alert = autoconf.build_solomon_ydb_userpool_cpu_utilization_alert(
            "ydb_userpool_cpu_utilization",
            "ydb_userpool_cpu_utilization",
            "/ru/speechkit_ops_alice_notificator/prod/matrix-queue-common",
        )
        autoconf.create_or_update_solomon_alert(alert)
        yield autoconf.build_juggler_check(
            cluster="ydb",
            alert_id="ydb_userpool_cpu_utilization",
            name="ydb_userpool_cpu_utilization",
            is_prod=True,
            children=[
                Child(
                    host="(host=ydb)&service=matrix_scheduler.ydb_userpool_cpu_utilization",
                    service="all",
                    group_type="EVENTS",
                    instance="all",
                ),
            ],
        )

        alert = autoconf.build_solomon_ydb_storage_utilization_alert(
            "ydb_storage_utilization",
            "ydb_storage_utilization",
            "/ru/speechkit_ops_alice_notificator/prod/matrix-queue-common",
        )
        autoconf.create_or_update_solomon_alert(alert)
        yield autoconf.build_juggler_check(
            cluster="ydb",
            alert_id="ydb_storage_utilization",
            name="ydb_storage_utilization",
            is_prod=True,
            children=[
                Child(
                    host="(host=ydb)&service=matrix_scheduler.ydb_storage_utilization",
                    service="all",
                    group_type="EVENTS",
                    instance="all",
                ),
            ],
        )


def main():
    with JugglerApi(
        "http://juggler-api.search.yandex.net",
        mark="matrix_scheduler_checks",
        oauth_token=os.environ["JUGGLER_TOKEN"],
    ) as api:
        autoconf = AutoConf()

        update_golovan(autoconf)

        for cluster in TESTING_CLUSTERS:
            for check in update_solomon_and_get_juggler_checks(autoconf, cluster, is_prod=False):
                api.upsert_check(check)

        for cluster in PROD_CLUSTERS:
            for check in update_solomon_and_get_juggler_checks(autoconf, cluster, is_prod=True):
                api.upsert_check(check)
