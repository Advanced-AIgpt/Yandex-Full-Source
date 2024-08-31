# coding: utf-8

import argparse
import attr
import collections
import json
import logging
import os
import re
import shutil
import yaml

logger = logging.getLogger(__name__)

# =============================================================================
# File utils


def make_parent_dir(file_path):
    dir_path = os.path.dirname(file_path)
    if dir_path:
        os.makedirs(dir_path, exist_ok=True)


def read_json_file(path):
    with open(path) as f:
        # return json.load(f)
        return yaml.full_load(f)


def format_json(d):
    return json.dumps(d, indent=2, ensure_ascii=False, sort_keys=True)


def read_text_file(path):
    with open(path) as f:
        return f.read()


def read_text_file_lines(path):
    with open(path) as f:
        return f.readlines()


def write_text_file(text, path):
    make_parent_dir(path)
    with open(path, 'w') as f:
        f.write(text)


def append_text_file(text, path):
    make_parent_dir(path)
    with open(path, 'a') as f:
        f.write(text)

# =============================================================================
# String utils


GRANET_USED_SYMBOLS = ':;|#{}<>[]().?+*%$\'"'
GRANET_RESERVED_SYMBOLS = '!,\\/@~&=`^-'
GRANET_ALL_SYMBOLS = GRANET_USED_SYMBOLS + GRANET_RESERVED_SYMBOLS


def has_char_set(chars, line):
    return any([c in line for c in chars])


def normalize_spaces(line):
    return re.sub(r'\s{2,}', r' ', line).strip()


def replace_char_set(chars, line):
    for c in chars:
        line = line.replace(c, ' ')
    return normalize_spaces(line)


def normalize_string(line):
    line = line.replace('​', '')
    line = line.replace(b'\xef\xbb\xbf'.decode('utf8'), '')
    line = replace_char_set(GRANET_ALL_SYMBOLS, line)
    line = replace_char_set('!?;-—–−":,._？¡«»›• …', line)
    return line


def escape_string(s):
    s = s.replace('\\', '\\\\')
    s = s.replace('"', '\\"')
    s = s.replace('\n', '\\n')
    s = s.replace('\t', '\\t')
    return s


def remove_affixes(text, prefix='', suffix=''):
    if not text.startswith(prefix):
        return None
    text = text[len(prefix):]
    if not text.endswith(suffix):
        return None
    return text[:-len(suffix)]

# =============================================================================
# Context


@attr.s
class Context:
    vins_dir = attr.ib(default='')
    pa_dir = attr.ib(default='')
    granet_dir = attr.ib(default='')
    vins_config = attr.ib(default=None)
    scenarios_config = attr.ib(default=None)
    entities = attr.ib(default={})
    intents = attr.ib(default={})
    nlus = attr.ib(default={})


def create_context(input_dir, output_dir):
    ctx = Context()
    ctx.vins_dir = os.path.normpath(input_dir) + '/'
    ctx.pa_dir = ctx.vins_dir + 'apps/personal_assistant/'
    ctx.granet_dir = os.path.normpath(output_dir) + '/'
    logger.info('ctx.vins_dir: %s', ctx.vins_dir)
    logger.info('ctx.pa_dir: %s', ctx.pa_dir)
    logger.info('ctx.granet_dir: %s', ctx.granet_dir)
    return ctx

# =============================================================================
# Load vins config


def load_vins_config(ctx):
    logger.info('Load vins config')
    ctx.vins_config = read_json_file(ctx.pa_dir + 'personal_assistant/config/Vinsfile.json')
    ctx.scenarios_config = read_json_file(ctx.pa_dir + 'personal_assistant/config/scenarios/VinsProjectfile.json')
    load_entities(ctx)
    load_intents(ctx)
    load_dm_configs(ctx)
    load_nlu_files(ctx)


def load_entities(ctx):
    logger.info('Load entities')
    for entity in ctx.scenarios_config['entities']:
        ctx.entities[entity['entity']] = entity


