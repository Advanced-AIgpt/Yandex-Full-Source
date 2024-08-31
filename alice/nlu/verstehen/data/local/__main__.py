import argparse
import logging

import verstehen.data.local.prepare_metrics_data as prepare_experiments_data
import verstehen.data.local.prepare_prod_data as prepare_big_data

logger = logging.getLogger(__name__)

if __name__ == '__main__':
    logging.basicConfig(format='%(asctime)s %(levelname)s:%(name)s %(message)s', level=logging.DEBUG)

    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers()

    # Convert toloka data route
    subparser = subparsers.add_parser('metrics_data')
    prepare_experiments_data.configure_parser(subparser)
    subparser.set_defaults(main=prepare_experiments_data.main)

    # Prepare data route
    subparser = subparsers.add_parser('prod_data')
    prepare_big_data.configure_parser(subparser)
    subparser.set_defaults(main=prepare_big_data.main)

    args = parser.parse_args()
    args.main(args)
