import argparse
import collections
import json
import sys
import nirvana.job_context as nv


def main():
    result = 0
    for input_file in nv.context().get_inputs().get_item_list('infiles'):
        with open(input_file.get_path()) as f:
            if input_file.get_link_name() == 'cut':
                result += json.load(f)['total_cuts']
            else:
                result += json.load(f)['accuracy'] * 100

    with open(nv.context().get_outputs().get('out_json'), 'w') as jfile:
        json.dump({'result': result}, jfile)


main()
