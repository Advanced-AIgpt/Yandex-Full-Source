# coding:utf-8

import os
import argparse
import subprocess


class Logger:
    def info(self, msg):
        print(msg)

logger = Logger()


class Canonizer:
    FILE_CANONIZER_YAMAKE_LOG = 'canonize_test_run.log'
    FILE_CANONIZER_FAILED_LOG = 'canonize_failed_tests.log'

    def __init__(self, target_dir):
        self._target_dir = target_dir

    def do_clean(self):
        logger.info('EXECUTING: clean')
        try:
            os.remove(Canonizer.FILE_CANONIZER_YAMAKE_LOG + '.1')
        except FileNotFoundError:
            pass

        if os.path.exists(Canonizer.FILE_CANONIZER_YAMAKE_LOG):
            os.rename(Canonizer.FILE_CANONIZER_YAMAKE_LOG, Canonizer.FILE_CANONIZER_YAMAKE_LOG + '.1')

        try:
            os.remove(Canonizer.FILE_CANONIZER_YAMAKE_LOG)
        except FileNotFoundError:
            pass

        try:
            os.remove(Canonizer.FILE_CANONIZER_FAILED_LOG)
        except FileNotFoundError:
            pass
        logger.info('Done')
        logger.info('')

    def do_run_runner(self, filter):
        logger.info('EXECUTING: runner')
        cmd = f'ya make {self._target_dir} -Ar '

        failed_tests = _read_stripped_lines(Canonizer.FILE_CANONIZER_FAILED_LOG)
        if failed_tests:
            cmd += self._make_cmd_filter_for_failed_tests(failed_tests)
            if filter:
                logger.info(f'Found {len(failed_tests)} failed tests. Ignoring --test-filter={filter} parameter')
        else:
            if filter:
                cmd += f'-F \'{filter}\' '
            else:
                logger.info('No info about failed tests found. Will run all tests')

        cmd += '2>&1 | '

        cmd += f'tee {Canonizer.FILE_CANONIZER_YAMAKE_LOG} | '

        cmd += f'awk \'/\[fail\]/ {{print $2}}\' > {Canonizer.FILE_CANONIZER_FAILED_LOG} '

        logger.info(f'Executing: `{cmd}`')
        logger.info('Please wait...')
        res = subprocess.run(cmd, shell=True)
        res.check_returncode()
        success = self._report_status()
        logger.info('')
        return success

    def do_run_generator(self, filter):
        logger.info('EXECUTING: generator')
        cmd = f'ya make {self._target_dir} -ArZ -DIT2_GENERATOR '

        failed_tests = _read_stripped_lines(Canonizer.FILE_CANONIZER_FAILED_LOG)
        if failed_tests:
            cmd += self._make_cmd_filter_for_failed_tests(failed_tests)
            if filter:
                logger.info(f'Found {len(failed_tests)} failed tests. Ignoring --test-filter={filter} parameter')
        else:
            if filter:
                cmd += f'-F \'{filter}\' '
            else:
                logger.info('No info about failed tests found. Nothing to canonize!')
                return True

        cmd += '2>&1 | '

        cmd += f'tee {Canonizer.FILE_CANONIZER_YAMAKE_LOG} | '

        # We search for lines in log similar to this one: "2021-09-17 12:28:15,474 ERROR ya.test: Cannot canonize broken test tests_navigator.py::TestsNavigatorOndemandHw::test_album_from_multiple_artists[navi]: alice/hollywood/library/scenarios/music/it2_music_client/tests_navigator.py:237: in test_album_from_multiple_artists"
        # And take from there $9 = "tests_navigator.py::TestsNavigatorOndemandHw::test_album_from_multiple_artists[navi]:"
        # And cut the trailing ":": "tests_navigator.py::TestsNavigatorOndemandHw::test_album_from_multiple_artists[navi]"
        cmd += f'awk \'/Cannot canonize broken test/ {{print substr($9, 0, length($9) - 1)}}\' > {Canonizer.FILE_CANONIZER_FAILED_LOG} '

        logger.info(f'Executing: `{cmd}`')
        logger.info('Please wait...')
        res = subprocess.run(cmd, shell=True)
        res.check_returncode()
        success = self._report_status()
        logger.info('')
        return success

    def _make_cmd_filter_for_failed_tests(self, failed_tests):
        cmd_filter = ''
        for failed_test in failed_tests:
            if not failed_test:
                continue
            cmd_filter += f' -F \'{failed_test}\' '
        return cmd_filter

    def _report_status(self):
        failed_tests = _read_stripped_lines(Canonizer.FILE_CANONIZER_FAILED_LOG)
        if not failed_tests:
            logger.info('No failed tests found, nothing to canonize.')
            return True
        logger.info(f'Found {len(failed_tests)} failed tests:')
        for failed_test in failed_tests:
            logger.info(f'{failed_test}')
        logger.info(f'See full test run log for details: {os.path.abspath(Canonizer.FILE_CANONIZER_YAMAKE_LOG)}')
        return False


def _read_stripped_lines(filename):
    result = []
    try:
        with open(filename) as in_f:
            for line in in_f.readlines():
                line_stripped = line.strip()
                if not line_stripped:
                    continue
                result.append(line_stripped)
    except FileNotFoundError:
        pass
    return result


def main():
    parser = argparse.ArgumentParser(description='''Tool that helps to regenerate stubs/canonize broken it2 tests. Basic usage: 1) canonize <IT2_TESTS_PATH> 2) Fix problems (or do nothing in case of flaky timeouts for example) 3) canonize <IT2_TESTS_PATH> --continue 4) Repeat two last steps as many times as needed until all your it2 tests are passed.''')
    parser.add_argument('-c', '--continue', action='store_true', help='Is an alias for `--exec=generator`. See description of --exec parameter')
    parser.add_argument('-e', '--exec', type=str, default='cleanup:runner:generator', help='Commands for canonizer to execute (for advanced usage). Any combination of `cleanup`, `runner`, `generator` joined with `:`')
    parser.add_argument('-F', '--test-filter', type=str, help='Run only tests that match <tests-filter>.')
    parser.add_argument('target', type=str, default='.', help='ya make target dir with it2 tests to canonize')

    args = parser.parse_args()

    canonizer = Canonizer(args.target)

    if getattr(args, 'continue'):
        commands = ['generator']
    else:
        commands = args.exec.split(':')
    logger.info(f'COMMANDS to execute: {commands}')

    for command in commands:
        if command == 'cleanup':
            canonizer.do_clean()
        elif command == 'runner':
            success = canonizer.do_run_runner(args.test_filter)
            if success:
                logger.info('No failed tests found, canonizer will not proceed')
                break
        elif command == 'generator':
            success = canonizer.do_run_generator(args.test_filter)
            if not success:
                logger.info('Now fix bugs and run canonizer with --continue parameter again')
        else:
            raise Exception(f'Unknown command {command}, found in `{commands}`')

if __name__ == '__main__':
    main()
    exit(0)