def load_intents(ctx):
    logger.info('Load intents')
    for intent in ctx.scenarios_config['intents']:
        ctx.intents[intent['intent']] = intent


def load_dm_configs(ctx):
    logger.info('Load intent dm configs')
    for intent_name, intent in ctx.intents.items():
        dm = intent.get('dm', {})
        if 'path' in dm:
            src_path = dm['path']
            dest_path = remove_affixes(src_path, 'personal_assistant/config/scenarios/intents/', '.json')
            if not dest_path:
                logger.warning('Bad path: %s', src_path)
                return
            dest_path = 'vins/intents/' + dest_path + '.grnt'
            intent['dm_config'] = read_json_file(ctx.pa_dir + src_path)
            intent['granet_path'] = dest_path
        elif 'data' in dm:
            intent['dm_config'] = dm['data']
            intent['granet_path'] = 'vins/intents/inline.grnt'
        else:
            intent['dm_config'] = {}
            logger.warning('No dm: %s', intent_name)


def load_nlu_files(ctx):
    logger.info('Load intent nlu files')
    for intent_name, intent in ctx.intents.items():
        for nlu in intent.get('nlu', []):
            nlu['is_valid'] = False
            if nlu['source'] != 'file':
                nlu['granet_comment'] = 'Skip nlu (microintent)'
                logger.warning('Microintent: %s', intent_name)
                continue
            src_path = nlu['path']
            dest_path = remove_affixes(src_path, 'personal_assistant/config/scenarios/intents/', '.nlu')
            if not dest_path:
                dest_path = remove_affixes(src_path, 'personal_assistant/config/general_conversation/intents/', '.nlu')
                if dest_path:
                    dest_path = 'gc/' + dest_path
            if not dest_path:
                nlu['granet_comment'] = 'Skip nlu (bad path): ' + src_path
                logger.warning('Bad path: %s', src_path)
                continue
            if not nlu.get('can_use_to_train_tagger', True) and intent['dm_config'].get('slots', []):
                comment = 'Skip nlu (!can_use_to_train_tagger): '
                comment += remove_affixes(src_path, 'personal_assistant/config/', '')
                nlu['granet_comment'] = comment
                continue
            nlu_name = dest_path.replace('/', '.')
            dest_path = 'vins/intents/' + dest_path + '.nlu'
            nlu['granet_name'] = nlu_name
            nlu['granet_path'] = dest_path
            nlu['is_valid'] = True
            ctx.nlus[nlu_name] = nlu

# =============================================================================
# Convert nlu_templates
# Result:
#   vins/templates.grnt
#   vins/templates/*.txt


def convert_nlu_templates(ctx):
    logger.info('Convert nlu templates')
    out = ''
    out += convert_pa_nlu_templates(ctx)
    out += convert_system_nlu_templates(ctx)
    write_text_file(out, ctx.granet_dir + 'vins/templates.grnt')


def convert_pa_nlu_templates(ctx):
    out = ''
    for name, src_path in sorted(ctx.vins_config['nlu']['custom_templates'].items()):
        dest_path = remove_affixes(src_path, 'personal_assistant/config/nlu_templates/', '.txt')
        if not dest_path:
            logger.warning('Bad path: %s', src_path)
            continue
        out += convert_nlu_template(ctx, name, ctx.pa_dir + src_path, dest_path)
    return out


def convert_system_nlu_templates(ctx):
    out = ''
    src_dir = ctx.vins_dir + 'core/vins_core/nlu/data/'
    for filename in os.listdir(src_dir):
        name = remove_affixes(filename, '', '.txt')
        if not name:
            continue
        out += convert_nlu_template(ctx, name, src_dir + filename, 'system/' + name)
    return out


