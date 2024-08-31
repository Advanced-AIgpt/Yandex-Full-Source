import yaml
from vins_core.utils.data import load_data_from_file


def load_config(stream):
    structure = load_data_from_file(stream, yaml_loader=yaml.SafeLoader)
    name = structure.keys()[0]
    config = {
        'name': name,
        'structure': structure[name]
    }
    return config
