def set_defaults_to_indexes_configs(node, indexes_defaults):
    """
    Given JSON node it traverses the whole tree recursively and if dict with `index_type` field,
    it fills necessary defaults if they are absent depending on what `index_type` this is.
    """

    if isinstance(node, dict) and 'index_type' in node:
        # putting the default values if not specified to index config
        defaults = indexes_defaults.get(node['index_type'], dict())
        for key, value in defaults.items():
            if key not in node:
                node[key] = value

    # recursive traversing
    children = []
    if isinstance(node, (list, tuple)):
        children = node
    elif isinstance(node, dict):
        children = node.values()
    for child_node in children:
        set_defaults_to_indexes_configs(child_node, indexes_defaults)


def validate_server_config(config):
    necessary_fields = ['server', 'apps']

    config_keys = config.keys()
    for field in necessary_fields:
        if field not in config_keys:
            raise ValueError('Not found mandatory `{}` config part'.format(field))

    indexes_defaults = config.get('indexes_defaults', {})

    # adding general defaults to each JSON config
    set_defaults_to_indexes_configs(config['apps'], indexes_defaults)
    return config
