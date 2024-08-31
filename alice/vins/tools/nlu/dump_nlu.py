# coding:utf-8

import argparse
import logging
import os

from vins_tools.nlu.inspection.nlu_processing_on_dataset import create_app, get_vinsfile_for_app


logger = logging.getLogger(__name__)


def do_main():
    os.environ['VINS_LOAD_TF_ON_CALL'] = '1'

    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--app', type=str, default='personal_assistant',
                        help="The name of the app to print nlu's from.")
    parser.add_argument('--count', action='store_true',
                        help='Count the number of generated lines instead of printing these lines')
    parser.add_argument('--intentSubstring', type=str, default='',
                        help='Filter intents by substring')
    args = parser.parse_args()

    vins_file, _ = get_vinsfile_for_app(args.app)
    app_obj = create_app(args.app, vins_file, force_rebuild=False, load_custom_entities=False)

    for intent_name, nlu_source_items in app_obj.nlu.nlu_sources_data.iteritems():
        if args.intentSubstring and args.intentSubstring not in intent_name:
            continue
        if args.count:
            print('{}\t{}'.format(intent_name, len(nlu_source_items)))
        else:
            for nlu_source_item in nlu_source_items:
                print('{}\t{}'.format(nlu_source_item.text.encode('utf-8'), intent_name))


if __name__ == '__main__':
    do_main()
