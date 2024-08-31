import click
import json
import logging
import os
import pathlib
import random
import re
import subprocess
import sys
import tempfile

from alice.library.python.utils.arcadia import arcadia_root


@click.command()
@click.option('--path-prefix', '-p', required=True, help='Path prefix where to search for it-tests and make ammo. Example: `--path-prefix=alice/hollywood/library/scenarios/random_number`.')
@click.option('--regexp-log-filename', '-m', default=r'log-\d+', required=False, help='Regular expression to match the app_host event log file (basename only).')
@click.option('--skip-tests-run', '-r', is_flag=True, required=False, help='Do not run `it` tests (to produce app_host event logs).')
@click.option('--test-filter', '-F', default='', required=False, help='Run only tests that match <tests-filter>. ATTENTION: put the value in quotes. Ignored with --skip-tests-run.')
@click.option('--out-file', '-o', default='', required=False, help='Output file path to save the ammo.')
@click.option('--ya-upload', '-u', is_flag=True, required=False, help='Upload the resulting ammo file to Sandbox as a resource.')
@click.option('--ya-upload-ttl', '-t', default='14', required=False, help='Time to live (days) for Sandbox resource (ignored without --ya-upload flag).')
@click.option('--skip-shuffle', '-s', is_flag=True, required=False, help='Do not shuffle the resulting ammo data.')
def main(path_prefix, regexp_log_filename, skip_tests_run, test_filter, out_file, ya_upload, ya_upload_ttl, skip_shuffle):
    '''Makes a tank ammo (same format as MAKE_HOLLYWOOD_AMMO task does)
to be used with START_HOLLYWOOD_PERF_TESTING task. The ammo is 'grepped' out from app_host
event logs that are produced by Hollywood integration tests (`it`-tests) run.

\b
Read more https://wiki.yandex-team.ru/alice/hollywood/perftests/#patronydljascenarijakotoryjjeshhenevprode

EXAMPLE 1:

\b
$ it_ammo_maker --path-prefix alice/hollywood/library/scenarios/random_number --ya-upload

\b
This command:
- runs all `it` tests of random_number scenario
- gets ammo from the app_host event logs produced
- uploads the ammo to the Sandbox resourse (prints to the stdout the
URL to the resource)

EXAMPLE 2:

\b
$ it_ammo_maker --path-prefix alice/hollywood/library/scenarios/random_number --skip-tests-run

\b
This command:
- does NOT run any tests (it's supposed that you've already run them
manually)
- gets ammo from the app_host event logs
- prints the resulting ammo to the STDOUT (because there are no `-o` nor `-u` options)


EXAMPLE 3:

\b
$ it_ammo_maker --path-prefix alice/hollywood/library/scenarios -F '*quasar*' --out-file ./ammo.out

\b
This command:
- runs all `it` tests of ALL Hollywood scenarios that match *quasar* filter
- gets ammo from the app_host event logs, it searches logs of all
Hollywood scenarios
- saves the ammo to the output file ./ammo.out
    '''
    logging.basicConfig(stream=sys.stderr, filemode='w', level=logging.INFO)

    arcadia_root_dir = pathlib.Path(arcadia_root())
    logging.info(f'Arcadia root is {arcadia_root_dir}')

    if skip_tests_run:
        logging.info(f'Skipping run `it` tests in {path_prefix}')
    else:
        _run_tests(path_prefix, test_filter)
        logging.info('Tests run is done')

    logging.info('Searching for app_host event logs which should have been produced by `it` tests run')
    log_files = _search_for_app_host_logs(path_prefix, regexp_log_filename)

    logging.info('Preparing to process the found app_host event log files...')
    event_log_dump_path = arcadia_root_dir / 'apphost/tools/event_log_dump/event_log_dump'
    out_ammo = _get_ammo_from_app_host_logs(log_files, event_log_dump_path)
    logging.info(f'Got {len(out_ammo)} ammo items in TOTAL')

    if not skip_shuffle:
        logging.info('Shuffling resulting ammo data.')
        random.shuffle(out_ammo)

    print_result_to_stdout = True
    if out_file:
        logging.info(f'Writing the ammo to file `{out_file}`.')
        _write_ammo_to_out_file(out_ammo, out_file)
        print_result_to_stdout = False

    if ya_upload:
        logging.info('Uploading ammo to Sandbox resource.')
        _upload_ammo_to_sandbox(out_ammo, ya_upload_ttl)
        print_result_to_stdout = False

    if print_result_to_stdout:
        logging.info('Neither --out_file nor --ya-upload options were found. Printing result to the stdout.')
        for ammo in out_ammo:
            print(ammo, end='')
    logging.info('Done!')


