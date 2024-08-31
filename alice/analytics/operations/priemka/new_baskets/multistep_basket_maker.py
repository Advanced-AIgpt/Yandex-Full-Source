import re
import math
from collections import defaultdict
import random
from copy import deepcopy


def random_part(i):
    return ''.join(map(lambda x: random.choice("0123456789abcdef"), range(i)))


def generate_id(req_id):
    return ''.join(['ffffffff-ffff-ffff-', random_part(4), '-', random_part(12)])


def generate_for_step(step_number, data_by_types, whole_data, params):
    size = len(data_by_types[f"step{step_number}"])
    steps_data = []
    steps_data.append(random.sample(data_by_types[f"step{step_number}"], size))
    for step in range(step_number - 1, 0, -1):
        valid_items = [item for item in whole_data[f"step{step}"] if
                       re.search(params['regexp_valid_for_steps'][step - 1], item['text'])]
        steps_data.append(random.choices(valid_items, k=size))

    for sequence in zip(*steps_data):

        session_id = generate_id(sequence[0]['request_id'])
        for i in range(len(sequence)):
            item = {
                'request_id_real': sequence[i]['request_id'],
                'request_id': generate_id(sequence[i]['request_id']),
                'session_id': session_id,
                'reversed_session_sequence': i,
                'type': sequence[i]['type'],
                'session_type': f"step{step_number}",
                'text': sequence[i]['text']
            }
            yield item


def generate_exit(data_by_types, whole_data, params):
    max_step = max(map(lambda x: int(x.strip("step")), [item for item in data_by_types.keys() if "step" in item]))
    size = len(data_by_types["exit"])
    steps_data = []
    steps_data.append(random.sample(data_by_types["exit"], size))
    for step in range(max_step, 0, -1):
        valid_items = [item for item in whole_data[f"step{step}"] if
                       re.search(params['regexp_valid_for_steps'][step - 1], item['text'])]
        steps_data.append(random.choices(valid_items, k=size))
    for sequence in zip(*steps_data):
        session_id = generate_id(sequence[0]['request_id'])
        sequence = [sequence[0]] + list(sequence[random.randint(1, max_step):])
        for i in range(len(sequence)):
            item = {
                'request_id_real': sequence[i]['request_id'],
                'request_id': generate_id(sequence[i]['request_id']),
                'session_id': session_id,
                'reversed_session_sequence': i,
                'type': sequence[i]['type'],
                'session_type': 'exit',
                'text': sequence[i]['text'],
            }
            yield item


def fix_input_according_to_splitting_methods(data_by_types, params):
    basket_partitioning = params.get('basket_partitioning')
    if not basket_partitioning:
        return
    if 'whole' in basket_partitioning:
        return
    if 'equal' in basket_partitioning:
        smallest_basket_len = min([len(data_by_types[key]) for key in data_by_types])
        biggest_basket_len = max([len(data_by_types[key]) for key in data_by_types])
        if smallest_basket_len < 100:
            raise RuntimeError(
                f"Your smallest basket is way to small. Should be at least 100, but your basket len is {smallest_basket_len}")
        if biggest_basket_len / smallest_basket_len > 2:
            import sys
            sys.stderr.write("\n This is really sad. Your biggest basket is way bigger, than your smallest basket."
                             "The majority of your big basket will be thrown away. Maybe choose another option?\n")

        for key in data_by_types:
            data_by_types[key] = random.sample(data_by_types[key], smallest_basket_len)
    else:
        percents = list(map(float, basket_partitioning.split(', ')))
        total = sum(percents)
        if len(percents) != len(data_by_types):
            raise RuntimeError(f"You should input {len(data_by_types)} numbers, your input: {len(percents)} numbers")
        if total != 100 and total != 1:
            import sys
            sys.stderr.write(f"Your percentages sum = {total}. It should be 1 or 100")
        if total == 0:
            raise RuntimeError(f"Your total is 0. It should be 100")
        has_exit = 'exit' in data_by_types
        percents = {f"step{i}" if (i != len(data_by_types) or not has_exit) else "exit": percents[i - 1] / total for i in
                    range(1, len(data_by_types) + 1)}
        estimated_basket_size = {
            step: len(data_by_types[step]) / percents[step] if percents[step] != 0 else float('inf') for step in
            data_by_types}
        real_basket_size = estimated_basket_size[min(estimated_basket_size, key=estimated_basket_size.get)]
        for key in data_by_types:
            data_by_types[key] = random.sample(data_by_types[key], math.floor(real_basket_size * percents[key]))


def generate_basket_with_long_queries(data, params):
    '''
    data: list of dicts. Each dict has request_id, text and type. Type can be 'step{i}' or 'exit'
    params:
    regexp_valid_for_steps includes a list of regexps for each 'step{i}', which match valid texts for these steps.
        Valid texts will be chosen from your input baskets and long sequences will contain only these valid texts for context.
    basket_partitioning:
    Optional. Can be one of "whole", "equal", or list of numbers (same amount, as baskets). Whole will take whole baskets.
    Equal will choose smallest basket, other baskets will be cut to the same size. Percents will determine percentages for each steps.
    '''
    data_by_types = defaultdict(list)
    for item in data:
        data_by_types[item["type"]].append(item)
    whole_data = deepcopy(data_by_types)
    fix_input_according_to_splitting_methods(data_by_types, params)
    max_step = max(map(lambda x: int(x.strip("step")), [item for item in data_by_types.keys() if "step" in item]))
    for i in range(1, max_step + 1):
        if not f"step{i}" in data_by_types:
            raise RuntimeError(f"You forgot to enter basket for step {i}")
    sample = []
    for i in range(1, max_step + 1):
        sample += list(generate_for_step(i, data_by_types, whole_data, params))
    if 'exit' in data_by_types:
        sample += generate_exit(data_by_types, whole_data, params)
    return sample
