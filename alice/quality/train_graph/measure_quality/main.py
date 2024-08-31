# -*- coding: utf-8 -*-

import argparse
import collections
import json
import logging


def load_data(path, name, catboost):
    logging.info('Collect %s', name)

    result = []
    with open(path, 'r') as predicts:
        for idx, row in enumerate(predicts):
            if idx == 0 and catboost:
                continue

            if catboost:
                i, predict, label = row.rstrip().split('\t')
                assert idx == int(i) + 1, '{}, {}'.format(idx, i)
            else:
                _, label, text, group, predict = row.rstrip().split('\t')
                if int(group):
                    predict = -1000

            result.append((float(predict), int(label), name))
    return result


def add_scenario(scenarios, postive_labels, path, name, catboost):
    if path is None:
        return 0
    scenarios[name] = load_data(path, name, catboost)
    count = len(scenarios[name])
    assert count > 0
    postive_labels[name] = sum([label for _, label, _ in scenarios[name]])
    for key in scenarios:
        assert len(scenarios[key]) == count
    return count


def run(args):
    log_level = logging.INFO if args.verbose else logging.ERROR
    logging.basicConfig(format='%(levelname)-8s %(asctime)-27s %(message)s', level=log_level)

    scenarios = {}
    postive_labels = {}
    scenarios_input = json.loads(args.scenarios_input)
    for scenario, path in scenarios_input.items():
        count = add_scenario(scenarios, postive_labels, path, scenario, args.catboost)

    true_positive = 0.0
    true_positives = collections.defaultdict(float)
    positives = collections.defaultdict(int)
    requests_winners = []
    discarded_scenarios = []
    for idx in range(count):
        discarded = []
        for name in scenarios:
            if scenarios[name][idx][0] == -1000:
                discarded.append(name)
        discarded_scenarios.append(discarded)

        _, label, name = sorted([scenarios[name][idx] for name in scenarios])[-1]
        true_positive += label
        true_positives[name] += label
        positives[name] += 1
        requests_winners.append(name)

    result = {
        'precision_' + key: (true_positives[key] / positives[key] if positives[key] > 0 else 'NaN') for key in scenarios
    }
    for key in scenarios:
        result['recall_' + key] = (true_positives[key] / postive_labels[key]) if postive_labels[key] > 0 else 'NaN'

    result['accuracy'] = true_positive / count

    logging.info('Accuracy: %f', result['accuracy'])
    for key in scenarios:
        logging.info('%s precision: %f', key, result['precision_' + key])
        logging.info('%s recall: %f', key, result['recall_' + key])

    with open(args.result, 'w') as metrics:
        json.dump(
            result,
            metrics,
            sort_keys=True,
            indent=4,
        )

    with open(args.requests_winners, 'w') as TSV_file:
        TSV_file.write('\n'.join(["{}\t{}\t{}".format(id, requests_winners[id], ','.join(discarded_scenarios[id])) for id in range(len(requests_winners))]) + '\n')

    with open(args.scenario_list, 'w') as scenario_list_file:
        scenarios = set(requests_winners)
        if 'search' in scenarios:
            scenarios.remove('search')
            scenario_list = list(scenarios)
            scenario_list.append('search')
        else:
            scenario_list = list(scenarios)
        json.dump({"scenarios": scenario_list}, scenario_list_file)


def main():
    argument_parser = argparse.ArgumentParser()

    argument_parser.add_argument(
        '--scenarios_input',
        required=True,
        help='Dict scenario:predicts_file',
    )
    argument_parser.add_argument(
        '-r', '--result',
        required=True,
        help='result metric',
    )
    argument_parser.add_argument(
        '--requests_winners',
        required=True,
        help='TSV with winner scenario for each request',
    )
    argument_parser.add_argument(
        '--scenario_list',
        required=True,
        help='JSON with list of scenarios',
    )
    argument_parser.add_argument(
        '-c', '--catboost',
        action='store_true',
        help='catboost input file format (mx_ops otherwise)',
    )
    argument_parser.add_argument(
        '--verbose',
        action='store_true',
        help='report progress',
    )

    run(argument_parser.parse_args())