def convert_nlu_template(ctx, name, src_path, short_dest_path):
    dest_path = 'vins/templates/' + short_dest_path + '.txt'
    make_parent_dir(ctx.granet_dir + dest_path)
    shutil.copyfile(src_path, ctx.granet_dir + dest_path)
    out = ''
    out += ('$txt.%s: ' % name).ljust(44)
    out += '%%include_raw %s\n' % dest_path
    return out

# =============================================================================
# Convert entities
# Result:
#   vins/entities.grnt
#   vins/entities/*.grnt


def convert_entities(ctx):
    logger.info('Convert nlu entities')

    imported_pathes = set()
    for name, entity in sorted(ctx.entities.items()):
        convert_entity(ctx, name, entity, imported_pathes)

    out = 'import:\n'
    for path in sorted(imported_pathes):
        out += '    %s\n' % path
    write_text_file(out, ctx.granet_dir + 'vins/entities.grnt')


def convert_entity(ctx, name, entity, imported_pathes):
    src_path = entity['path']
    dest_path = remove_affixes(src_path, 'personal_assistant/config/scenarios/entities/', '.json')
    if not dest_path:
        logger.warning('Bad path: %s', src_path)
        return
    dest_path = 'vins/entities/' + dest_path + '.grnt'
    imported_pathes.add(dest_path)

    out = 'entity custom.%s:\n' % name
    for param in ['inflect', 'inflect_numbers', 'use_as_template']:
        if entity.get(param, False):
            out += '    # %s\n' % param
    out += '    root:\n'
    out += '        $ce.%s\n' % name  # todo: inflect
    out += '\n'
    out += '$ce.%s:\n' % name
    out += '    %%type custom.%s\n' % name
    for value, phrases in sorted(read_json_file(ctx.pa_dir + src_path).items()):
        out += '\n'
        out += '    %%value "%s"\n' % escape_string(value)
        for phrase in phrases:
            out += '    %s\n' % normalize_string(phrase)
    write_text_file(out, ctx.granet_dir + dest_path)

# =============================================================================
# Convert nlus:
# Results:
#   vins/intents/*.nlu


def convert_nlus(ctx):
    logger.info('Convert nlu files')

    nlu_line_counters = {}
    errors = collections.defaultdict(list)

    for name, nlu in sorted(ctx.nlus.items()):
        out = ''
        with open(ctx.pa_dir + nlu['path']) as f:
            for line in f:
                line = convert_nlu_line(line, errors)
                if line is None:
                    continue
                out += line + '\n'
        nlu_line_counters[name] = out.count('\n')
        write_text_file(out, ctx.granet_dir + nlu['granet_path'])

    report_nlu_line_errors(errors)
    report_nlu_line_counters(nlu_line_counters)


NLU_LINE_SYNTAX = [
    ('TAG_END', r"'\s*\(\+?\w+\)"),
    ('TAG_BEGIN', r"'"),
    ('ELEMENT', r"@[0-9A-Za-z_]+"),
    ('SUFFIX', r"\([\w\d\,:]*\)"),
    ('SPACE', r"\s+"),
    ('WORD', r"[0-9A-Za-zА-Яа-я]+(?=\s)"),
    ('OTHER', r"[^@'\s]+"),
    ('MISMATCH', r"."),
]

NLU_LINE_REGEX = re.compile('|'.join('(?P<%s>%s)' % pair for pair in NLU_LINE_SYNTAX))

GRAMMEME_RENAMING = {
    'plur': 'pl',
    'nomn': 'nom',
    'datv': 'dat',
    'loct': 'loc',
    'accs': 'acc',
    'gent': 'gen',
    'ablt': 'abl',
}

NluLineToken = collections.namedtuple('Token', ['kind', 'value'])


def convert_nlu_template_suffix(s):
    if not s or re.fullmatch(r'\d*', s):
        return ''
    m = re.fullmatch(r'(?:\d*:)?([A-Za-z_,]+)(?::\d*)?', s)
    if m is None:
        return None
    s = ',' + m.group(1) + ','
    for src, dst in GRAMMEME_RENAMING.items():
        s = s.replace(',' + src + ',', ',' + dst + ',')
    return '<g:%s>' % s[1:-1]


