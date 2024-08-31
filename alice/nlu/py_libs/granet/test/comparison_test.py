import filecmp
import re
import resource
import subprocess
from time import clock

import pandas as pd
import pytest
import yatest.common as yc

from granet import Grammar, TsvSampleDataset, SourceTextCollection

GRAMMAR_TEMP_FILENAME = yc.source_path('alice/nlu/data/ru/granet/tmp.grnt')
GRAMMARS_DIR = yc.source_path('alice/nlu/data/ru/granet/')
GRANET_DIR = yc.binary_path('alice/nlu/granet/tools/granet')
TEST_TSV_PATH = yc.source_path(
    'alice/nlu/py_libs/granet/data/alice/nlu/data/ru/test/pool/alice6v2.tsv')

dataset = TsvSampleDataset()
dataset.load(TEST_TSV_PATH)


def output_tsv_path(tmp_path, prefix):
    if tmp_path is None:
        return yc.test_source_path(prefix + '_test.tsv')
    return (tmp_path / (prefix + '_test.tsv')).as_posix()


def get_form_name(grammar):
    return re.search('(?<=form\s)[a-z\._]+(?=\:)', grammar).group(0)


def prepare_grammar(path):
    '''
    return grammar with absolute and relative pathes in imports (relative needed for tool)
    '''
    with open(path, 'rb') as f:
        grammar = f.read()
    grammar = grammar.replace('\t', '    ')
    form = get_form_name(grammar)

    sources = SourceTextCollection()
    sources.update_main_text(grammar)
    sources.add_all_from_path(GRAMMARS_DIR)

    return sources, form


@pytest.fixture
def grammars_to_check():
    grammars = [
        yc.source_path('alice/nlu/data/ru/granet/experiment/frame_filler/train_schedule.grnt'),
        yc.source_path('alice/nlu/data/ru/granet/music/radio_play.grnt'),
        yc.source_path('alice/nlu/data/ru/granet/other/random_number.grnt')
    ]
    return [(g, prepare_grammar(g)) for g in grammars]


@pytest.fixture
def grammar_to_check_time():
    path = yc.source_path('alice/nlu/data/ru/granet/experiment/frame_filler/train_schedule.grnt')
    return path, prepare_grammar(path)


def compare_files(tsv1, tsv2):
    return filecmp.cmp(tsv1, tsv2)


def get_pylib_parse_results(sources):
    results = []
    start = clock()
    grammar = Grammar(sources)
    for sample in dataset:
        res = grammar.parse(sample)
        if res.is_positive(0):
            results.append(res.get_best_variant_as_str(0))
    return results, clock() - start


def get_pylib_batch_parse_results(sources):
    start = clock()
    grammar = Grammar(sources)
    results = dataset.parse(grammar)
    return results, clock() - start


def get_tool_parse_results(path, form, output_file=output_tsv_path(None, 'granet_tool')):

    granet_cmd = ['./granet', 'dataset', 'select', '-i', TEST_TSV_PATH,
                  '-g', path, '--lang', 'ru', '--form', form, '--source-dir', GRAMMARS_DIR,
                  '-p', output_file, '--print-slots', '--keep-weight', 'NO']

    start = resource.getrusage(resource.RUSAGE_CHILDREN)
    popen = subprocess.Popen(granet_cmd, cwd=GRANET_DIR)
    popen.wait()
    with open(output_file, 'r') as f:
        results = [line.split('\t')[0] for i, line in enumerate(f) if i != 0]
    end = resource.getrusage(resource.RUSAGE_CHILDREN)

    return results, end.ru_utime - start.ru_utime


def get_pylib_results_to_file(sources, form, tmp_path):
    output_file = output_tsv_path(tmp_path, form + '.granet_pylib')

    def output_to_tsv(results):
        df = pd.DataFrame({'text': results})
        df.to_csv(output_file, sep='\t', index=False)

    results, _ = get_pylib_parse_results(sources)
    output_to_tsv(results)
    return output_file


def get_tool_results_to_file(grammar_path, form, tmp_path):
    output_file = output_tsv_path(tmp_path, form + '.granet_tool')
    get_tool_parse_results(grammar_path, form, output_file)
    return output_file


def test_equivalence(grammars_to_check, tmp_path):
    for grammar_path, (sources, form) in grammars_to_check:
        tool_output_file = get_tool_results_to_file(grammar_path, form, tmp_path)
        pylib_output_file = get_pylib_results_to_file(sources, form, tmp_path)
        assert(compare_files(pylib_output_file, tool_output_file))


def test_time(grammar_to_check_time, tmp_path):
    path, (sources, form) = grammar_to_check_time
    _, tool_time = get_tool_parse_results(path, form, output_tsv_path(tmp_path, 'granet_tool'))
    _, pylib_time = get_pylib_parse_results(sources)
    _, pylib_batch_time = get_pylib_batch_parse_results(sources)
    print('Pylib time: {}, tool time {}, pylib batch time {}'.format(
        pylib_time, tool_time, pylib_batch_time))
