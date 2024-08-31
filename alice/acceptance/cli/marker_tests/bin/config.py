# coding: utf-8

import attrdict
import attr
import yaml

import vh


def init_config(config_path):
    with open(config_path) as f:
        conf_dict = yaml.safe_load(f.read())
    return attrdict.AttrDict(conf_dict)


@attr.s
class GlobalOptions:
    cache_sync = attr.ib()

    yt_token = attr.ib()
    yt_proxy = attr.ib()
    yt_pool = attr.ib()
    mr_account = attr.ib()
    mr_output_ttl = attr.ib()
    mr_output_path = attr.ib()

    yql_token = attr.ib()
    arcanum_token = attr.ib()

    uniproxy_url = attr.ib()
    vins_url = attr.ib()
    ya_plus_token = attr.ib()


global_options = GlobalOptions(
    cache_sync=vh.add_global_option('cache_sync', 'integer', default=0),

    yt_token=vh.add_global_option('yt-token', vh.Secret, required=True),
    yt_proxy=vh.add_global_option('yt-proxy', 'string', required=True),
    yt_pool=vh.add_global_option('yt-pool', 'string', default=None),
    mr_account=vh.add_global_option('mr-account', 'string', required=True),
    mr_output_ttl=vh.add_global_option('mr-output-ttl', 'integer', required=True),
    mr_output_path=vh.add_global_option('mr-output-path', 'string', default=''),

    yql_token=vh.add_global_option('yql-token', vh.Secret, required=True),
    arcanum_token=vh.add_global_option('arcanum-token', vh.Secret, required=True),

    uniproxy_url=vh.add_global_option('uniproxy-url', 'string', required=True),
    vins_url=vh.add_global_option('vins-url', 'string', default=''),
    ya_plus_token=vh.add_global_option('ya-plus-token', vh.Secret, required=True),
)