def convert_nlu_line(line, errors):
    line = line.strip()
    if not line or line.startswith('#'):
        return line
    tokens = []
    has_tag_begin = False
    for m in NLU_LINE_REGEX.finditer(line):
        kind = m.lastgroup
        value = m.group()
        if kind == 'TAG_END':
            if not has_tag_begin:
                errors['Error 1: Ending of tag without beginning'].append(line)
                return None
            has_tag_begin = False
            value = re.sub(r"'\s*\(\+?", "'(", value)
        elif kind == 'TAG_BEGIN':
            if has_tag_begin:
                errors['Error 2: Duplicated beginning of tag'].append(line)
                return None
            has_tag_begin = True
        elif kind == 'ELEMENT':
            if value.startswith('@ce_'):
                value = '$ce.' + value[4:]
            else:
                value = '$txt.' + value[1:]
        elif kind == 'SUFFIX':
            if tokens and tokens[-1].kind == 'SPACE':
                tokens.pop()
            if not tokens:
                errors['Error 3: Unexpected parentheses'].append(line)
                return None
            if tokens[-1].kind != 'ELEMENT':
                errors['Error 4: Unexpected parentheses'].append(line)
                return None
            value = convert_nlu_template_suffix(value[1:-1])
            if value is None:
                errors['Error 5: Bad nlu template params'].append(line)
                return None
        elif kind == 'SPACE':
            value = ' '
        elif kind == 'WORD':
            if tokens and tokens[-1].kind in ['ELEMENT', 'SUFFIX']:
                errors['Error 6: Invalid params of element or tag'].append(line)
                return None
        elif kind == 'OTHER':
            value = replace_char_set('!?;-—–":,._？¡«»', value)  # todo: escape string
            value = value.replace('​', '')
            value = value.replace(b'\xef\xbb\xbf'.decode('utf8'), '')
            if has_char_set(GRANET_ALL_SYMBOLS, value):
                errors['Error 7: Prohibited symbol'].append(line)  # todo
                return None
            if tokens and tokens[-1].kind in ['ELEMENT', 'SUFFIX']:
                errors['Error 8: Invalid params of element or tag'].append(line)
                return None
        elif kind == 'MISMATCH':
            errors['Error 9: Unexpected symbol'].append(line)
            return None
        tokens.append(NluLineToken(kind, value))
    if has_tag_begin:
        errors['Error 10: Beginning of tag without ending'].append(line)
        return None
    return normalize_spaces(''.join([t.value for t in tokens]))


def report_nlu_line_errors(errors):
    for kind, errors_of_kind in sorted(errors.items()):
        out = '%s (%d errors):\n' % (kind, len(errors_of_kind))
        for i, error in enumerate(errors_of_kind):
            if i > 10:
                out += '    ...\n'
                break
            out += '    %s\n' % error
        logger.warning(out)


def report_nlu_line_counters(counters):
    out = 'Number of lines in nlu files:\n'
    for c, n in sorted([(-c, n) for n, c in counters.items()]):
        out += '%8d: %s\n' % (-c, n)
    logger.debug(out)

# =============================================================================
# Convert intents:
# Results:
#   vins/intents/*.grnt
#   vins/intents.grnt


def convert_intents(ctx):
    logger.info('Convert nlu intents')

    set_of_dest_paths = set()
    unknown_types = collections.defaultdict(list)

    for name, intent in ctx.intents.items():
        dest_path = intent.get('granet_path', None)
        if dest_path is None:
            continue
        if dest_path not in set_of_dest_paths:
            set_of_dest_paths.add(dest_path)
            write_intent_grammar_header(ctx, dest_path)
        converted = convert_intent(ctx, name, intent, unknown_types)
        append_text_file(converted, ctx.granet_dir + dest_path)

    out = 'import:\n'
    for path in sorted(set_of_dest_paths):
        out += '    %s\n' % path
    write_text_file(out, ctx.granet_dir + 'vins/intents.grnt')

    report_unknown_types(unknown_types)


