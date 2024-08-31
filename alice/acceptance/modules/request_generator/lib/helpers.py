import json
import urllib.parse


def decode_value(value):
    if isinstance(value, dict):
        new_value = {}
        for (k, v) in value.items():
            new_value[decode_value(k)] = decode_value(v)
        return new_value
    elif isinstance(value, list):
        return [decode_value(elem) for elem in value]
    elif isinstance(value, bytes):
        try:
            return value.decode('utf-8', 'ignore')
        except UnicodeDecodeError:
            return value
    else:
        return value


def encode_value(value):
    if isinstance(value, dict):
        new_value = {}
        for (k, v) in value.items():
            new_value[encode_value(k)] = encode_value(v)
        return new_value
    elif isinstance(value, list):
        return [encode_value(elem) for elem in value]
    elif isinstance(value, str):
        return value.encode('utf-8')
    else:
        return value


def get_schema(source):
    return [{'name': name, 'type': type} for name, type in source]


def parse_experiments_from_options(experiments_str):
    try:
        return json.loads(experiments_str)
    except json.JSONDecodeError:
        experiments = [e.strip() for e in experiments_str.split(',')]
        result_experiments = {}
        for exp in experiments:
            if not exp:
                continue
            (name, value) = exp.rsplit('=', 1)
            (name, value) = (urllib.parse.unquote(name), urllib.parse.unquote(value))
            if value == 'null':
                value = None
            result_experiments[name] = value
        return result_experiments


def filter_experiments(experiments, filter_experiments):
    if isinstance(experiments, dict):
        return {k: v for k, v in experiments.items() if k not in filter_experiments}

    if isinstance(experiments, list):
        return [v for v in experiments if v not in filter_experiments]

    return experiments
