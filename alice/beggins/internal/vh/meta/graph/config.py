import copy


def _merge_dicts_inplace(accumulator, other):
    for key, value in copy.deepcopy(other).items():
        if (
            key in accumulator
            and isinstance(accumulator[key], dict)
            and isinstance(value, dict)
        ):
            _merge_dicts_inplace(accumulator[key], value)
        else:
            accumulator[key] = value


def _merge_dict_list(dicts):
    accumulator = {}
    for current in dicts:
        _merge_dicts_inplace(accumulator, current)
    return accumulator


def _load_manifest(intent_config, load_yaml_func):
    path = intent_config.get('manifest_path')
    loaded = load_yaml_func(path) if path else {}
    embedded = intent_config.get('manifest', {})
    intent_config['manifest'] = _merge_dict_list([loaded, embedded])


def _load_intent_config(intent, load_yaml_func):
    path = intent.get('config_path')
    loaded = load_yaml_func(path) if path else {}
    embedded = intent.get('config', {})
    _load_manifest(loaded, load_yaml_func)
    _load_manifest(embedded, load_yaml_func)
    intent['config'] = _merge_dict_list([loaded, embedded])


def _load_intent_configs(meta_config, load_yaml_func):
    for intent in meta_config['intents']:
        _load_intent_config(intent, load_yaml_func)


def _adjust_flow_config(meta_config, flow_name, visited):
    if flow_name in visited:
        return
    visited.add(flow_name)
    parents = meta_config['flows'][flow_name].get('parent', [])
    for parent_name in parents:
        _adjust_flow_config(meta_config, parent_name, visited)

    flow = {}
    for parent_name in parents + [flow_name]:
        parent = meta_config['flows'][parent_name]
        for key, value in parent.items():
            if key == 'required_data':
                flow[key] = flow.get(key, []) + value
            elif key in ['defaults', 'override']:
                flow[key] = _merge_dict_list([flow.get(key, {}), value])
            else:
                flow[key] = value
    meta_config['flows'][flow_name] = flow

    intent_configs = []
    defaults = flow.get('defaults', {})
    override = flow.get('override', {})
    for original in meta_config['intents']:
        intent_configs.append(_merge_dict_list([defaults, original['config'], override]))
    flow['intents'] = intent_configs


def _adjust_flow_configs(meta_config):
    visited = set()
    for flow_name in meta_config['flows']:
        _adjust_flow_config(meta_config, flow_name, visited)


def load_meta_config(config_path, load_yaml_func):
    meta_config = load_yaml_func(config_path)
    _load_intent_configs(meta_config, load_yaml_func)
    _adjust_flow_configs(meta_config)
    return meta_config


def get_by_key_path(d, path):
    for key in path.split('.'):
        if not isinstance(d, dict):
            return None
        d = d.get(key)
    return d