def write_intent_grammar_header(ctx, path):
    out = ''
    out += 'import:\n'
    out += '    vins/entities.grnt\n'
    out += '    vins/templates.grnt\n'
    out += 'filler:\n'
    out += '    $nonsense\n'
    write_text_file(out, ctx.granet_dir + path)


def convert_intent(ctx, name, intent, unknown_types):
    out = '\n'
    out += 'form personal_assistant.scenarios.%s:\n' % name
    out += '    slots:\n'
    for slot in intent['dm_config'].get('slots', {}):
        slot_name = slot['slot']
        out += '        %s:\n' % slot_name
        types = convert_slot_types(ctx, intent, slot, unknown_types)
        if types:
            out += '            type: %s\n' % ', '.join(types)
        if 'matching_type' in slot:
            out += '            matching_type: %s\n' % slot['matching_type']

    out += '    root:\n'
    if 'nlu' in intent:
        for nlu in intent['nlu']:
            if not nlu['is_valid']:
                out += '        # %s\n' % nlu['granet_comment']
            else:
                out += '        $root.%s\n' % nlu['granet_name']
        for nlu in intent.get('nlu', []):
            if not nlu['is_valid']:
                continue
            out += '    $root.%s:\n' % nlu['granet_name']
            out += '        %%include %s\n' % nlu['granet_path']
    return out


SYS_TYPE_NAMES = {
    'album', 'artist', 'calc', 'currency', 'date', 'datetime', 'datetime_range',
    'films_100_750', 'films_50_filtered', 'fio', 'float', 'geo', 'num',
    'poi_category_ru', 'site', 'soft', 'swear', 'time', 'track', 'units_time', 'weekdays'
}


def convert_slot_types(ctx, intent, slot, unknown_types):
    if 'type' in slot:
        src_types = [slot['type']]
    elif 'types' in slot:
        src_types = slot['types']
    else:
        src_types = []

    dest_types = []
    for t in src_types:
        if t == 'string':
            continue
        if t in ctx.entities:
            dest_types.append('custom.' + t)
            continue
        if t in SYS_TYPE_NAMES:
            dest_types.append('sys.' + t)
            continue
        dest_types.append('custom.unknown.' + t)
        unknown_types[t].append((slot['slot'], intent['intent']))
    return dest_types


def report_unknown_types(unknown_types):
    if not unknown_types:
        return
    out = 'Unknown types:\n'
    for type_name in unknown_types:
        out += '    type "%s":\n' % type_name
        for slot_name, intent_name in sorted(unknown_types[type_name]):
            out += '        in slot "%s" of intent "%s"\n' % (slot_name, intent_name)
    logger.warning(out)

# =============================================================================


def convert_vins(input_dir, output_dir):
    logger.info('Convert vins')

    ctx = create_context(input_dir, output_dir)
    load_vins_config(ctx)
    convert_nlu_templates(ctx)
    convert_entities(ctx)
    convert_nlus(ctx)
    convert_intents(ctx)


def init_logging(is_debug):
    level = logging.INFO
    if is_debug:
        level = logging.DEBUG
    logging.basicConfig(level=level, format='[%(asctime)s] [%(levelname)s] %(message)s')


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('-i', '--input', required=True,
                        help='Directory of converted VINS application. Example: alice/vins')
    parser.add_argument('-o', '--output', required=True,
                        help='Directory for results')
    parser.add_argument('--debug', default=False, action='store_true',
                        help='Set log level to debug')
    return parser.parse_args()


def main():
    args = parse_args()
    init_logging(args.debug)
    convert_vins(args.input, args.output)
    logger.info('Done')


if __name__ == '__main__':
    main()
