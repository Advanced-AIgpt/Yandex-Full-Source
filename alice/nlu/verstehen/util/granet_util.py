import json

import requests


def get_weak_grammar(grammar_text, indent_size=4):
    # adds .* to grammar's filler
    grammar_lines = grammar_text.split('\n')
    filler_line = None
    filler_indent = 0
    is_in_root = False
    root_indent = None
    line_after_root = None
    for i, line in enumerate(grammar_lines):
        if is_in_root:
            indent = len(line) - len(line.lstrip())
            if indent <= root_indent:
                is_in_root = False
                line_after_root = i
        if line.strip().startswith('filler:'):
            filler_indent = len(line) - len(line.lstrip())
            filler_line = i
        if line.strip().startswith('root:'):
            is_in_root = True
            root_indent = len(line) - len(line.lstrip())

    if filler_line is not None:
        # add .* to the filler
        weak_token = ' ' * (filler_indent + indent_size) + '.*'
        grammar_lines.insert(filler_line + 1, weak_token)
    else:
        # insert filler after root
        filler_lines = [' ' * root_indent + 'filler:',
                        ' ' * (root_indent + indent_size) + '.*']
        grammar_lines = grammar_lines[:line_after_root] + \
            filler_lines + grammar_lines[line_after_root:]

    return '\n'.join(grammar_lines)


def markup_into_slots(text, slots):
    markup = []
    last_end = 0
    for slot in slots.iter_nonterminals():
        start, end = slot.get_interval()
        if start - last_end > 0:
            markup.append((None, text[last_end:start], (last_end, start)))

        markup.append((
            slot.get_name(),
            text[start:end],
            (start, end)
        ))

        last_end = end
    if len(text) - last_end > 0:
        markup.append((None, text[last_end:], (last_end, len(text))))
    return markup


def fetch_synonyms(text, start, end, host, grammar_synonyms=[], noise=0, nMasks=1, nSamples=20, max_cum_score=0.9, **kwargs):
    synonyms = []
    req = {
        'text': text,
        'interval': [start, end],
        'synonyms': [text[start:end]] + grammar_synonyms,
        'noise': 0,
        'n_masks': nMasks,
        'n_samples': nSamples,
        'max_cum_score': max_cum_score
    }
    response = requests.post(host + '/process', data=json.dumps(req))
    response.raise_for_status()
    response = json.loads(response.text)
    for syn in response['synonyms']:
        synonyms.append(syn)
    return synonyms


def get_grammar_synonyms(grammar, slots, interval):
    if interval is None:
        return []
    for nonterminal in slots.iter_nonterminals():
        if nonterminal.get_interval()[0] == interval[0] and nonterminal.get_interval()[1] == interval[1]:
            return nonterminal.get_words_from_rule(grammar)


def add_rule_to_grammar(grammar_text, nonterminal, added_value, suffix='  # auto-generated', indent_size=4):
    nonterminal_line = None
    lines = grammar_text.split('\n')
    for i, line in enumerate(lines):
        if line.strip().startswith(nonterminal + ':'):
            nonterminal_line = i
            break
    if nonterminal_line is None:
        print('No such nonterminal ({})'.format(nonterminal))
        return grammar_text
    n_spaces = len(lines[nonterminal_line]) - \
        len(lines[nonterminal_line].lstrip())
    new_line = ' ' * (n_spaces + indent_size)
    new_line += added_value
    new_line += suffix
    lines.insert(nonterminal_line + 1, new_line)
    return '\n'.join(lines)


def update_nonterminal(sample, interval, grammar, value):
    text = sample.get_text()
    result = grammar.parse(sample)
    slots = result.get_best_variant_slots(0)
    start, end = interval
    original_nonterminal = None
    for nonterminal in slots.iter_nonterminals():
        if list(nonterminal.get_interval()) == interval:
            original_nonterminal = nonterminal.get_name()
            break
    if len(text) == end:
        token = value[start:]
    else:
        token = value[start:-(len(text) - end)]
    grammar_text = add_rule_to_grammar(
        grammar.get_text(), original_nonterminal, token)
    return grammar_text
