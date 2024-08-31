# -*- coding: utf-8 -*-

"""Generates .nlg file from microintents.yaml config on alice/nlg/bin build
"""

import argparse
import yaml

from codecs import open

def generate_chooseitem(phrases, indent):
    if len(phrases) == 1:
        return phrases[0]
    result = []
    result.append(u' ' * indent + u'{% chooseitem %}')
    for i, phrase in enumerate(phrases):
        if i > 0:
            result.append(u' ' * indent + u'{% or %}')
        result.append(u' '* (2 + indent) + phrase)
    result.append(u' ' * indent + '{% endchooseitem %}')
    return u"\n".join(result)

def generate_intent_with_condition(intent_data):
    assert(len(intent_data) == 2)
    assert('else' in intent_data)
    result = []
    for key in intent_data:
        if key == 'else':
            continue
        result.append(u' ' * 4 + u'{% if context.' + key + u'%}')
        result.append(generate_chooseitem(intent_data[key], 6))
    result.append(u' ' * 4 + u'{% else %}')
    result.append(generate_chooseitem(intent_data['else'], 6))
    result.append(u' ' * 4 + u'{% endif %}')
    return u"\n".join(result)



def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-i', '--input-path')
    parser.add_argument('-o', '--output-path')

    args = parser.parse_args()

    with open(args.input_path, encoding='utf8') as f:
        microintents_data = yaml.safe_load(f)['intents']

    with open(args.output_path, 'w', encoding='utf8') as f:
        f.write(u'{% phrase render_result %}\n')

        for i, intent in enumerate(microintents_data):
            current = [u' ' * 2, u'{% ', u'if', u' context.microintent == "', intent, u'" %}']
            if i > 0:
                current[2] = u'elif'
            f.write(u''.join(current) + u'\n') 
            if isinstance(microintents_data[intent]['nlg'], dict):
                f.write(generate_intent_with_condition(microintents_data[intent]['nlg']) + u'\n')
            else:
                f.write(generate_chooseitem(microintents_data[intent]['nlg'], 4) + u'\n')
        f.write(u' ' *2 + '{% endif %}\n')
        f.write(u'{% endphrase %}\n')


if __name__ == '__main__':
  main()
