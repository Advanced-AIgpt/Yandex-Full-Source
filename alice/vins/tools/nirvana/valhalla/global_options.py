# coding: utf-8

import attr

import vh


@attr.s
class GlobalOptions(object):
    yt_token = attr.ib()
    yql_token = attr.ib()
    yt_cluster = attr.ib()
    mr_account = attr.ib()
    mr_output_ttl = attr.ib()
    yt_pool = attr.ib()

    sandbox_token = attr.ib()
    sandbox_resources_owner = attr.ib()
    sandbox_vault_owner_yt_token = attr.ib()
    sandbox_vault_name_yt_token = attr.ib()

    scraper_pool = attr.ib()
    yaplus_token = attr.ib()


vins_global_options = GlobalOptions(
    yt_token=vh.add_global_option('yt_token', vh.Secret, required=True),
    yql_token=vh.add_global_option('yql_token', vh.Secret, required=True),
    yt_cluster=vh.add_global_option('yt_cluster', str, default=None),
    mr_account=vh.add_global_option('mr_account', str, default=None),
    mr_output_ttl=vh.add_global_option('mr_output_ttl', int, default=None),
    yt_pool=vh.add_global_option('yt_pool', str, default=None),
    sandbox_token=vh.add_global_option('sandbox_token', vh.Secret, required=True),
    sandbox_resources_owner=vh.add_global_option('sandbox_resources_owner', str, default=None),
    sandbox_vault_owner_yt_token=vh.add_global_option('sandbox_vault_owner_yt_token', str, default=None),
    sandbox_vault_name_yt_token=vh.add_global_option('sandbox_vault_name_yt_token', str, default=None),
    scraper_pool=vh.add_global_option('scraper_pool', str, default=None),
    yaplus_token=vh.add_global_option('yaplus_token', vh.Secret, required=True),
)
