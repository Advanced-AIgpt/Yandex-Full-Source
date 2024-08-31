import argparse
import logging

import alice.apphost.graph_generator.combinator as combinator
import alice.apphost.graph_generator.rpc_handler as rpc_handler
import alice.apphost.graph_generator.scenario as scenario


def main(args):
    logging.basicConfig(level=logging.INFO)
    graph_path = 'apphost/conf/verticals/ALICE'
    config_path = 'alice/megamind/configs'

    scenario.generate(
        config_path,
        graph_path,
        force_if=lambda _: args.all or _ == args.name,
        use_deprecated=args.deprecated,
    )

    combinator.generate(
        config_path,
        graph_path,
    )

    rpc_handler.generate(
        config_path,
        graph_path,
    )


def parse_args(args=None):
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('-a', '--all', action='store_true', help='force to rebuild all apphost graph configs')
    parser.add_argument('-n', '--name', default=None, help='force to rebuild specific scenario graph')
    parser.add_argument('-d', '--deprecated', action='store_true', help='use deprecated scenario run graph')
    return parser.parse_args(args)


if __name__ == '__main__':
    main(parse_args())
