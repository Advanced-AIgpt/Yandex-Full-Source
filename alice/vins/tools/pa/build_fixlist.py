# coding: utf-8

import argparse
import codecs
import os
import re
import sys
import yaml

from vins_core.utils.data import load_data_from_file


def read_regexes(filename, add_word_boundaries, add_arbitrary_prefix, add_arbitrary_suffix):
    result = set()

    for line in codecs.open(filename, 'r', 'utf-8'):
        line = line.strip()

        if line == '' or line.startswith('#'):
            continue

        regex = line

        if add_word_boundaries:
            if not regex.startswith(r'\b'):
                regex = r'\b' + regex
            if not regex.endswith(r'\b'):
                regex = regex + r'\b'

        if not regex.startswith('.*') and add_arbitrary_prefix:
            regex = '.*' + regex
        if not regex.endswith('.*') and add_arbitrary_suffix:
            regex = regex + '.*'

        # Try to compile
        try:
            compiled_regex = re.compile(regex)
        except re.error as e:
            print>>sys.stderr, 'Cannot compile regex "%s" from %s' % (regex, filename)
            print>>sys.stderr, 'Error: %s' % e

        if compiled_regex.groups > 0:
            print>> sys.stderr, 'Regex "%s" from %s seems to have a non-capturing group' % (regex, filename)

        result.add(regex)

    return result


def main():
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--manifest-file', metavar='FILE', required=True,
                        help='A manifest file describing the fixlist to build')
    parser.add_argument('--output-file', metavar='FILE', required=True, help='Output file')

    args = parser.parse_args()

    manifest = load_data_from_file(args.manifest_file)

    manifest_dir = os.path.dirname(args.manifest_file)

    result = {}
    for intent, intent_files in manifest.iteritems():
        intent_regexps = set()
        for file in intent_files:
            file_path = os.path.join(manifest_dir, file['filename'])
            intent_regexps |= read_regexes(
                file_path, file['add_word_boundaries'], file['add_arbitrary_prefix'], file['add_arbitrary_suffix'])

        result[intent] = list(intent_regexps)

    with codecs.open(args.output_file, 'w', 'utf-8') as output_file:
        yaml.dump(result, output_file, default_flow_style=False, allow_unicode=True)


if __name__ == '__main__':
    main()
