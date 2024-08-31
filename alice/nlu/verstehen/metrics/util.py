import json
import logging

logger = logging.getLogger(__name__)


def get_json_data(path):
    with open(path) as f:
        data = json.load(f)
    return data


def flatten_dict_of_lists(dict_of_lists):
    """
    Given a dict of lists return two lists of the same length where the first list is keys
    and the second one is list of values.
    """

    keys, values = [], []
    for key in dict_of_lists:
        for value in dict_of_lists[key]:
            keys.append(key)
            values.append(value)
    return keys, values
