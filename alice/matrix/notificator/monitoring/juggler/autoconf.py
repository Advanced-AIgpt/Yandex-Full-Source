import os

from alice.matrix.library.monitoring.juggler.autoconf_base import JugglerAutoConfBase

from juggler_sdk import (
    Child,
    JugglerApi,
)


TESTING_CLUSTERS = ["test"]
PROD_CLUSTERS = ["prestable", "production"]

HTTP_ROUTES = [
    "/delivery",
    "/delivery/on_connect",
    "/delivery/push",

    "/devices",

    "/directive/change_status",
    "/directive/status",

    "/locator",

    "/notifications",

    "/subscriptions",
    "/subscriptions/manage",
    "/subscriptions/user_list",
]
HTTP_ROUTES_WITH_SUBWAY = [
    "/delivery",
    "/delivery/demo",
    "/delivery/on_connect",
    "/delivery/push",

    "/gdpr",

    "/notifications/change_status",

    "/subscriptions/devices",
]
HTTP_PROXY_ROUTES = [
    "/delivery/sup",
    "/delivery/sup_card",

    "/personal_cards/delete",
]
APPHOSTED_ROUTES = [
    "/update_connected_clients",
    "/update_device_environment",
]


class AutoConf(JugglerAutoConfBase):
    def __init__(self):
        super(AutoConf, self).__init__(
            "matrix",
            "https://docs.yandex-team.ru/alice-matrix/pages/notificator/development/links",
        )


def route_to_id(route):
    return route[1:].replace("/", "_")


def get_errors_percentage_juggler_service_id(route, weak):
    name = f"errors_percentage_{route_to_id(route)}"
    if weak:
        name += "_weak"

    return name


def get_errors_percentage_alert_id(route, weak, cluster):
    return f"{get_errors_percentage_juggler_service_id(route, weak)}_{cluster}"


def get_errors_percent(is_prod, weak):
    if is_prod:
        return 0.05 if weak else 0.01
    else:
        return 0.33


def update_golovan(autoconf):
    check_name = "matrix.balancer.service_total_fail"

    autoconf.create_or_update_golovan_check(
        check_name,
        autoconf.build_balancer_total_fails_check(
            "notificator.alice.yandex.net",
            check_name,
            is_prod=True,
            crit_from=200,
        ),
    )