def _run_tests(path_prefix, test_filter):
    cmd = ['ya', 'make', '-A']
    if test_filter != '':
        cmd.extend(['-F', test_filter])
    cmd.append(str(path_prefix))
    logging.info(f'Running `it` tests with command `{" ".join(cmd)}`')
    tests_run_code = subprocess.call(cmd)
    if tests_run_code:
        raise Exception(f'Tests run failed with exit code {tests_run_code}')


def _search_for_app_host_logs(path_prefix, regexp_log_filename):
    log_files = []
    regexp = re.compile(regexp_log_filename)
    for root, dirs, files in os.walk(path_prefix, followlinks=True):
        for file_ in files:
            if not regexp.match(file_):
                continue
            log_file = pathlib.Path(root) / file_
            logging.info(f'Found log file {log_file}')
            log_files.append(log_file)
    return log_files


def _get_ammo_from_app_host_logs(log_files, event_log_dump_path):
    out_ammo = []
    for index, log_file in enumerate(log_files):
        logging.info(f'Processing {log_file}')
        cmd_1 = [event_log_dump_path, '--dump-mode=Legacy', '--event-list=TSourceRequest', log_file]
        cmd_2 = ['awk', '{print $5}']
        try:
            ps_1 = subprocess.Popen(cmd_1, stdout=subprocess.PIPE)
            output = subprocess.check_output(cmd_2, stdin=ps_1.stdout, text=True)
            ret_code = ps_1.wait()
            if ret_code:
                raise Exception(f'Failed to execute `{cmd_1}`, return code is {ret_code}')
            count = 0
            for line in output.split():
                out_ammo.append(json.dumps({'Data': line.strip()}) + '\n')
                count += 1
            logging.info(f'Got {count} ammo items')
        except subprocess.CalledProcessError as err:
            raise Exception(f'Failed to execute `{cmd_2}`, return code is {err.returncode}')
    return out_ammo


def _write_ammo_to_out_file(out_ammo, out_file):
    pathlib.Path(out_file).parents[0].mkdir(parents=True, exist_ok=True)
    with open(out_file, 'w') as out_f:
        out_f.writelines(out_ammo)


def _upload_ammo_to_sandbox(out_ammo, ya_upload_ttl):
    with tempfile.TemporaryDirectory() as tmp_dir:
        # START_HOLLYWOOD_PERF_TESTING sandbox task needs that ammo resource is in 'hollywood_ammo/ammo.out' file path
        tmp_out_file = pathlib.Path(tmp_dir) / 'hollywood_ammo' / 'ammo.out'
        logging.info(f'Writing the ammo to tempfile {tmp_out_file}')

        tmp_out_file.parents[0].mkdir(parents=True, exist_ok=True)
        with open(tmp_out_file, 'w') as out_f:
            out_f.writelines(out_ammo)

        logging.info('Preparing ya upload command...')
        cmd = ['ya', 'upload', f'--ttl={ya_upload_ttl}', '--type=HOLLYWOOD_AMMO', f'--root={tmp_dir}', tmp_out_file]
        cmd_code = subprocess.call(cmd)
        if cmd_code:
            raise Exception(f'Command `{" ".join(cmd)}` failed with exit code {cmd_code}')


if __name__ == '__main__':
    main()
