import os

import attr
import attrdict
import vh
import yaml

from deepmerge import always_merger as dict_merger

from library.python import resource


@attr.s
class GlobalOptions:
    yql_token = attr.ib()
    ya_plus_token = attr.ib()
    cache_sync = attr.ib()
    soy_token = attr.ib()


def init_config(user_config_path):
    conf_dict = yaml.safe_load(resource.find('/config.yaml'))
    expand = os.path.expanduser('~')
    user_config_path = user_config_path.replace('~', expand)
    if os.path.exists(user_config_path):
        with open(user_config_path) as f:
            conf_dict = dict_merger.merge(conf_dict, yaml.safe_load(f))
    return attrdict.AttrDict(conf_dict)


global_options = GlobalOptions(
    yql_token=vh.add_global_option('yql_token', vh.Secret, required=True),
    ya_plus_token=vh.add_global_option('user_oauth', vh.Secret, required=True),
    cache_sync=vh.add_global_option('cache_sync', 'integer', required=True),
    soy_token=vh.add_global_option('soy_token', 'secret', required=True),
)
