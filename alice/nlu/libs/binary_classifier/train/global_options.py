# coding: utf-8

import attr
import vh


@attr.s
class GlobalOptions(object):
    yt_token = attr.ib()
    yql_token = attr.ib()
    yt_cluster = attr.ib()
    mr_account = attr.ib()
    yt_pool = attr.ib()
    sandbox_owner = attr.ib()
    sandbox_token = attr.ib()


nirvana_global_options = GlobalOptions(
    yt_token=vh.add_global_option('yt_token', type=vh.Secret, required=True),
    yql_token=vh.add_global_option('yql_token', type=vh.Secret, required=True),
    yt_cluster=vh.add_global_option('yt_cluster', type=str, default=None),
    mr_account=vh.add_global_option('mr_account', type=str, default=None),
    yt_pool=vh.add_global_option('yt_pool', type=str, default=None),
    sandbox_owner=vh.add_global_option('sandbox_owner', type=str, default=None),
    sandbox_token=vh.add_global_option('sandbox_token', type=vh.Secret, default=None),
)
