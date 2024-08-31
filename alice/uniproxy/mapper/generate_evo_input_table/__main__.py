# coding: utf-8

import json
import os
import subprocess
import tempfile

import click
import yt.wrapper as yt


with os.popen('ya dump root') as f:
    ARCADIA_ROOT = f.read()
DEFAULT_EVO_BINARY_PATH = os.path.join(ARCADIA_ROOT, 'alice/tests/integration_tests/alice-tests-integration_tests')


def generate_file_with_tests_names(evo_binary_path):
    file_with_tests = tempfile.NamedTemporaryFile(delete=False).name
    subprocess.run([
        evo_binary_path,
        '--collect-only',
        '--test-param',
        f'scraper-input-table={file_with_tests}',
        '--output-dir',
        './',
    ], capture_output=True)
    return file_with_tests


@click.command()
@click.option('--output-table', default='', show_default=True,
              help='Table in Hahn cluster for writing test names. Write tests names to stdout without this parameter')
@click.option('--evo-binary-path', default=DEFAULT_EVO_BINARY_PATH, show_default=True,
              help='Binary with alice evo tests')
@click.option('--tests-number', default=-1, show_default=True,
              help='Write only N first tests names. Use -1 to write all tests names')
def main(output_table, evo_binary_path, tests_number):
    file_with_tests = generate_file_with_tests_names(evo_binary_path)
    print('tests generated %d' % len([1 for line in open(file_with_tests)]))

    rows = []
    prev_test = ''
    with open(file_with_tests) as f:
        for idx, test in enumerate(f):
            if tests_number != -1 and idx > tests_number:
                break

            test = test[:-1]
            test = test.split('[')[0]
            if prev_test == test:
                continue
            prev_test = test
            rows.append({
                'session_id': str(idx),
                'session_sequence': 0,
                'reversed_session_sequence': 0,
                'test_filter': test,
            })
    print('rows generated')

    if len(output_table) > 0:
        if yt.exists(output_table):
            yt.remove(output_table)
        yt.write_table(output_table, rows, format="json")
    else:
        print(json.dumps(rows))
    os.unlink(file_with_tests)


if __name__ == '__main__':
    main()
