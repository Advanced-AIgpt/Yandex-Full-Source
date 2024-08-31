import argparse
import json

from collections import Counter


def get_config():
    config = {
        'language': 'ru',
        'cases': [],
    }

    return config


def get_short_form(form):
    return form.split('.')[-1]


def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--forms', type=str, nargs='*', default=['<DEFINE FORM>'])
    parser.add_argument('--dataset-name', type=str, default='<DEFINE DATASET>')
    parser.add_argument('--output', type=str, default='config.json')
    parser.add_argument('--has-negatives', action='store_true')

    return parser.parse_args()


def main():
    args = get_args()
    config = get_config()

    short_form_counter = Counter([get_short_form(form) for form in args.forms])
    for form in args.forms:
        short_form = get_short_form(form)
        case = {
            'name': short_form if short_form_counter[short_form] < 2 else form,
            'form': form,
            'positive': 'base/{}.tsv'.format(args.dataset_name),
            'consider_slots': False,
            'disable_auto_test': True,
            'collect_blockers': True,
        }
        if args.has_negatives:
            case['negative'] = 'target/{}.neg.tsv'.format(args.dataset_name)
        config['cases'].append(case)

    with open(args.output, 'w') as f:
        json.dump(config, f, indent=2)
        f.write('\n')