def update_solomon_and_get_juggler_checks(autoconf, cluster, is_prod, weak):
    for route in HTTP_ROUTES_WITH_SUBWAY:
        def get_all_lines():
            return autoconf.build_default_solomon_sensor_selector("notificator", "*", f"{route_to_id(route)}.{route[1:]}.response", cluster)

        def get_successes_lines():
            return autoconf.build_default_solomon_sensor_selector("notificator", "2*", f"{route_to_id(route)}.{route[1:]}.response", cluster)

        def get_subway_errors_lines():
            return autoconf.build_default_solomon_sensor_selector("notificator", "*", f"{route_to_id(route)}.subway_client.send_request", cluster)

        alert = autoconf.build_solomon_program_alert(
            get_errors_percentage_alert_id(route, weak, cluster),
            get_errors_percentage_juggler_service_id(route, weak),
            cluster,
            f"{route} HTTP response codes ({autoconf.format_cluster(cluster)}){' (weak)' if weak else ''}",
            f"""
                no_data_if(size({get_all_lines()}) == 0);
                no_data_if(size({get_successes_lines()}) == 0);

                let all_lines_sm = group_lines("sum", {get_all_lines()});
                let successes_lines_sm = group_lines("sum", {get_successes_lines()});
                let subway_errors_lines_sm = group_lines("sum", {get_subway_errors_lines()});

                let all = sum(all_lines_sm);

                let successes = sum(successes_lines_sm);
                let subway_errors = sum(subway_errors_lines_sm);

                let all_errors = all - successes;
                let errors = all - successes - subway_errors;

                let successes_str = {autoconf.double_to_str("successes")};
                let subway_errors_str = {autoconf.double_to_str("subway_errors")};
                let all_errors_str = {autoconf.double_to_str("all_errors")};
                let errors_str = {autoconf.double_to_str("errors")};
                let successes_percent_str = {autoconf.get_percent_str("successes", "all")};
                let subway_errors_percent_str = {autoconf.get_percent_str("subway_errors", "all")};
                let all_errors_percent_str = {autoconf.get_percent_str("all_errors", "all")};
                let errors_percent_str = {autoconf.get_percent_str("errors", "all")};
            """.replace("  ", ""),
            autoconf.build_solomon_check_percent_check_expression(percent=get_errors_percent(is_prod, weak)),
            extra_annotations={
                "description": autoconf.build_solomon_alert_description_annotation([
                    "Successes: {{expression.successes_str}} ({{expression.successes_percent_str}}%)",
                    "Subway errors: {{expression.subway_errors_str}} ({{expression.subway_errors_percent_str}}%)",
                    "All errors: {{expression.all_errors_str}} ({{expression.all_errors_percent_str}}%)",
                    "Errors no subway: {{expression.errors_str}} ({{expression.errors_percent_str}}%)",
                ]),
            },
            resolved_empty_policy="RESOLVED_EMPTY_MANUAL",
        )
        autoconf.create_or_update_solomon_alert(alert)
        yield autoconf.build_juggler_check(
            cluster=cluster,
            alert_id=get_errors_percentage_alert_id(route, weak, cluster),
            name=get_errors_percentage_juggler_service_id(route, weak),
            is_prod=is_prod,
            children=[
                Child(
                    host=f"(host={cluster})&service=matrix.{get_errors_percentage_juggler_service_id(route, weak)}",
                    service="all",
                    group_type="EVENTS",
                    instance="all",
                ),
            ],
            weak=weak,
        )

    for route in HTTP_ROUTES:
        if route in HTTP_ROUTES_WITH_SUBWAY:
            continue

        alert = autoconf.build_solomon_program_alert(
            get_errors_percentage_alert_id(route, weak, cluster),
            get_errors_percentage_juggler_service_id(route, weak),
            cluster,
            f"{route} HTTP response codes ({autoconf.format_cluster(cluster)}){' (weak)' if weak else ''}",
            f"""
                let all_lines_sm = group_lines("sum", {autoconf.build_default_solomon_sensor_selector("notificator", "*", f"{route_to_id(route)}.{route[1:]}.response", cluster)});
                let successes_lines_sm = group_lines("sum", {autoconf.build_default_solomon_sensor_selector("notificator", "2*", f"{route_to_id(route)}.{route[1:]}.response", cluster)});

                let all = sum(all_lines_sm);
                let successes = sum(successes_lines_sm);
                let errors = all - successes;

                let successes_str = {autoconf.double_to_str("successes")};
                let errors_str = {autoconf.double_to_str("errors")};
                let successes_percent_str = {autoconf.get_percent_str("successes", "all")};
                let errors_percent_str = {autoconf.get_percent_str("errors", "all")};
            """.replace("  ", ""),
            autoconf.build_solomon_check_percent_check_expression(percent=get_errors_percent(is_prod, weak)),
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
            alert_id=get_errors_percentage_alert_id(route, weak, cluster),
            name=get_errors_percentage_juggler_service_id(route, weak),
            is_prod=is_prod,
            children=[
                Child(
                    host=f"(host={cluster})&service=matrix.{get_errors_percentage_juggler_service_id(route, weak)}",
                    service="all",
                    group_type="EVENTS",
                    instance="all",
                ),
            ],
            weak=weak,
        )

    for route in HTTP_PROXY_ROUTES:
        alert = autoconf.build_solomon_program_alert(
            get_errors_percentage_alert_id(route, weak, cluster),
            get_errors_percentage_juggler_service_id(route, weak),
            cluster,
            f"{route} HTTP proxy response codes ({autoconf.format_cluster(cluster)}){' (weak)' if weak else ''}",
            f"""
                let all_lines_sm = group_lines("sum", {autoconf.build_default_solomon_sensor_selector("notificator", "*", f"proxy.{route[1:]}.response", cluster)});
                let successes_lines_sm = group_lines("sum", {autoconf.build_default_solomon_sensor_selector("notificator", "2*", f"proxy.{route[1:]}.response", cluster)});

                let all = sum(all_lines_sm);
                let successes = sum(successes_lines_sm);
                let errors = all - successes;

                let successes_str = {autoconf.double_to_str("successes")};
                let errors_str = {autoconf.double_to_str("errors")};
                let successes_percent_str = {autoconf.get_percent_str("successes", "all")};
                let errors_percent_str = {autoconf.get_percent_str("errors", "all")};
            """.replace("  ", ""),
            autoconf.build_solomon_check_percent_check_expression(percent=get_errors_percent(is_prod, weak)),
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
            alert_id=get_errors_percentage_alert_id(route, weak, cluster),
            name=get_errors_percentage_juggler_service_id(route, weak),
            is_prod=is_prod,
            children=[
                Child(
                    host=f"(host={cluster})&service=matrix.{get_errors_percentage_juggler_service_id(route, weak)}",
                    service="all",
                    group_type="EVENTS",
                    instance="all",
                ),
            ],
            weak=weak,
        )

    for route in APPHOSTED_ROUTES:
        alert = autoconf.build_solomon_program_alert(
            get_errors_percentage_alert_id(route, weak, cluster),
            get_errors_percentage_juggler_service_id(route, weak),
            cluster,
            f"{route} apphost responses ({autoconf.format_cluster(cluster)}){' (weak)' if weak else ''}",
            f"""
                let all_lines_sm = group_lines("sum", {autoconf.build_default_solomon_sensor_selector("notificator", "*", f"{route_to_id(route)}.self.response", cluster)});
                let successes_lines_sm = group_lines("sum", {autoconf.build_default_solomon_sensor_selector("notificator", "ok", f"{route_to_id(route)}.self.response", cluster)});

                let all = sum(all_lines_sm);
                let successes = sum(successes_lines_sm);
                let errors = all - successes;

                let successes_str = {autoconf.double_to_str("successes")};
                let errors_str = {autoconf.double_to_str("errors")};
                let successes_percent_str = {autoconf.get_percent_str("successes", "all")};
                let errors_percent_str = {autoconf.get_percent_str("errors", "all")};
            """.replace("  ", ""),
            autoconf.build_solomon_check_percent_check_expression(percent=get_errors_percent(is_prod, weak)),
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
            alert_id=get_errors_percentage_alert_id(route, weak, cluster),
            name=get_errors_percentage_juggler_service_id(route, weak),
            is_prod=is_prod,
            children=[
                Child(
                    host=f"(host={cluster})&service=matrix.{get_errors_percentage_juggler_service_id(route, weak)}",
                    service="all",
                    group_type="EVENTS",
                    instance="all",
                ),
            ],
            weak=weak,
        )

    if cluster == "production" and is_prod and not weak:
        alert = autoconf.build_solomon_ydb_userpool_cpu_utilization_alert(
            "ydb_userpool_cpu_utilization",
            "ydb_userpool_cpu_utilization",
            "/ru/alice/prod/notificator",
        )
        autoconf.create_or_update_solomon_alert(alert)
        yield autoconf.build_juggler_check(
            cluster="ydb",
            alert_id="ydb_userpool_cpu_utilization",
            name="ydb_userpool_cpu_utilization",
            is_prod=True,
            children=[
                Child(
                    host="(host=ydb)&service=matrix.ydb_userpool_cpu_utilization",
                    service="all",
                    group_type="EVENTS",
                    instance="all",
                ),
            ],
        )

        alert = autoconf.build_solomon_ydb_storage_utilization_alert(
            "ydb_storage_utilization",
            "ydb_storage_utilization",
            "/ru/alice/prod/notificator",
        )
        autoconf.create_or_update_solomon_alert(alert)
        yield autoconf.build_juggler_check(
            cluster="ydb",
            alert_id="ydb_storage_utilization",
            name="ydb_storage_utilization",
            is_prod=True,
            children=[
                Child(
                    host="(host=ydb)&service=matrix.ydb_storage_utilization",
                    service="all",
                    group_type="EVENTS",
                    instance="all",
                ),
            ],
        )


def main():
    with JugglerApi(
        "http://juggler-api.search.yandex.net",
        mark="matrix_checks",
        oauth_token=os.environ["JUGGLER_TOKEN"],
    ) as api:
        autoconf = AutoConf()

        update_golovan(autoconf)

        for cluster in TESTING_CLUSTERS:
            for check in update_solomon_and_get_juggler_checks(autoconf, cluster, is_prod=False, weak=False):
                api.upsert_check(check)

        for cluster in PROD_CLUSTERS:
            for weak in (False, True):
                for check in update_solomon_and_get_juggler_checks(autoconf, cluster, is_prod=True, weak=weak):
                    api.upsert_check(check)
