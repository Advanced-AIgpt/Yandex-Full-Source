# coding: utf-8
import json
import argparse

from generators.time import GeneratorTime

GENERATORS_BY_TYPE = {
    'time': GeneratorTime
}

if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--entity-type', required=True, help='Entity type.',
                        dest='entity_type', choices=GENERATORS_BY_TYPE.keys())
    parser.add_argument('--out', required=True, help='Output dictionary file path.', dest='out')
    args = parser.parse_args()

    generator = GENERATORS_BY_TYPE[args.entity_type]()
    pattern_to_value = generator.generate()

    with open(args.out, 'w') as f:
        f.write('\n'.join(('\t'.join([pattern, value]) for pattern, value in pattern_to_value.items())))
