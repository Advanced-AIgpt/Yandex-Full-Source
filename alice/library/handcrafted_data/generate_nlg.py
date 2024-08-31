# -*- coding: utf-8 -*-

"""Generates .nlg file from microintents.yaml config on alice/nlg/bin build
"""

import argparse
import yaml

from codecs import open

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-i', '--input-path')
    parser.add_argument('-o', '--output-path')

    args = parser.parse_args()

    with open(args.input_path, encoding='utf8') as f:
        microintents_data = yaml.safe_load(f)

    with open(args.output_path, 'w', encoding='utf8') as f:
        for intent, data in microintents_data['intents'].iteritems():
            f.write(u'{% phrase ' + intent + u' %}\n    {% chooseline %}\n')
            for text in data['nlg']:
                f.write((u' ' * 8) + u'{}\n'.format('\\n'.join(text.split('\n'))))
            f.write((u' ' * 4) + u'{% endchooseline %}\n{% endphrase %}\n\n')


if __name__ == '__main__':
  main()
