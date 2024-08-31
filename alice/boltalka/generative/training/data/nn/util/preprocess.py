import argparse
import re
import string

from alice.boltalka.tools.dssm_preprocessing.preprocessing.lib.main import TextNormalizer

import vh
import yt.wrapper as yt


def punct_separate(text):
    # space out punctuations
    for c in string.punctuation:
        text = text.replace(c, ' {} '.format(c))
    return text


def preprocess_contexts_(contexts):
    # removing \n and \t from contexts
    contexts = [ctx.replace('\n', '').replace('\t', '') for ctx in contexts]

    # ё -> е
    contexts = [ctx.replace('ё', 'е') for ctx in contexts]

    # removing <speaker_voice='aa'>
    contexts = [re.sub('<.*>', ' ', ctx) for ctx in contexts]

    # space out punctuations
    contexts = [punct_separate(ctx) for ctx in contexts]

    # TODO remove the "-" removal when it is fixed in production
    contexts = [ctx.replace(' - ', ' ') for ctx in contexts]

    # filtering empty contexts
    contexts = [ctx for ctx in contexts if len(ctx) > 0]

    # lowering
    contexts = [ctx.lower() for ctx in contexts]

    # remove numbers  # TODO move in production
    contexts = [re.sub('^\(?\d+[.;:,\-\) ]{0,3} *', '', ctx) for ctx in contexts]

    # strip and double space removal  # TODO move in production (and tfnn bucket making)
    contexts = [ctx.strip() for ctx in contexts]
    contexts = [ctx.replace('  ', ' ') for ctx in contexts]

    # removing trailing dot  # TODO move in production
    contexts = [ctx[:-2] if ctx.endswith(' .') and not ctx.endswith('.  .') else ctx for ctx in contexts]

    return contexts


class PreprocessContextsMapper:
    def __init__(self, columns):
        self.columns = columns

    def __call__(self, row):
        for column in self.columns:
            ctx = preprocess_contexts_([row[column]])
            row[column] = ctx[0] if len(ctx) > 0 else ''
        yield row


@vh.lazy.hardware_params(vh.HardwareParams(max_ram=1000))
@vh.lazy.from_annotations
def preprocess_contexts(input_table: vh.YTTable, columns: vh.mkinput(str, nargs='*')) -> vh.YTTable:
    out_table = yt.TablePath(yt.create_temp_table())
    yt.run_map(PreprocessContextsMapper(columns=columns), input_table, out_table)
    return out_table


# manual declaration since tokenizer doesn't work in Py3
HYPHEN_PREFIXES = {
    'кто', 'кого', 'кому', 'кем', 'ком',
    'что', 'чего', 'чему', 'чем',
    'где',
    'когда',
    'куда',
    'откуда',
    'почему',
    'зачем',
    'как',
    'сколько', 'скольких', 'скольким', 'сколькими',
    'какой', 'какого', 'какому', 'каким', 'каком', 'какие', 'каких', 'какими',
    'чей', 'чьего', 'чьему', 'чьим', 'чьем', 'чьи', 'чьими', 'чьих'
}
HYPHEN_POSTFIXES = {'то', 'нибудь', 'либо'}


def should_insert_hyphen(word1, word2):
    return word1 == 'кое' and word2 in HYPHEN_PREFIXES or word1 in HYPHEN_PREFIXES and word2 in HYPHEN_POSTFIXES


def insert_hyphens(text):
    result_string = []

    prev_token = None
    for token in text.split(' '):
        if prev_token is not None:
            if should_insert_hyphen(prev_token, token):
                result_string.append('-')
            else:
                result_string.append(' ')

        result_string.append(token)
        prev_token = token

    return ''.join(result_string)


def process_response(text):
    text = text.replace(' `', '')

    for c in string.punctuation:
        text = text.replace(' {} '.format(c), '{} '.format(c))
        text = text.replace(' {}'.format(c), '{}'.format(c))

    # TODO remove the "-" addition when it is fixed in production
    text = insert_hyphens(text)

    return text


class LowerAndPunctSepMapper:
    def __init__(self, columns, join_turns):
        parser = argparse.ArgumentParser()
        TextNormalizer.add_args(parser)
        args = parser.parse_args([
            '--separate-punctuation',
            '--lowercase'
        ])
        self.normalizer = TextNormalizer(args)
        self.columns = columns
        self.join_turns = join_turns

    def __call__(self, row):
        for column in self.columns:
            value = row[column]
            if isinstance(value, list):
                processed = [self.normalizer(el) for el in value]
            else:
                processed = self.normalizer(value)
            if processed is None:
                processed = ''
            if self.join_turns and isinstance(processed, list):
                processed = ' [SPECIAL_SEPARATOR_TOKEN] '.join(processed)
            row[column] = processed
        yield row


@vh.lazy.hardware_params(vh.HardwareParams(max_ram=1000))
@vh.lazy.from_annotations
def lower_and_punct_separator_lazy(input_table: vh.YTTable, columns: vh.mkinput(str, nargs='*'), join_turns: bool = True) -> vh.YTTable:
    out_table = yt.TablePath(yt.create_temp_table())
    yt.run_map(LowerAndPunctSepMapper(columns, join_turns), input_table, out_table, ordered=True)
    return out_table
