import os
import string
from tempfile import NamedTemporaryFile
from traitlets import Unicode
from urllib.parse import unquote

import escapism
# import toml

from contextlib import contextmanager
from collections import namedtuple


class KVStorePrefix(Unicode):
    def validate(self, obj, value):
        u = super().validate(obj, value)
        if not u.endswith("/"):
            u = u + "/"

        proxy_class = type(obj).__name__
        if "Consul" in proxy_class and u.startswith("/"):
            u = u[1:]

        return u


def generate_rule(routespec):
    routespec = unquote(routespec)
    if routespec.startswith("/"):
        # Path-based route, e.g. /proxy/path/
        rule = "".join([
            "PathPrefix(`",
            routespec,
            "`)"
        ])
    else:
        # Host-based routing, e.g. host.tld/proxy/path/
        host, path_prefix = routespec.split("/", 1)
        path_prefix = "/" + path_prefix
        rule = " && ".join([
            "".join([
               "Host(`",
               host,
               "`)"
            ]),
            "".join([
                "PathPrefix(`",
                path_prefix,
                "`)"
            ])
        ])
    return rule


def generate_alias(routespec, server_type=""):
    safe = string.ascii_letters + string.digits + "_-"
    return server_type + "_" + escapism.escape(routespec, safe=safe)


def generate_service_entry(
    proxy, service_alias, separator="/", url=False, weight=False
):
    service_entry = ""
    if separator == "/":
        service_entry = proxy.kv_traefik_prefix
    service_entry += separator.join([
        "http", "services", service_alias, "loadbalancer", "servers", "0"
    ])
    if url is True:
        service_entry += separator + "url"
    elif weight is True:
        service_entry += separator + "weight"

    return service_entry


def generate_router_service_entry(proxy, router_alias, separator="/"):
    return proxy.kv_traefik_prefix + separator.join(
        ["http", "routers", router_alias, "service"]
    )


def generate_router_rule_entry(proxy, router_alias, separator="/"):
    router_rule_entry = separator.join(
        ["http", "routers", router_alias]
    )
    if separator == "/":
        router_rule_entry = (
            proxy.kv_traefik_prefix + router_rule_entry + separator + "rule"
        )

    return router_rule_entry


def generate_route_keys(proxy, routespec, separator="/"):
    service_alias = generate_alias(routespec, "service")
    router_alias = generate_alias(routespec, "router")

    RouteKeys = namedtuple(
        "RouteKeys",
        [
            "service_alias",
            "service_url_path",
            "router_alias",
            "router_service_path",
            "router_rule_path",
        ],
    )

    if separator != ".":
        service_url_path = generate_service_entry(proxy, service_alias, url=True)
        router_rule_path = generate_router_rule_entry(proxy, router_alias)
        router_service_path = generate_router_service_entry(proxy, router_alias)
    else:
        service_url_path = generate_service_entry(
            proxy, service_alias, separator=separator
        )
        router_rule_path = generate_router_rule_entry(
            proxy, router_alias, separator=separator
        )
        # service_weight_path = ""
        router_service_path = ""

    return RouteKeys(
        service_alias,
        service_url_path,
        router_alias,
        router_service_path,
        router_rule_path,
    )


# atomic writing adapted from jupyter/notebook 5.7
# unlike atomic writing there, which writes the canonical path
# and only use the temp file for recovery,
# we write the temp file and then replace the canonical path
# to ensure that traefik never reads a partial file


@contextmanager
def atomic_writing(path):
    """Write temp file before copying it into place

    Avoids a partial file ever being present in `path`,
    which could cause traefik to load a partial routing table.
    """
    fileobj = NamedTemporaryFile(
        prefix=os.path.abspath(path) + "-tmp-", mode="w", delete=False
    )
    try:
        with fileobj as f:
            yield f
        os.replace(fileobj.name, path)
    finally:
        try:
            os.unlink(fileobj.name)
        except FileNotFoundError:
            # already deleted by os.replace above
            pass


# def persist_static_conf(file, static_conf_dict):
#     with open(file, "w") as f:
#         toml.dump(static_conf_dict, f)
#
#
# def persist_routes(file, routes_dict):
#     with atomic_writing(file) as config_fd:
#         toml.dump(routes_dict, config_fd)
#
#
# def load_routes(file):
#     try:
#         with open(file, "r") as config_fd:
#             return toml.load(config_fd)
#     except:
#         raise
